#include <osmocom/simtrace2/gsmtap.h>

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

/* global GSMTAP instance */
static struct gsmtap_inst *g_gti;

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
