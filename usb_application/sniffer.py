#!/usr/bin/env python3

import usb.core
import usb.util
import sys
import array

from apdu_split import Apdu_splitter, apdu_states
from gsmtap import gsmtap_send_apdu

from constants import PHONE_RD, ERR_TIMEOUT, ERR_NO_SUCH_DEV

# main code
def sniff(dev):
    ans = array.array('B', [])

    apdus = []
    apdu = Apdu_splitter()

    while True:
        try:
            ans += dev.read(PHONE_RD, 64, 1000)
        except KeyboardInterrupt:
            print("Bye")
            sys.exit()
        except Exception as e:
            if e.errno != ERR_TIMEOUT and e.errno != ERR_NO_SUCH_DEV:
                raise
            print e

        if len(ans) >= 1:
#            print("".join("%02x " % b for b in ans))
            for c in ans:
                apdu.split(c)
                if apdu.state == apdu_states.APDU_S_FIN:
                    apdus.append(apdu)
                    gsmtap_send_apdu(apdu.buf)
                    apdu = Apdu_splitter()
            ans = array.array('B', [])
