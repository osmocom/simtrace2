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

WAIT_RST = 0
WAIT_CMD = 1

def handle_wait_rst(dev):
    # ATR handling
    arr = dev.read(0x83, 64, 1000)    # Notification endpoint
    print("arr: ", arr)
    c=arr.pop()
    print(c)

    if c == ord('R'):
        # We received a Reset, so we send ATR
        written = dev.write(0x1, atr, 1000)
        print("Written data: ")
        print(written)
        state = WAIT_CMD;
    return state

def handle_wait_cmd(dev):
    # Read phone request
    print("Wait cmd")
    cmd = dev.read(0x82, 64, 1000)
    print("Received request!: ")
    print("".join("%02x " % b for b in cmd))

    send_response(dev, cmd);
    return WAIT_CMD

handle_msg_funcs = { WAIT_RST: handle_wait_rst,
                        WAIT_CMD: handle_wait_cmd }

def handle_phone_request(dev, state):
    state = handle_msg_funcs[state](dev)
    return state

INS = 1

def send_response(dev, cmd):
    print("Write response to ")
    print("".join("%02x " % b for b in cmd))
# FIXME: We could get data of length 5 as well! Implement another distinct criteria!
    if len(cmd) == 5:         # Received cmd from phone
        if cmd[INS] == 0xA4:
            resp = [cmd[INS]]       # Respond with INS byte
        elif cmd[INS] == 0xC0:
            data = [0x00, 0x00, 0x00, 0x00,
                    0x7F, 0x20, 0x02, 0x00,
                    0x00, 0x00, 0x00, 0x00,
                    0x09, 0x91, 0x00, 0x17,
                    0x04, 0x00, 0x83, 0x8A,
                    0x83, 0x8A]
            SW = [0x90, 0x00]
            resp = [cmd[INS], data, SW]       # Respond with INS byte
        else:
            print("Unknown cmd")
            resp = [0x60, 0x00]
    elif len(cmd) == 2:
        resp = [0x9F, 0x16]
    else:
        resp = [0x60, 0x00]

    print("Cmd, resp: ")
    print("".join("%02x " % b for b in cmd))
    print("".join("%02x " % b for b in resp))

    written = dev.write(0x01, resp, 10000);
    if written > 0:
        print("Bytes written:")
        print(written)

def emulate_sim():
    dev = find_dev()
    state = WAIT_RST;

    while True:
        try:
            state = handle_phone_request(dev, state)

        except usb.USBError as e:
            print e
