#!/usr/bin/python3

"""
Sends data from a file to UDP

For usage info call
    python3 SendTraffic.py -h


MIT License

Copyright (c) 2022 B.Hoppe

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import os
import sys
import socket
import time
import argparse                     # handling command line arguments

_tsDiff = 0

""" === Compute and wait for timestamp """
def compWaitTS(ts_s: str) -> str:
    global _tsDiff

    # current time and convert timestamp
    now = int(time.time())
    ts = float(ts_s)

    # First time called? -> compute initial timestamp difference
    if not _tsDiff:
        _tsDiff = now - ts - args.bufPeriod
        if args.verbose:
            print ("Timestamp difference: {}".format(_tsDiff))

    # What's the required timestamp to wait for and then return?
    ts += _tsDiff

    # if that's in the future then wait
    if (ts > now):
        if args.verbose:
            print ("Waiting for {} seconds...".format(ts-now), end='\r')
        time.sleep (ts-now)

    # Adjust returned timestamp value for historic timestamp
    ts -= args.historic

    return str(ts)

""" === Handle traffic data ==="""
def sendTrafficData(ln: str) -> int:
	# Send the data
    sock.sendto(ln.encode('ascii'), (args.host, args.port))
    if args.verbose:
	    print (ln)
    return 1

""" === MAIN === """

# --- Handling command line argumens ---
parser = argparse.ArgumentParser(description='SendData 0.1.0: Sends data out as UDP broadcast',fromfile_prefix_chars='@')
parser.add_argument('inFile', help='Data file, each line is a record, <stdin> by default', nargs='?', type=argparse.FileType('r'), default=sys.stdin)
parser.add_argument('-l', '--loop', help='Endless loop: restart from the beginning when reaching end of file. Will work best if data contains loop with last position(s) being roughly equal to first position(s).', action='store_true')
parser.add_argument('--host', metavar='NAME_OR_IP', help='UDP target host or ip to send the data to, defaults to \'localhost\'', default='localhost')
parser.add_argument('--port', metavar='NUM', help='UDP port to send traffic data to, defaults to 49005', type=int, default=49005)
parser.add_argument('-v', '--verbose', help='Verbose output: Informs of each sent record', action='store_true')

args = parser.parse_args()

# --- open the UDP socket ---
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Outer loop helps with endless looping
while 1:
    _tsDiff = 0
    # --- open and loop the input file ---
    for line in args.inFile:
        sendTrafficData(line)

    # Endless loop?
    if (not args.loop): break           # no, end replay
    args.inFile.seek(0, os.SEEK_SET)    # restart file from beginning

# --- Cleanup ---
args.inFile.close
sock.close
