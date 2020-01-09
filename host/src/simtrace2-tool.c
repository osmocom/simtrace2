/* simtrace2-tool - main program for the host PC to provide a remote SIM
 * using the SIMtrace 2 firmware in card emulation mode
 *
 * (C) 2019 by Harald Welte <hwelte@hmw-consulting.de>
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
#define _GNU_SOURCE
#include <getopt.h>

#include <sys/types.h>

#include <libusb.h>

#include <osmocom/usb/libusb.h>
#include <osmocom/simtrace2/simtrace2_api.h>
#include <osmocom/simtrace2/simtrace_prot.h>
#include <osmocom/simtrace2/gsmtap.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/msgb.h>

/***********************************************************************
 * Incoming Messages
 ***********************************************************************/

static void print_welcome(void)
{
	printf("simtrace2-tool\n"
	       "(C) 2019 Harald Welte <laforge@gnumonks.org>\n");
}

static void print_help(void)
{
	printf( "simtrace2-tool [OPTIONS] COMMAND\n\n");
	printf( "Options:\n"
		"\t-h\t--help\n"
		"\t-V\t--usb-vendor\tVENDOR_ID\n"
		"\t-P\t--usb-product\tPRODUCT_ID\n"
		"\t-C\t--usb-config\tCONFIG_ID\n"
		"\t-I\t--usb-interface\tINTERFACE_ID\n"
		"\t-S\t--usb-altsetting ALTSETTING_ID\n"
		"\t-A\t--usb-address\tADDRESS\n"
		"\t-H\t--usb-path\tPATH\n"
		"\n"
		);
	printf( "Commands:\n"
		"\tmodem reset (enable|disable|cycle)\n"
		"\tmodem sim-switch (local|remote)\n"
		"\n");
}

static const struct option opts[] = {
	{ "help", 0, 0, 'h' },
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
	uint8_t buf[16*265];
	int xfer_len;
	int rc;

	while (1) {
		/* read data from SIMtrace2 device */
		rc = libusb_bulk_transfer(transp->usb_devh, transp->usb_ep.in,
					  buf, sizeof(buf), &xfer_len, 100);
		if (rc < 0 && rc != LIBUSB_ERROR_TIMEOUT &&
			      rc != LIBUSB_ERROR_INTERRUPTED &&
			      rc != LIBUSB_ERROR_IO) {
			fprintf(stderr, "BULK IN transfer error; rc=%d\n", rc);
			return;
		}
		/* break the loop if no new messages arrive within 100ms */
		if (rc == LIBUSB_ERROR_TIMEOUT)
			return;
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

/* perform a modem reset */
static int do_modem_reset(int argc, char **argv)
{
	char *command;
	if (argc < 1)
		command = "cycle";
	else {
		command = argv[0];
		argc--;
		argv++;
	}

	if (!strcmp(command, "enable")) {
		printf("Activating Modem RESET\n");
		return osmo_st2_modem_reset_active(ci->slot);
	} else if (!strcmp(command, "disable")) {
		printf("Deactivating Modem RESET\n");
		return osmo_st2_modem_reset_inactive(ci->slot);
	} else if (!strcmp(command, "cycle")) {
		printf("Pulsing Modem RESET (1s)\n");
		return osmo_st2_modem_reset_pulse(ci->slot, 1000);
	} else {
		fprintf(stderr, "Unsupported modem reset command: '%s'\n", command);
		return -EINVAL;
	}
	return 0;
}

/* switch between local and remote (emulated) SIM */
static int do_modem_sim_switch(int argc, char **argv)
{
	char *command;
	if (argc < 1)
		return -EINVAL;
	command = argv[0];
	argc--;
	argv++;

	if (!strcmp(command, "local")) {
		printf("Setting SIM=LOCAL; Modem reset recommended\n");
		return osmo_st2_modem_sim_select_local(ci->slot);
	} else if (!strcmp(command, "remote")) {
		printf("Setting SIM=REMOTE; Modem reset recommended\n");
		return osmo_st2_modem_sim_select_remote(ci->slot);
	} else {
		fprintf(stderr, "Unsupported modem sim-switch command: '%s'\n", command);
		return -EINVAL;
	}
	return 0;
}

static int do_subsys_modem(int argc, char **argv)
{
	char *command;
	int rc;

	if (argc < 1)
		return -EINVAL;
	command = argv[0];
	argc--;
	argv++;

	if (!strcmp(command, "reset")) {
		rc = do_modem_reset(argc, argv);
	} else if (!strcmp(command, "sim-switch")) {
		rc = do_modem_sim_switch(argc, argv);
	} else {
		fprintf(stderr, "Unsupported command for subsystem modem: '%s'\n", command);
		return -EINVAL;
	}

	return rc;
}

static int do_command(int argc, char **argv)
{
	char *subsys;
	int rc;

	if (argc < 1)
		return -EINVAL;
	subsys = argv[0];
	argc--;
	argv++;

	if (!strcmp(subsys, "modem"))
		rc = do_subsys_modem(argc, argv);
	else {
		fprintf(stderr, "Unsupported subsystem '%s'\n", subsys);
		rc = -EINVAL;
	}

	return rc;
}


int main(int argc, char **argv)
{
	struct osmo_st2_transport *transp = ci->slot->transp;
	int rc;
	int c, ret = 1;
	int if_num = 0, vendor_id = -1, product_id = -1;
	int config_id = -1, altsetting = 0, addr = -1;
	char *path = NULL;

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "hV:P:C:I:S:A:H:", opts, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			print_help();
			exit(0);
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

	if ((vendor_id < 0 || product_id < 0)) {
		fprintf(stderr, "You have to specify the vendor and product ID\n");
		goto do_exit;
	}

	transp->udp_fd = -1;

	rc = libusb_init(NULL);
	if (rc < 0) {
		fprintf(stderr, "libusb initialization failed\n");
		goto do_exit;
	}

	print_welcome();

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

		if (argc - optind <= 0) {
			fprintf(stderr, "You have to specify a command to execute\n");
			exit(1);
		}

		rc = do_command(argc-optind, argv+optind);
		switch (rc) {
		case 0:
			break;
		case -EINVAL:
			fprintf(stderr, "Error: Invalid command/syntax\n");
			exit(1);
			break;
		default:
			fprintf(stderr, "Error executing command: %d\n", rc);
			exit(1);
			break;
		}

		run_mainloop(ci);
		ret = 0;

		libusb_release_interface(transp->usb_devh, 0);
close_exit:
		if (transp->usb_devh)
			libusb_close(transp->usb_devh);
	} while (0);

	libusb_exit(NULL);
do_exit:
	return ret;
}
