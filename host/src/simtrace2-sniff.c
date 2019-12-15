/* simtrace2-sniff - main program for the host PC to communicate with the 
 * SIMtrace 2 firmware in sniffer mode
 * 
 * (C) 2016 by Harald Welte <hwelte@hmw-consulting.de>
 * (C) 2018 by sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
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
#include <osmocom/simtrace2/simtrace_usb.h>
#include <osmocom/simtrace2/simtrace_prot.h>

#include <osmocom/simtrace2/gsmtap.h>

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
};

const struct value_string change_flags[] = {
	{ SNIFF_CHANGE_FLAG_CARD_INSERT, "card inserted" },
	{ SNIFF_CHANGE_FLAG_CARD_EJECT, "card ejected" },
	{ SNIFF_CHANGE_FLAG_RESET_ASSERT, "reset asserted" },
	{ SNIFF_CHANGE_FLAG_RESET_DEASSERT, "reset de-asserted" },
	{ SNIFF_CHANGE_FLAG_TIMEOUT_WT, "data transfer timeout" },
	{ 0, NULL }
};

const struct value_string data_flags[] = {
	{ SNIFF_DATA_FLAG_ERROR_INCOMPLETE, "incomplete" },
	{ SNIFF_DATA_FLAG_ERROR_MALFORMED, "malformed" },
	{ SNIFF_DATA_FLAG_ERROR_CHECKSUM, "checksum error" },
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

static int process_change(const uint8_t *buf, int len)
{
	/* check if there is enough data for the structure */
	if (len < sizeof(struct sniff_change)) {
		return -1;
	}
	struct sniff_change *change = (struct sniff_change *)buf;

	printf("Card state change: ");
	if (change->flags) {
		print_flags(change_flags, ARRAY_SIZE(change_flags), change->flags);
		printf("\n");
	} else {
		printf("no changes\n");
	}

	return 0;
}

/* Table 7 of ISO 7816-3:2006 */
static const uint16_t fi_table[] = { 372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048, 0, 0, };

/* Table 8 from ISO 7816-3:2006 */
static const uint8_t di_table[] = { 0, 1, 2, 4, 8, 16, 32, 64, 12, 20, 2, 4, 8, 16, 32, 64, };

static int process_fidi(const uint8_t *buf, int len)
{
	/* check if there is enough data for the structure */
	if (len<sizeof(struct sniff_fidi)) {
		return -1;
	}
	struct sniff_fidi *fidi = (struct sniff_fidi *)buf;

	printf("Fi/Di switched to %u/%u\n", fi_table[fidi->fidi>>4], di_table[fidi->fidi&0x0f]);
	return 0;
}

static int process_data(enum simtrace_msg_type_sniff type, const uint8_t *buf, int len)
{
	/* check if there is enough data for the structure */
	if (len < sizeof(struct sniff_data)) {
		return -1;
	}
	struct sniff_data *data = (struct sniff_data *)buf;

	/* check if the data is available */
	if (len < sizeof(struct sniff_data) + data->length) {
		return -2;
	}

	/* check type */
	if (type != SIMTRACE_MSGT_SNIFF_ATR && type != SIMTRACE_MSGT_SNIFF_PPS && type != SIMTRACE_MSGT_SNIFF_TPDU) {
		return -3;
	}

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
	if (data->flags) {
		printf(" (");
		print_flags(data_flags, ARRAY_SIZE(data_flags), data->flags);
		printf(")");
	}
	printf(": ");
	uint16_t i;
	for (i = 0; i < data->length; i++) {
		printf("%02x ", data->data[i]);
	}
	printf("\n");

	/* Send message as GSNTAP */
	switch (type) {
	case SIMTRACE_MSGT_SNIFF_ATR:
		osmo_st2_gsmtap_send_apdu(GSMTAP_SIM_ATR, data->data, data->length);
		break;
	case SIMTRACE_MSGT_SNIFF_TPDU:
		/* TPDU is now considered as APDU since SIMtrace sends complete TPDU */
		osmo_st2_gsmtap_send_apdu(GSMTAP_SIM_APDU, data->data, data->length);
		break;
	default:
		break;
	}

	return 0;
}

/*! \brief Process an incoming message from the SIMtrace2 */
static int process_usb_msg(const uint8_t *buf, int len)
{
	/* check if enough data for the header is present */
	if (len < sizeof(struct simtrace_msg_hdr)) {
		return 0;
	}

	/* check if message is complete */
	struct simtrace_msg_hdr *msg_hdr = (struct simtrace_msg_hdr *)buf;
	if (len < msg_hdr->msg_len) {
		return 0;
	}
	//printf("msg: %s\n", osmo_hexdump(buf, msg_hdr->msg_len));

	/* check for message class */
	if (SIMTRACE_MSGC_SNIFF != msg_hdr->msg_class) { /* we only care about sniffing messages */
		return msg_hdr->msg_len; /* discard non-sniffing messaged */
	}

	/* process sniff message payload */
	buf += sizeof(struct simtrace_msg_hdr);
	len -= sizeof(struct simtrace_msg_hdr);
	switch (msg_hdr->msg_type) {
	case SIMTRACE_MSGT_SNIFF_CHANGE:
		process_change(buf, len);
		break;
	case SIMTRACE_MSGT_SNIFF_FIDI:
		process_fidi(buf, len);
		break;
	case SIMTRACE_MSGT_SNIFF_ATR:
	case SIMTRACE_MSGT_SNIFF_PPS:
	case SIMTRACE_MSGT_SNIFF_TPDU:
		process_data(msg_hdr->msg_type, buf, len);
		break;
	default:
		printf("unknown SIMtrace msg type 0x%02x\n", msg_hdr->msg_type);
		break;
	}

	return msg_hdr->msg_len;
}

/*! Transport to SIMtrace device (e.g. USB handle) */
static struct st_transport _transp;

static void run_mainloop()
{
	int rc;
	uint8_t buf[16*256];
	unsigned int i, buf_i = 0;
	int xfer_len;

	printf("Entering main loop\n");

	while (true) {
		/* read data from SIMtrace2 device (via USB) */
		rc = libusb_bulk_transfer(_transp.usb_devh, _transp.usb_ep.in,
					  &buf[buf_i], sizeof(buf)-buf_i, &xfer_len, 100000);
		if (rc < 0 && rc != LIBUSB_ERROR_TIMEOUT &&
		              rc != LIBUSB_ERROR_INTERRUPTED &&
		              rc != LIBUSB_ERROR_IO) {
			fprintf(stderr, "BULK IN transfer error; rc=%d\n", rc);
			return;
		}
		/* dispatch any incoming data */
		if (xfer_len > 0) {
			//printf("URB: %s\n", osmo_hexdump(&buf[buf_i], xfer_len));
			buf_i += xfer_len;
			if (buf_i >= sizeof(buf)) {
				perror("preventing USB buffer overflow");
				return;
			}
			int processed;
			while ((processed = process_usb_msg(buf, buf_i)) > 0) {
				if (processed > buf_i) {
					break;
				}
				for (i = processed; i < buf_i; i++) {
					buf[i-processed] = buf[i];
				}
				buf_i -= processed;
			}
		}
	}
}

static void print_welcome(void)
{
	printf("simtrace2-sniff - Phone-SIM card communication sniffer \n"
	       "(C) 2010-2017 by Harald Welte <laforge@gnumonks.org>\n"
	       "(C) 2018 by Kevin Redon <kredon@sysmocom.de>\n"
	       "\n"
	);
}

static void print_help(void)
{
	printf(
		"\t-h\t--help\n"
		"\t-i\t--gsmtap-ip\tA.B.C.D\n"
		"\t-k\t--keep-running\n"
		"\t-V\t--usb-vendor\tVENDOR_ID\n"
		"\t-P\t--usb-product\tPRODUCT_ID\n"
		"\t-C\t--usb-config\tCONFIG_ID\n"
		"\t-I\t--usb-interface\tINTERFACE_ID\n"
		"\t-S\t--usb-altsetting ALTSETTING_ID\n"
		"\t-A\t--usb-address\tADDRESS\n"
		"\n"
		);
}

static const struct option opts[] = {
	{ "help", 0, 0, 'h' },
	{ "gsmtap-ip", 1, 0, 'i' },
	{ "keep-running", 0, 0, 'k' },
	{ "usb-vendor", 1, 0, 'V' },
	{ "usb-product", 1, 0, 'P' },
	{ "usb-config", 1, 0, 'C' },
	{ "usb-interface", 1, 0, 'I' },
	{ "usb-altsetting", 1, 0, 'S' },
	{ "usb-address", 1, 0, 'A' },
	{ NULL, 0, 0, 0 }
};

/* Known USB device with SIMtrace firmware supporting sniffer */
static const struct dev_id compatible_dev_ids[] = {
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_SIMTRACE2 },
	{ 0, 0 }
};

static void signal_handler(int signal)
{
	switch (signal) {
	case SIGINT:
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char **argv)
{
	int i, rc, ret;
	print_welcome();

	/* Parse arguments */
	char *gsmtap_host = "127.0.0.1";
	int keep_running = 0;
	int vendor_id = -1, product_id = -1, addr = -1, config_id = -1, if_num = -1, altsetting = -1;

	while (1) {
		int option_index = 0;

		char c = getopt_long(argc, argv, "hi:kV:P:C:I:S:A:", opts, &option_index);
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
		}
	}

	/* Scan for available SIMtrace USB devices supporting sniffing */
	rc = libusb_init(NULL);
	if (rc < 0) {
		fprintf(stderr, "libusb initialization failed\n");
		goto do_exit;
	}
	struct usb_interface_match ifm_scan[16];
	int num_interfaces = osmo_libusb_find_matching_interfaces(NULL, compatible_dev_ids,
				 USB_CLASS_PROPRIETARY, SIMTRACE_SNIFFER_USB_SUBCLASS, -1, ifm_scan, ARRAY_SIZE(ifm_scan));
	if (num_interfaces <= 0) {
		perror("No compatible USB devices found");
		goto do_exit;
	}

	/* Only keep USB matching arguments */
	struct usb_interface_match ifm_filtered[ARRAY_SIZE(ifm_scan)];
	int num_filtered = 0;
	for (i = 0; i < num_interfaces; i++) {
		if (vendor_id>=0 && vendor_id!=ifm_scan[i].vendor) {
			continue;
		}
		if (product_id>=0 && product_id!=ifm_scan[i].product) {
			continue;
		}
		if (config_id>=0 && config_id!=ifm_scan[i].configuration) {
			continue;
		}
		if (if_num>=0 && if_num!=ifm_scan[i].interface) {
			continue;
		}
		if (altsetting>=0 && altsetting!=ifm_scan[i].altsetting) {
			continue;
		}
		if (addr>=0 && addr!=ifm_scan[i].addr) {
			continue;
		}
		ifm_filtered[num_filtered++] = ifm_scan[i];
	}
	if (1!=num_filtered) {
		perror("No individual matching USB devices found");
		printf("Available USB devices:\n");
		for (i = 0; i < num_interfaces; i++) {
			printf("\t%04x:%04x Addr=%u, Path=%s, Cfg=%u, Intf=%u, Alt=%u: %d/%d/%d ",
			ifm_scan[i].vendor, ifm_scan[i].product, ifm_scan[i].addr, ifm_scan[i].path,
			ifm_scan[i].configuration, ifm_scan[i].interface, ifm_scan[i].altsetting,
			ifm_scan[i].class, ifm_scan[i].sub_class, ifm_scan[i].protocol);
			libusb_device_handle *dev_handle;
			rc = libusb_open(ifm_scan[i].usb_dev, &dev_handle);
			if (rc < 0) {
				printf("\n");
				perror("Cannot open device");
				continue;
			}
			char strbuf[256];
			rc = libusb_get_string_descriptor_ascii(dev_handle, ifm_scan[i].string_idx,
					(unsigned char *)strbuf, sizeof(strbuf));
			libusb_close(dev_handle);
			if (rc < 0) {
				printf("\n");
				perror("Cannot read string");
				continue;
			}
			printf("(%s)\n", strbuf);
		}
		goto do_exit;
	}
	struct usb_interface_match ifm_selected = ifm_filtered[0];
	printf("Using USB device %04x:%04x Addr=%u, Path=%s, Cfg=%u, Intf=%u, Alt=%u: %d/%d/%d ",
	ifm_selected.vendor, ifm_selected.product, ifm_selected.addr, ifm_selected.path,
	ifm_selected.configuration, ifm_selected.interface, ifm_selected.altsetting,
	ifm_selected.class, ifm_selected.sub_class, ifm_selected.protocol);
	libusb_device_handle *dev_handle;
	rc = libusb_open(ifm_selected.usb_dev, &dev_handle);
	if (rc < 0) {
		printf("\n");
		perror("Cannot open device");
	}
	char strbuf[256];
	rc = libusb_get_string_descriptor_ascii(dev_handle, ifm_selected.string_idx,
			(unsigned char *)strbuf, sizeof(strbuf));
	libusb_close(dev_handle);
	if (rc < 0) {
		printf("\n");
		perror("Cannot read string");
	}
	printf("(%s)\n", strbuf);

	rc = osmo_st2_gsmtap_init(gsmtap_host);
	if (rc < 0) {
		perror("unable to open GSMTAP");
		goto close_exit;
	}

	signal(SIGINT, &signal_handler);

	do {
		_transp.usb_devh = osmo_libusb_open_claim_interface(NULL, NULL, &ifm_selected);
		if (!_transp.usb_devh) {
			fprintf(stderr, "can't open USB device\n");
			goto close_exit;
		}

		rc = libusb_claim_interface(_transp.usb_devh, ifm_selected.interface);
		if (rc < 0) {
			fprintf(stderr, "can't claim interface %d; rc=%d\n", ifm_selected.interface, rc);
			goto close_exit;
		}

		rc = osmo_libusb_get_ep_addrs(_transp.usb_devh, ifm_selected.interface, &_transp.usb_ep.out,
					      &_transp.usb_ep.in, &_transp.usb_ep.irq_in);
		if (rc < 0) {
			fprintf(stderr, "can't obtain EP addrs; rc=%d\n", rc);
			goto close_exit;
		}

		run_mainloop();
		ret = 0;

		if (_transp.usb_devh)
			libusb_release_interface(_transp.usb_devh, 0);
close_exit:
		if (_transp.usb_devh)
			libusb_close(_transp.usb_devh);
		if (keep_running)
			sleep(1);
	} while (keep_running);

	libusb_exit(NULL);
do_exit:
	return ret;
}
