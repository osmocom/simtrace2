#pragma once

#include <stdint.h>
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


int cardem_request_card_insert(struct cardem_inst *ci, bool inserted);
int cardem_request_pb_and_rx(struct cardem_inst *ci, uint8_t pb, uint8_t le);
int cardem_request_pb_and_tx(struct cardem_inst *ci, uint8_t pb,
			     const uint8_t *data, uint16_t data_len_in);
int cardem_request_sw_tx(struct cardem_inst *ci, const uint8_t *sw);
int cardem_request_set_atr(struct cardem_inst *ci, const uint8_t *atr, unsigned int atr_len);


int st_modem_reset_pulse(struct st_slot *slot, uint16_t duration_ms);
int st_modem_reset_active(struct st_slot *slot);
int st_modem_reset_inactive(struct st_slot *slot);
int st_modem_sim_select_local(struct st_slot *slot);
int st_modem_sim_select_remote(struct st_slot *slot);
int st_modem_get_status(struct st_slot *slot);
