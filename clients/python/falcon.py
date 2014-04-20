#!/usr/bin/env python

host = "localhost"
port = 4242

import sys

# Thrift files 
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from FalconService import *
from ttypes import *

try:
    transport = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = Client(protocol)
    transport.open()

    # Retrieve the pid of the daemon.
    pid = client.getPid()
    print pid

    transport.close()

except Thrift.TException, tx:
    print 'Exception : %s' % (tx.message)

