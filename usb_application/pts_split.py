#!/usr/bin/env python

from enum import Enum

# FIXME: response!
class pts_states(Enum):
    PTSS = 0
    PTS0 = 1
    PTS1 = 2
    PTS2 = 3
    PTS3 = 4
    PCK	 = 5

class Pts_splitter:

    def __init__(self):
        self.state = pts_states.PTSS
        self.buf = []

    def func_PTS_PTSS(self, ptss):
        self.state = pts_states.PTS0

    def func_PTS_PTS0(self, pts0):
        self.pts0 = pts0
        self.state = pts_states.PTS1

    def func_PTS_PTS1(self, fidi):
        print("pts0: ", self.pts0)
        if (self.pts0 & 1<<4) != 0:
            self.fidi = fidi
            print("FiDi: ", fidi)
            self.state = pts_states.PTS2
        else:
            self.PTS_S[pts_states.PTS2](self, c)

    def func_PTS_PTS2(self, c):
        if (self.pts0 & 1<<5) != 0:
            print("ETU: ", c)
            self.state = pts_states.PTS3
        else:
            self.PTS_S[pts_states.PTS3](self, c)

    def func_PTS_PTS3(self, c):
        if (self.pts0 & 1<<6) != 0:
            print("RFU: ", c)
            self.state = pts_states.PCK
        else:
            self.PTS_S[pts_states.PCK](self, c)

    def func_PTS_PCK(self, c):
        print("PCK: ", c)
        self.state = pts_states.PCK

    PTS_S = {
        pts_states.PTSS :   func_PTS_PTSS,
        pts_states.PTS0 :   func_PTS_PTS0,
        pts_states.PTS1 :   func_PTS_PTS1,
        pts_states.PTS2 :   func_PTS_PTS2,
        pts_states.PTS3 :   func_PTS_PTS3,
        pts_states.PCK  :   func_PTS_PCK
    }

    def split(self, c):
        self.PTS_S[self.state](self, c)

if __name__ == '__main__':
    pts_msg1 = [0xff, 0x00, 0xff]
    pts_msg_fidi = [0xff, (1<<4), 0x18, 0xff]
    pts_msg_fidi_etu = [0xff, ((1<<4) | (1<<5)), 0x18, 0x01, 0xff]
    pts = Pts_splitter()
    for c in pts_msg1 + pts_msg_fidi + pts_msg_fidi_etu:
        pts.split(c)
        if (pts.state == pts_states.PCK):
            pts = Pts_splitter()
