/* gsmtap - How to encapsulate SIM protocol traces in GSMTAP
 *
 * (C) 2016-2019 by Harald Welte <hwelte@hmw-consulting.de>
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

#include <osmocom/simtrace2/gsmtap.h>

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

/*! global GSMTAP instance */
static struct gsmtap_inst *g_gti;

/*! initialize the global GSMTAP instance for SIM traces */
int osmo_st2_gsmtap_init(const char *gsmtap_host)
{
	if (g_gti)
		return -EEXIST;

	g_gti = gsmtap_source_init(gsmtap_host, GSMTAP_UDP_PORT, 0);
	if (!g_gti) {
		perror("unable to open GSMTAP");
		return -EIO;
	}
	gsmtap_source_add_sink(g_gti);

	return 0;
}

/*! log one APDU via the global GSMTAP instance.
 *  \param[in] sub_type GSMTAP sub-type (GSMTAP_SIM_* constant)
 *  \param[in] apdu User-provided buffer with APDU to log
 *  \param[in] len Length of apdu in bytes
 */
int osmo_st2_gsmtap_send_apdu(uint8_t sub_type, const uint8_t *apdu, unsigned int len)
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
	gh->sub_type = sub_type;

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
