/* simtrace2-cardem-pcsc - main program for the host PC to provide a remote SIM
 * using the SIMtrace 2 firmware in card emulation mode
 *
 * (C) 2016-2022 by Harald Welte <hwelte@hmw-consulting.de>
 * (C) 2018, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#define _GNU_SOURCE
#include <getopt.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libusb.h>

#include <osmocom/usb/libusb.h>
#include <osmocom/simtrace2/simtrace2_api.h>
#include <osmocom/simtrace2/simtrace_prot.h>
#include <osmocom/simtrace2/apdu_dispatch.h>
#include <osmocom/simtrace2/gsmtap.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/select.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>
#include <osmocom/sim/class_tables.h>
#include <osmocom/sim/sim.h>

#define LOGCI(ci, lvl, fmt, args ...) LOGP(DLGLOBAL, lvl,  fmt, ## args)

static void atr_update_csum(uint8_t *atr, unsigned int atr_len)
{
	uint8_t csum = 0;
	int i;

	for (i = 1; i < atr_len - 1; i++)
		csum = csum ^ atr[i];

	atr[atr_len-1] = csum;
}

static void cemu_status_flags2str(char *out, unsigned int out_len, uint32_t flags)
{
	snprintf(out, out_len, "%s%s%s%s%s",
		 flags & CEMU_STATUS_F_RESET_ACTIVE ? "RESET " : "",
		 flags & CEMU_STATUS_F_VCC_PRESENT ? "VCC " : "",
		 flags & CEMU_STATUS_F_CLK_ACTIVE ? "CLK " : "",
		 flags & CEMU_STATUS_F_CARD_INSERT ? "CARD_PRES " : "",
		 flags & CEMU_STATUS_F_RCEMU_ACTIVE ? "RCEMU " : "");
}

static uint32_t last_status_flags = 0;

#define NO_RESET 0
#define COLD_RESET 1
#define WARM_RESET 2

static void update_status_flags(struct osmo_st2_cardem_inst *ci, uint32_t flags)
{
	struct osim_card_hdl *card = ci->chan->card;
	int reset = NO_RESET;

	/* check if card is _now_ operational: VCC+CLK present, RST absent */
	if ((flags & CEMU_STATUS_F_VCC_PRESENT) && (flags & CEMU_STATUS_F_CLK_ACTIVE) &&
	    !(flags & CEMU_STATUS_F_RESET_ACTIVE)) {
		if (last_status_flags & CEMU_STATUS_F_RESET_ACTIVE) {
			/* a reset has just ended, forward it to the real card */
			if (last_status_flags & CEMU_STATUS_F_VCC_PRESENT)
				reset = WARM_RESET;
			else
				reset = COLD_RESET;
		} else if (!(last_status_flags & CEMU_STATUS_F_VCC_PRESENT)) {
			/* power-up has just happened, perform cold reset */
			reset = COLD_RESET;
		}
	} else if (flags == CEMU_STATUS_F_VCC_PRESENT &&
		   !(last_status_flags & CEMU_STATUS_F_VCC_PRESENT)) {
		/* improper power-up: Only power enabled, but no reset active. */
		reset = COLD_RESET;
	}

	if (reset) {
		LOGCI(ci, LOGL_NOTICE, "%s Resetting card in reader...\n",
			reset == COLD_RESET ? "Cold" : "Warm");
		osim_card_reset(card, reset == COLD_RESET ? true : false);

		/* Mark reset event in GSMTAP wireshark trace */
		osmo_st2_gsmtap_send_apdu(GSMTAP_SIM_ATR, card->atr, card->atr_len);
	}

	last_status_flags = flags;
}

static const char *cemu_data_flags2str(uint32_t flags)
{
	static char out[64];
	snprintf(out, sizeof(out), "%s%s%s%s",
		 flags & CEMU_DATA_F_TPDU_HDR ? "HDR " : "",
		 flags & CEMU_DATA_F_FINAL ? "FINAL " : "",
		 flags & CEMU_DATA_F_PB_AND_TX ? "PB_AND_TX " : "",
		 flags & CEMU_DATA_F_PB_AND_RX ? "PB_AND_RX" : "");
	return out;
}

/***********************************************************************
 * Incoming Messages
 ***********************************************************************/

/*! \brief Process a STATUS message from the SIMtrace2 */
static int process_do_status(struct osmo_st2_cardem_inst *ci, uint8_t *buf, int len)
{
	struct cardemu_usb_msg_status *status = (struct cardemu_usb_msg_status *) buf;
	char fbuf[80];

	cemu_status_flags2str(fbuf, sizeof(fbuf), status->flags);
	LOGCI(ci, LOGL_NOTICE, "=> STATUS: flags=0x%x, fi=%u, di=%u, wi=%u wtime=%u (%s)\n",
		status->flags, status->fi, status->di, status->wi,
		status->waiting_time, fbuf);

	update_status_flags(ci, status->flags);

	return 0;
}

/*! \brief Process a PTS indication message from the SIMtrace2 */
static int process_do_pts(struct osmo_st2_cardem_inst *ci, uint8_t *buf, int len)
{
	struct cardemu_usb_msg_pts_info *pts;
	pts = (struct cardemu_usb_msg_pts_info *) buf;

	LOGCI(ci, LOGL_NOTICE, "=> PTS req: %s\n", osmo_hexdump(pts->req, pts->pts_len));

	return 0;
}

/*! \brief Process a RX-DATA indication message from the SIMtrace2 */
static int process_do_rx_da(struct osmo_st2_cardem_inst *ci, uint8_t *buf, int len)
{
	static struct osmo_apdu_context ac;
	struct cardemu_usb_msg_rx_data *data;
	int rc;

	data = (struct cardemu_usb_msg_rx_data *) buf;

	LOGCI(ci, LOGL_INFO, "=> DATA: flags=0x%02x (%s), %s\n ", data->flags,
	      cemu_data_flags2str(data->flags), osmo_hexdump(data->data, data->data_len));

	rc = osmo_apdu_segment_in(&ac, data->data, data->data_len,
				  data->flags & CEMU_DATA_F_TPDU_HDR);
	if (rc < 0) {
		/* At this point the communication is broken.  We cannot keep running, as we
		 * don't know if we should continue transmitting or receiving.  Only a successful
		 * return value by osmo_apdu_segment_in() would allow us to know this. */
		LOGCI(ci, LOGL_FATAL, "Failed to recognize APDU, terminating\n");
		exit(1);
	}

	if (rc & APDU_ACT_TX_CAPDU_TO_CARD) {
		struct msgb *tmsg = msgb_alloc(1024, "TPDU");
		struct osim_reader_hdl *rh = ci->chan->card->reader;
		uint8_t *cur;

		/* Copy TPDU header */
		cur = msgb_put(tmsg, sizeof(ac.hdr));
		memcpy(cur, &ac.hdr, sizeof(ac.hdr));
		/* Copy D(c), if any */
		if (ac.lc.tot) {
			cur = msgb_put(tmsg, ac.lc.tot);
			memcpy(cur, ac.dc, ac.lc.tot);
		}
		/* send to actual card */
		tmsg->l3h = tmsg->tail;
		rc = rh->ops->transceive(rh, tmsg);
		if (rc < 0) {
			fprintf(stderr, "error during transceive: %d\n", rc);
			msgb_free(tmsg);
			return rc;
		}
		/* send via GSMTAP for wireshark tracing */
		osmo_st2_gsmtap_send_apdu(GSMTAP_SIM_APDU, tmsg->data, msgb_length(tmsg));

		msgb_apdu_sw(tmsg) = msgb_get_u16(tmsg);
		ac.sw[0] = msgb_apdu_sw(tmsg) >> 8;
		ac.sw[1] = msgb_apdu_sw(tmsg) & 0xff;
		if (msgb_l3len(tmsg))
			osmo_st2_cardem_request_pb_and_tx(ci, ac.hdr.ins, tmsg->l3h, msgb_l3len(tmsg));
		osmo_st2_cardem_request_sw_tx(ci, ac.sw);
	} else if (ac.lc.tot > ac.lc.cur) {
		osmo_st2_cardem_request_pb_and_rx(ci, ac.hdr.ins, ac.lc.tot - ac.lc.cur);
	}
	return 0;
}

/*! \brief Process an incoming message from the SIMtrace2 */
static int process_usb_msg(struct osmo_st2_cardem_inst *ci, uint8_t *buf, int len)
{
	struct simtrace_msg_hdr *sh = (struct simtrace_msg_hdr *)buf;
	int rc;

	buf += sizeof(*sh);

	switch (sh->msg_type) {
	case SIMTRACE_MSGT_BD_CEMU_STATUS:
		rc = process_do_status(ci, buf, len);
		break;
	case SIMTRACE_MSGT_DO_CEMU_PTS:
		rc = process_do_pts(ci, buf, len);
		break;
	case SIMTRACE_MSGT_DO_CEMU_RX_DATA:
		rc = process_do_rx_da(ci, buf, len);
		break;
	case SIMTRACE_MSGT_BD_CEMU_CONFIG:
		/* firmware confirms configuration change; ignore */
		rc = 0;
		break;
	default:
		printf("unknown simtrace msg type 0x%02x\n", sh->msg_type);
		rc = -1;
		break;
	}

	return rc;
}

/*! \brief Process a STATUS message on IRQ endpoint from the SIMtrace2 */
static int process_irq_status(struct osmo_st2_cardem_inst *ci, const uint8_t *buf, int len)
{
	const struct cardemu_usb_msg_status *status = (struct cardemu_usb_msg_status *) buf;
	char fbuf[80];

	cemu_status_flags2str(fbuf, sizeof(fbuf), status->flags);
	LOGCI(ci, LOGL_NOTICE, "=> IRQ STATUS: flags=0x%x, fi=%u, di=%u, wi=%u wtime=%u (%s)\n",
		status->flags, status->fi, status->di, status->wi,
		status->waiting_time, fbuf);

	update_status_flags(ci, status->flags);

	return 0;
}

static int process_usb_msg_irq(struct osmo_st2_cardem_inst *ci, const uint8_t *buf, unsigned int len)
{
	struct simtrace_msg_hdr *sh = (struct simtrace_msg_hdr *)buf;
	int rc;

	buf += sizeof(*sh);

	switch (sh->msg_type) {
	case SIMTRACE_MSGT_BD_CEMU_STATUS:
		rc = process_irq_status(ci, buf, len);
		break;
	default:
		LOGCI(ci, LOGL_ERROR, "unknown simtrace msg type 0x%02x\n", sh->msg_type);
		rc = -1;
		break;
	}

	return rc;
}

static void usb_in_xfer_cb(struct libusb_transfer *xfer)
{
	struct osmo_st2_cardem_inst *ci = xfer->user_data;
	int rc;

	switch (xfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		/* hand the message up the stack */
		process_usb_msg(ci, xfer->buffer, xfer->actual_length);
		break;
	case LIBUSB_TRANSFER_ERROR:
		LOGCI(ci, LOGL_FATAL, "USB IN transfer error, trying resubmit\n");
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		LOGCI(ci, LOGL_FATAL, "USB device disappeared\n");
		exit(1);
		break;
	default:
		LOGCI(ci, LOGL_FATAL, "USB IN transfer failed, status=%u\n", xfer->status);
		exit(1);
		break;
	}

	/* re-submit the IN transfer */
	rc = libusb_submit_transfer(xfer);
	OSMO_ASSERT(rc == 0);
}


static void allocate_and_submit_in(struct osmo_st2_cardem_inst *ci)
{
	struct osmo_st2_transport *transp = ci->slot->transp;
	struct libusb_transfer *xfer;
	int rc;

	xfer = libusb_alloc_transfer(0);
	OSMO_ASSERT(xfer);
	xfer->dev_handle = transp->usb_devh;
	xfer->flags = 0;
	xfer->type = LIBUSB_TRANSFER_TYPE_BULK;
	xfer->endpoint = transp->usb_ep.in;
	xfer->timeout = 0;
	xfer->user_data = ci;
	xfer->length = 16*256;

	xfer->buffer = libusb_dev_mem_alloc(xfer->dev_handle, xfer->length);
	OSMO_ASSERT(xfer->buffer);
	xfer->callback = usb_in_xfer_cb;

	/* submit the IN transfer */
	rc = libusb_submit_transfer(xfer);
	OSMO_ASSERT(rc == 0);
}


static void usb_irq_xfer_cb(struct libusb_transfer *xfer)
{
	struct osmo_st2_cardem_inst *ci = xfer->user_data;
	int rc;

	switch (xfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		process_usb_msg_irq(ci, xfer->buffer, xfer->actual_length);
		break;
	case LIBUSB_TRANSFER_ERROR:
		LOGCI(ci, LOGL_FATAL, "USB INT transfer error, trying resubmit\n");
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		LOGCI(ci, LOGL_FATAL, "USB device disappeared\n");
		exit(1);
		break;
	default:
		LOGCI(ci, LOGL_FATAL, "USB INT transfer failed, status=%u\n", xfer->status);
		exit(1);
		break;
	}

	/* re-submit the IN transfer */
	rc = libusb_submit_transfer(xfer);
	OSMO_ASSERT(rc == 0);
}


static void allocate_and_submit_irq(struct osmo_st2_cardem_inst *ci)
{
	struct osmo_st2_transport *transp = ci->slot->transp;
	struct libusb_transfer *xfer;
	int rc;

	xfer = libusb_alloc_transfer(0);
	OSMO_ASSERT(xfer);
	xfer->dev_handle = transp->usb_devh;
	xfer->flags = 0;
	xfer->type = LIBUSB_TRANSFER_TYPE_INTERRUPT;
	xfer->endpoint = transp->usb_ep.irq_in;
	xfer->timeout = 0;
	xfer->user_data = ci;
	xfer->length = 64;

	xfer->buffer = libusb_dev_mem_alloc(xfer->dev_handle, xfer->length);
	OSMO_ASSERT(xfer->buffer);
	xfer->callback = usb_irq_xfer_cb;

	/* submit the IN transfer */
	rc = libusb_submit_transfer(xfer);
	OSMO_ASSERT(rc == 0);
}



static void print_welcome(void)
{
	printf("simtrace2-cardem-pcsc - Using PC/SC reader as SIM\n"
	       "(C) 2010-2022, Harald Welte <laforge@gnumonks.org>\n"
	       "(C) 2018, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>\n\n");
}

static void print_help(void)
{
	printf( "\t-h\t--help\n"
		"\t-i\t--gsmtap-ip\tA.B.C.D\n"
		"\t-a\t--skip-atr\n"
		"\t-t\t--set-atr\tATR-STRING in HEX\n"
		"\t-k\t--keep-running\n"
		"\t-n\t--pcsc-reader-num\n"
		"\t-V\t--usb-vendor\tVENDOR_ID\n"
		"\t-P\t--usb-product\tPRODUCT_ID\n"
		"\t-C\t--usb-config\tCONFIG_ID\n"
		"\t-I\t--usb-interface\tINTERFACE_ID\n"
		"\t-S\t--usb-altsetting ALTSETTING_ID\n"
		"\t-A\t--usb-address\tADDRESS\n"
		"\t-H\t--usb-path\tPATH\n"
		"\n"
		);
}

static const struct option opts[] = {
	{ "gsmtap-ip", 1, 0, 'i' },
	{ "skip-atr", 0, 0, 'a' },
	{ "set-atr", 1, 0, 't' },
	{ "help", 0, 0, 'h' },
	{ "keep-running", 0, 0, 'k' },
	{ "pcsc-reader-num", 1, 0, 'n' },
	{ "usb-vendor", 1, 0, 'V' },
	{ "usb-product", 1, 0, 'P' },
	{ "usb-config", 1, 0, 'C' },
	{ "usb-interface", 1, 0, 'I' },
	{ "usb-altsetting", 1, 0, 'S' },
	{ "usb-address", 1, 0, 'A' },
	{ "usb-path", 1, 0, 'H' },
	{ NULL, 0, 0, 0 }
};

static void run_mainloop(struct osmo_st2_cardem_inst *ci)
{
	printf("Entering main loop\n");
	while (1) {
		osmo_select_main(0);
	}
}

static struct osmo_st2_transport _transp;

static struct osmo_st2_slot _slot = {
	.transp = &_transp,
	.slot_nr = 0,
};

struct osmo_st2_cardem_inst _ci = {
	.slot = &_slot,
};

struct osmo_st2_cardem_inst *ci = &_ci;

static void signal_handler(int signal)
{
	switch (signal) {
	case SIGINT:
		osmo_st2_cardem_request_card_insert(ci, false);
		osmo_st2_modem_sim_select_local(ci->slot);
		exit(0);
		break;
	default:
		break;
	}
}

static struct log_info log_info = {};

int main(int argc, char **argv)
{
	struct osmo_st2_transport *transp = ci->slot->transp;
	char *gsmtap_host = "127.0.0.1";
	int rc;
	int c, ret = 1;
	int skip_atr = 0;
	char *atr = NULL;
	uint8_t override_atr[OSIM_MAX_ATR_LEN];
	int override_atr_len = 0;
	int keep_running = 0;
	int if_num = 0, vendor_id = -1, product_id = -1;
	int config_id = -1, altsetting = 0, addr = -1;
	int reader_num = 0;
	char *path = NULL;
	struct osim_reader_hdl *reader;
	struct osim_card_hdl *card;

	print_welcome();

	osmo_init_logging2(NULL, &log_info);

	rc = osmo_libusb_init(NULL);
	if (rc < 0) {
		fprintf(stderr, "libusb initialization failed\n");
		return rc;
	}

	log_set_print_category_hex(osmo_stderr_target, false);
	log_set_print_category(osmo_stderr_target, true);
	log_set_print_level(osmo_stderr_target, true);
	log_set_print_filename_pos(osmo_stderr_target, LOG_FILENAME_POS_LINE_END);
	log_set_print_filename2(osmo_stderr_target, LOG_FILENAME_NONE);
	log_set_category_filter(osmo_stderr_target, DLINP, 1, LOGL_DEBUG);
	log_set_category_filter(osmo_stderr_target, DLGLOBAL, 1, LOGL_DEBUG);

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "hi:V:P:C:I:S:A:H:akn:t:", opts, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			print_help();
			exit(0);
			break;
		case 'i':
			gsmtap_host = optarg;
			break;
		case 'a':
			skip_atr = 1;
			break;
		case 't':
		        atr = optarg;
			break;
		case 'k':
			keep_running = 1;
			break;
		case 'n':
			reader_num = atoi(optarg);
			break;
		case 'V':
			vendor_id = strtol(optarg, NULL, 16);
			break;
		case 'P':
			product_id = strtol(optarg, NULL, 16);
			break;
		case 'C':
			config_id = atoi(optarg);
			break;
		case 'I':
			if_num = atoi(optarg);
			break;
		case 'S':
			altsetting = atoi(optarg);
			break;
		case 'A':
			addr = atoi(optarg);
			break;
		case 'H':
			path = optarg;
			break;
		}
	}

	if (atr) {
		override_atr_len = osmo_hexparse(atr, override_atr, sizeof(override_atr));
		if (override_atr_len < 2) {
			fprintf(stderr, "Invalid ATR - please omit a leading 0x and only use valid hex "
				"digits and whitespace. ATRs need to be between 2 and 33 bytes long.\n");
			goto do_exit;
		}
	}

	if (vendor_id < 0 || product_id < 0) {
		fprintf(stderr, "You have to specify the vendor and product ID\n");
		goto do_exit;
	}

	ci->card_prof = &osim_uicc_sim_cic_profile;

	rc = osmo_st2_gsmtap_init(gsmtap_host);
	if (rc < 0) {
		perror("unable to open GSMTAP");
		goto close_exit;
	}

	reader = osim_reader_open(OSIM_READER_DRV_PCSC, reader_num, "", NULL);
	if (!reader) {
		perror("unable to open PC/SC reader");
		goto close_exit;
	}

	card = osim_card_open(reader, OSIM_PROTO_T0);
	if (!card) {
		perror("unable to open SIM card");
		goto close_exit;
	}

	ci->chan = llist_entry(card->channels.next, struct osim_chan_hdl, list);
	if (!ci->chan) {
		perror("SIM card has no channel?!?");
		goto close_exit;
	}

	signal(SIGINT, &signal_handler);

	do {
		struct usb_interface_match _ifm, *ifm = &_ifm;
		memset(ifm, 0, sizeof(*ifm));
		ifm->vendor = vendor_id;
		ifm->product = product_id;
		ifm->configuration = config_id;
		ifm->interface = if_num;
		ifm->altsetting = altsetting;
		if (addr > 0 && addr < 256)
			ifm->addr = addr;
		if (path)
			osmo_strlcpy(ifm->path, path, sizeof(ifm->path));
		transp->udp_fd = -1;
		transp->usb_async = true;
		transp->usb_devh = osmo_libusb_open_claim_interface(NULL, NULL, ifm);
		if (!transp->usb_devh) {
			fprintf(stderr, "can't open USB device: %s\n", strerror(errno));
			goto close;
		}

		rc = libusb_claim_interface(transp->usb_devh, if_num);
		if (rc < 0) {
			fprintf(stderr, "can't claim interface %d; rc=%d\n", if_num, rc);
			goto close;
		}

		rc = osmo_libusb_get_ep_addrs(transp->usb_devh, if_num, &transp->usb_ep.out,
				      &transp->usb_ep.in, &transp->usb_ep.irq_in);
		if (rc < 0) {
			fprintf(stderr, "can't obtain EP addrs; rc=%d\n", rc);
			goto close;
		}

		allocate_and_submit_irq(ci);
		for (int i = 0; i < 4; i++)
			allocate_and_submit_in(ci);

		/* request firmware to generate STATUS on IRQ endpoint */
		osmo_st2_cardem_request_config(ci, CEMU_FEAT_F_STATUS_IRQ);

		/* simulate card-insert to modem (owhw, not qmod) */
		osmo_st2_cardem_request_card_insert(ci, true);

		/* select remote (forwarded) SIM */
		osmo_st2_modem_sim_select_remote(ci->slot);

		if (!skip_atr) {
			/* set the ATR */
			if (override_atr_len) {
				/* user has specified an override-ATR */
				atr_update_csum(override_atr, override_atr_len);
				osmo_st2_cardem_request_set_atr(ci, override_atr, override_atr_len);
			} else {
				/* use the real ATR of the card */
				osmo_st2_cardem_request_set_atr(ci, card->atr, card->atr_len);
			}
		}

		/* select remote (forwarded) SIM */
		osmo_st2_modem_reset_pulse(ci->slot, 300);

		run_mainloop(ci);
		ret = 0;

		libusb_release_interface(transp->usb_devh, 0);

close:
		if (transp->usb_devh) {
			libusb_close(transp->usb_devh);
			transp->usb_devh = NULL;
		}
		if (keep_running)
			sleep(1);
	} while (keep_running);

close_exit:
	if (transp->usb_devh)
		libusb_close(transp->usb_devh);

	osmo_libusb_exit(NULL);
do_exit:
	return ret;
}
