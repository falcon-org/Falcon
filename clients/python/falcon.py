#!/usr/bin/env python

host = "localhost"
cmd_port = 4242
stream_port = 4343

default_log_level = "1"
default_log_dir = "/tmp/"

import os
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

def spawnDaemon(log_level, log_dir):
  """Start the falcon daemon in a sub process"""
  p = ["falcon", "--log-level", log_level, "--log-dir", log_dir,
      "--working-directory", os.getcwd()]
  FNULL = open(os.devnull, 'w')
  r = subprocess.call(p, stdin=FNULL, stdout=FNULL, stderr=FNULL)
  return r == 0

def connect():
  """Connect to the falcon daemon. Throws an exception if the daemon is not
  running. Return (transport, client)"""
  transport = TSocket.TSocket(host, cmd_port)
  transport = TTransport.TBufferedTransport(transport)
  protocol = TBinaryProtocol.TBinaryProtocol(transport)
  client = Client(protocol)
  transport.open()
  return (transport, client)

def connectLoop():
  """Try to connect to the daemon. Wait and try again on failure until succeeded
  or we tried too many times. This can be used if we just started the daemon, in
  order to give it time to set up its server.
  Return (transport, client) or None on failure."""
  # We started the daemon, try to connect at regular intervals.
  tries = 0
  while (tries < 100):
    try:
      return connect()
    except Thrift.TException:
      tries += 1
      time.sleep(0.05)
  return None

def startDaemon(log_level, log_dir):
  """Start the daemon. Return false if the daemon is already
  running. Return True of False on failure."""
  try:
    (transport, client) = connect()
    transport.close()
    print "The falcon daemon is already running"
    return False
  except Thrift.TException:
    r = spawnDaemon(log_level, log_dir)
    if not r:
      print "Could not start the daemon"
    return r

def stopDaemon():
  """Stop the running daemon. Returns False if the daemon was not running."""
  r = connectLoop()
  if not r:
    return False
  (transport, client) = r
  client.shutdown()
  transport.close()
  return True

def connectSpawnIfNeeded():
  """Connect to the falcon daemon. Start the daemon if needed.
  Return (transport, client) or None on failure."""
  r = connectLoop()
  if not r:
    if not spawnDaemon(default_log_level, default_log_dir):
      return None
    return connectLoop()
  else:
    return r

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

def setDirty(transport, client, targets):
  """Set the given list of targets as dirty."""
  ret = True
  for target in targets:
    try:
      client.setDirty(target)
      print "target", target, "is marked dirty"
    except TargetNotFound:
      print "target", target, "not found"
      ret = False
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

###############################################################################
#                             Main function                                   #
###############################################################################

def main(argv):
  parser = argparse.ArgumentParser(description="Falcon client.")

  group = parser.add_mutually_exclusive_group()
  group.add_argument('--start', metavar=('<log-level>', '<log-dir>'), nargs=2,
      help="Start the falcon daemon. "
           "Has no effect if the daemon is already started.")
  group.add_argument('--restart', metavar=('<log-level>', '<log-dir>'), nargs=2,
      help="Restart the falcon daemon. "
           "Will start the daemon if it was not running.")
  group.add_argument('--stop', action='store_true',
      help="Stop the falcon daemon. "
           "Has no effect if the daemon is not running."
           " Will interrupt the current build, if any.")
  group.add_argument('--no-start', action='store_true',
      help="Do no start the daemon if not started")

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

  # Handle start/stop/restart/no-start commands.
  if args.start:
    (log_level, log_dir) = args.start
    if not startDaemon(log_level, log_dir):
      sys.exit(1)
  elif args.restart:
    (log_level, log_dir) = args.restart
    stopDaemon()
    if not startDaemon(log_level, log_dir):
      sys.exit(1)
  elif args.stop:
    r = stopDaemon()
    if not r:
      print "Could not stop the daemon"
    sys.exit(0 if r else 1)


  # Commands that do not cause the daemon to be started.
  if args.pid or args.set_dirty or args.interrupt:
    connectInfo = connectLoop()
    if not connectInfo:
      print "Daemon is not started. Use --start to start it."
      sys.exit(1)
    if args.pid:
      print connectInfo[1].getPid()
      sys.exit(0)
    elif args.set_dirty != None:
      r = setDirty(connectInfo[0], connectInfo[1], args.set_dirty)
      sys.exit(0 if r else 1)
    elif args.interrupt:
      connectInfo[1].interruptBuild()

  # The rest of the commands require the client to be connected and will spawn
  # the daemon if needed.

  if args.no_start or args.start or args.restart:
    # Either the user specifield "--no-start" or already started the daemon with
    # "--start" or "--restart". In either case, we only want to try to connect
    # to the daemon but won't spawn it here.
    connectInfo = connectLoop()
  else:
    # Otherwise, take care of spawning the daemon if needed.
    connectInfo = connectSpawnIfNeeded()
  if not connectInfo:
    print "Could not connect to the falcon daemon."
    sys.exit(1)

  client = connectInfo[1]
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

  connectInfo[0].close()
  sys.exit(ret)

if __name__ == "__main__":
  main(sys.argv[1:])
