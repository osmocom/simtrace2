#!/usr/bin/env python

import array
from constants import *


# Address book entries
name = 'deine mudda'
phone = '0123456789abcdef'

def replace(data):
    print(replace.last_req)
    if data is None:
        raise MITMReplaceError
    else:
        try:
            if data[0] == 0xA0:
                print("INS: ", hex(data[1]))
                replace.last_req = data
                return data

            if data[0] == 0x3B:
                return data
                #print("*** Replace ATR")
                #return array('B', NEW_ATR)
            elif data[0] == 0x9F:
                return data
#                print("*** Replace return val")
#                return array('B', [0x60, 0x00])
            elif replace.last_req[1:5] == array('B', [0xB2, 0x01, 0x04, 0x1A]):   # phone book request
                print("*** Replace phone book")
#                return array('B',  [0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0xff, 0xff, 0xff, 0xff, 0x09, 0x81, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0xff, 0x90, 0x00])
                resp = map(ord, name) + ([0xff]*(12-len(name))) + [len(name) + 1] + [0x81]
                for x in range(1,len(phone)/2+1):
                    list.append(resp, int(phone[x*2-2:2*x:], 16))
                resp += ([0xff]*(replace.last_req[4]-len(resp))) + [0x90, 0x00]
                return array('B', resp)
        except ValueError:
            print("*** Value error! ")
    return data

replace.last_req = array('B')

if __name__ == '__main__':
    print("Replacing PHONE_BOOK_REQ", PHONE_BOOK_REQ, "with", replace(PHONE_BOOK_REQ))
    print("Replacing PHONE_BOOK_RESP", PHONE_BOOK_RESP, "with", replace(PHONE_BOOK_RESP))
