
/* simtrace2-protocol - USB protocol library code for SIMtrace2
 *
 * (C) 2016-2019 by Harald Welte <hwelte@hmw-consulting.de>
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

#include <osmocom/simtrace2/simtrace_prot.h>
#include <osmocom/simtrace2/simtrace2_api.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/msgb.h>
#include <osmocom/sim/class_tables.h>
#include <osmocom/sim/sim.h>

/***********************************************************************
 * SIMTRACE core protocol
 ***********************************************************************/

/*! \brief allocate a message buffer for simtrace use */
static struct msgb *st_msgb_alloc(void)
{
	return msgb_alloc_headroom(1024+32, 32, "SIMtrace");
}


static void usb_out_xfer_cb(struct libusb_transfer *xfer)
{
	struct msgb *msg = xfer->user_data;

	switch (xfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		fprintf(stderr, "USB device disappeared\n");
		exit(1);
		break;
	default:
		fprintf(stderr, "USB OUT transfer failed, status=%u\n", xfer->status);
		exit(1);
		break;
	}

	msgb_free(msg);
	libusb_free_transfer(xfer);
}


static int st2_transp_tx_msg_usb_async(struct osmo_st2_transport *transp, struct msgb *msg)
{
	struct libusb_transfer *xfer;
	int rc;

	xfer = libusb_alloc_transfer(0);
	OSMO_ASSERT(xfer);
	xfer->dev_handle = transp->usb_devh;
	xfer->flags = 0;
	xfer->type = LIBUSB_TRANSFER_TYPE_BULK;
	xfer->endpoint = transp->usb_ep.out;
	xfer->timeout = 100000;
	xfer->user_data = msg;
	xfer->length = msgb_length(msg);
	xfer->buffer = msgb_data(msg);
	xfer->callback = usb_out_xfer_cb;

	rc = libusb_submit_transfer(xfer);
	OSMO_ASSERT(rc == 0);

	return rc;
}

/*! \brief Transmit a given command to the SIMtrace2 device */
static int st2_transp_tx_msg_usb_sync(struct osmo_st2_transport *transp, struct msgb *msg)
{
	int rc;
	int xfer_len;
	rc = libusb_bulk_transfer(transp->usb_devh, transp->usb_ep.out,
				  msgb_data(msg), msgb_length(msg),
				  &xfer_len, 100000);
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
int osmo_st2_slot_tx_msg(struct osmo_st2_slot *slot, struct msgb *msg,
			 uint8_t msg_class, uint8_t msg_type)
{
	struct osmo_st2_transport *transp = slot->transp;
	int rc;

	OSMO_ASSERT(transp);

	st_push_hdr(msg, msg_class, msg_type, slot->slot_nr);
	printf("SIMtrace <- %s\n", msgb_hexdump(msg));

	if (transp->udp_fd < 0) {
		if (transp->usb_async)
			rc = st2_transp_tx_msg_usb_async(transp, msg);
		else
			rc = st2_transp_tx_msg_usb_sync(transp, msg);
	} else {
		rc = write(transp->udp_fd, msgb_data(msg), msgb_length(msg));
		msgb_free(msg);
	}
	return rc;
}

/***********************************************************************
 * Card Emulation protocol
 ***********************************************************************/


/*! \brief Request the SIMtrace2 to generate a card-insert signal */
int osmo_st2_cardem_request_card_insert(struct osmo_st2_cardem_inst *ci, bool inserted)
{
	struct msgb *msg = st_msgb_alloc();
	struct cardemu_usb_msg_cardinsert *cins;

	cins = (struct cardemu_usb_msg_cardinsert *) msgb_put(msg, sizeof(*cins));
	memset(cins, 0, sizeof(*cins));
	if (inserted)
		cins->card_insert = 1;

	return osmo_st2_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_CARDINSERT);
}

/*! \brief Request the SIMtrace2 to transmit a Procedure Byte, then Rx */
int osmo_st2_cardem_request_pb_and_rx(struct osmo_st2_cardem_inst *ci, uint8_t pb, uint8_t le)
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

	return osmo_st2_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_TX_DATA);
}

/*! \brief Request the SIMtrace2 to transmit a Procedure Byte, then Tx */
int osmo_st2_cardem_request_pb_and_tx(struct osmo_st2_cardem_inst *ci, uint8_t pb,
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

	return osmo_st2_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_TX_DATA);
}

/*! \brief Request the SIMtrace2 to send a Status Word */
int osmo_st2_cardem_request_sw_tx(struct osmo_st2_cardem_inst *ci, const uint8_t *sw)
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

	return osmo_st2_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_TX_DATA);
}

int osmo_st2_cardem_request_set_atr(struct osmo_st2_cardem_inst *ci, const uint8_t *atr, unsigned int atr_len)
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

	return osmo_st2_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DT_CEMU_SET_ATR);
}

int osmo_st2_cardem_request_config(struct osmo_st2_cardem_inst *ci, uint32_t features)
{
	struct msgb *msg = st_msgb_alloc();
	struct cardemu_usb_msg_config *cfg;

	cfg = (struct cardemu_usb_msg_config *) msgb_put(msg, sizeof(*cfg));

	printf("<= %s(%08x)\n", __func__, features);

	memset(cfg, 0, sizeof(*cfg));
	cfg->features = features;

	return osmo_st2_slot_tx_msg(ci->slot, msg, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_BD_CEMU_CONFIG);
}

/***********************************************************************
 * Modem Control protocol
 ***********************************************************************/

static int _modem_reset(struct osmo_st2_slot *slot, uint8_t asserted, uint16_t pulse_ms)
{
	struct msgb *msg = st_msgb_alloc();
	struct st_modem_reset *sr ;

	sr = (struct st_modem_reset *) msgb_put(msg, sizeof(*sr));
	sr->asserted = asserted;
	sr->pulse_duration_msec = pulse_ms;

	return osmo_st2_slot_tx_msg(slot, msg, SIMTRACE_MSGC_MODEM, SIMTRACE_MSGT_DT_MODEM_RESET);
}

/*! \brief pulse the RESET line of the modem for \a duration_ms milli-seconds*/
int osmo_st2_modem_reset_pulse(struct osmo_st2_slot *slot, uint16_t duration_ms)
{
	return _modem_reset(slot, 2, duration_ms);
}

/*! \brief assert the RESET line of the modem */
int osmo_st2_modem_reset_active(struct osmo_st2_slot *slot)
{
	return _modem_reset(slot, 1, 0);
}

/*! \brief de-assert the RESET line of the modem */
int osmo_st2_modem_reset_inactive(struct osmo_st2_slot *slot)
{
	return _modem_reset(slot, 0, 0);
}

static int _modem_sim_select(struct osmo_st2_slot *slot, uint8_t remote_sim)
{
	struct msgb *msg = st_msgb_alloc();
	struct st_modem_sim_select *ss;

	ss = (struct st_modem_sim_select *) msgb_put(msg, sizeof(*ss));
	ss->remote_sim = remote_sim;

	return osmo_st2_slot_tx_msg(slot, msg, SIMTRACE_MSGC_MODEM, SIMTRACE_MSGT_DT_MODEM_SIM_SELECT);
}

/*! \brief select local (physical) SIM for given slot */
int osmo_st2_modem_sim_select_local(struct osmo_st2_slot *slot)
{
	return _modem_sim_select(slot, 0);
}

/*! \brief select remote (emulated/forwarded) SIM for given slot */
int osmo_st2_modem_sim_select_remote(struct osmo_st2_slot *slot)
{
	return _modem_sim_select(slot, 1);
}

/*! \brief Request slot to send us status information about the modem */
int osmo_st2_modem_get_status(struct osmo_st2_slot *slot)
{
	struct msgb *msg = st_msgb_alloc();

	return osmo_st2_slot_tx_msg(slot, msg, SIMTRACE_MSGC_MODEM, SIMTRACE_MSGT_BD_MODEM_STATUS);
}
