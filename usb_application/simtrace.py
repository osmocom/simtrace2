#!/usr/bin/env python 

import argparse
import sniffer
import ccid
import ccid_select
import mitm

import usb.core
import usb.util
import sys
import time

def find_dev():
    dev = usb.core.find(idVendor=0x16c0, idProduct=0x0762)
    if dev is None:
        raise ValueError("Device not found")
    else:
        print("Found device")
    return dev

# main code
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-C", "--conf", type=int, choices=[1, 2, 3, 4], help="Set USB config")
    parser.add_argument("-b", "--read_bin", help="read ICCID, IMSI, etc.", action='store_true')
    parser.add_argument("-c", "--cmd", help="cmds to send to sim card (Not supported yet)", 
        choices=["cmd1", "cmd2", "cmd_poweron", "cmd_poweroff", "cmd_get_slot_stat", "cmd_get_param"])
    parser.add_argument("-s", "--sniff", help="Sniff communication!", action='store_true') 
    parser.add_argument("-S", "--select_file", help="Transmit SELECT cmd!", action='store_true')
    parser.add_argument("-p", "--phone", help="Emulates simcard", action='store_true')
    parser.add_argument("-m", "--mitm", help="Intercept communication (MITM)", action='store_true')
    
    args = parser.parse_args()
    print("args: ", args)

    if args.conf is not None:
        dev = find_dev()
        dev.set_configuration(args.conf)
        # Give pcsclite time to find the device
        time.sleep(1)
    if args.read_bin is True: 
        ccid.pySim_read() 
    if args.sniff is True:
        sniffer.sniff(dev)
    if args.select_file is True:
        ccid_select.select()
    if args.phone is True:
        mitm.do_mitm(dev, sim_emul=True)
    if args.mitm is True:
        mitm.do_mitm(dev, sim_emul=False)
    return

main()
