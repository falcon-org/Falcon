#!/usr/bin/env python

host = "localhost"
cmd_port = 4242
stream_port = 4343

import sys
import argparse
import json
import socket
import subprocess
import time

# Thrift files
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from FalconService import *
from ttypes import *

def startDaemon():
  """Start the falcon daemon in a sub process"""
  r = subprocess.call(["falcon", "--log-level", "0"])
  return r == 0

def connect():
  """Connect to the falcon daemon. Throws an exception if the daemon is not
  running"""
  transport = TSocket.TSocket(host, cmd_port)
  transport = TTransport.TBufferedTransport(transport)
  protocol = TBinaryProtocol.TBinaryProtocol(transport)
  client = Client(protocol)
  transport.open()
  return (transport, client)

def startAndConnect():
  """Connect to the falcon daemon. Start the daemon if needed"""
  # Try to connect first, if we can't, spawn the daemin.
  try:
    return connect()
  except Thrift.TException:
    print "Starting daemon..."
    if not startDaemon():
      print "Could not start falcon daemon."
      sys.exit(1)

  # We started the daemon, try to connect at regular intervals.
  tries = 0
  while (tries < 40):
    try:
      return connect()
    except Thrift.TException:
      tries += 1
      time.sleep(0.05)
  print "Could not connect to daemon..."
  sys.exit(1)

def build(client):
  """Start a build"""
  r = client.startBuild()
  if r == 1:
    print "Already building..."
    return

  # Connect to the streaming server and read indefinitely.
  sock = socket.socket()
  sock.connect(("localhost", stream_port))
  while 1:
    data = sock.recv(1024)
    if not data: break
    sys.stdout.write(data)
    sys.stdout.flush()

def stopDaemon():
  """Stop the running daemon. Does nothing if the daemon is not running"""
  try:
    (transport, client) = connect()
    client.shutdown()
    transport.close()
    return True
  except Thrift.TException:
    print "Daemon was not running."
    return False

def getPid(client, transport):
  """Connects to the daemon, retrieves its pid and prints it
  on the standard output. If client is not None, use it for the connection
  instead of creating a new connection"""
  if not client:
    try:
      (transport, client) = connect()
    except Thrift.TException:
      print "Daemon is not running."
      return False
  print client.getPid()
  transport.close()
  return True

def setDirty(client, transport, targets):
  """Connects to the daemon. Set the given list of targets as dirty. If client
  is not None, use it for the connection instead of creating a new connection"""
  if not client:
    try:
      (transport, client) = connect()
    except Thrift.TException:
      print "Daemon is not running."
      return False
  ret = True
  for target in targets:
    try:
      client.setDirty(target)
      print "target", target, "is marked dirty"
    except TargetNotFound:
      print "target", target, "not found"
      ret = False
  transport.close()
  return ret

def getInputsOf(client, target):
  try:
    data = client.getInputsOf(target)
    print json.dumps(list(data))
  except TargetNotFound:
    print "target", target, "not found"
    return False
  return True

def getOutputsOf(client, target):
  try:
    data = client.getOutputsOf(target)
    print json.dumps(list(data))
  except TargetNotFound:
    print "target", target, "not found"
    return False
  return True

def main(argv):
  parser = argparse.ArgumentParser(description="Falcon client.")

  group = parser.add_mutually_exclusive_group()
  group.add_argument('--start', action='store_true',
      help="Start the falcon daemon. "
           "Has no effect if the daemon is already started.")
  group.add_argument('--stop', action='store_true',
      help="Stop the falcon daemon. "
           "Has no effect if the daemon is not running."
           " Will interrupt the current build, if any.")
  group.add_argument('--restart', action='store_true',
      help="Restart the falcon daemon. "
           "Will start the daemon if it was not running.")

  group = parser.add_mutually_exclusive_group()
  group.add_argument('-b', '--build', metavar='TARGET', nargs='*',
      help="Start building the given targets. "
           "Build everything if no target is given. "
           "Will start the daemon if it is not running.")
  group.add_argument('--get-graphviz', action='store_true',
      help="Print the graphviz representation of the graph.")
  group.add_argument('--interrupt', action='store_true',
      help="Interrupt the current build.")
  group.add_argument('--set-dirty', metavar='TARGET', nargs='+',
      help="Mark the given targets dirty.")
  group.add_argument('--get-dirty-sources', action='store_true',
      help="Print a json array containing the list of dirty sources.")
  group.add_argument('--get-dirty-targets', action='store_true',
      help="Print a json array containing the list of dirty targets,"
      " including the dirty sources.")
  group.add_argument('--get-inputs-of', metavar='TARGET', nargs=1,
      help="Print a json array containing the list of inputs needed to build"
      " the given target.")
  group.add_argument('--get-outputs-of', metavar='TARGET', nargs=1,
      help="Print a json array containing the list of outputs for which the"
      " the given target is an input.")
  group.add_argument('-p', '--pid', action='store_true',
      help="Print the pid of the daemon")

  args = parser.parse_args(argv)

  transport = None
  client = None

  # Handle start/stop/restart commands.
  if args.start:
    (transport, client) = startAndConnect()
  elif args.restart:
    stopDaemon()
    (transport, client) = startAndConnect()
  elif args.stop:
    stopDaemon()
    sys.exit(0)

  # Commands that do not cause the daemon to be started.
  if args.pid:
    r = getPid(client, transport)
    sys.exit(0 if r else 1)
  elif args.set_dirty != None:
    r = setDirty(client, transport, args.set_dirty)
    sys.exit(0 if r else 1)

  # The rest of the commands require the client to be connected and will spawn
  # the daemon if needed.
  if not client:
    (transport, client) = startAndConnect()

  ret = 0

  if args.build != None:
    # TODO: pass the array of targets to build.
    build(client)
  elif args.get_dirty_sources:
    data = client.getDirtySources()
    print json.dumps(list(data))
  elif args.get_dirty_targets:
    data = client.getDirtyTargets()
    print json.dumps(list(data))
  elif args.get_inputs_of != None:
    if not getInputsOf(client, args.get_inputs_of[0]):
      ret = 1
  elif args.get_outputs_of != None:
    if not getOutputsOf(client, args.get_outputs_of[0]):
      ret = 1
  elif args.get_graphviz:
    print client.getGraphviz()

  transport.close()
  sys.exit(ret)

if __name__ == "__main__":
  main(sys.argv[1:])
