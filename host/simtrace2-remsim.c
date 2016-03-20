/* simtrace2-remsim - main program for the host PC
 *
 * (C) 2010-2016 by Harald Welte <hwelte@hmw-consulting.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "simtrace.h"
#include "cardemu_prot.h"
#include "apdu_dispatch.h"
#include "simtrace2-discovery.h"

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/socket.h>
#include <osmocom/sim/class_tables.h>
#include <osmocom/sim/sim.h>

static struct gsmtap_inst *g_gti;
struct libusb_device_handle *g_devh;
const struct osim_cla_ins_card_profile *g_prof;
static uint8_t g_in_ep;
static uint8_t g_out_ep;
static int g_udp_fd = -1;
static struct osim_chan_hdl *g_chan;

static int gsmtap_send_sim(const uint8_t *apdu, unsigned int len)
{
	struct gsmtap_hdr *gh;
	unsigned int gross_len = len + sizeof(*gh);
	uint8_t *buf = malloc(gross_len);
	int rc;

	if (!buf)
		return -ENOMEM;

	memset(buf, 0, sizeof(*gh));
	gh = (struct gsmtap_hdr *) buf;
	gh->version = GSMTAP_VERSION;
	gh->hdr_len = sizeof(*gh)/4;
	gh->type = GSMTAP_TYPE_SIM;

	memcpy(buf + sizeof(*gh), apdu, len);

	rc = write(gsmtap_inst_fd(g_gti), buf, gross_len);
	if (rc < 0) {
		perror("write gsmtap");
		free(buf);
		return rc;
	}

	free(buf);
	return 0;
}

#if 0
static void apdu_out_cb(uint8_t *buf, unsigned int len, void *user_data)
{
	printf("APDU: %s\n", osmo_hexdump(buf, len));
	gsmtap_send_sim(buf, len);
}
#endif

/*! \brief Transmit a given command to the SIMtrace2 device */
static int tx_to_dev(uint8_t *buf, unsigned int len)
{
	struct cardemu_usb_msg_hdr *mh = (struct cardemu_usb_msg_hdr *) buf;
	int xfer_len;

	mh->msg_len = len;

	printf("<- %s\n", osmo_hexdump(buf, len));

	if (g_udp_fd < 0) {
		return libusb_bulk_transfer(g_devh, g_out_ep, buf, len,
					    &xfer_len, 100000);
	} else {
		return write(g_udp_fd, buf, len);
	}
}

/*! \brief Request the SIMtrace2 to generate a card-insert signal */
static int request_card_insert(bool inserted)
{
	struct cardemu_usb_msg_cardinsert cins;

	memset(&cins, 0, sizeof(cins));
	cins.hdr.msg_type = CEMU_USB_MSGT_DT_CARDINSERT;
	if (inserted)
		cins.card_insert = 1;

	return tx_to_dev((uint8_t *)&cins, sizeof(cins));
}

/*! \brief Request the SIMtrace2 to transmit a Procedure Byte, then Rx */
static int request_pb_and_rx(uint8_t pb, uint8_t le)
{
	struct cardemu_usb_msg_tx_data *txd;
	uint8_t buf[sizeof(*txd) + 1];
	txd = (struct cardemu_usb_msg_tx_data *) buf;

	printf("<= request_pb_and_rx(%02x, %d)\n", pb, le);

	memset(txd, 0, sizeof(*txd));
	txd->data_len = 1;
	txd->hdr.msg_type = CEMU_USB_MSGT_DT_TX_DATA;
	txd->flags = CEMU_DATA_F_PB_AND_RX;
	txd->data[0] = pb;

	return tx_to_dev((uint8_t *)txd, sizeof(*txd)+txd->data_len);
}

/*! \brief Request the SIMtrace2 to transmit a Procedure Byte, then Tx */
static int request_pb_and_tx(uint8_t pb, const uint8_t *data, uint8_t data_len_in)
{
	uint32_t data_len = data_len_in;
	struct cardemu_usb_msg_tx_data *txd;
	uint8_t buf[sizeof(*txd) + 1 + data_len_in];
	txd = (struct cardemu_usb_msg_tx_data *) buf;

	printf("<= request_pb_and_tx(%02x, %s, %d)\n", pb, osmo_hexdump(data, data_len_in), data_len_in);

	memset(txd, 0, sizeof(*txd));
	txd->hdr.msg_type = CEMU_USB_MSGT_DT_TX_DATA;
	txd->data_len = 1 + data_len_in;
	txd->flags = CEMU_DATA_F_PB_AND_TX;
	txd->data[0] = pb;
	memcpy(txd->data+1, data, data_len_in);

	return tx_to_dev(buf, sizeof(*txd)+txd->data_len);
}

/*! \brief Request the SIMtrace2 to send a Status Word */
static int request_sw_tx(const uint8_t *sw)
{
	struct cardemu_usb_msg_tx_data *txd;
	uint8_t buf[sizeof(*txd) + 2];
	txd = (struct cardemu_usb_msg_tx_data *) buf;

	printf("<= request_sw_tx(%02x %02x)\n", sw[0], sw[1]);

	memset(txd, 0, sizeof(*txd));
	txd->hdr.msg_type = CEMU_USB_MSGT_DT_TX_DATA;
	txd->data_len = 2;
	txd->flags = CEMU_DATA_F_PB_AND_TX | CEMU_DATA_F_FINAL;
	txd->data[0] = sw[0];
	txd->data[1] = sw[1];

	return tx_to_dev((uint8_t *)txd, sizeof(*txd)+txd->data_len);
}

static void atr_update_csum(uint8_t *atr, unsigned int atr_len)
{
	uint8_t csum = 0;
	int i;

	for (i = 1; i < atr_len - 1; i++)
		csum = csum ^ atr[i];

	atr[atr_len-1] = csum;
}

static int request_set_atr(const uint8_t *atr, unsigned int atr_len)
{
	struct cardemu_usb_msg_set_atr *satr;
	uint8_t buf[sizeof(*satr) + atr_len];
	satr = (struct cardemu_usb_msg_set_atr *) buf;

	printf("<= request_set_atr(%s)\n", osmo_hexdump(atr, atr_len));

	memset(satr, 0, sizeof(*satr));
	satr->hdr.msg_type = CEMU_USB_MSGT_DT_SET_ATR;
	satr->atr_len = atr_len;
	memcpy(satr->atr, atr, atr_len);

	return tx_to_dev((uint8_t *)satr, sizeof(buf));
}

/*! \brief Process a STATUS message from the SIMtrace2 */
static int process_do_status(uint8_t *buf, int len)
{
	struct cardemu_usb_msg_status *status;
	status = (struct cardemu_usb_msg_status *) buf;

	printf("=> STATUS: flags=0x%x, fi=%u, di=%u, wi=%u wtime=%u\n",
		status->flags, status->fi, status->di, status->wi,
		status->waiting_time);

	return 0;
}

/*! \brief Process a PTS indication message from the SIMtrace2 */
static int process_do_pts(uint8_t *buf, int len)
{
	struct cardemu_usb_msg_pts_info *pts;
	pts = (struct cardemu_usb_msg_pts_info *) buf;

	printf("=> PTS req: %s\n", osmo_hexdump(pts->req, sizeof(pts->req)));

	return 0;
}

/*! \brief Process a ERROR indication message from the SIMtrace2 */
static int process_do_error(uint8_t *buf, int len)
{
	struct cardemu_usb_msg_error *err;
	err = (struct cardemu_usb_msg_error *) buf;

	printf("=> ERROR: %u/%u/%u: %s\n",
		err->severity, err->subsystem, err->code,
		err->msg_len ? err->msg : "");

	return 0;
}

/*! \brief Process a RX-DATA indication message from the SIMtrace2 */
static int process_do_rx_da(uint8_t *buf, int len)
{
	static struct apdu_context ac;
	struct cardemu_usb_msg_rx_data *data;
	const uint8_t sw_success[] = { 0x90, 0x00 };
	int rc;

	data = (struct cardemu_usb_msg_rx_data *) buf;

	printf("=> DATA: flags=%x, %s: ", data->flags,
		osmo_hexdump(data->data, data->data_len));

	rc = apdu_segment_in(&ac, data->data, data->data_len,
			     data->flags & CEMU_DATA_F_TPDU_HDR);

	if (rc & APDU_ACT_TX_CAPDU_TO_CARD) {
		struct msgb *tmsg = msgb_alloc(1024, "TPDU");
		struct osim_reader_hdl *rh = g_chan->card->reader;
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
			request_pb_and_tx(ac.hdr.ins, tmsg->l3h, msgb_l3len(tmsg));
		request_sw_tx(ac.sw);
	} else if (ac.lc.tot > ac.lc.cur) {
		request_pb_and_rx(ac.hdr.ins, ac.lc.tot - ac.lc.cur);
	}
	return 0;
}

/*! \brief Process an incoming message from the SIMtrace2 */
static int process_usb_msg(uint8_t *buf, int len)
{
	struct cardemu_usb_msg_hdr *sh = (struct cardemu_usb_msg_hdr *)buf;
	uint8_t *payload;
	int payload_len;
	int rc;

	printf("-> %s\n", osmo_hexdump(buf, len));

	switch (sh->msg_type) {
	case CEMU_USB_MSGT_DO_STATUS:
		rc = process_do_status(buf, len);
		break;
	case CEMU_USB_MSGT_DO_PTS:
		rc = process_do_pts(buf, len);
		break;
	case CEMU_USB_MSGT_DO_ERROR:
		rc = process_do_error(buf, len);
		break;
	case CEMU_USB_MSGT_DO_RX_DATA:
		rc = process_do_rx_da(buf, len);
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
	       "(C) 2010-2016 by Harald Welte <laforge@gnumonks.org>\n\n");
}

static void print_help(void)
{
	printf( "\t-i\t--gsmtap-ip\tA.B.C.D\n"
		"\t-a\t--skip-atr\n"
		"\t-h\t--help\n"
		"\t-k\t--keep-running\n"
		"\n"
		);
}

static const struct option opts[] = {
	{ "gsmtap-ip", 1, 0, 'i' },
	{ "skip-atr", 0, 0, 'a' },
	{ "help", 0, 0, 'h' },
	{ "keep-running", 0, 0, 'k' },
	{ NULL, 0, 0, 0 }
};

static void run_mainloop(void)
{
	unsigned int msg_count, byte_count = 0;
	char buf[16*265];
	int xfer_len;
	int rc;

	printf("Entering main loop\n");

	while (1) {
		/* read data from SIMtrace2 device (local or via USB) */
		if (g_udp_fd < 0) {
			rc = libusb_bulk_transfer(g_devh, g_in_ep, buf, sizeof(buf), &xfer_len, 100000);
			if (rc < 0 && rc != LIBUSB_ERROR_TIMEOUT) {
				fprintf(stderr, "BULK IN transfer error; rc=%d\n", rc);
				return;
			}
		} else {
			rc = read(g_udp_fd, buf, sizeof(buf));
			if (rc <= 0) {
				fprintf(stderr, "shor read from UDP\n");
				return;
			}
			xfer_len = rc;
		}
		/* dispatch any incoming data */
		if (xfer_len > 0) {
			//printf("URB: %s\n", osmo_hexdump(buf, rc));
			process_usb_msg(buf, xfer_len);
			msg_count++;
			byte_count += xfer_len;
		}
	}
}

static void signal_handler(int signal)
{
	switch (signal) {
	case SIGINT:
		request_card_insert(false);
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char **argv)
{
	char *gsmtap_host = "127.0.0.1";
	int rc;
	int c, ret = 1;
	int skip_atr = 0;
	int keep_running = 0;
	int remote_udp_port = 52342;
	int if_num = 0;
	char *remote_udp_host = NULL;
	struct osim_reader_hdl *reader;
	struct osim_card_hdl *card;

	print_welcome();

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "r:p:hi:I:ak", opts, &option_index);
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
		case 'I':
			if_num = atoi(optarg);
			break;
		case 'a':
			skip_atr = 1;
			break;
		case 'k':
			keep_running = 1;
			break;
		}
	}

	g_prof = &osim_uicc_sim_cic_profile;

	if (!remote_udp_host) {
		rc = libusb_init(NULL);
		if (rc < 0) {
			fprintf(stderr, "libusb initialization failed\n");
			goto do_exit;
		}
	} else {
		g_udp_fd = osmo_sock_init(AF_INET, SOCK_DGRAM, IPPROTO_UDP, remote_udp_host,
					  remote_udp_port+if_num, OSMO_SOCK_F_CONNECT);
		if (g_udp_fd < 0) {
			fprintf(stderr, "error binding UDP port\n");
			goto do_exit;
		}
	}

	g_gti = gsmtap_source_init(gsmtap_host, GSMTAP_UDP_PORT, 0);
	if (!g_gti) {
		perror("unable to open GSMTAP");
		goto close_exit;
	}
	gsmtap_source_add_sink(g_gti);

	reader = osim_reader_open(OSIM_READER_DRV_PCSC, 0, "", NULL);
	if (!reader) {
		perror("unable to open PC/SC reader");
		goto close_exit;
	}

	card = osim_card_open(reader, OSIM_PROTO_T0);
	if (!card) {
		perror("unable to open SIM card");
		goto close_exit;
	}

	g_chan = llist_entry(card->channels.next, struct osim_chan_hdl, list);
	if (!g_chan) {
		perror("SIM card has no channel?!?");
		goto close_exit;
	}

	signal(SIGINT, &signal_handler);

	do {
		if (g_udp_fd < 0) {
			g_devh = libusb_open_device_with_vid_pid(NULL, SIMTRACE_USB_VENDOR, SIMTRACE_USB_PRODUCT);
			if (!g_devh) {
				fprintf(stderr, "can't open USB device\n");
				goto close_exit;
			}

			rc = libusb_claim_interface(g_devh, if_num);
			if (rc < 0) {
				fprintf(stderr, "can't claim interface %d; rc=%d\n", if_num, rc);
				goto close_exit;
			}

			rc = get_usb_ep_addrs(g_devh, if_num, &g_out_ep, &g_in_ep, NULL);
			if (rc < 0) {
				fprintf(stderr, "can't obtain EP addrs; rc=%d\n", rc);
				goto close_exit;
			}
		}

		request_card_insert(true);
		uint8_t real_atr[] = { 0x3B, 0x9F, 0x96, 0x80, 0x1F, 0xC7, 0x80, 0x31,
					     0xA0, 0x73, 0xBE, 0x21, 0x13, 0x67, 0x43, 0x20,
					     0x07, 0x18, 0x00, 0x00, 0x01, 0xA5 };
		atr_update_csum(real_atr, sizeof(real_atr));
		request_set_atr(real_atr, sizeof(real_atr));

		run_mainloop();
		ret = 0;

		if (g_udp_fd < 0)
			libusb_release_interface(g_devh, 0);
close_exit:
		if (g_devh)
			libusb_close(g_devh);
		if (keep_running)
			sleep(1);
	} while (keep_running);

release_exit:
	if (g_udp_fd < 0)
		libusb_exit(NULL);
do_exit:
	return ret;
}
