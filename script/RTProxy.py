#!/usr/bin/python3

# Reads RTTFC data from port 49005,
# converts it to XPPTraffic.json,
# and sends it in batches (arrays) to the provided multicast address

import sys
import socket
import argparse                     # handling command line arguments
import json

_buf = []

#
# Convert RTTFC to XPPTraffic, and send if in single-record mode
#
def Convert(d: bytes, s: socket):
    global _buf
    
    # split into its fields
    ln = d.decode("utf-8")
    fields = ln.split(',')
    if len(fields) < 29:
    	print("ERROR, too few CSV fields: {}".format(ln))
    	return
    	
    # Test for RTTFC
    if fields[0] != "RTTFC":
    	print("ERROR, not RTTFC: {}".format(ln))
    	return
    	
    # Convert to dictionary
    fd = {}
    
    # Just for the fun of it, to test both number and hex strings, we store odd numbers as numbers as even numbers as hex string
    if (int(fields[1]) % 2) == 0:
        fd['id'] = "{:08x}".format(int(fields[1]))
    else:
        fd['id'] = int(fields[1])
    
    # ident object
    fd['ident'] = {}
    if len(fields[11]) > 0: fd['ident']['reg'] = fields[11]
    if len(fields[9]) > 0: fd['ident']['call'] = fields[9]
    
    # If airports are available compute a label containing from/to airports
    if len(fields[9]) > 0 and (len(fields[12]) > 0 or len(fields[13]) > 0):
        fd['ident']['label'] = "{} ({}): {} -> {}".format(fields[9], fields[10], fields[12], fields[13])
    
    # type object
    fd['type'] = {}
    if len(fields[10]) > 0:
        fd['type']['icao'] = fields[10]
    elif fields[28] == "C1" or fields[28] == "C2":          # goodie: convert category "service vehicle" to special type "ZZZC"
        fd['type']['icao'] = "ZZZC"

    # position object (mandatory)
    fd['position'] = {}
    fd['position']['lat'] = float(fields[2])
    fd['position']['lon'] = float(fields[3])
    # altitude is ideally from the geo alt field, otherwise baro alt (without barometric conversion here for simplicity)
    fd['position']['alt_geo'] = int(fields[18]) if int(fields[18]) >= 0 else int (fields[4])
    fd['position']['gnd'] = True if fields[6] == '1' else False
    fd['position']['timestamp'] = float(fields[14]) 

    # attitude object
    fd['attitude'] = {}
    if fields[23] != "-1.0": fd['attitude']['roll'] = float(fields[23])
    if fields[25] != "-1.00":
        fd['attitude']['heading'] = float(fields[25])
    elif fields[24] != "-1.00":
        fd['attitude']['heading'] = float(fields[24])

    # Send it out if in single-record mode
    if args.single:
        sJson = json.dumps(fd)
        if args.verbose:
            print (sJson)
        s.sendto(sJson.encode('ascii'), (args.toAddress, args.toPort))
    # otherwise add it to the global buffer
    else:
        _buf.append(fd)
        # Did array grow too large for network message?
        if len(json.dumps(_buf)) > args.bufSize:
            # remove the just added element and send out what was in before
            del _buf[-1]
            SendArr(s)
            # then add the item back in
            _buf.append(fd)        
   
    return

#
# Send out the buffered array data to the MCST socket
#
def SendArr(s: socket):
    global _buf
    if len(_buf) > 0:
        sJson = json.dumps(_buf)
        if args.verbose:
            print ("{} traffic records:".format(len(_buf)))
            print (sJson)
        s.sendto(sJson.encode('ascii'), (args.toAddress, args.toPort))
        _buf = []

#
############ MAIN ###############
#

# --- Handling command line argumens ---
parser = argparse.ArgumentParser(description='RTProxy: Receive RTTFC, convert to XPPTraffic, forward to multicast')
parser.add_argument('-f', '--fromPort', metavar='NUM', help='UDP port to listen to for RTTFC data, defaults to 49005', type=int, default=49005)
parser.add_argument('-a', '--toAddress', metavar='NAME_OR_IP', help='Multicast group address to send the data to, defaults to \'239.255.1.1\'', default='239.255.1.1')
parser.add_argument('-p', '--toPort', metavar='NUM', help='Multicast port to send traffic data to, defaults to 49900', type=int, default=49900)
parser.add_argument('--ttl', metavar='NUM', help='Time to live, defaults to 8', type=int, default=8)
parser.add_argument('--bufSize', metavar='NUM', help='Receiving buffer size, defaults to 8192', type=int, default=8192)
parser.add_argument('--single', help='Send single JSON records instead of collacted arrays', action='store_true')
parser.add_argument('-v', '--verbose', help='Verbose output: Informs of each sent record', action='store_true')

args = parser.parse_args()

if args.verbose:
    print ("UDP From: {}".format(args.fromPort))
    print ("MCST To:  {}:{}, ttl={}, bufSize={}".format(args.toAddress, args.toPort, args.ttl, args.bufSize))

#--- UDP socket creation (listen)
sockListen = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sockListen.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
if sys.platform != "win32":
    sockListen.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
try:
    sockListen.bind(('', args.fromPort))
except socket.error:
    print ("sockListen failed")
    sockListen.close()
    sys.exit()

#--- MCST socket creation (send)
sockSend = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sockSend.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, args.ttl)


# Send it out


#--- Main loop, interrupted only by Ctrl+C
while 1:
    try:
        data = sockListen.recv(args.bufSize)
    except socket.timeout:
        data = bytes()
        pass

    # Received anything? -> process it
    if len(data) > 0:
        Convert(data, sockSend)
        if not args.single:
            sockListen.settimeout(0.5)          # Wait max half a second for data (to finish collating array data)
    # Received nothing? -> was a timeout, send the buffered data
    else:
        SendArr(sockSend)
        sockListen.settimeout(None)             # Wait eternally for next data
