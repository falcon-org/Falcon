import ijson.backends.python as ijson
import sys

# Unfortunately, ijson only works on files. This hack makes it possible to use
# ijson with a socket. We create a wrapper class around a socket, which
# provides the read method.
class SocketWrapper:
  def __init__(self, sock):
    self._sock = sock
  def read(self, buf_size):
    return self._sock.recv(buf_size)

class BuildOutputParser:
  """Helper class for reading the json output of a build and printing it in a
  convenient format on stdout."""

  def __init__(self, sock):
    self._f = SocketWrapper(sock)
    self._parser = ijson.parse(self._f)

  def parse(self):
    result = ''
    nb_commands = 0
    for prefix, event, value in self._parser:
      if event == 'start_map' and prefix == 'cmds.item':
        nb_commands += 1
      if event == 'number':
        if prefix == 'id':
          # For now we do not use the build id.
          pass
        elif prefix == 'cmds.item.id':
          # For now we do not use the command id.
          pass
      elif event == 'string':
        if prefix == 'cmds.item.cmd':
          print value
        elif prefix == 'cmds.item.stdout':
          print value
        elif prefix == 'cmds.item.stderr':
          # This is an error. Print it in red on stderr.
          sys.stderr.write('\033[91m')
          sys.stderr.write(value)
          sys.stderr.write('\033[0m')
        elif prefix == 'result':
          result = value
        elif prefix == 'cmds.item.cache':
          print 'Retrieving', value, 'from cache'

    if result != 'SUCCEEDED':
      return False
    if nb_commands == 0:
      print 'Everything is up to date.'
    return True
