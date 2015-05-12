#!/usr/bin/env python

from scapy.all import IP, UDP, sr1

ip="127.0.0.1"
port=4729
sp=58621
gsmtap_hdr="\x02\x04\x04"+"\x00"*13

# FIXME: Is ATR something special?

def gsmtap_send_apdu(data):
# Do we have performance penalty because the socket stays open?
    p=IP(dst=ip, src=ip)/UDP(sport=sp, dport=port)/(gsmtap_hdr+data)
# FIXME: remove show and ans
    if p:
        p.show()

    ans = sr1(p, timeout=2)
    if ans:
        print(ans)

if __name__ == '__main__':
    cmds = ("\xa0\xa4\x00\x00\x02\x6f\x7e\x9f\x0f",
            "\xa0\xd6\x00\x00\x0b\xff\xff\xff\xff\x09\xf1\x07\xff\xfe\x00\x03\x90\x00",
            );
    for cmd in cmds:
        gsmtap_send_apdu(cmd)
