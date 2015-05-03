#!/usr/bin/env python

import constants
import array

INS = 1
LEN = 4

class SmartCardEmulator:
    def getATR(self):
        return array.array('B', constants.ATR_SYSMOCOM1)

    def send_receive_cmd(self, cmd):
        if cmd[INS] == 0xA4:
            resp = [0x9F, 0x16]
        elif cmd == [0xff, 0x00, 0xff]:
            resp = cmd
        elif len(cmd) == 5 and  cmd[INS] == 0xC0:
            data = self.ans_from_len[cmd[LEN]]
            SW = [0x90, 0x00]
            resp = data + SW       # Respond with INS byte
            #state = WAIT_RST
        else:
            print("Unknown cmd")
            resp = [0x60, 0x00]

        print("Cmd, resp: ")
        print("".join("%02x " % b for b in cmd))
        print("".join("%02x " % b for b in resp))

        return array.array('B', resp)

    def reset_card():
        pass

    def close(self):
        pass

    ans_from_len = {0x16: [0x00, 0x00, 0x00, 0x00, 0x7F, 0x20, 0x02, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x09, 0x91, 0x00, 0x17,
                    0x04, 0x00, 0x83, 0x8A, 0x83, 0x8A],
                    }
