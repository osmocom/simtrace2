#!/usr/bin/env python3 

import usb.core
import usb.util
import sys

# SuperSIM ATR
atr= [0x3B, 0x9A, 0x94, 0x00, 0x92, 0x02, 0x75, 0x93, 0x11, 0x00, 0x01, 0x02, 0x02, 0x19]

def find_dev():
    dev = usb.core.find(idVendor=0x03eb, idProduct=0x6004)
    if dev is None:
        raise ValueError("Device not found")
    else:
        print("Found device")
    return dev

def find_eps(dev):
    dev.set_configuration(3)

    cfg = dev.get_active_configuration()
    print("Active config: ")
    print(cfg)
    intf = cfg[(0,0)]

    ep_in = usb.util.find_descriptor(
        intf, 
        custom_match = \
        lambda e: \
            usb.util.endpoint_direction(e.bEndpointAddress) == \
            usb.util.ENDPOINT_IN)

    assert ep_in is not None

    ep_out = usb.util.find_descriptor(
        intf, 
        custom_match = \
        lambda e: \
            usb.util.endpoint_direction(e.bEndpointAddress) == \
            usb.util.ENDPOINT_OUT)

    assert ep_out is not None
    print("****")
    print(ep_in)
    print(ep_out)
    return (ep_in, ep_out)

def emulate_sim():
    dev = find_dev()
#    (epi, epo) = find_eps(dev)

    while True:
        #ep_out.write("Hello")
        try:
            # ATR handling
            arr = dev.read(0x83, 64, 1000)    # Notification endpoint
            print("arr: ", arr)
            c=arr.pop()
            print(c)
            if c == ord('R'):
                written = dev.write(0x1, atr, 1000)     # Probably we received a Reset, so we send ATR
                print("Written data: " + written)
            

            ans = dev.read(0x82, 64, 1000)
            print("".join("%02x " % b for b in ans))
        except KeyboardInterrupt:
            print("Bye")
            sys.exit()
        except: 
            print("Timeout")
    #    print(ep_in.read(1, 5000));
