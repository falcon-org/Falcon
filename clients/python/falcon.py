#!/usr/bin/env python

host = "localhost"
port = 4242

import sys
import getopt

from optparse import OptionParser

# Thrift files 
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from FalconService import *
from ttypes import *

def help():
  print "falcon.py [OPTION]"
  print "options:"
  print "  -b                         Start a build"
  print "  -g                         Print the list of dirty sources"
  print "  -p                         Print the pid of the daemon"
  print "  -d <file> | --dirty=<file> Mark <file> to be dirty"


def main(argv):

  try:
    opts, args = getopt.getopt(argv, "hbgpd:", ["--dirty="])
  except getopt.GetoptError:
    help()
    sys.exit(2)

  if len(opts) != 1:
    print "Error: Please provide one option:"
    help()
    sys.exit(2)

  opt, arg = opts[0]
  if opt == '-h':
    help()
    return

  ret = 0

  try:
    transport = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = Client(protocol)
    transport.open()

    if opt == '-b':
      # Start a build
      r = client.startBuild()
      if r == 1:
        print "Already building..."

    elif opt == '-g':
      # Retrieve the dirty sources
      src = client.getDirtySources()
      print src
    elif opt == '-p':
      # Retrieve the pid of the daemon.
      pid = client.getPid()
      print pid
    elif opt in ("-d", "--dirty"):
      if arg == "":
        print "Error: no source file provided"
        help()
        ret = 1
      else:
        # Mark a source as dirty
        try:
          client.setDirty(arg)
        except TargetNotFound:
          ret = 1
          print arg, "is not a source file"

    transport.close()
  except Thrift.TException, tx:
    ret = 1
    print 'Exception : %s' % (tx.message)

  sys.exit(ret)

if __name__ == "__main__":
  main(sys.argv[1:])
