#!/usr/bin/env python3 

import usb.core
import usb.util
import sys

import time         # needed for sleep()
import traceback    # Exception timeout

# Sniffed Phone to SIM card communication:
# phone < sim : ATR
# phone > sim : A0 A4 00 00 02 (Select File)
# phone < sim : A4 (INS repeated)
# phone > sim : 7F 02 (= ??)
# phone < sim : 9F 16 (9F: success, can deliver 0x16 (=22) byte)
# phone > sim : ?? (A0 C0 00 00 16)
# phone < sim : C0 (INS repeated)
# phone < sim : 00 00 00 00   7F 20 02 00   00 00 00 00   09 91 00 17   04 00 83 8A (data of length 22)
# phone < sim : 90 00 (OK, everything went fine)
# phone ? sim : 00 (??)

# SuperSIM ATR
atr= [0x3B, 0x9A, 0x94, 0x00, 0x92, 0x02, 0x75, 0x93, 0x11, 0x00, 0x01, 0x02, 0x02, 0x19]
RESP_OK = [0x60, 0x00]

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

def handle_phone_request():
    # ATR handling
    try:
        arr = dev.read(0x83, 64, 100)    # Notification endpoint
        print("arr: ", arr)
        c=arr.pop()
        print(c)

        if c == ord('R'):
            try:
                written = dev.write(0x1, atr, 1000)     # Probably we received a Reset, so we send ATR
                print("Written data: " + written)
            except:
                print("Timeout sending ATR!")
                return

    except:
        #print("Timeout receiving atr!")
        pass
    
    # Read phone request
    try:
        cmd = dev.read(0x82, 64, 10000000)
        print("Received request!: ")
        print("".join("%02x " % b for b in ans))

        print("Write response");
        try:
            written = dev.write(0x01, RESP_OK, 10000000);
            print("Bytes written:")
            print(written)
        except:
            print("Timeout in send response")
    
    except:
        #print("Timeout in receive cmd")
        pass


def emulate_sim():
    dev = find_dev()

    while True:
        try:
            handle_phone_request()

        except KeyboardInterrupt:
            print("Bye")
            sys.exit()
        except: 
            print("Timeout")
