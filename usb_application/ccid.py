#!/usr/bin/env python3 

import usb.core
import usb.util
import sys

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

# main code
def main():
    devs = usb.core.find(find_all=1, custom_match=find_class(0xb))  # 0xb = Smartcard
    for dev in devs:
        dev.set_configuration(2)
    dev.reset()
    dev.write(0x1, {0x63, 0x63})    # PC_TO_RDR_ICCPOWEROFF
    ret = dev.read(0x82, 64)
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
