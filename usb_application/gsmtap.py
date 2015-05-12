#!/usr/bin/env python

from scapy.all import IP, UDP, sr1

ip="127.0.0.1"
port=4729
sp=58621
data=("\x02\x04\x04"+"\x00"*13+"\x9f\x16\xa0\xc0\x00\x00\x16", "\x9f\x16\xa0\xc0\x00\x00\x16")

p=IP(dst=ip, src=ip)/UDP(sport=sp, dport=port)/data[0]

if p:
    p.show()

ans = sr1(p, timeout=2)
if ans:
    print(a)
