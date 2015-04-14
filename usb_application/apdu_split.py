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

class Apdu_splitter:

    APDU_S_CLA = 1
    APDU_S_INS = 2
    APDU_S_P1 = 3
    APDU_S_P2 = 4
    APDU_S_P3 = 5
    APDU_S_DATA = 6
    APDU_S_DATA_SINGLE = 7
    APDU_S_SW1 = 8
    APDU_S_SW2 = 9
    APDU_S_FIN = 10

    def __init__(self):
        self.state = APDU_S_CLA
        self.buf = []

    def split(self, c):
        Apdu_S[state](c)

    def func_APDU_S_INS(self, c):
        self.ins = c

    def func_APDU_S_CLA_P1_P2(self, c):
        self.buf.append(c)
        self.state += 1

    def func_APDU_S_P3(self, c):
        self.buf.append(c)
        self.data_remaining = 256 if c == 0 else c
        self.state = func_APDU_S_SW1

    def func_APDU_S_DATA(self, c):
        self.buf.append(c)
        self.data_remaining -= 1
        if data_remaining == 0:
            self.state = APDU_S_SW1;

    def func_APDU_S_DATA_SINGLE(self, c):
        self.buf.append(c)
        self.data_remaining -= 1
        self.state = APDU_S_SW1

    def func_APDU_S_SW1(self, c):
        if (c == 0x60):
            print("APDU_S_SW1: NULL")
        else:
            # check for 'all remaining' type ACK
            if c == self.ins or c == self.ins + 1 or c == ~(self.ins+1):
                print("ACK")
                self.state = APDU_S_DATA
            else:
                # check for 'only next byte' type ACK */
                if c == ~(self.ins):
                    self.state = APDU_S_DATA_SINGLE
                else:
                    # must be SW1
                    self.buf.append(c)
                    self.state = func_APDU_S_SW2

    def func_APDU_S_SW2(self, c):
        self.buf.append(c)
        print("APDU:", self.buf)
        self.state = APDU_S_FIN

    Apdu_S = {
            APDU_S_CLA :            func_APDU_S_CLA_P1_P2,
            APDU_S_INS :            func_APDU_S_INS,
            APDU_S_P1 :             func_APDU_S_CLA_P1_P2,
            APDU_S_P2 :             func_APDU_S_CLA_P1_P2,
            APDU_S_P3 :             func_APDU_S_P3,
            APDU_S_DATA :           func_APDU_S_DATA,
            APDU_S_DATA_SINGLE :    func_APDU_S_DATA_SINGLE,
            APDU_S_SW1 :            func_APDU_S_SW1,
            APDU_S_SW2 :            func_APDU_S_SW2 }


if __name__ == '__main__':
    msg = [0xA0, 0xA4, 0x00, 0x00, 0x02]
    apdus = Apdu_splitter()

    for c in msg:
        print(c)
        apdus.split(c)


