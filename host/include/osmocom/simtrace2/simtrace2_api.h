#pragma once

#include <stdint.h>
#include <osmocom/sim/sim.h>

/* transport to a SIMtrace device */
struct osmo_st2_transport {
	/* USB */
	struct libusb_device_handle *usb_devh;
	struct {
		uint8_t in;
		uint8_t out;
		uint8_t irq_in;
	} usb_ep;
	/* use non-blocking / asynchronous libusb I/O */
	bool usb_async;

	/* UDP */
	int udp_fd;
};

/* a SIMtrace slot; communicates over a transport */
struct osmo_st2_slot {
	/* transport through which the slot can be reached */
	struct osmo_st2_transport *transp;
	/* number of the slot within the transport */
	uint8_t slot_nr;
};

/* One istance of card emulation */
struct osmo_st2_cardem_inst {
	/* slot on which this card emulation instance runs */
	struct osmo_st2_slot *slot;
	/* libosmosim SIM card profile */
	const struct osim_cla_ins_card_profile *card_prof;
	/* libosmosim SIM card channel */
	struct osim_chan_hdl *chan;
	/* path of the underlying USB device */
	char *usb_path;
	/* opaque data TBD by user */
	void *priv;
};

int osmo_st2_slot_tx_msg(struct osmo_st2_slot *slot, struct msgb *msg,
                         uint8_t msg_class, uint8_t msg_type);


int osmo_st2_cardem_request_card_insert(struct osmo_st2_cardem_inst *ci, bool inserted);
int osmo_st2_cardem_request_pb_and_rx(struct osmo_st2_cardem_inst *ci, uint8_t pb, uint8_t le);
int osmo_st2_cardem_request_pb_and_tx(struct osmo_st2_cardem_inst *ci, uint8_t pb,
				      const uint8_t *data, uint16_t data_len_in);
int osmo_st2_cardem_request_sw_tx(struct osmo_st2_cardem_inst *ci, const uint8_t *sw);
int osmo_st2_cardem_request_set_atr(struct osmo_st2_cardem_inst *ci, const uint8_t *atr,
				    unsigned int atr_len);
int osmo_st2_cardem_request_config(struct osmo_st2_cardem_inst *ci, uint32_t features);


int osmo_st2_modem_reset_pulse(struct osmo_st2_slot *slot, uint16_t duration_ms);
int osmo_st2_modem_reset_active(struct osmo_st2_slot *slot);
int osmo_st2_modem_reset_inactive(struct osmo_st2_slot *slot);
int osmo_st2_modem_sim_select_local(struct osmo_st2_slot *slot);
int osmo_st2_modem_sim_select_remote(struct osmo_st2_slot *slot);
int osmo_st2_modem_get_status(struct osmo_st2_slot *slot);
