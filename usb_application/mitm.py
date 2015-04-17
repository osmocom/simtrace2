import usb.core
import usb.util

from ccid_raw import SmartcardConnection
from smartcard_emulator import SmartCardEmulator

from contextlib import closing

from util import HEX
from constants import *
from apdu_split import Apdu_splitter, apdu_states

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

def do_mitm(sim_emul=True):
    dev = find_dev()
    if sim_emul == True:
        my_class = SmartCardEmulator
    else:
        my_class = SmartcardConnection
    with closing(my_class()) as sm_con:
        atr = sm_con.getATR()

        apdus = []
        apdu = Apdu_splitter()

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
                for c in cmd:
                    apdu.split(c)
                    if apdu.state == apdu_states.APDU_S_SW1:
                        if len(apdu.data) == 0:
                            # FIXME: implement other ACK types
                            write_phone(dev, apdu.ins)
                            apdu.split(apdu.ins)
                        else:
                            sim_data = sm_con.send_receive_cmd(apdu.buf)
                            write_phone(dev, sim_data)
                            for c in sim_data:
                                apdu.split(c)
                    elif apdu.state == apdu_states.APDU_S_FIN:
                        apdus.append(apdu)
                        apdu = Apdu_splitter()
