#include "board.h"

static volatile bool write_to_host_in_progress = false;
static struct iso7816_3_handle ih = {0};
static bool check_for_pts = false;
static enum pts_state state;

void USB_write_callback(uint8_t *pArg, uint8_t status, uint32_t transferred, uint32_t remaining)
{
    if (status != USBD_STATUS_SUCCESS) {
        TRACE_ERROR("USB err status: %d (%s)\n", __FUNCTION__, status);
    }
    write_to_host_in_progress = false;
    TRACE_DEBUG("WR_CB\n");
}

int send_to_host()
{
    static uint8_t msg[RING_BUFLEN];
    int ret = 0;
    unsigned int i;

    for(i = 0; !rbuf_is_empty(&sim_rcv_buf) && i < sizeof(msg); i++) {
        msg[i] = rbuf_read(&sim_rcv_buf);
    }
    TRACE_DEBUG("Wr %d\n", i);
    write_to_host_in_progress = true;
    ret = USBD_Write( PHONE_DATAIN, msg, i, (TransferCallback)&USB_write_callback, 0 );
    if (ret != USBD_STATUS_SUCCESS) {
        TRACE_ERROR("Error sending to host (%x)\n", ret);
        write_to_host_in_progress = false;
    }
    return ret;
}

int check_data_from_phone()
{
    int ret = 0;

    if((rbuf_is_empty(&sim_rcv_buf) || write_to_host_in_progress == true)) {
        return ret;
    }
    if ((check_for_pts == false) && (rbuf_peek(&sim_rcv_buf) == 0xff)) {
// FIXME: set var to false
        check_for_pts = true;
        ih = (struct iso7816_3_handle){0};
    }
    if (check_for_pts == true) {
        while (!rbuf_is_empty(&sim_rcv_buf) && (ih.pts_state != PTS_END)) {
            state = process_byte_pts(&ih, rbuf_read(&sim_rcv_buf));
        }
        if (ih.pts_bytes_processed > 6 && ih.pts_state != PTS_END) {
            int i;
            for (i = 0; i < ih.pts_bytes_processed; i++)
                printf("s: %x", ih.pts_req[i]);
                check_for_pts = false;
                rbuf_write(&sim_rcv_buf, ih.pts_req[i]);
        } else {
            printf("fin pts\n", ih.pts_state);
            check_for_pts = false;
        }
    }
    ret = send_to_host();
    return ret;
}
