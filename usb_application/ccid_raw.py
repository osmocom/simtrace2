#!/usr/bin/env python

from smartcard.scard import *
import smartcard.util

import array

class SmartcardException(Exception):
    pass

class SmartcardConnection:
#    hcard, dwActiveProtocol, hcontext, reader

    def __init__(self):
        self.establish_context()
        self.connect_card()

    def getATR(self):
        hresult, reader, state, protocol, atr = SCardStatus(self.hcard)
        if hresult != SCARD_S_SUCCESS:
            print 'failed to get status: ' + SCardGetErrorMessage(hresult)
        print 'Reader:', reader
        print 'State:', state
        print 'Protocol:', protocol
        print 'ATR:', smartcard.util.toHexString(atr, smartcard.util.HEX)
        return array.array('B', atr)

    def reset_card(self):
        hresult, self.dwActiveProtocol = SCardReconnect(self.hcard, SCARD_SHARE_SHARED,
            SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, SCARD_RESET_CARD)
        if hresult != SCARD_S_SUCCESS:
            raise SmartcardException('Unable to retrieve Atr: ' +
                SCardGetErrorMessage(hresult))

    def connect_card(self):
        hresult, self.hcard, self.dwActiveProtocol = SCardConnect(self.hcontext, self.reader,
            SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)
        if hresult != SCARD_S_SUCCESS:
            raise SmartcardException('Unable to connect: ' +
                SCardGetErrorMessage(hresult))
        print 'Connected with active protocol', self.dwActiveProtocol

    def establish_context(self):
        hresult, self.hcontext = SCardEstablishContext(SCARD_SCOPE_USER)
        if hresult != SCARD_S_SUCCESS:
            raise SmartcardException('Failed to establish context : ' +
                SCardGetErrorMessage(hresult))
        print 'Context established!'

        hresult, readers = SCardListReaders(self.hcontext, [])
        if hresult != SCARD_S_SUCCESS:
            raise SmartcardException('Failed to list readers: ' +
                SCardGetErrorMessage(hresult))
        print 'PCSC Readers:', readers

        if len(readers) < 1:
            raise SmartcardException('No smart card readers')

        self.reader = readers[0]
        print "Using reader:", self.reader

    def release_context(self):
        hresult = SCardReleaseContext(self.hcontext)
        if hresult != SCARD_S_SUCCESS:
            raise SmartcardException('Failed to release context: ' +
                    SCardGetErrorMessage(hresult))
        print 'Released context.'

    def send_receive_cmd(self, cmd):
        print("Cmd: ")
        hresult, resp = SCardTransmit(self.hcard, self.dwActiveProtocol,
            cmd.tolist())
        if hresult != SCARD_S_SUCCESS:
            raise SmartcardException('Failed to transmit: ' +
                SCardGetErrorMessage(hresult))
        print 'Ans: ' + smartcard.util.toHexString(resp,
            smartcard.util.HEX)
        return array.array('B', resp)

    def disconnect_card(self):
        hresult = SCardDisconnect(self.hcard, SCARD_UNPOWER_CARD)
        if hresult != SCARD_S_SUCCESS:
            raise SmartcardException('Failed to disconnect: ' +
                SCardGetErrorMessage(hresult))
        print 'Disconnected'


    def close(self):
        self.disconnect_card()
        self.release_context()


if __name__ == '__main__':
    import constants

    sm_con = SmartcardConnection()
    sm_con.getATR()
    print(sm_con.send_receive_cmd(constants.CMD_SEL_ROOT))
    print(sm_con.send_receive_cmd(constants.CMD_SEL_FILE))
    print(sm_con.send_receive_cmd(constants.CMD_GET_DATA))
    sm_con.close()
