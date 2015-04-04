import usb.core
import usb.util

import ccid_raw
import phone

def find_dev():
    dev = usb.core.find(idVendor=0x03eb, idProduct=0x6004)
    if dev is None:
        raise ValueError("Device not found")
    else:
        print("Found device")
    return dev


SIM_WR = 0x1
SIM_RD = 0x82
SIM_INT = 0x83

PHONE_WR = 0x4
PHONE_RD = 0x85
PHONE_INT = 0x86

def check_msg_phone():
    cmd = dev.read(PHONE_RD, 64, 100)
    if cmd is not None:
        print("Phone sent: " + cmd)
        return cmd
    cmd = dev.read(PHONE_INT, 64, 100)
    if cmd is not None:
        print("Phone sent int")
        return cmd

def write_phone(resp):
    dev.write(PHONE_WR, resp, 100)

def write_sim(data):
    return do_intercept(data, dwActiveProtocol)

def do_mitm():
    dev = find_dev()
    hcard, hcontext, dwActiveProtocol = ccid_raw.ccid_raw_init()

    try:
        try:
            while True:
                cmd = check_msg_phone()
                if (cmd is not None):
                    resp = write_sim(cmd, dwActiveProtocol)
                if (resp is not None):
                    write_phone(resp)
                else:
                    print("No responses.")
        finally:
            ccid_raw.ccid_raw_exit(hcard, hcontext)

    except usb.USBError as e:
        print(e)
        pass
