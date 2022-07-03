#!python

import socket
import argparse                     # handling command line arguments

# --- Handling command line argumens ---
parser = argparse.ArgumentParser(description='udp_send: Send a UDP datagram to a given host and port.')
parser.add_argument('datagram', help='The text to send')
parser.add_argument('--host', metavar='NAME_OR_IP', help='UDP target host or ip to send the data to, defaults to \'localhost\'', default='localhost')
parser.add_argument('-p', '--port', metavar='NUM', help='UDP port to send traffic data to, defaults to 49005', type=int, default=49005)
parser.add_argument('-v', '--verbose', help='Verbose output: Informs of each sent record', action='store_true')

args = parser.parse_args()

if args.verbose:
    print ("Adress:   {}:{}".format(args.host, args.port))
    print ("Datagram: {}".format(args.datagram))

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.sendto(args.datagram.encode('utf-8'), (args.host, args.port))
