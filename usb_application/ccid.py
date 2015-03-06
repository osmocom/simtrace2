#!/usr/bin/env python 

import usb.core
import usb.util
import sys

from pySim.commands import SimCardCommands
from pySim.utils import h2b, swap_nibbles, rpad, dec_imsi, dec_iccid
from pySim.transport.pcsc import PcscSimLink


import hashlib
import os
import random
import re


class find_class(object):
    def __init__(self, class_):
        self._class = class_
    def __call__(self, device):
        # first, let's check the device
        if device.bDeviceClass == self._class:
            return True
        # ok, transverse all devices to find an
        # interface that matches our class
        for cfg in device:
            # find_descriptor: what's it?
            intf = usb.util.find_descriptor(
                                        cfg,
                                        bInterfaceClass=self._class
                                )
            if intf is not None:
                return True

        return False


def pySim_read():
    sl = PcscSimLink(0)

    # Create command layer
    scc = SimCardCommands(transport=sl)

    # Wait for SIM card
    sl.wait_for_card()

    # Program the card
    print("Reading ...")

    # EF.ICCID
    (res, sw) = scc.read_binary(['3f00', '2fe2'])
    if sw == '9000':
            print("ICCID: %s" % (dec_iccid(res),))
    else:
            print("ICCID: Can't read, response code = %s" % (sw,))

    # EF.IMSI
    (res, sw) = scc.read_binary(['3f00', '7f20', '6f07'])
    if sw == '9000':
            print("IMSI: %s" % (dec_imsi(res),))
    else:
            print("IMSI: Can't read, response code = %s" % (sw,))

    # EF.SMSP
    (res, sw) = scc.read_record(['3f00', '7f10', '6f42'], 1)
    if sw == '9000':
            print("SMSP: %s" % (res,))
    else:
            print("SMSP: Can't read, response code = %s" % (sw,))

    # EF.HPLMN
#   (res, sw) = scc.read_binary(['3f00', '7f20', '6f30'])
#   if sw == '9000':
#           print("HPLMN: %s" % (res))
#           print("HPLMN: %s" % (dec_hplmn(res),))
#   else:
#           print("HPLMN: Can't read, response code = %s" % (sw,))
    # FIXME

    # EF.ACC
    (res, sw) = scc.read_binary(['3f00', '7f20', '6f78'])
    if sw == '9000':
            print("ACC: %s" % (res,))
    else:
            print("ACC: Can't read, response code = %s" % (sw,))

    # EF.MSISDN
    try:
    #       print(scc.record_size(['3f00', '7f10', '6f40']))
            (res, sw) = scc.read_record(['3f00', '7f10', '6f40'], 1)
            if sw == '9000':
                    if res[1] != 'f':
                            print("MSISDN: %s" % (res,))
                    else:
                            print("MSISDN: Not available")
            else:
                    print("MSISDN: Can't read, response code = %s" % (sw,))
    except:
            prin#t("MSISDN: Can't read. Probably not existing file")

    # Done for this card and maybe for everything ?
    print("Done !\n")

cmd1 = {0x00, 0x10, 0x00, 0x00}
cmd2 =  {0x00, 0x20, 0x00, 0x00, 0x02}
cmd_poweron = {0x62, 0x62, 0x00, 0x00}
cmd_poweroff = {0x63, 0x63, 0x00, 0x00}
cmd_get_slot_stat = {0x65, 0x65, 0x00, 0x00}
cmd_get_param = {0x00, 0x6C, 0x00, 0x00}

# main code
def main():
    devs = usb.core.find(find_all=1, custom_match=find_class(0xb))  # 0xb = Smartcard
    for dev in devs:
        dev.set_configuration(2)
       
        pySim_read() 

#        dev.write(0x1, cmd_poweroff)
#        dev.write(0x1, cmd_poweron)
#        dev.write(0x1, cmd2)
#        dev.write(0x1, cmd_get_slot_stat)
#        ret = dev.read(0x82, 64)
        print(ret)
        #dev.write(0x1, {0x62, 0x62})    # PC_TO_RDR_ICCPOWERON
    return

#    (epi, epo) = find_eps(dev)
    while True:
        #ep_out.write("Hello")
        try:
            ans = dev.read(0x82, 64, 1000)
            print("".join("%02x " % b for b in ans))
        except KeyboardInterrupt:
            print("Bye")
            sys.exit()
        except: 
            print("Timeout")
    #    print(ep_in.read(1, 5000));

main()
