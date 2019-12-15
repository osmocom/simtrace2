/* simtrace - main program for the host PC
 *
 * (C) 2010-2016 by Harald Welte <hwelte@hmw-consulting.de>
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
#include <time.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <poll.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libusb.h>

#include <osmocom/usb/libusb.h>
#include <osmocom/simtrace2/simtrace_usb.h>
#include <osmocom/simtrace2/simtrace_prot.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/select.h>

struct libusb_device_handle *g_devh;
static struct sockaddr_in g_sa_remote;
static struct osmo_fd g_udp_ofd;

static void print_welcome(void)
{
	printf("usb2udp - UDP/IP forwarding of SIMtrace card emulation\n"
	       "(C) 2016 by Harald Welte <laforge@gnumonks.org>\n\n");
}

static void print_help(void)
{
	printf( "\t-h\t--help\n"
		"\t-i\t--interface <0-255>\n"
		"\n"
		);
}

struct ep_buf {
	uint8_t ep;
	uint8_t buf[1024];
	struct libusb_transfer *xfer;
};
static struct ep_buf g_buf_in;
static struct ep_buf g_buf_out;

static void usb_in_xfer_cb(struct libusb_transfer *xfer)
{
	int rc;

	printf("xfer_cb(ep=%02x): status=%d, flags=0x%x, type=%u, len=%u, act_len=%u\n",
		xfer->endpoint, xfer->status, xfer->flags, xfer->type, xfer->length, xfer->actual_length);
	switch (xfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		if (xfer->endpoint == g_buf_in.ep) {
			/* process the data */
			printf("read %d bytes from SIMTRACE, forwarding to UDP\n", xfer->actual_length);
			rc = sendto(g_udp_ofd.fd, xfer->buffer, xfer->actual_length, 0, (struct sockaddr *)&g_sa_remote, sizeof(g_sa_remote));
			if (rc <= 0) {
				fprintf(stderr, "error writing to UDP\n");
			}
			/* and re-submit the URB */
			libusb_submit_transfer(xfer);
		} else if (xfer->endpoint == g_buf_out.ep) {
			/* re-enable reading from the UDP side */
			g_udp_ofd.when |= BSC_FD_READ;
		}
		break;
	default:
		fprintf(stderr, "xfer_cb(ERROR '%s')\n", osmo_hexdump_nospc(xfer->buffer, xfer->actual_length));
		break;
	}
}

static void init_ep_buf(struct ep_buf *epb)
{
	if (!epb->xfer)
		epb->xfer = libusb_alloc_transfer(0);

	epb->xfer->flags = 0;

	libusb_fill_bulk_transfer(epb->xfer, g_devh, epb->ep, epb->buf, sizeof(epb->buf), usb_in_xfer_cb, NULL, 0);
}

/***********************************************************************
 * libosmocore main loop integration of libusb async I/O
 ***********************************************************************/

static int g_libusb_pending = 0;

static int ofd_libusb_cb(struct osmo_fd *ofd, unsigned int what)
{
	/* FIXME */
	g_libusb_pending = 1;

	return 0;
}

/* call-back when libusb adds a FD */
static void libusb_fd_added_cb(int fd, short events, void *user_data)
{
	struct osmo_fd *ofd = talloc_zero(NULL, struct osmo_fd);

	printf("%s(%u, %x)\n", __func__, fd, events);

	ofd->fd = fd;
	ofd->cb = &ofd_libusb_cb;
	if (events & POLLIN)
		ofd->when |= BSC_FD_READ;
	if (events & POLLOUT)
		ofd->when |= BSC_FD_WRITE;

	osmo_fd_register(ofd);
}

/* call-back when libusb removes a FD */
static void libusb_fd_removed_cb(int fd, void *user_data)
{

	printf("%s(%u)\n", __func__, fd);
#if 0
	struct osmo_fd *ofd;
	/* FIXME: This needs new export in libosmocore! */
	ofd = osmo_fd_get_by_fd(fd);

	if (ofd) {
		osmo_fd_unregister(ofd);
		talloc_free(ofd);
	}
#endif
}

/* call-back when the UDP socket is readable */
static int ofd_udp_cb(struct osmo_fd *ofd, unsigned int what)
{
	int rc;
	socklen_t addrlen = sizeof(g_sa_remote);

	rc = recvfrom(ofd->fd, g_buf_out.buf, sizeof(g_buf_out.buf), 0,
			(struct sockaddr *)&g_sa_remote, &addrlen);
	if (rc <= 0) {
		fprintf(stderr, "error reading from UDP\n");
		return 0;
	}
	printf("read %d bytes from UDP, forwarding to SIMTRACE\n", rc);
	g_buf_out.xfer->length = rc;

	/* disable further READ interest for the UDP socket */
	ofd->when &= ~BSC_FD_READ;

	/* submit the URB on the OUT end point */
	libusb_submit_transfer(g_buf_out.xfer);

	return 0;
}

static void run_mainloop(void)
{
	int rc;

	printf("Entering main loop\n");

	while (1) {
		osmo_select_main(0);
		if (g_libusb_pending) {
			struct timeval tv;
			memset(&tv, 0, sizeof(tv));
			rc = libusb_handle_events_timeout_completed(NULL, &tv, NULL);
			if (rc != 0) {
				fprintf(stderr, "handle_events_timeout_completed == %d\n", rc);
				break;
			}
		}
	}
}

int main(int argc, char **argv)
{
	int rc;
	int c, ret = 1;
	int local_udp_port = 52342;
	unsigned int if_num = 0;

	print_welcome();

	while (1) {
		int option_index = 0;
		static const struct option opts[] = {
			{ "udp-port", 1, 0, 'u' },
			{ "interface", 1, 0, 'I' },
			{ "help", 0, 0, 'h' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "u:I:h", opts, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'u':
			local_udp_port = atoi(optarg);
			break;
		case 'I':
			if_num = atoi(optarg);
			break;
		case 'h':
			print_help();
			exit(0);
			break;
		}
	}

	rc = libusb_init(NULL);
	if (rc < 0) {
		fprintf(stderr, "libusb initialization failed\n");
		goto close_exit;
	}

	libusb_set_pollfd_notifiers(NULL, &libusb_fd_added_cb, &libusb_fd_removed_cb, NULL);

	g_devh = libusb_open_device_with_vid_pid(NULL, USB_VENDOR_OPENMOKO, USB_PRODUCT_OWHW_SAM3);
	if (!g_devh) {
		fprintf(stderr, "can't open USB device\n");
		goto close_exit;
	}

	rc = libusb_claim_interface(g_devh, if_num);
	if (rc < 0) {
		fprintf(stderr, "can't claim interface %u; rc=%d\n", if_num, rc);
		goto close_exit;
	}

	/* open UDP socket, register with select handling and mark it
	 * readable */
	g_udp_ofd.cb = ofd_udp_cb;
	osmo_sock_init_ofd(&g_udp_ofd, AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, local_udp_port + if_num, OSMO_SOCK_F_BIND);

	rc = osmo_libusb_get_ep_addrs(g_devh, if_num, &g_buf_out.ep, &g_buf_in.ep, NULL);
	if (rc < 0) {
		fprintf(stderr, "couldn't find enpdoint addresses; rc=%d\n", rc);
		goto close_exit;
	}
	/* initialize USB buffers / transfers */
	init_ep_buf(&g_buf_out);
	init_ep_buf(&g_buf_in);

	/* submit the first transfer for the IN endpoint */
	libusb_submit_transfer(g_buf_in.xfer);

	run_mainloop();

	ret = 0;

	libusb_release_interface(g_devh, 0);
close_exit:
	if (g_devh)
		libusb_close(g_devh);

	libusb_exit(NULL);
	return ret;
}
