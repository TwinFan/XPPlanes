#!python
# see https://stackoverflow.com/a/1794373

import sys
import socket

MCAST_GRP = '239.255.1.1'
MCAST_PORT = 49900
MCAST_TTL = 8

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, MCAST_TTL)

# For Python 3, change next line to 'sock.sendto(b"robot", ...' to avoid the
# "bytes-like object is required" msg (https://stackoverflow.com/a/42612820)
sock.sendto(bytes (sys.argv[1],'ascii'), (MCAST_GRP, MCAST_PORT))
