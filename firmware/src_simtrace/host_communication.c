#include "board.h"

static volatile bool write_to_host_in_progress = false;

void USB_write_callback(uint8_t *pArg, uint8_t status, uint32_t transferred, uint32_t remaining)
{
    if (status != USBD_STATUS_SUCCESS) {
        TRACE_ERROR("USB err status: %d (%s)\n", __FUNCTION__, status);
    }
    write_to_host_in_progress = false;
    printf("WR_CB\n");
}

int send_to_host()
{
    static uint8_t msg[RING_BUFLEN];
    int ret = 0;
    unsigned int i;

    for(i = 0; !rbuf_is_empty(&sim_rcv_buf) && i < sizeof(msg); i++) {
        msg[i] = rbuf_read(&sim_rcv_buf);
    }
    printf("Wr %d\n", i);
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
    ret = send_to_host();
    return ret;
}
