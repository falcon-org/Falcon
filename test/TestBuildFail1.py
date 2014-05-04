#!/usr/bin/env python

import time

makefile = '''
{
  "rules":
    [
    {
      "inputs": [ "source1", "source2" ],
      "outputs": [ "output" ],
      "cmd": "exit 1"
    }
    ]
}
'''

def run(test):
  test.create_makefile(makefile)
  test.write_file("source1", "1")
  test.write_file("source2", "2")
  test.start()

  # At the beginning, all the targets are dirty.
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))

  # Build. The build should fail.
  # TODO: test that the build actually failed.
  test.build()

  # Everything should remain dirty.
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))
