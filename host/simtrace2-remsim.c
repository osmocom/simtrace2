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

#include "libusb_util.h"
#include "simtrace.h"
#include "simtrace_prot.h"
#include "apdu_dispatch.h"
#include "simtrace2-discovery.h"

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/msgb.h>
#include <osmocom/sim/class_tables.h>
#include <osmocom/sim/sim.h>

/* transport to a SIMtrace device */
struct st_transport {
	/* USB */
	struct libusb_device_handle *usb_devh;
	struct {
		uint8_t in;
		uint8_t out;
		uint8_t irq_in;
	} usb_ep;

	/* UDP */
	int udp_fd;
};

/* a SIMtrace slot; communicates over a transport */
struct st_slot {
	/* transport through which the slot can be reached */
	struct st_transport *transp;
	/* number of the slot within the transport */
	uint8_t slot_nr;
};

/* One istance of card emulation */
struct cardem_inst {
	/* slot on which this card emulation instance runs */
	struct st_slot *slot;
	/* libosmosim SIM card profile */
	const struct osim_cla_ins_card_profile *card_prof;
	/* libosmosim SIM card channel */
	struct osim_chan_hdl *chan;
};

/* global GSMTAP instance */
static struct gsmtap_inst *g_gti;

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

/***********************************************************************
 * SIMTRACE pcore protocol
 ***********************************************************************/

/*! \brief allocate a message buffer for simtrace use */
static struct msgb *st_msgb_alloc(void)
{
	return msgb_alloc_headroom(1024+32, 32, "SIMtrace");
}

#if 0
static void apdu_out_cb(uint8_t *buf, unsigned int len, void *user_data)
{
	printf("APDU: %s\n", osmo_hexdump(buf, len));
	gsmtap_send_sim(buf, len);
}
#endif

/*! \brief Transmit a given command to the SIMtrace2 device */
int st_transp_tx_msg(struct st_transport *transp, struct msgb *msg)
{
	int rc;

	printf("<- %s\n", msgb_hexdump(msg));

	if (transp->udp_fd < 0) {
		int xfer_len;

		rc = libusb_bulk_transfer(transp->usb_devh, transp->usb_ep.out,
					  msgb_data(msg), msgb_length(msg),
					  &xfer_len, 100000);
	} else {
		rc = write(transp->udp_fd, msgb_data(msg), msgb_length(msg));
	}

	msgb_free(msg);
	return rc;
}

static struct simtrace_msg_hdr *st_push_hdr(struct msgb *msg, uint8_t msg_class, uint8_t msg_type,
					    uint8_t slot_nr)
{
	struct simtrace_msg_hdr *sh;

	sh = (struct simtrace_msg_hdr *) msgb_push(msg, sizeof(*sh));
	memset(sh, 0, sizeof(*sh));
	sh->msg_class = msg_class;
	sh->msg_type = msg_type;
	sh->slot_nr = slot_nr;
	sh->msg_len = msgb_length(msg);

	return sh;
}

/* transmit a given message to a specified slot. Expects all headers
 * present before calling the function */
int st_slot_tx_msg(struct st_slot *slot, struct msgb *msg,
		   uint8_t msg_class, uint8_t msg_type)
{
	st_push_hdr(msg, msg_class, msg_type, slot->slot_nr);

	return st_transp_tx_msg(slot->transp, msg);
}

/***********************************************************************
 * Card Emulation protocol
 ***********************************************************************/


/*! \brief Request the SIMtrace2 to generate a card-insert signal */
static int cardem_request_card_insert(struct cardem_inst *ci, bool inserted)
{
	struct msgb *msg = st_msgb_alloc();
	struct cardemu_usb_msg_cardinsert *cins;

	cins = (struct cardemu_usb_msg_cardinsert *) msgb_put(msg, sizeof(*cins));
	memset(cins, 0, sizeof(*cins));
	if (inserted)
		cins->card_insert = 1;

	return st_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_CARDINSERT);
}

/*! \brief Request the SIMtrace2 to transmit a Procedure Byte, then Rx */
static int cardem_request_pb_and_rx(struct cardem_inst *ci, uint8_t pb, uint8_t le)
{
	struct msgb *msg = st_msgb_alloc();
	struct cardemu_usb_msg_tx_data *txd;
	txd = (struct cardemu_usb_msg_tx_data *) msgb_put(msg, sizeof(*txd));

	printf("<= %s(%02x, %d)\n", __func__, pb, le);

	memset(txd, 0, sizeof(*txd));
	txd->data_len = 1;
	txd->flags = CEMU_DATA_F_PB_AND_RX;
	/* one data byte */
	msgb_put_u8(msg, pb);

	return st_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_TX_DATA);
}

/*! \brief Request the SIMtrace2 to transmit a Procedure Byte, then Tx */
static int cardem_request_pb_and_tx(struct cardem_inst *ci, uint8_t pb,
				    const uint8_t *data, uint16_t data_len_in)
{
	struct msgb *msg = st_msgb_alloc();
	struct cardemu_usb_msg_tx_data *txd;
	uint8_t *cur;

	txd = (struct cardemu_usb_msg_tx_data *) msgb_put(msg, sizeof(*txd));

	printf("<= %s(%02x, %s, %d)\n", __func__, pb,
		osmo_hexdump(data, data_len_in), data_len_in);

	memset(txd, 0, sizeof(*txd));
	txd->data_len = 1 + data_len_in;
	txd->flags = CEMU_DATA_F_PB_AND_TX;
	/* procedure byte */
	msgb_put_u8(msg, pb);
	/* data */
	cur = msgb_put(msg, data_len_in);
	memcpy(cur, data, data_len_in);

	return st_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_TX_DATA);
}

/*! \brief Request the SIMtrace2 to send a Status Word */
static int cardem_request_sw_tx(struct cardem_inst *ci, const uint8_t *sw)
{
	struct msgb *msg = st_msgb_alloc();
	struct cardemu_usb_msg_tx_data *txd;
	uint8_t *cur;

	txd = (struct cardemu_usb_msg_tx_data *) msgb_put(msg, sizeof(*txd));

	printf("<= %s(%02x %02x)\n", __func__, sw[0], sw[1]);

	memset(txd, 0, sizeof(*txd));
	txd->data_len = 2;
	txd->flags = CEMU_DATA_F_PB_AND_TX | CEMU_DATA_F_FINAL;
	cur = msgb_put(msg, 2);
	cur[0] = sw[0];
	cur[1] = sw[1];

	return st_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_TX_DATA);
}

static void atr_update_csum(uint8_t *atr, unsigned int atr_len)
{
	uint8_t csum = 0;
	int i;

	for (i = 1; i < atr_len - 1; i++)
		csum = csum ^ atr[i];

	atr[atr_len-1] = csum;
}

static int cardem_request_set_atr(struct cardem_inst *ci, const uint8_t *atr, unsigned int atr_len)
{
	struct msgb *msg = st_msgb_alloc();
	struct cardemu_usb_msg_set_atr *satr;
	uint8_t *cur;

	satr = (struct cardemu_usb_msg_set_atr *) msgb_put(msg, sizeof(*satr));

	printf("<= %s(%s)\n", __func__, osmo_hexdump(atr, atr_len));

	memset(satr, 0, sizeof(*satr));
	satr->atr_len = atr_len;
	cur = msgb_put(msg, atr_len);
	memcpy(cur, atr, atr_len);

	return st_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_SET_ATR);
}

/***********************************************************************
 * Modem Control protocol
 ***********************************************************************/

static int _modem_reset(struct st_slot *slot, uint8_t asserted, uint16_t pulse_ms)
{
	struct msgb *msg = st_msgb_alloc();
	struct st_modem_reset *sr ;

	sr = (struct st_modem_reset *) msgb_put(msg, sizeof(*sr));
	sr->asserted = asserted;
	sr->pulse_duration_msec = pulse_ms;

	return st_slot_tx_msg(slot, msg, SIMTRACE_MSGC_MODEM, SIMTRACE_MSGT_DT_MODEM_RESET);
}

/*! \brief pulse the RESET line of the modem for \a duration_ms milli-seconds*/
int st_modem_reset_pulse(struct st_slot *slot, uint16_t duration_ms)
{
	return _modem_reset(slot, 2, duration_ms);
}

/*! \brief assert the RESET line of the modem */
int st_modem_reset_active(struct st_slot *slot)
{
	return _modem_reset(slot, 1, 0);
}

/*! \brief de-assert the RESET line of the modem */
int st_modem_reset_inactive(struct st_slot *slot)
{
	return _modem_reset(slot, 0, 0);
}

static int _modem_sim_select(struct st_slot *slot, uint8_t remote_sim)
{
	struct msgb *msg = st_msgb_alloc();
	struct st_modem_sim_select *ss;

	ss = (struct st_modem_sim_select *) msgb_put(msg, sizeof(*ss));
	ss->remote_sim = remote_sim;

	return st_slot_tx_msg(slot, msg, SIMTRACE_MSGC_MODEM, SIMTRACE_MSGT_DT_MODEM_SIM_SELECT);
}

/*! \brief select local (physical) SIM for given slot */
int st_modem_sim_select_local(struct st_slot *slot)
{
	return _modem_sim_select(slot, 0);
}

/*! \brief select remote (emulated/forwarded) SIM for given slot */
int st_modem_sim_select_remote(struct st_slot *slot)
{
	return _modem_sim_select(slot, 1);
}

/*! \brief Request slot to send us status information about the modem */
int st_modem_get_status(struct st_slot *slot)
{
	struct msgb *msg = st_msgb_alloc();

	return st_slot_tx_msg(slot, msg, SIMTRACE_MSGC_MODEM, SIMTRACE_MSGT_BD_MODEM_STATUS);
}


/***********************************************************************
 * Incoming Messages
 ***********************************************************************/

/*! \brief Process a STATUS message from the SIMtrace2 */
static int process_do_status(struct cardem_inst *ci, uint8_t *buf, int len)
{
	struct cardemu_usb_msg_status *status;
	status = (struct cardemu_usb_msg_status *) buf;

	printf("=> STATUS: flags=0x%x, fi=%u, di=%u, wi=%u wtime=%u\n",
		status->flags, status->fi, status->di, status->wi,
		status->waiting_time);

	return 0;
}

/*! \brief Process a PTS indication message from the SIMtrace2 */
static int process_do_pts(struct cardem_inst *ci, uint8_t *buf, int len)
{
	struct cardemu_usb_msg_pts_info *pts;
	pts = (struct cardemu_usb_msg_pts_info *) buf;

	printf("=> PTS req: %s\n", osmo_hexdump(pts->req, sizeof(pts->req)));

	return 0;
}

/*! \brief Process a ERROR indication message from the SIMtrace2 */
static int process_do_error(struct cardem_inst *ci, uint8_t *buf, int len)
{
	struct cardemu_usb_msg_error *err;
	err = (struct cardemu_usb_msg_error *) buf;

	printf("=> ERROR: %u/%u/%u: %s\n",
		err->severity, err->subsystem, err->code,
		err->msg_len ? (char *)err->msg : "");

	return 0;
}

/*! \brief Process a RX-DATA indication message from the SIMtrace2 */
static int process_do_rx_da(struct cardem_inst *ci, uint8_t *buf, int len)
{
	static struct apdu_context ac;
	struct cardemu_usb_msg_rx_data *data;
	int rc;

	data = (struct cardemu_usb_msg_rx_data *) buf;

	printf("=> DATA: flags=%x, %s: ", data->flags,
		osmo_hexdump(data->data, data->data_len));

	rc = apdu_segment_in(&ac, data->data, data->data_len,
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
			cardem_request_pb_and_tx(ci, ac.hdr.ins, tmsg->l3h, msgb_l3len(tmsg));
		cardem_request_sw_tx(ci, ac.sw);
	} else if (ac.lc.tot > ac.lc.cur) {
		cardem_request_pb_and_rx(ci, ac.hdr.ins, ac.lc.tot - ac.lc.cur);
	}
	return 0;
}

#if 0
	case SIMTRACE_CMD_DO_ERROR
		rc = process_do_error(ci, buf, len);
		break;
#endif

/*! \brief Process an incoming message from the SIMtrace2 */
static int process_usb_msg(struct cardem_inst *ci, uint8_t *buf, int len)
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
	{ "usb-vendor", 1, 0, 'V' },
	{ "usb-product", 1, 0, 'P' },
	{ "usb-config", 1, 0, 'C' },
	{ "usb-interface", 1, 0, 'I' },
	{ "usb-altsetting", 1, 0, 'S' },
	{ "usb-address", 1, 0, 'A' },
	{ "usb-path", 1, 0, 'H' },
	{ NULL, 0, 0, 0 }
};

static void run_mainloop(struct cardem_inst *ci)
{
	struct st_transport *transp = ci->slot->transp;
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

static struct st_transport _transp;

static struct st_slot _slot = {
	.transp = &_transp,
	.slot_nr = 0,
};

struct cardem_inst _ci = {
	.slot = &_slot,
};

struct cardem_inst *ci = &_ci;

static void signal_handler(int signal)
{
	switch (signal) {
	case SIGINT:
		cardem_request_card_insert(ci, false);
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char **argv)
{
	struct st_transport *transp = ci->slot->transp;
	char *gsmtap_host = "127.0.0.1";
	int rc;
	int c, ret = 1;
	int skip_atr = 0;
	int keep_running = 0;
	int remote_udp_port = 52342;
	int if_num = 0, vendor_id = -1, product_id = -1;
	int config_id = -1, altsetting = 0, addr = -1;
	char *remote_udp_host = NULL;
	char *path = NULL;
	struct osim_reader_hdl *reader;
	struct osim_card_hdl *card;

	print_welcome();

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "r:p:hi:V:P:C:I:S:A:H:ak", opts, &option_index);
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
			transp->usb_devh = usb_open_claim_interface(NULL, ifm);
			if (!transp->usb_devh) {
				fprintf(stderr, "can't open USB device\n");
				goto close_exit;
			}

			rc = libusb_claim_interface(transp->usb_devh, if_num);
			if (rc < 0) {
				fprintf(stderr, "can't claim interface %d; rc=%d\n", if_num, rc);
				goto close_exit;
			}

			rc = get_usb_ep_addrs(transp->usb_devh, if_num, &transp->usb_ep.out,
					      &transp->usb_ep.in, &transp->usb_ep.irq_in);
			if (rc < 0) {
				fprintf(stderr, "can't obtain EP addrs; rc=%d\n", rc);
				goto close_exit;
			}
		}

		/* simulate card-insert to modem (owhw, not qmod) */
		cardem_request_card_insert(ci, true);

		/* select remote (forwarded) SIM */
		st_modem_sim_select_remote(ci->slot);

		if (!skip_atr) {
			/* set the ATR */
			uint8_t real_atr[] = {  0x3B, 0x9F, 0x96, 0x80, 0x1F, 0xC7, 0x80, 0x31,
						0xA0, 0x73, 0xBE, 0x21, 0x13, 0x67, 0x43, 0x20,
						0x07, 0x18, 0x00, 0x00, 0x01, 0xA5 };
			atr_update_csum(real_atr, sizeof(real_atr));
			cardem_request_set_atr(ci, real_atr, sizeof(real_atr));
		}

		/* select remote (forwarded) SIM */
		st_modem_reset_pulse(ci->slot, 300);

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
