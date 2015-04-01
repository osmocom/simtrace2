#!/usr/bin/env python

from smartcard.scard import *
import smartcard.util

CMD_SEL_ROOT = [0xA0, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00]
CMD_SEL_FILE = [0xA0, 0xA4, 0x00, 0x00, 0x02, 0x7F, 0x20]
CMD_GET_DATA = [0xA0, 0xC0, 0x00, 0x00, 0x16]
# SuperSIM ATR
atr_supersim= [0x3B, 0x9A, 0x94, 0x00, 0x92, 0x02, 0x75, 0x93, 0x11, 0x00, 0x01, 0x02, 0x02, 0x19]

# Faster sysmocom SIM
ATR_SYSMOCOM1 = [0x3B, 0x99, 0x18, 0x00, 0x11, 0x88, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x60]
ATR_SYSMOCOM2 = [0x3B, 0x99, 0x11, 0x00, 0x11, 0x88, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x60]
NEW_ATR = ATR_SYSMOCOM2

def pattern_match(inpt):
    if (inpt == ATR_SYSMOCOM1):
        return NEW_ATR
    elif (inpt == CMD_SEL_FILE):
        return CMD_SEL_ROOT 
    else:
        return inpt        

def connect_to_card(hcontext, reader):
    hresult, hcard, dwActiveProtocol = SCardConnect(hcontext, reader,
        SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)
    if hresult != SCARD_S_SUCCESS:
        raise Exception('Unable to connect: ' +
            SCardGetErrorMessage(hresult))
    print 'Connected with active protocol', dwActiveProtocol
    return hresult, hcard, dwActiveProtocol
 
def establish_context(): 
    hresult, hcontext = SCardEstablishContext(SCARD_SCOPE_USER)
    if hresult != SCARD_S_SUCCESS:
        raise Exception('Failed to establish context : ' +
            SCardGetErrorMessage(hresult))
    print 'Context established!'

    hresult, readers = SCardListReaders(hcontext, [])
    if hresult != SCARD_S_SUCCESS:
        raise Exception('Failed to list readers: ' +
            SCardGetErrorMessage(hresult))
    print 'PCSC Readers:', readers

    if len(readers) < 1:
        raise Exception('No smart card readers')

    reader = readers[0]
    print "Using reader:", reader
    
    return (hcontext, reader)


def release_context(hcontext):
    hresult = SCardReleaseContext(hcontext)
    if hresult != SCARD_S_SUCCESS:
        raise Exception('Failed to release context: ' +
                SCardGetErrorMessage(hresult))
    print 'Released context.'

def send_receive_cmd(cmd, hcard, dwActiveProtocol):
    print("Response: ")
    new_cmd = pattern_match(cmd)
    print(' '.join([hex(i) for i in cmd]))
    hresult, response = SCardTransmit(hcard, dwActiveProtocol,
        new_cmd)
    if hresult != SCARD_S_SUCCESS:
        raise Exception('Failed to transmit: ' +
            SCardGetErrorMessage(hresult))
    resp = pattern_match(response)
    print 'Ans: ' + smartcard.util.toHexString(resp,
        smartcard.util.HEX)
    return response

def disconnect_card(hcard):
    hresult = SCardDisconnect(hcard, SCARD_UNPOWER_CARD)
    if hresult != SCARD_S_SUCCESS:
        raise Exception('Failed to disconnect: ' +
            SCardGetErrorMessage(hresult))
    print 'Disconnected'


def do_intercept(cmd, dwActiveProtocol):
    send_receive_cmd(cmd, hcard, dwActiveProtocol)

def init():
    (hcontext, reader) = establish_context()
    hresult, hcard, dwActiveProtocol = connect_to_card(hcontext, reader)
    return hcard, hcontext, dwActiveProtocol
    
def exit(hcard, hcontext):
    disconnect_card(hcard)
    release_context(hcontext)

hcard, hcontext, dwActiveProtocol = init()

do_intercept(CMD_SEL_ROOT, dwActiveProtocol)
do_intercept(CMD_SEL_FILE, dwActiveProtocol)
do_intercept(CMD_GET_DATA, dwActiveProtocol)

exit(hcard, hcontext)
