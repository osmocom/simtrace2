import sys
import traceback

from pySim.commands import SimCardCommands
from pySim.utils import h2b, swap_nibbles, rpad, dec_imsi, dec_iccid
from pySim.transport.pcsc import PcscSimLink



def pySim_read():
    sl = PcscSimLink(0)

    # Create command layer
    scc = SimCardCommands(transport=sl)

    # Wait for SIM card
    sl.wait_for_card()

    # Program the card
    print("Reading ...")

    # EF.ICCID
    try:
        (res, sw) = scc.read_binary(['3f00', '2fe2'])
        if sw == '9000':
                print("ICCID: %s" % (dec_iccid(res),))
        else:
                print("ICCID: Can't read, response code = %s" % (sw,))
    except:
        print("Unexpected error:", sys.exc_info()[0])
        print(traceback.format_exc())

    # EF.IMSI
    try:
        (res, sw) = scc.read_binary(['3f00', '7f20', '6f07'])
        if sw == '9000':
                print("IMSI: %s" % (dec_imsi(res),))
        else:
                print("IMSI: Can't read, response code = %s" % (sw,))
    except:
        print("Unexpected error:", sys.exc_info()[0])
        print(traceback.format_exc())

    # EF.SMSP
    try:
        (res, sw) = scc.read_record(['3f00', '7f10', '6f42'], 1)
        if sw == '9000':
                print("SMSP: %s" % (res,))
        else:
                print("SMSP: Can't read, response code = %s" % (sw,))
    except:
        print("Unexpected error:", sys.exc_info()[0])
        print(traceback.format_exc())

    # EF.HPLMN
    try:
        (res, sw) = scc.read_binary(['3f00', '7f20', '6f30'])
        if sw == '9000':
            print("HPLMN: %s" % (res))
            print("HPLMN: %s" % (dec_hplmn(res),))
        else:
            print("HPLMN: Can't read, response code = %s" % (sw,))
    except:
        print("Unexpected error:", sys.exc_info()[0])
        print(traceback.format_exc())

    # EF.ACC
    try: 
        (res, sw) = scc.read_binary(['3f00', '7f20', '6f78'])
        if sw == '9000':
                print("ACC: %s" % (res,))
        else:
                print("ACC: Can't read, response code = %s" % (sw,))
    except:
        print("Unexpected error:", sys.exc_info()[0])
        print(traceback.format_exc())

    # EF.MSISDN
    try:
    #       print(scc.record_size(['3f00', '7f10', '6f40']))
        (res, sw) = scc.read_record(['3f00', '7f10', '6f40'], 1)
        if sw == '9000':
            if res[1] != 'f':
                print("MSISDN: %s" % (res,))
            else:
                print("MSISDN: Not available")
        else:
            print("MSISDN: Can't read, response code = %s" % (sw,))
    except:
        print("MSISDN: Can't read. Probably not existing file, error: ", sys.exc_info()[0])
        print(traceback.format_exc())

    print("Done !\n")
