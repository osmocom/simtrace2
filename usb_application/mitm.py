import usb.core
import usb.util

from ccid_raw import SmartcardConnection
import phone

from contextlib import closing


def find_dev():
    dev = usb.core.find(idVendor=0x03eb, idProduct=0x6004)
    if dev is None:
        raise ValueError("Device not found")
    else:
        print("Found device")
    return dev

def pattern_match(inpt):
    print("Matching inpt", inpt)
    if (inpt == ATR_SYSMOCOM1):
        return NEW_ATR
    elif (inpt == CMD_SEL_FILE):
        return CMD_SEL_ROOT
    else:
        return inpt

SIM_WR = 0x1
SIM_RD = 0x82
SIM_INT = 0x83

PHONE_WR = 0x4
PHONE_RD = 0x85
PHONE_INT = 0x86

ERR_TIMEOUT = 110

def poll_ep(dev, ep):
    try:
        return dev.read(ep, 64, 1000)
    except usb.core.USBError as e:
        if e.errno != ERR_TIMEOUT:
            raise
        return None

def write_phone(dev, resp):
    dev.write(PHONE_WR, resp, 1000)

def do_mitm():
    dev = find_dev()
    with closing(SmartcardConnection()) as sm_con:
        atr = sm_con.getATR()
        while True:
            cmd = poll_ep(dev, PHONE_INT)
            if cmd is not None:
                print(cmd)
                assert cmd[0] == ord('R')
# FIXME: restart card anyways?
#               sm_con.reset_card()
                write_phone(dev, atr)

            cmd = poll_ep(dev, PHONE_RD)
            if cmd is not None:
                print(cmd)
                sim_data = sm_con.send_receive_cmd(cmd)
                write_phone(dev, sim_data)
