#!python
# see https://stackoverflow.com/a/1794373

import socket
import argparse                     # handling command line arguments

# --- Handling command line argumens ---
parser = argparse.ArgumentParser(description='mc_send: Send a Multicast datagram to a given address and port.')
parser.add_argument('datagram', help='The text to send')
parser.add_argument('-a', '--address', metavar='NAME_OR_IP', help='Multicast group address to send the data to, defaults to \'239.255.1.1\'', default='239.255.1.1')
parser.add_argument('-p', '--port', metavar='NUM', help='UDP port to send traffic data to, defaults to 49900', type=int, default=49900)
parser.add_argument('--ttl', metavar='NUM', help='Time to live, defaults to 8', type=int, default=8)
parser.add_argument('-v', '--verbose', help='Verbose output: Informs of each sent record', action='store_true')

args = parser.parse_args()

if args.verbose:
    print ("Adress:   {}:{}, ttl={}".format(args.address, args.port, args.ttl))
    print ("Datagram: {}".format(args.datagram))


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, args.ttl)
sock.sendto(args.datagram.encode('utf-8'), (args.address, args.port))
