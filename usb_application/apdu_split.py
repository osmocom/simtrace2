#!/usr/bin/env python

# Code ported from simtrace host program apdu_split.c
#
# (C) 2010 by Harald Welte <hwelte@hmw-consulting.de>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

from enum import Enum
from util import HEX
from array import array

class apdu_states(Enum):
    APDU_S_CLA = 1
    APDU_S_INS = 2
    APDU_S_P1 = 3
    APDU_S_P2 = 4
    APDU_S_P3 = 5
    APDU_S_SEND_DATA = 6
    APDU_S_DATA = 7
    APDU_S_DATA_SINGLE = 8
    APDU_S_SW1 = 9
    APDU_S_SW2 = 10
    APDU_S_FIN = 11
    PTS = 12

class Apdu_splitter:

    def __init__(self):
        self.state = apdu_states.APDU_S_CLA
        self.buf = array('B', [])
        self.pts_buf = array('B', [])
        self.data = array('B', [])
        self.ins = array('B', [])
        self.data_remainig = 0

    def func_APDU_S_INS(self, c):
        self.ins = c
        self.buf.append(c)
        self.state = apdu_states(self.state.value + 1)

    def func_PTS(self, c):
        self.pts_buf.append(c)
        print("PTS: ", self.pts_buf)
        if self.pts_buf == [0xff, 0x00, 0xff]:
            self.state = apdu_states.APDU_S_FIN

    def func_APDU_S_CLA_P1_P2(self, c):
        if self.state == apdu_states.APDU_S_CLA and c == 0xff:
            self.state = apdu_states.PTS
            self.pts_buf = [c]
        else:
            self.buf.append(c)
            self.state = apdu_states(self.state.value + 1)

    def func_APDU_S_P3(self, c):
        self.buf.append(c)
        self.data_remaining = 256 if c == 0  else c
        self.state = apdu_states.APDU_S_SW1

    def func_APDU_S_DATA(self, c):
        self.buf.append(c)
        self.data.append(c)
        self.data_remaining -= 1
        if self.data_remaining == 0:
            self.state = apdu_states.APDU_S_SW1

    def func_APDU_S_DATA_SINGLE(self, c):
        self.buf.append(c)
        self.data_remaining -= 1
        self.state = apdu_states.APDU_S_SW1

    def func_APDU_S_SW1(self, c):
        if (c == 0x60):
            print("APDU_S_SW1: NULL")
        else:
            # check for 'all remaining' type ACK
            if c == self.ins or c == self.ins + 1 or c == ~(self.ins+1):
                print("ACK")
                self.data = []
                if self.ins in self.INS_data_expected:
                    self.state = apdu_states.APDU_S_SEND_DATA
                else:
                    self.state = apdu_states.APDU_S_DATA
            else:
                # check for 'only next byte' type ACK */
                if c == ~(self.ins):
                    self.state = apdu_states.APDU_S_DATA_SINGLE
                else:
                    # must be SW1
                    self.sw1 = c
                    self.buf.append(c)
                    self.state = apdu_states.APDU_S_SW2

    def func_APDU_S_SW2(self, c):
        self.buf.append(c)
        self.sw2 = c
        print("APDU:", HEX(self.ins), HEX(self.buf))
        self.state = apdu_states.APDU_S_FIN

    Apdu_S = {
            apdu_states.APDU_S_CLA :            func_APDU_S_CLA_P1_P2,
            apdu_states.APDU_S_INS :            func_APDU_S_INS,
            apdu_states.APDU_S_P1 :             func_APDU_S_CLA_P1_P2,
            apdu_states.APDU_S_P2 :             func_APDU_S_CLA_P1_P2,
            apdu_states.APDU_S_P3 :             func_APDU_S_P3,
            apdu_states.APDU_S_SEND_DATA :      func_APDU_S_DATA,
            apdu_states.APDU_S_DATA :           func_APDU_S_DATA,
            apdu_states.APDU_S_DATA_SINGLE :    func_APDU_S_DATA_SINGLE,
            apdu_states.APDU_S_SW1 :            func_APDU_S_SW1,
            apdu_states.APDU_S_SW2 :            func_APDU_S_SW2,
            apdu_states.PTS :                   func_PTS }

    INS_data_expected = [0xC0, 0xB0, 0xB2, 0x12, 0xF2]

    def split(self, c):
       # if c == 0xA0:
       #     self.state = apdu_states.APDU_S_CLA
        print("state: ", self.state, hex(c))
        self.Apdu_S[self.state](self, c)


if __name__ == '__main__':
    msg1 = [0xA0, 0xA4, 0x00, 0x00, 0x02, 0xA4, 0x7F, 0x20, 0x9F, 0x16]
    msg2 = [0xA0, 0xC0, 0x00, 0x00, 0x16, 0xC0,
            0x00, 0x00, 0x00, 0x00, 0x7F, 0x20,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x09, 0x91, 0x00, 0x17, 0x04, 0x00, 0x00, 0x00,
            0x83, 0x8A, 0x90, 0x00]
    msg3 = [0xa0, 0xc0, 0x00, 0x00, 0x16, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x7f,
            0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x91, 0x00, 0x17,
            0x04, 0x00, 0x83, 0x8a, 0x83, 0x8a, 0x90]

    pts = [0xff, 0x00, 0xff]
    apdus = []
    apdu = Apdu_splitter()
    for c in pts + msg2 + msg1 + msg3:
        apdu.split(c)
        if apdu.state == apdu_states.APDU_S_FIN:
            apdus.append(apdu)
            apdu = Apdu_splitter()
    for a in apdus:
        print(HEX(a.buf))
