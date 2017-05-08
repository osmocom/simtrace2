#pragma once

#include <stdint.h>

/* SIMtrace2 USB protocol */

/* (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/***********************************************************************
 * COMMON HEADER
 ***********************************************************************/

enum simtrace_msg_class {
	SIMTRACE_MSGC_GENERIC = 0,
	/* Card Emulation / Forwarding */
	SIMTRACE_MSGC_CARDEM,
	/* Modem Control (if modem is attached next to device */
	SIMTRACE_MSGC_MODEM,
	/* SIM protocol tracing */
	SIMTRACE_MSGC_TRACE,

	/* first vendor-specific request */
	_SIMTRACE_MGSC_VENDOR_FIRST = 127,
};

enum simtrace_msg_type_generic {
	/* Generic Error Message */
	SIMTRACE_CMD_DO_ERROR	= 0,
	/* Request/Response for simtrace_board_info */
	SIMTRACE_CMD_BD_BOARD_INFO,
};

/* SIMTRACE_MSGC_CARDEM */
enum simtrace_msg_type_cardem {
	/* TPDU Data to be transmitted to phone */
	SIMTRACE_MSGT_DT_CEMU_TX_DATA = 1,
	/* Set the ATR to be returned at phone-SIM reset */
	SIMTRACE_MSGT_DT_CEMU_SET_ATR,
	/* Get Statistics Request / Response */
	SIMTRACE_MSGT_BD_CEMU_STATS,
	/* Get Status Request / Response */
	SIMTRACE_MSGT_BD_CEMU_STATUS,
	/* Request / Confirm emulated card insert */
	SIMTRACE_MSGT_DT_CEMU_CARDINSERT,
	/* TPDU Data received from phomne */
	SIMTRACE_MSGT_DO_CEMU_RX_DATA,
	/* Indicate PTS request from phone */
	SIMTRACE_MSGT_DO_CEMU_PTS,
};

/* SIMTRACE_MSGC_MODEM */
enum simtrace_msg_type_modem {
	/* Modem Control: Reset an attached modem */
	SIMTRACE_MSGT_DT_MODEM_RESET = 1,
	/* Modem Control: Select local / remote SIM */
	SIMTRACE_MSGT_DT_MODEM_SIM_SELECT,
	/* Modem Control: Status (WWAN LED, SIM Presence) */
	SIMTRACE_MSGT_BD_MODEM_STATUS,
};

/* SIMTRACE_MSGC_TRACE */
enum simtrace_msg_type_trace {
	/* FIXME */
	_dummy,
};

/* common message header */
struct simtrace_msg_hdr {
	uint8_t msg_class;	/* simtrace_msg_class */
	uint8_t msg_type;	/* simtrace_msg_type_xxx */
	uint8_t seq_nr;
	uint8_t slot_nr;	/* SIM slot number */
	uint16_t _reserved;
	uint16_t msg_len;	/* length including header */
	uint8_t payload[0];
} __attribute__ ((packed));

/***********************************************************************
 * CARD EMULATOR / FORWARDER
 ***********************************************************************/

/* generic capabilities */
enum simtrace_capability_generic {
	/* compatible with 5V SIM card interface */
	SIMTRACE_CAP_VOLT_5V,
	/* compatible with 3.3V SIM card interface */
	SIMTRACE_CAP_VOLT_3V3,
	/* compatible with 1.8V SIM card interface */
	SIMTRACE_CAP_VOLT_1V8,
	/* Has LED1 */
	SIMTRACE_CAP_LED_1,
	/* Has LED2 */
	SIMTRACE_CAP_LED_2,
	/* Has Single-Pole Dual-Throw (local/remote SIM */
	SIMTRACE_CAP_SPDT,
	/* Has Bus-Switch (trace / MITM) */
	SIMTRACE_CAP_BUS_SWITCH,
	/* Can read VSIM via ADC */
	SIMTRACE_CAP_VSIM_ADC,
	/* Can read temperature via ADC */
	SIMTRACE_CAP_TEMP_ADC,
	/* Supports DFU for firmware update */
	SIMTRACE_CAP_DFU,
	/* Supports Ctrl EP command for erasing flash / return to SAM-BA */
	SIMTRACE_CAP_ERASE_FLASH,
	/* Can read the status of card insert contact */
	SIMTRACE_CAP_READ_CARD_DET,
	/* Can control the status of a simulated card insert */
	SIMTRACE_CAP_ASSERT_CARD_DET,
	/* Can toggle the hardware reset of an attached modem */
	SIMTRACE_CAP_ASSERT_MODEM_RST,
};

/* vendor-specific capabilities of sysmoocm devices */
enum simtrace_capability_vendor {
	/* Can erase a peer SAM3 controller */
	SIMTRACE_CAP_SYSMO_QMOD_ERASE_PEER,
	/* Can read/write an attached EEPROM */
	SIMTRACE_CAP_SYSMO_QMOD_RW_EEPROM,
	/* can reset an attached USB hub */
	SIMTRACE_CAP_SYSMO_QMOD_RESET_HUB,
};


/* SIMTRACE_CMD_BD_BOARD_INFO */
struct simtrace_board_info {
	struct {
		char manufacturer[32];
		char model[32];
		char version[32];
	} hardware;
	struct {
		/* who provided this software? */
		char provider[32];
		/* name of software image */
		char name[32];
		/* (git) version at build time */
		char version[32];
		/* built on which machine? */
		char buildhost[32];
		/* CRC-32 over software image */
		uint32_t crc;
	} software;
	struct {
		/* Maximum baud rate supported */
		uint32_t max_baud_rate;
	} speed;
	/* number of bytes of generic capability bit-mask */
	uint8_t cap_generic_bytes;
	/* number of bytes of vendor capability bit-mask */
	uint8_t cap_vendor_bytes;
	uint8_t data[0];
	/* cap_generic + cap_vendor */
} __attribute__ ((packed));

/***********************************************************************
 * CARD EMULATOR / FORWARDER
 ***********************************************************************/

/* indicates a TPDU header is present in this message */
#define CEMU_DATA_F_TPDU_HDR	0x00000001
/* indicates last part of transmission in this direction */
#define CEMU_DATA_F_FINAL	0x00000002
/* incdicates a PB is present and we should continue with TX */
#define CEMU_DATA_F_PB_AND_TX	0x00000004
/* incdicates a PB is present and we should continue with RX */
#define CEMU_DATA_F_PB_AND_RX	0x00000008

/* CEMU_USB_MSGT_DT_CARDINSERT */
struct cardemu_usb_msg_cardinsert {
	uint8_t card_insert;
} __attribute__ ((packed));

/* CEMU_USB_MSGT_DT_SET_ATR */
struct cardemu_usb_msg_set_atr {
	uint8_t atr_len;
	/* variable-length ATR data */
	uint8_t atr[0];
} __attribute__ ((packed));

/* CEMU_USB_MSGT_DT_TX_DATA */
struct cardemu_usb_msg_tx_data {
	uint32_t flags;
	uint16_t data_len;
	/* variable-length TPDU data */
	uint8_t data[0];
} __attribute__ ((packed));

/* CEMU_USB_MSGT_DO_RX_DATA */
struct cardemu_usb_msg_rx_data {
	uint32_t flags;
	uint16_t data_len;
	/* variable-length TPDU data */
	uint8_t data[0];
} __attribute__ ((packed));

#define CEMU_STATUS_F_VCC_PRESENT	0x00000001
#define CEMU_STATUS_F_CLK_ACTIVE	0x00000002
#define CEMU_STATUS_F_RCEMU_ACTIVE	0x00000004
#define CEMU_STATUS_F_CARD_INSERT	0x00000008
#define CEMU_STATUS_F_RESET_ACTIVE	0x00000010

/* CEMU_USB_MSGT_DO_STATUS */
struct cardemu_usb_msg_status {
	uint32_t flags;
	/* phone-applied target voltage in mV */
	uint16_t voltage_mv;
	/* Fi/Di related information */
	uint8_t fi;
	uint8_t di;
	uint8_t wi;
	uint32_t waiting_time;
} __attribute__ ((packed));

/* CEMU_USB_MSGT_DO_PTS */
struct cardemu_usb_msg_pts_info {
	uint8_t pts_len;
	/* PTS request as sent from reader */
	uint8_t req[6];
	/* PTS response as sent by card */
	uint8_t resp[6];
} __attribute__ ((packed));

/* CEMU_USB_MSGT_DO_ERROR */
struct cardemu_usb_msg_error {
	uint8_t severity;
	uint8_t subsystem;
	uint16_t code;
	uint8_t msg_len;
	/* human-readable error message */
	uint8_t msg[0];
} __attribute__ ((packed));

/***********************************************************************
 * MODEM CONTROL
 ***********************************************************************/

/* SIMTRACE_MSGT_DT_MODEM_RESET */
struct st_modem_reset {
	/* 0: de-assert reset, 1: assert reset, 2: poulse reset */
	uint8_t asserted;
	/* if above is '2', duration of pulse in ms */
	uint16_t pulse_duration_msec;
} __attribute__((packed));

/* SIMTRACE_MSGT_DT_MODEM_SIM_SELECT */
struct st_modem_sim_select {
	/* remote (1), local (0) */
	uint8_t remote_sim;
} __attribute__((packed));

/* SIMTRACE_MSGT_BD_MODEM_STATUS */
#define ST_MDM_STS_BIT_WWAN_LED		(1 << 0)
#define ST_MDM_STS_BIT_CARD_INSERTED	(1 << 1)
struct st_modem_status {
	/* bit-field of supported status bits */
	uint8_t supported_mask;
	/* bit-field of current status bits */
	uint8_t status_mask;
	/* bit-field of changed status bits */
	uint8_t changed_mask;
} __attribute__((packed));
