/* simtrace2-remsim - main program for the host PC to provide a remote SIM
 * using the SIMtrace 2 firmware in card emulation mode
 *
 * (C) 2016-2017 by Harald Welte <hwelte@hmw-consulting.de>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <osmocom/sim/class_tables.h>
#include <osmocom/sim/sim.h>

static void atr_update_csum(uint8_t *atr, unsigned int atr_len)
{
	uint8_t csum = 0;
	int i;

	for (i = 1; i < atr_len - 1; i++)
		csum = csum ^ atr[i];

	atr[atr_len-1] = csum;
}

/***********************************************************************
 * Incoming Messages
 ***********************************************************************/

/*! \brief Process a STATUS message from the SIMtrace2 */
static int process_do_status(struct osmo_st2_cardem_inst *ci, uint8_t *buf, int len)
{
	struct cardemu_usb_msg_status *status;
	status = (struct cardemu_usb_msg_status *) buf;

	printf("=> STATUS: flags=0x%x, F=%u, D=%u, WI=%u WT=%u\n",
		status->flags, status->f, status->d, status->wi,
		status->wt);

	return 0;
}

/*! \brief Process a PTS indication message from the SIMtrace2 */
static int process_do_pts(struct osmo_st2_cardem_inst *ci, uint8_t *buf, int len)
{
	struct cardemu_usb_msg_pts_info *pts;
	pts = (struct cardemu_usb_msg_pts_info *) buf;

	printf("=> PTS req: %s\n", osmo_hexdump(pts->req, sizeof(pts->req)));

	return 0;
}

/*! \brief Process a RX-DATA indication message from the SIMtrace2 */
static int process_do_rx_da(struct osmo_st2_cardem_inst *ci, uint8_t *buf, int len)
{
	static struct osmo_apdu_context ac;
	struct cardemu_usb_msg_rx_data *data;
	int rc;

	data = (struct cardemu_usb_msg_rx_data *) buf;

	printf("=> DATA: flags=%x, %s: ", data->flags,
		osmo_hexdump(data->data, data->data_len));

	rc = osmo_apdu_segment_in(&ac, data->data, data->data_len,
				  data->flags & CEMU_DATA_F_TPDU_HDR);

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
		msgb_apdu_sw(tmsg) = msgb_get_u16(tmsg);
		ac.sw[0] = msgb_apdu_sw(tmsg) >> 8;
		ac.sw[1] = msgb_apdu_sw(tmsg) & 0xff;
		printf("SW=0x%04x, len_rx=%d\n", msgb_apdu_sw(tmsg), msgb_l3len(tmsg));
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

	printf("-> %s\n", osmo_hexdump(buf, len));

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
	default:
		printf("unknown simtrace msg type 0x%02x\n", sh->msg_type);
		rc = -1;
		break;
	}

	return rc;
}

static void print_welcome(void)
{
	printf("simtrace2-remsim - Remote SIM card forwarding\n"
	       "(C) 2010-2017, Harald Welte <laforge@gnumonks.org>\n"
	       "(C) 2018, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>\n\n");
}

static void print_help(void)
{
	printf( "\t-r\t--remote-udp-host HOST\n"
		"\t-p\t--remote-udp-port PORT\n"
		"\t-h\t--help\n"
		"\t-i\t--gsmtap-ip\tA.B.C.D\n"
		"\t-a\t--skip-atr\n"
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
	{ "remote-udp-host", 1, 0, 'r' },
	{ "remote-udp-port", 1, 0, 'p' },
	{ "gsmtap-ip", 1, 0, 'i' },
	{ "skip-atr", 0, 0, 'a' },
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
	struct osmo_st2_transport *transp = ci->slot->transp;
	unsigned int msg_count, byte_count = 0;
	uint8_t buf[16*265];
	int xfer_len;
	int rc;

	printf("Entering main loop\n");

	while (1) {
		/* read data from SIMtrace2 device (local or via USB) */
		if (transp->udp_fd < 0) {
			rc = libusb_bulk_transfer(transp->usb_devh, transp->usb_ep.in,
						  buf, sizeof(buf), &xfer_len, 100);
			if (rc < 0 && rc != LIBUSB_ERROR_TIMEOUT &&
				      rc != LIBUSB_ERROR_INTERRUPTED &&
				      rc != LIBUSB_ERROR_IO) {
				fprintf(stderr, "BULK IN transfer error; rc=%d\n", rc);
				return;
			}
		} else {
			rc = read(transp->udp_fd, buf, sizeof(buf));
			if (rc <= 0) {
				fprintf(stderr, "shor read from UDP\n");
				return;
			}
			xfer_len = rc;
		}
		/* dispatch any incoming data */
		if (xfer_len > 0) {
			printf("URB: %s\n", osmo_hexdump(buf, xfer_len));
			process_usb_msg(ci, buf, xfer_len);
			msg_count++;
			byte_count += xfer_len;
		}
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
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char **argv)
{
	struct osmo_st2_transport *transp = ci->slot->transp;
	char *gsmtap_host = "127.0.0.1";
	int rc;
	int c, ret = 1;
	int skip_atr = 0;
	int keep_running = 0;
	int remote_udp_port = 52342;
	int if_num = 0, vendor_id = -1, product_id = -1;
	int config_id = -1, altsetting = 0, addr = -1;
	int reader_num = 0;
	char *remote_udp_host = NULL;
	char *path = NULL;
	struct osim_reader_hdl *reader;
	struct osim_card_hdl *card;

	print_welcome();

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "r:p:hi:V:P:C:I:S:A:H:akn:", opts, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'r':
			remote_udp_host = optarg;
			break;
		case 'p':
			remote_udp_port = atoi(optarg);
			break;
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

	if (!remote_udp_host && (vendor_id < 0 || product_id < 0)) {
		fprintf(stderr, "You have to specify the vendor and product ID\n");
		goto do_exit;
	}

	transp->udp_fd = -1;

	ci->card_prof = &osim_uicc_sim_cic_profile;

	if (!remote_udp_host) {
		rc = libusb_init(NULL);
		if (rc < 0) {
			fprintf(stderr, "libusb initialization failed\n");
			goto do_exit;
		}
	} else {
		transp->udp_fd = osmo_sock_init(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
						remote_udp_host, remote_udp_port+if_num,
						OSMO_SOCK_F_CONNECT);
		if (transp->udp_fd < 0) {
			fprintf(stderr, "error binding UDP port\n");
			goto do_exit;
		}
	}

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
		if (transp->udp_fd < 0) {
			struct usb_interface_match _ifm, *ifm = &_ifm;
			ifm->vendor = vendor_id;
			ifm->product = product_id;
			ifm->configuration = config_id;
			ifm->interface = if_num;
			ifm->altsetting = altsetting;
			ifm->addr = addr;
			if (path)
				osmo_strlcpy(ifm->path, path, sizeof(ifm->path));
			transp->usb_devh = osmo_libusb_open_claim_interface(NULL, NULL, ifm);
			if (!transp->usb_devh) {
				fprintf(stderr, "can't open USB device\n");
				goto close_exit;
			}

			rc = libusb_claim_interface(transp->usb_devh, if_num);
			if (rc < 0) {
				fprintf(stderr, "can't claim interface %d; rc=%d\n", if_num, rc);
				goto close_exit;
			}

			rc = osmo_libusb_get_ep_addrs(transp->usb_devh, if_num, &transp->usb_ep.out,
					      &transp->usb_ep.in, &transp->usb_ep.irq_in);
			if (rc < 0) {
				fprintf(stderr, "can't obtain EP addrs; rc=%d\n", rc);
				goto close_exit;
			}
		}

		/* simulate card-insert to modem (owhw, not qmod) */
		osmo_st2_cardem_request_card_insert(ci, true);

		/* select remote (forwarded) SIM */
		osmo_st2_modem_sim_select_remote(ci->slot);

		if (!skip_atr) {
			/* set the ATR */
			uint8_t real_atr[] = {  0x3B, 0x00 }; // the simplest ATR
			atr_update_csum(real_atr, sizeof(real_atr));
			osmo_st2_cardem_request_set_atr(ci, real_atr, sizeof(real_atr));
		}

		/* select remote (forwarded) SIM */
		osmo_st2_modem_reset_pulse(ci->slot, 300);

		run_mainloop(ci);
		ret = 0;

		if (transp->udp_fd < 0)
			libusb_release_interface(transp->usb_devh, 0);
close_exit:
		if (transp->usb_devh)
			libusb_close(transp->usb_devh);
		if (keep_running)
			sleep(1);
	} while (keep_running);

	if (transp->udp_fd < 0)
		libusb_exit(NULL);
do_exit:
	return ret;
}
