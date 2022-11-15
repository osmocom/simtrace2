/* SIMtrace 2 sniffer mode
 *
 * (C) 2016-2017 by Harald Welte <hwelte@hmw-consulting.de>
 * (C) 2018 by sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/* This code implement the Sniffer mode to sniff the communication between a 
 * SIM card (or any ISO 7816 smart card) and a phone (or any ISO 7816 card 
 * reader).
 * For historical reasons (i.e. SIMtrace hardware) the USART peripheral 
 * connected to the SIM card is used.
 * TODO put common ISO7816-3 code is separate library (and combine clean with 
 * iso7816_4)
 */
#include "board.h"
#include "simtrace.h"

#ifdef HAVE_SNIFFER

/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include <string.h>
#include "utils.h"
#include "iso7816_fidi.h"
/* USB related libraries */
#include "osmocom/core/linuxlist.h"
#include "osmocom/core/msgb.h"
#include "llist_irqsafe.h"
#include "usb_buf.h"
#include "simtrace_usb.h"
#include "simtrace_prot.h"

/*------------------------------------------------------------------------------
 *         Internal definitions
 *------------------------------------------------------------------------------*/

/*! Maximum Answer-To-Reset (ATR) size in bytes
 *  @note defined in ISO/IEC 7816-3:2006(E) section 8.2.1 as 32, on top the initial character TS of section 8.1
 *  @remark technical there is no size limitation since Yi present in T0,TDi will indicate if more interface bytes are present, including TDi+i
 */
#define MAX_ATR_SIZE 33
/*! Maximum Protocol and Parameters Selection (PPS) size in bytes
 *  @note defined in ISO/IEC 7816-3:2006(E) section 9.2
 */
#define MAX_PPS_SIZE 6

/*! ISO 7816-3 states relevant to the sniff mode */
enum iso7816_3_sniff_state {
	ISO7816_S_RESET, /*!< in Reset */
	ISO7816_S_WAIT_ATR, /*!< waiting for ATR to start */
	ISO7816_S_IN_ATR, /*!< while we are receiving the ATR */
	ISO7816_S_WAIT_TPDU, /*!< waiting for start of new TPDU */
	ISO7816_S_IN_TPDU, /*!< inside a single TPDU */
	ISO7816_S_IN_PPS_REQ, /*!< while we are inside the PPS request */
	ISO7816_S_WAIT_PPS_RSP, /*!< waiting for start of the PPS response */
	ISO7816_S_IN_PPS_RSP, /*!< while we are inside the PPS request */
};

/*! Answer-To-Reset (ATR) sub-states of ISO7816_S_IN_ATR
 *  @note defined in ISO/IEC 7816-3:2006(E) section 8
 */
enum atr_sniff_state {
	ATR_S_WAIT_TS, /*!< initial byte */
	ATR_S_WAIT_T0, /*!< format byte */
	ATR_S_WAIT_TA, /*!< first sub-group interface byte */
	ATR_S_WAIT_TB, /*!< second sub-group interface byte */
	ATR_S_WAIT_TC, /*!< third sub-group interface byte */
	ATR_S_WAIT_TD, /*!< fourth sub-group interface byte */
	ATR_S_WAIT_HIST, /*!< historical byte */
	ATR_S_WAIT_TCK, /*!< check byte */
};

/*! Protocol and Parameters Selection (PPS) sub-states of ISO7816_S_IN_PTS_REQ/ISO7816_S_IN_PTS_RSP
 *  @note defined in ISO/IEC 7816-3:2006(E) section 9
 */
enum pps_sniff_state {
	PPS_S_WAIT_PPSS, /*!< initial byte */
	PPS_S_WAIT_PPS0, /*!< format byte */
	PPS_S_WAIT_PPS1, /*!< first parameter byte */
	PPS_S_WAIT_PPS2, /*!< second parameter byte */
	PPS_S_WAIT_PPS3, /*!< third parameter byte */
	PPS_S_WAIT_PCK, /*!< check byte */
	PPS_S_WAIT_END, /*!< all done */
};

/*! Transport Protocol Data Unit (TPDU) sub-states of ISO7816_S_IN_TPDU
 *  @note defined in ISO/IEC 7816-3:2006(E) section 10 and 12
 *  @remark APDUs are formed by one or more command+response TPDUs
 */
enum tpdu_sniff_state {
	TPDU_S_CLA, /*!< class byte */
	TPDU_S_INS, /*!< instruction byte */
	TPDU_S_P1, /*!< first parameter byte for the instruction */
	TPDU_S_P2, /*!< second parameter byte for the instruction */
	TPDU_S_P3, /*!< third parameter byte encoding the data length */
	TPDU_S_PROCEDURE, /*!< procedure byte (could also be SW1) */
	TPDU_S_DATA_REMAINING, /*!< remaining data bytes */
	TPDU_S_DATA_SINGLE, /*!< single data byte */
	TPDU_S_SW1, /*!< first status word */
	TPDU_S_SW2, /*!< second status word */
};

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/* note: the sniffer code is currently designed to support only one sniffing interface, but the hardware would support a second one.
 * to support a second sniffer interface the code should be restructured to use handles.
 */
/* Pin configurations */
/*! Pin configuration to sniff communication (using USART connection card) */
static const Pin pins_sniff[] = { PINS_SIM_SNIFF };
/*! Pin configuration to interconnect phone and card using the bus switch */
static const Pin pins_bus[] = { PINS_BUS_SNIFF };
/*! Pin configuration to power the card by the phone */
static const Pin pins_power[] = { PINS_PWR_SNIFF };
/*! Pin configuration for timer counter to measure ETU timing */
static const Pin pins_tc[] = { PINS_TC };
/*! Pin configuration for card reset line */
static const Pin pin_rst = PIN_SIM_RST_SNIFF;

/* USART related variables */
/*! USART peripheral used to sniff communication */
static struct Usart_info sniff_usart = {
	.base = USART_SIM,
	.id = ID_USART_SIM,
	.state = USART_RCV,
};
/*! Ring buffer to store sniffer communication data */
static struct ringbuf sniff_buffer;

/* Flags  to know is the card status changed (see SIMTRACE_MSGT_DT_SNIFF_CHANGE flags) */
volatile uint32_t change_flags = 0;

/* ISO 7816 variables */
/*! ISO 7816-3 state */
enum iso7816_3_sniff_state iso_state = ISO7816_S_RESET;
/*! ATR state */
enum atr_sniff_state atr_state;
/*! ATR data
 *  @remark can be used to check later protocol changes
 */
uint8_t atr[MAX_ATR_SIZE];
/*! Current index in the ATR data */
uint8_t atr_i = 0;
/*! If convention conversion is needed */
bool convention_convert = false;
/*! The supported T protocols */
uint16_t t_protocol_support = 0;
/*! PPS state 
 *  @remark it is shared between request and response since they aren't simultaneous but follow the same procedure
 */
enum pps_sniff_state pps_state;
/*! PPS request data
 *  @remark can be used to check PPS response
 */
uint8_t pps_req[MAX_PPS_SIZE];
/*! PPS response data */
uint8_t pps_rsp[MAX_PPS_SIZE];
/*! TPDU state */
enum tpdu_sniff_state tpdu_state;
/*! Final TPDU packet
 *  @note this is the complete command+response TPDU, including header, data, and status words
 *  @remark this does not include the procedure bytes
 */
uint8_t tpdu_packet[5+256+2];
/*! Current index in TPDU packet */
uint16_t tpdu_packet_i = 0;

/*! Waiting Time (WT)
 *  @note defined in ISO/IEC 7816-3:2006(E) section 8.1 and 10.2
 */
uint32_t wt = 9600;

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/*! Convert data between direct and inverse convention
 *  @note direct convention is LSb first and HIGH=1; inverse conversion in MSb first and LOW=1
 *  @remark use a look up table to speed up conversion
 */
static const uint8_t convention_convert_lut[256] = { 0xff, 0x7f, 0xbf, 0x3f, 0xdf, 0x5f, 0x9f, 0x1f, 0xef, 0x6f, 0xaf, 0x2f, 0xcf, 0x4f, 0x8f, 0x0f, 0xf7, 0x77, 0xb7, 0x37, 0xd7, 0x57, 0x97, 0x17, 0xe7, 0x67, 0xa7, 0x27, 0xc7, 0x47, 0x87, 0x07, 0xfb, 0x7b, 0xbb, 0x3b, 0xdb, 0x5b, 0x9b, 0x1b, 0xeb, 0x6b, 0xab, 0x2b, 0xcb, 0x4b, 0x8b, 0x0b, 0xf3, 0x73, 0xb3, 0x33, 0xd3, 0x53, 0x93, 0x13, 0xe3, 0x63, 0xa3, 0x23, 0xc3, 0x43, 0x83, 0x03, 0xfd, 0x7d, 0xbd, 0x3d, 0xdd, 0x5d, 0x9d, 0x1d, 0xed, 0x6d, 0xad, 0x2d, 0xcd, 0x4d, 0x8d, 0x0d, 0xf5, 0x75, 0xb5, 0x35, 0xd5, 0x55, 0x95, 0x15, 0xe5, 0x65, 0xa5, 0x25, 0xc5, 0x45, 0x85, 0x05, 0xf9, 0x79, 0xb9, 0x39, 0xd9, 0x59, 0x99, 0x19, 0xe9, 0x69, 0xa9, 0x29, 0xc9, 0x49, 0x89, 0x09, 0xf1, 0x71, 0xb1, 0x31, 0xd1, 0x51, 0x91, 0x11, 0xe1, 0x61, 0xa1, 0x21, 0xc1, 0x41, 0x81, 0x01, 0xfe, 0x7e, 0xbe, 0x3e, 0xde, 0x5e, 0x9e, 0x1e, 0xee, 0x6e, 0xae, 0x2e, 0xce, 0x4e, 0x8e, 0x0e, 0xf6, 0x76, 0xb6, 0x36, 0xd6, 0x56, 0x96, 0x16, 0xe6, 0x66, 0xa6, 0x26, 0xc6, 0x46, 0x86, 0x06, 0xfa, 0x7a, 0xba, 0x3a, 0xda, 0x5a, 0x9a, 0x1a, 0xea, 0x6a, 0xaa, 0x2a, 0xca, 0x4a, 0x8a, 0x0a, 0xf2, 0x72, 0xb2, 0x32, 0xd2, 0x52, 0x92, 0x12, 0xe2, 0x62, 0xa2, 0x22, 0xc2, 0x42, 0x82, 0x02, 0xfc, 0x7c, 0xbc, 0x3c, 0xdc, 0x5c, 0x9c, 0x1c, 0xec, 0x6c, 0xac, 0x2c, 0xcc, 0x4c, 0x8c, 0x0c, 0xf4, 0x74, 0xb4, 0x34, 0xd4, 0x54, 0x94, 0x14, 0xe4, 0x64, 0xa4, 0x24, 0xc4, 0x44, 0x84, 0x04, 0xf8, 0x78, 0xb8, 0x38, 0xd8, 0x58, 0x98, 0x18, 0xe8, 0x68, 0xa8, 0x28, 0xc8, 0x48, 0x88, 0x08, 0xf0, 0x70, 0xb0, 0x30, 0xd0, 0x50, 0x90, 0x10, 0xe0, 0x60, 0xa0, 0x20, 0xc0, 0x40, 0x80, 0x00, };

/*! Update Waiting Time (WT)
 *  @param[in] wi Waiting Integer (0 if unchanged)
 *  @param[in] d Baud Rate divider (0 if unchanged)
 *  @note set wt to be used by the receiver timeout
 *  @note defined in ISO/IEC 7816-3:2006(E) section 8.1 and 10.2
 */
static void update_wt(uint8_t wi, uint8_t d)
{
	static uint8_t wt_wi = 10; /* Waiting time Integer (WI), used to calculate the Waiting Time (WT) */
	static uint8_t wt_d = 1; /* baud rate adjustment integer (the actual value, not the table index) */
	
	if (0 != wi) {
		wt_wi = wi;
	}
	if (0 != d) {
		wt_d = d;
	}
	wt = wt_wi * 960UL * wt_d;
	TRACE_INFO("WT updated to %lu ETU\n\r", wt);
}

/*! Allocate USB buffer and push + initialize simtrace_msg_hdr
 *  @param[in] ep USB IN endpoint where the message will be sent to
 *  @param[in] msg_class SIMtrace USB message class
 *  @param[in] msg_type SIMtrace USB message type
 *  @return USB message with allocated ans initialized header, or NULL if allocation failed
 */
static struct msgb *usb_msg_alloc_hdr(uint8_t ep, uint8_t msg_class, uint8_t msg_type)
{
	/* Only allocate message if not too many are already in the queue */
	struct llist_head *head = usb_get_queue(SIMTRACE_USB_EP_CARD_DATAIN);
	if (!head) {
		return NULL;
	}
	if (llist_count(head) > 5) {
		return NULL;
	}
	struct msgb *usb_msg = usb_buf_alloc(SIMTRACE_USB_EP_CARD_DATAIN);
	if (!usb_msg) {
		return NULL;
	}
	struct simtrace_msg_hdr *usb_msg_header;
	usb_msg->l1h = msgb_put(usb_msg, sizeof(*usb_msg_header));
	usb_msg_header = (struct simtrace_msg_hdr *) usb_msg->l1h;
	memset(usb_msg_header, 0, sizeof(*usb_msg_header));
	usb_msg_header->msg_class = msg_class;
	usb_msg_header->msg_type = msg_type;
	usb_msg->l2h = usb_msg->l1h + sizeof(*usb_msg_header);

	return usb_msg;
}

/* update SIMtrace header msg_len and submit USB buffer
 * param[in] usb_msg USB message to update and send
 */
void usb_msg_upd_len_and_submit(struct msgb *usb_msg)
{
	struct simtrace_msg_hdr *usb_msg_header = (struct simtrace_msg_hdr *) usb_msg->l1h;
	usb_msg_header->msg_len = msgb_length(usb_msg);
	usb_buf_submit(usb_msg);
}

/*! Update the ISO 7816-3 state
 *  @param[in] iso_state_new new ISO 7816-3 state to update to
 */
static void change_state(enum iso7816_3_sniff_state iso_state_new)
{
	/* sanity check */
	if (iso_state_new == iso_state) {
		TRACE_WARNING("Already in ISO 7816 state %u\n\r", iso_state);
		return;
	}

	/* handle actions to perform when switching state */
	switch (iso_state_new) {
	case ISO7816_S_RESET:
		update_fidi(&sniff_usart, 0x11); /* reset baud rate to default Di/Fi values */
		update_wt(10, 1); /* reset WT time-out */
		break;
	case ISO7816_S_WAIT_ATR:
		rbuf_reset(&sniff_buffer); /* reset buffer for new communication */
		break;
	case ISO7816_S_IN_ATR:
		atr_i = 0;
		convention_convert = false;
		t_protocol_support = 0;
		atr_state = ATR_S_WAIT_TS;
		break;
	case ISO7816_S_IN_PPS_REQ:
	case ISO7816_S_IN_PPS_RSP:
		pps_state = PPS_S_WAIT_PPSS;
		break;
	case ISO7816_S_WAIT_TPDU:
		tpdu_state = TPDU_S_CLA;
		tpdu_packet_i = 0;
		break;
	default:
		break;
	}

	/* save new state */
	iso_state = iso_state_new;
	TRACE_INFO("Changed to ISO 7816-3 state %u\n\r", iso_state);
}

const struct value_string data_flags[] = {
	{ SNIFF_DATA_FLAG_ERROR_INCOMPLETE,	"incomplete" },
	{ SNIFF_DATA_FLAG_ERROR_MALFORMED,	"malformed" },
	{ SNIFF_DATA_FLAG_ERROR_CHECKSUM,	"checksum error" },
	{ 0, NULL }
};

static void print_flags(const struct value_string* flag_meanings, uint32_t nb_flags, uint32_t flags) {
	uint32_t i;
	for (i = 0; i < nb_flags; i++) {
		if (flags & flag_meanings[i].value) {
			printf("%s", flag_meanings[i].str);
			flags &= ~flag_meanings[i].value;
			if (flags) {
				printf(", ");
			}
		}
	}
}

static void usb_send_data(enum simtrace_msg_type_sniff type, const uint8_t* data, uint16_t length, uint32_t flags)
{
	/* Sanity check */
	if (type != SIMTRACE_MSGT_SNIFF_ATR && type != SIMTRACE_MSGT_SNIFF_PPS && type != SIMTRACE_MSGT_SNIFF_TPDU) {
		return;
	}

	/* Show activity on LED */
	led_blink(LED_GREEN, BLINK_2F_O);

	/* Print message */
	switch (type) {
	case SIMTRACE_MSGT_SNIFF_ATR:
		printf("ATR");
		break;
	case SIMTRACE_MSGT_SNIFF_PPS:
		printf("PPS");
		break;
	case SIMTRACE_MSGT_SNIFF_TPDU:
		printf("TPDU");
		break;
	default:
		printf("???");
		break;
	}
	if (flags) {
		printf(" (");
		print_flags(data_flags, ARRAY_SIZE(data_flags), flags);
		putchar(')');
	}
	printf(": ");
	uint16_t i;
	for (i = 0; i < length; i++) {
		printf("%02x ", data[i]);
	}
	printf("\n\r");

	/* Send data over USB */
	struct msgb *usb_msg = usb_msg_alloc_hdr(SIMTRACE_USB_EP_CARD_DATAIN, SIMTRACE_MSGC_SNIFF, type);
	if (!usb_msg) {
		return;
	}
	struct sniff_data *usb_sniff_data = (struct sniff_data *) msgb_put(usb_msg, sizeof(*usb_sniff_data));
	usb_sniff_data->flags = flags;
	usb_sniff_data->length = length;
	uint8_t *sniff_data = msgb_put(usb_msg, usb_sniff_data->length);
	memcpy(sniff_data, data, length);
	usb_msg_upd_len_and_submit(usb_msg);

}

/*! Send current ATR over USB
 *  @param[in] flags SNIFF_DATA_FLAG_ data flags 
 *  @note Also print the ATR to debug console
 */
static void usb_send_atr(uint32_t flags)
{
	/* Check state */
	if (ISO7816_S_IN_ATR != iso_state) {
		TRACE_WARNING("Can't print ATR in ISO 7816-3 state %u\n\r", iso_state);
		return;
	}
	if (atr_i >= ARRAY_SIZE(atr)) {
		TRACE_ERROR("ATR buffer overflow\n\r");
		return;
	}

	/* Send ATR over USB */
	usb_send_data(SIMTRACE_MSGT_SNIFF_ATR, atr, atr_i, flags);
}

/*! Process ATR byte
 *  @param[in] byte ATR byte to process
 */
static void process_byte_atr(uint8_t byte)
{
	static uint8_t atr_hist_len = 0; /* store the number of expected historical bytes */
	static uint8_t y = 0; /* last mask of the upcoming TA, TB, TC, TD interface bytes */
	static uint8_t i = 0; /* interface byte subgroup number */
	static uint32_t flags = 0; /* error flag */

	/* sanity check */
	if (ISO7816_S_IN_ATR != iso_state) {
		TRACE_ERROR("Processing ATR data in wrong ISO 7816-3 state %u\n\r", iso_state);
		return;
	}
	if (atr_i >= ARRAY_SIZE(atr)) {
		TRACE_ERROR("ATR data overflow\n\r");
		return;
	}

	/* save data for use by other functions */
	atr[atr_i++] = byte;

	/* handle ATR byte depending on current state */
	switch (atr_state) {
	case ATR_S_WAIT_TS: /* see ISO/IEC 7816-3:2006 section 8.1 */
		flags = 0;
		switch (byte) {
		case 0x23: /* direct convention used, but decoded using inverse convention (a parity error should also have occurred) */
		case 0x30: /* inverse convention used, but decoded using direct convention (a parity error should also have occurred) */
			convention_convert = !convention_convert;
		case 0x3b: /* direct convention used and correctly decoded */
		case 0x3f: /* inverse convention used and correctly decoded */
			atr_state = ATR_S_WAIT_T0; /* wait for format byte */
			break;
		default:
			TRACE_WARNING("Invalid TS received\n\r");
			led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
			usb_send_atr(SNIFF_DATA_FLAG_ERROR_MALFORMED); /* send ATR to host software using USB */
			change_state(ISO7816_S_WAIT_ATR); /* reset state */
			break;
		}
		i = 0; /* first interface byte sub-group is coming (T0 is kind of TD0) */
		break;
	case ATR_S_WAIT_T0: /* see ISO/IEC 7816-3:2006 section 8.2.2 */
	case ATR_S_WAIT_TD: /* see ISO/IEC 7816-3:2006 section 8.2.3 */
		if (ATR_S_WAIT_T0 == atr_state) {
			atr_hist_len = (byte & 0x0f); /* save the number of historical bytes */
		} else if (ATR_S_WAIT_TD == atr_state) {
			t_protocol_support |= (1<<(byte & 0x0f)); /* remember supported protocol to know if TCK will be present */
		}
		y = (byte & 0xf0); /* remember upcoming interface bytes */
		i++; /* next interface byte sub-group is coming */
		if (y & 0x10) {
			atr_state = ATR_S_WAIT_TA; /* wait for interface byte TA */
			break;
		}
	case ATR_S_WAIT_TA: /* see ISO/IEC 7816-3:2006 section 8.2.3 */
		if (y & 0x20) {
			atr_state = ATR_S_WAIT_TB; /* wait for interface byte TB */
			break;
		}
	case ATR_S_WAIT_TB: /* see ISO/IEC 7816-3:2006 section 8.2.3 */
		if (y & 0x40) {
			atr_state = ATR_S_WAIT_TC; /* wait for interface byte TC */
			break;
		}
	case ATR_S_WAIT_TC: /* see ISO/IEC 7816-3:2006 section 8.2.3 */
		/* retrieve WI encoded in TC2*/
		if (ATR_S_WAIT_TC==atr_state && 2==i) {
			if (0 == byte) {
				update_wt(10, 0);
			} else {
				update_wt(byte, 0);
			}
		}
		if (y & 0x80) {
			atr_state = ATR_S_WAIT_TD; /* wait for interface byte TD */
			break;
		} else if (atr_hist_len) {
			atr_state = ATR_S_WAIT_HIST; /* wait for historical bytes */
			break;
		}
	case ATR_S_WAIT_HIST: /* see ISO/IEC 7816-3:2006 section 8.2.4 */
		if (atr_hist_len) {
			atr_hist_len--;
		}
		if (0 == atr_hist_len) {
			if (t_protocol_support > 1) {
				atr_state = ATR_S_WAIT_TCK; /* wait for check bytes */
				break;
			}
		} else {
			break;
		}
	case ATR_S_WAIT_TCK:  /* see ISO/IEC 7816-3:2006 section 8.2.5 */
		/* verify checksum if present */
		if (ATR_S_WAIT_TCK == atr_state) {
			uint8_t ui;
			uint8_t checksum = 0;
			for (ui = 1; ui < atr_i; ui++) {
				checksum ^= atr[ui];
			}
			if (checksum) {
				flags |= SNIFF_DATA_FLAG_ERROR_CHECKSUM;
				/* We still consider the data as valid (e.g. for WT) even is the checksum is wrong.
				 * It is up to the reader to handle this error (e.g. by resetting)
				 */
			}
		}
		usb_send_atr(flags); /* send ATR to host software using USB */
		change_state(ISO7816_S_WAIT_TPDU); /* go to next state */
		break;
	default:
		TRACE_INFO("Unknown ATR state %u\n\r", atr_state);
	}
}

/*! Send current PPS over USB
 *  @param[in] flags SNIFF_DATA_FLAG_ data flags
 *  @note Also print the PPS over the debug console
 */
static void usb_send_pps(uint32_t flags)
{
	uint8_t *pps_cur; /* current PPS (request or response) */

	/* Sanity check */
	if (ISO7816_S_IN_PPS_REQ == iso_state) {
		pps_cur = pps_req;
	} else if (ISO7816_S_IN_PPS_RSP == iso_state) {
		pps_cur = pps_rsp;
	} else {
		TRACE_ERROR("Can't print PPS in ISO 7816-3 state %u\n\r", iso_state);
		return;
	}

	/* Get only relevant data */
	uint8_t pps[6];
	uint8_t pps_i = 0;
	if (pps_state > PPS_S_WAIT_PPSS) {
		pps[pps_i++] = pps_cur[0];
	}
	if (pps_state > PPS_S_WAIT_PPS0) {
		pps[pps_i++] = pps_cur[1];
	}
	if (pps_state > PPS_S_WAIT_PPS1 && pps_cur[1] & 0x10) {
		pps[pps_i++] = pps_cur[2];
	}
	if (pps_state > PPS_S_WAIT_PPS2 && pps_cur[1] & 0x20) {
		pps[pps_i++] = pps_cur[3];
	}
	if (pps_state > PPS_S_WAIT_PPS3 && pps_cur[1] & 0x40) {
		pps[pps_i++] = pps_cur[4];
	}
	if (pps_state > PPS_S_WAIT_PCK) {
		pps[pps_i++] = pps_cur[5];
	}

	/* Send message over USB */
	usb_send_data(SIMTRACE_MSGT_SNIFF_PPS, pps, pps_i, flags);
}

/*! Send Fi/Di change over USB
 *  @param[in] fidi Fi/Di factor as encoded in TA1 
 */
static void usb_send_fidi(uint8_t fidi)
{
	/* Send message over USB */
	struct msgb *usb_msg = usb_msg_alloc_hdr(SIMTRACE_USB_EP_CARD_DATAIN, SIMTRACE_MSGC_SNIFF, SIMTRACE_MSGT_SNIFF_FIDI);
	if (!usb_msg) {
		return;
	}
	struct sniff_fidi *usb_sniff_fidi = (struct sniff_fidi *) msgb_put(usb_msg, sizeof(*usb_sniff_fidi));
	usb_sniff_fidi->fidi = fidi;
	usb_msg_upd_len_and_submit(usb_msg);
}

static void process_byte_pps(uint8_t byte)
{
	uint8_t *pps_cur; /* current PPS (request or response) */
	static uint32_t flags = 0; /* error flag */

	/* sanity check */
	if (ISO7816_S_IN_PPS_REQ == iso_state) {
		pps_cur = pps_req;
	} else if (ISO7816_S_IN_PPS_RSP == iso_state) {
		pps_cur = pps_rsp;
	} else {
		TRACE_ERROR("Processing PPS data in wrong ISO 7816-3 state %u\n\r", iso_state);
		return;
	}

	/* handle PPS byte depending on current state */
	switch (pps_state) { /* see ISO/IEC 7816-3:2006 section 9.2 */
	case PPS_S_WAIT_PPSS: /*!< initial byte */
		flags = 0;
		if (0xff) {
			pps_cur[0] = byte;
			pps_state = PPS_S_WAIT_PPS0; /* go to next state */
		} else {
			TRACE_INFO("Invalid PPSS received\n\r");
			led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
			usb_send_pps(SNIFF_DATA_FLAG_ERROR_MALFORMED); /* send ATR to host software using USB */
			change_state(ISO7816_S_WAIT_TPDU); /* go back to TPDU state */
		}
		break;
	case PPS_S_WAIT_PPS0: /*!< format byte */
		pps_cur[1] = byte;
		if (pps_cur[1] & 0x10) {
			pps_state = PPS_S_WAIT_PPS1; /* go to next state */
			break;
		}
	case PPS_S_WAIT_PPS1: /*!< first parameter byte */
		pps_cur[2] = byte; /* not always right but doesn't affect the process */
		if (pps_cur[1] & 0x20) {
			pps_state = PPS_S_WAIT_PPS2; /* go to next state */
			break;
		}
	case PPS_S_WAIT_PPS2: /*!< second parameter byte */
		pps_cur[3] = byte; /* not always right but doesn't affect the process */
		if (pps_cur[1] & 0x40) {
			pps_state = PPS_S_WAIT_PPS3; /* go to next state */
			break;
		}
	case PPS_S_WAIT_PPS3: /*!< third parameter byte */
		pps_cur[4] = byte; /* not always right but doesn't affect the process */
		pps_state = PPS_S_WAIT_PCK; /* go to next state */
		break;
	case PPS_S_WAIT_PCK: /*!< check byte */
		pps_cur[5] = byte; /* not always right but doesn't affect the process */
		/* verify the checksum */
		uint8_t check = 0; 
		check ^= pps_cur[0];
		check ^= pps_cur[1];
		if (pps_cur[1] & 0x10) {
			check ^= pps_cur[2];
		}
		if (pps_cur[1] & 0x20) {
			check ^= pps_cur[3];
		}
		if (pps_cur[1] & 0x40) {
			check ^= pps_cur[4];
		}
		check ^= pps_cur[5];
		if (check) {
			flags |= SNIFF_DATA_FLAG_ERROR_CHECKSUM;
		}
		pps_state = PPS_S_WAIT_END;
		usb_send_pps(flags); /* send PPS to host software using USB */
		if (ISO7816_S_IN_PPS_REQ == iso_state) {
			if (0 == check) { /* checksum is valid */
				change_state(ISO7816_S_WAIT_PPS_RSP); /* go to next state */
			} else { /* checksum is invalid */
				change_state(ISO7816_S_WAIT_TPDU); /* go to next state */
			}
		} else if (ISO7816_S_IN_PPS_RSP == iso_state) {
			if (0 == check) { /* checksum is valid */
				uint8_t fn, dn;
				if (pps_cur[1] & 0x10) {
					fn = (pps_cur[2] >> 4);
					dn = (pps_cur[2] & 0x0f);
				} else {
					fn = 1;
					dn = 1;
				}
				TRACE_INFO("PPS negotiation successful: Fn=%u Dn=%u\n\r",
					   iso7816_3_fi_table[fn], iso7816_3_di_table[dn]);
				update_fidi(&sniff_usart, pps_cur[2]);
				update_wt(0, iso7816_3_di_table[dn]);
				usb_send_fidi(pps_cur[2]); /* send Fi/Di change notification to host software over USB */
			} else { /* checksum is invalid */
				TRACE_INFO("PPS negotiation failed\n\r");
			}
			change_state(ISO7816_S_WAIT_TPDU); /* go to next state */
		}
		break;
	case PPS_S_WAIT_END:
		TRACE_WARNING("Unexpected PPS received %u\n\r", pps_state);
		break;
	default:
		TRACE_WARNING("Unknown PPS state %u\n\r", pps_state);
		break;
	}
}

/*! Send current TPDU over USB
 *  @param[in] flags SNIFF_DATA_FLAG_ data flags
 *  @note Also print the TPDU over the debug console
 */
static void usb_send_tpdu(uint32_t flags)
{
	/* Check state */
	if (ISO7816_S_IN_TPDU != iso_state) {
		TRACE_WARNING("Can't print TPDU in ISO 7816-3 state %u\n\r", iso_state);
		return;
	}

	/* Send ATR over USB */
	usb_send_data(SIMTRACE_MSGT_SNIFF_TPDU, tpdu_packet, tpdu_packet_i, flags);
}

static void process_byte_tpdu(uint8_t byte)
{
	/* sanity check */
	if (ISO7816_S_IN_TPDU != iso_state) {
		TRACE_ERROR("Processing TPDU data in wrong ISO 7816-3 state %u\n\r", iso_state);
		return;
	}
	if (tpdu_packet_i >= ARRAY_SIZE(tpdu_packet)) {
		TRACE_ERROR("TPDU data overflow\n\r");
		return;
	}

	/* handle TPDU byte depending on current state */
	switch (tpdu_state) {
	case TPDU_S_CLA:
		if (0xff == byte) {
			TRACE_WARNING("0xff is not a valid class byte\n\r");
			led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
			usb_send_tpdu(SNIFF_DATA_FLAG_ERROR_MALFORMED); /* send ATR to host software using USB */
			change_state(ISO7816_S_WAIT_TPDU); /* go back to TPDU state */
			return;
		}
		tpdu_packet_i = 0;
		tpdu_packet[tpdu_packet_i++] = byte;
		tpdu_state = TPDU_S_INS;
		break;
	case TPDU_S_INS:
		if ((0x60 == (byte & 0xf0)) || (0x90 == (byte & 0xf0))) {
			TRACE_WARNING("invalid INS 0x%02x\n\r", byte);
			led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
			usb_send_tpdu(SNIFF_DATA_FLAG_ERROR_MALFORMED); /* send ATR to host software using USB */
			change_state(ISO7816_S_WAIT_TPDU); /* go back to TPDU state */
			return;
		}
		tpdu_packet_i = 1;
		tpdu_packet[tpdu_packet_i++] = byte;
		tpdu_state = TPDU_S_P1;
		break;
	case TPDU_S_P1:
		tpdu_packet_i = 2;
		tpdu_packet[tpdu_packet_i++] = byte;
		tpdu_state = TPDU_S_P2;
		break;
	case TPDU_S_P2:
		tpdu_packet_i = 3;
		tpdu_packet[tpdu_packet_i++] = byte;
		tpdu_state = TPDU_S_P3;
		break;
	case TPDU_S_P3:
		tpdu_packet_i = 4;
		tpdu_packet[tpdu_packet_i++] = byte;
		tpdu_state = TPDU_S_PROCEDURE;
		break;
	case TPDU_S_PROCEDURE:
		if (0x60 == byte) { /* wait for next procedure byte */
			break;
		} else if (tpdu_packet[1] == byte) { /* get all remaining data bytes */
			tpdu_state = TPDU_S_DATA_REMAINING;
			break;
		} else if ((~tpdu_packet[1]) == byte) { /* get single data byte */
			tpdu_state = TPDU_S_DATA_SINGLE;
			break;
		}
	case TPDU_S_SW1:
		if ((0x60 == (byte & 0xf0)) || (0x90 == (byte & 0xf0))) { /* this procedure byte is SW1 */
			tpdu_packet[tpdu_packet_i++] = byte;
			tpdu_state = TPDU_S_SW2;
		} else {
			TRACE_WARNING("invalid SW1 0x%02x\n\r", byte);
			led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
			usb_send_tpdu(SNIFF_DATA_FLAG_ERROR_MALFORMED); /* send ATR to host software using USB */
			change_state(ISO7816_S_WAIT_TPDU); /* go back to TPDU state */
			return;
		}
		break;
	case TPDU_S_SW2:
		tpdu_packet[tpdu_packet_i++] = byte;
		usb_send_tpdu(0); /* send TPDU to host software using USB */
		change_state(ISO7816_S_WAIT_TPDU); /* this is the end of the TPDU */
		break;
	case TPDU_S_DATA_SINGLE:
	case TPDU_S_DATA_REMAINING:
		tpdu_packet[tpdu_packet_i++] = byte;
		if (0 == tpdu_packet[4]) {
			if (5+256 <= tpdu_packet_i) {
				tpdu_state = TPDU_S_PROCEDURE;
			}
		} else {
			if (5+tpdu_packet[4] <= tpdu_packet_i) {
				tpdu_state = TPDU_S_PROCEDURE;
			}
		}
		if (TPDU_S_DATA_SINGLE == tpdu_state) {
			tpdu_state = TPDU_S_PROCEDURE;
		}
		break;
	default:
		TRACE_ERROR("unhandled TPDU state %u\n\r", tpdu_state);
	}
}

/*! Interrupt Service Routine called on USART activity */
void Sniffer_usart_isr(void)
{
	/* Remaining Waiting Time (WI) counter (>16 bits) */
	static volatile uint32_t wt_remaining = 9600;

	/* Read channel status register */
	uint32_t csr = sniff_usart.base->US_CSR;
	/* Verify if there was an error */
	if (csr & US_CSR_OVRE)
		TRACE_ERROR("USART overrun error\n\r");
	if (csr & US_CSR_FRAME)
		TRACE_ERROR("USART framing error\n\r");
	if (csr & US_CSR_PARE)
		TRACE_ERROR("USART parity error\n\r");
	if (csr & (US_CSR_OVRE|US_CSR_FRAME|US_CSR_PARE))
		sniff_usart.base->US_CR |= US_CR_RSTSTA;

	/* Verify if character has been received */
	if (csr & US_CSR_RXRDY) {
		/* Read communication data byte between phone and SIM */
		uint8_t byte = sniff_usart.base->US_RHR;
		/* Reset WT timer */
		wt_remaining = wt;
		/* Store sniffed data into buffer (also clear interrupt */
		if (rbuf_is_full(&sniff_buffer)) {
			TRACE_ERROR("USART buffer full\n\r");
		} else {
			rbuf_write(&sniff_buffer, byte);
		}
	}

	/* Verify it WT timeout occurred, to detect unresponsive card */
	if (csr & US_CSR_TIMEOUT) {
		if (wt_remaining <= (sniff_usart.base->US_RTOR & 0xffff)) {
			/* Just set the flag and let the main loop handle it */
			change_flags |= SNIFF_CHANGE_FLAG_TIMEOUT_WT;
			/* Reset timeout value */
			wt_remaining = wt;
		} else {
			wt_remaining -= (sniff_usart.base->US_RTOR & 0xffff); /* be sure to subtract the actual timeout since the new might not have been set and reloaded yet */
		}
		if (wt_remaining > 0xffff) {
			sniff_usart.base->US_RTOR = 0xffff;
		} else {
			sniff_usart.base->US_RTOR = wt_remaining;
		}
		/* Stop timeout until next character is received (and clears the timeout flag) */
		sniff_usart.base->US_CR |= US_CR_STTTO;
		if (!(change_flags & SNIFF_CHANGE_FLAG_TIMEOUT_WT)) {
			/* Immediately restart the counter it the WT timeout did not occur (needs the timeout flag to be cleared) */
			sniff_usart.base->US_CR |= US_CR_RETTO;
		}
	}
}

/** PIO interrupt service routine to checks if the card reset line has changed
 */
static void Sniffer_reset_isr(const Pin* pPin)
{
	/* Ensure an edge on the reset pin cause the interrupt */
	if (pPin->id != pin_rst.id || 0 == (pPin->mask & pin_rst.mask)) {
		TRACE_ERROR("Pin other than reset caused a interrupt\n\r");
		return;
	}
	/* Update the ISO state according to the reset change (reset is active low) */
	if (PIO_Get(&pin_rst)) {
		change_flags |= SNIFF_CHANGE_FLAG_RESET_DEASSERT; /* set flag and let main loop send it */
	} else {
		change_flags |= SNIFF_CHANGE_FLAG_RESET_ASSERT; /* set flag and let main loop send it */
	}
}

/*------------------------------------------------------------------------------
 *         Global functions
 *------------------------------------------------------------------------------*/

void Sniffer_usart1_irq(void)
{
	if (ID_USART1 == sniff_usart.id) {
		Sniffer_usart_isr();
	}
}

void Sniffer_usart0_irq(void)
{
	if (ID_USART0 == sniff_usart.id) {
		Sniffer_usart_isr();
	}
}

/*-----------------------------------------------------------------------------
 *          Initialization routine
 *-----------------------------------------------------------------------------*/

/* Called during USB enumeration after device is enumerated by host */
void Sniffer_configure(void)
{
	TRACE_INFO("Sniffer config\n\r");
}

/* called when *different* configuration is set by host */
void Sniffer_exit(void)
{
	TRACE_INFO("Sniffer exit\n\r");
	/* Disable USART */
	USART_DisableIt(sniff_usart.base, US_IER_RXRDY);
	/* NOTE: don't forget to set the IRQ according to the USART peripheral used */
	NVIC_DisableIRQ(IRQ_USART_SIM);
	USART_SetReceiverEnabled(sniff_usart.base, 0);
	/* Disable RST IRQ */
	PIO_DisableIt(&pin_rst);
	NVIC_DisableIRQ(PIOA_IRQn); /* CAUTION this needs to match to the correct port */
}

/* called when *Sniffer* configuration is set by host */
void Sniffer_init(void)
{
	TRACE_INFO("Sniffer Init\n\r");

	/* Configure pins to sniff communication between phone and card */
	PIO_Configure(pins_sniff, PIO_LISTSIZE(pins_sniff));
	/* Configure pins to connect phone to card */
	PIO_Configure(pins_bus, PIO_LISTSIZE(pins_bus));
	/* Configure pins to forward phone power to card */
	PIO_Configure(pins_power, PIO_LISTSIZE(pins_power));
	/* Enable interrupts on port with reset line */
	NVIC_EnableIRQ(PIOA_IRQn); /* CAUTION this needs to match to the correct port */
	/* Register ISR to handle card reset change */
	PIO_ConfigureIt(&pin_rst, &Sniffer_reset_isr);
	/* Enable interrupt on card reset pin */
	PIO_EnableIt(&pin_rst);

	/* Clear ring buffer containing the sniffed data */
	rbuf_reset(&sniff_buffer);
	/* Configure USART to as ISO-7816 slave communication to sniff communication */
	ISO7816_Init(&sniff_usart, CLK_SLAVE);
	/* Only receive data when sniffing */
	USART_SetReceiverEnabled(sniff_usart.base, 1);
	/* Enable Receiver time-out to detect waiting time (WT) time-out (e.g. unresponsive cards) */
	sniff_usart.base->US_RTOR = wt;
	/* Enable interrupt to indicate when data has been received or timeout occurred */
	USART_EnableIt(sniff_usart.base, US_IER_RXRDY | US_IER_TIMEOUT);
	/* Set USB priority lower than USART to not miss sniffing data (both at 0 per default) */
	if (NVIC_GetPriority(IRQ_USART_SIM) >= NVIC_GetPriority(UDP_IRQn)) {
		NVIC_SetPriority(UDP_IRQn, NVIC_GetPriority(IRQ_USART_SIM) + 2);
	}
	/* Enable interrupt requests for the USART peripheral */
	NVIC_EnableIRQ(IRQ_USART_SIM);

	/* Reset state */
	if (ISO7816_S_RESET != iso_state) {
		change_state(ISO7816_S_RESET);
	}
}

/*! Send card change flags over USB
 *  @param[in] flags change flags corresponding to SIMTRACE_MSGT_SNIFF_CHANGE 
 */
static void usb_send_change(uint32_t flags)
{
	/* Check flags */
	if(0 == flags) { /* no changes */
		return;
	}

	if (flags & SNIFF_CHANGE_FLAG_TIMEOUT_WT) {
		printf("waiting time (WT) timeout\n\r");
	}

	/* Send message over USB */
	struct msgb *usb_msg = usb_msg_alloc_hdr(SIMTRACE_USB_EP_CARD_DATAIN, SIMTRACE_MSGC_SNIFF, SIMTRACE_MSGT_SNIFF_CHANGE);
	if (!usb_msg) {
		return;
	}
	struct sniff_change *usb_sniff_change = (struct sniff_change *) msgb_put(usb_msg, sizeof(*usb_sniff_change));
	usb_sniff_change->flags = flags;
	usb_msg_upd_len_and_submit(usb_msg);
}

/* Main (idle/busy) loop of this USB configuration */
void Sniffer_run(void)
{
	/* Handle USB queue */
	/* first try to send any pending messages on INT */
	usb_refill_to_host(SIMTRACE_USB_EP_CARD_INT);
	/* then try to send any pending messages on IN */
	usb_refill_to_host(SIMTRACE_USB_EP_CARD_DATAIN);
	/* ensure we can handle incoming USB messages from the host */
	/* currently we don't need any incoming data
	usb_refill_from_host(SIMTRACE_USB_EP_CARD_DATAOUT);
	struct llist_head *queue = usb_get_queue(SIMTRACE_USB_EP_CARD_DATAOUT);
	process_any_usb_commands(queue);
	*/

	/* WARNING: the signal data and flags are not synchronized. We have to hope 
	 * the processing is fast enough to not land in the wrong state while data
	 * is remaining
	 */
	/* Handle sniffed data */
	if (!rbuf_is_empty(&sniff_buffer)) { /* use if instead of while to let the main loop restart the watchdog */
		uint8_t byte = rbuf_read(&sniff_buffer);
		/* Convert convention if required */
		if (convention_convert) {
			byte = convention_convert_lut[byte];
		}
		//TRACE_ERROR_WP(">%02x", byte);
		switch (iso_state) { /* Handle byte depending on state */
		case ISO7816_S_RESET: /* During reset we shouldn't receive any data */
			break;
		case ISO7816_S_WAIT_ATR: /* After a reset we expect the ATR */
			change_state(ISO7816_S_IN_ATR); /* go to next state */
		case ISO7816_S_IN_ATR: /* More ATR data incoming */
			process_byte_atr(byte);
			break;
		case ISO7816_S_WAIT_TPDU: /* After the ATR we expect TPDU or PPS data */
		case ISO7816_S_WAIT_PPS_RSP:
			if (0xff == byte) {
				if (ISO7816_S_WAIT_PPS_RSP == iso_state) {
					change_state(ISO7816_S_IN_PPS_RSP); /* Go to PPS state */
				} else {
					change_state(ISO7816_S_IN_PPS_REQ); /* Go to PPS state */
				}
				process_byte_pps(byte);
				break;
			}
		case ISO7816_S_IN_TPDU: /* More TPDU data incoming */
			if (ISO7816_S_WAIT_TPDU == iso_state) {
				change_state(ISO7816_S_IN_TPDU);
			}
			process_byte_tpdu(byte);
			break;
		case ISO7816_S_IN_PPS_REQ:
		case ISO7816_S_IN_PPS_RSP:
			process_byte_pps(byte);
			break;
		default:
			TRACE_ERROR("Data received in unknown state %u\n\r", iso_state);
		}
	}

	/* Handle flags */
	if (change_flags) { /* WARNING this is not synced with the data buffer handling */
		if (change_flags & SNIFF_CHANGE_FLAG_RESET_ASSERT) {
			switch (iso_state) {
			case ISO7816_S_IN_ATR:
				led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
				usb_send_atr(SNIFF_DATA_FLAG_ERROR_INCOMPLETE); /* send incomplete ATR to host software using USB */
				break;
			case ISO7816_S_IN_TPDU:
				led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
				usb_send_tpdu(SNIFF_DATA_FLAG_ERROR_INCOMPLETE); /* send incomplete PPS to host software using USB */
				break;
			case ISO7816_S_IN_PPS_REQ:
			case ISO7816_S_IN_PPS_RSP:
				led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
				usb_send_pps(SNIFF_DATA_FLAG_ERROR_INCOMPLETE); /* send incomplete TPDU to host software using USB */
				break;
			default:
				break;
			}
			if (ISO7816_S_RESET != iso_state) {
				change_state(ISO7816_S_RESET);
				printf("reset asserted\n\r");
			}
		}
		if (change_flags & SNIFF_CHANGE_FLAG_RESET_DEASSERT) {
			if (ISO7816_S_WAIT_ATR != iso_state) {
				change_state(ISO7816_S_WAIT_ATR);
				printf("reset de-asserted\n\r");
			}
		}
		if (change_flags & SNIFF_CHANGE_FLAG_TIMEOUT_WT) {
			/* Use timeout to detect interrupted data transmission */
			switch (iso_state) {
			case ISO7816_S_IN_ATR:
				led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
				usb_send_atr(SNIFF_DATA_FLAG_ERROR_INCOMPLETE); /* send incomplete ATR to host software using USB */
				change_state(ISO7816_S_WAIT_ATR);
				break;
			case ISO7816_S_IN_TPDU:
				led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
				usb_send_tpdu(SNIFF_DATA_FLAG_ERROR_INCOMPLETE); /* send incomplete PPS to host software using USB */
				change_state(ISO7816_S_WAIT_TPDU);
				break;
			case ISO7816_S_IN_PPS_REQ:
			case ISO7816_S_IN_PPS_RSP:
				led_blink(LED_RED, BLINK_2F_O); /* indicate error to user */
				usb_send_pps(SNIFF_DATA_FLAG_ERROR_INCOMPLETE); /* send incomplete TPDU to host software using USB */
				change_state(ISO7816_S_WAIT_TPDU);
				break;
			default:
				change_flags &= ~SNIFF_CHANGE_FLAG_TIMEOUT_WT; /* We don't care about the timeout is all other cases */
				break;
			}
		}
		if (change_flags) {
			usb_send_change(change_flags); /* send timeout to host software over USB */
			change_flags = 0; /* Reset flags */
		}
	}
}
#endif /* HAVE_SNIFFER */
