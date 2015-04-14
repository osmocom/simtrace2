import usb.core
import usb.util

from ccid_raw import SmartcardConnection
import phone

from contextlib import closing

from util import HEX
from constants import *

def find_dev():
    dev = usb.core.find(idVendor=0x03eb, idProduct=0x6004)
    if dev is None:
        raise ValueError("Device not found")
    else:
        print("Found device")
    return dev

def pattern_match(inpt):
    print("Matching inpt", inpt)
    if (inpt == ATR_SYSMOCOM1) or (inpt == ATR_STRANGE_SIM):
        print("ATR: ", inpt)
        return NEW_ATR
    elif (inpt == CMD_SEL_FILE):
        print("CMD_SEL_FILE:", inpt)
        return CMD_SEL_ROOT
    elif (inpt == CMD_GET_DATA):
        print("CMD_DATA:", inpt)
        return CMD_SEL_ROOT
    else:
        return inpt

def poll_ep(dev, ep):
    try:
        return dev.read(ep, 64, 10)
    except usb.core.USBError as e:
        if e.errno != ERR_TIMEOUT:
            raise
        return None

def write_phone(dev, resp):
    print("WR: ", HEX(resp))
    dev.write(PHONE_WR, resp, 10)

def do_mitm():
    dev = find_dev()
    with closing(SmartcardConnection()) as sm_con:
        atr = sm_con.getATR()
        while True:
            cmd = poll_ep(dev, PHONE_INT)
            if cmd is not None:
                print("Int line ", HEX(cmd))
                assert cmd[0] == ord('R')
# FIXME: restart card anyways?
#               sm_con.reset_card()
                print("Write atr: ", HEX(atr))
                write_phone(dev, atr)

            cmd = poll_ep(dev, PHONE_RD)
            if cmd is not None:
                print("RD: ", HEX(cmd))
                sim_data = sm_con.send_receive_cmd(cmd)
                write_phone(dev, sim_data)
