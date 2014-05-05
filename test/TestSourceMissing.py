#!/usr/bin/env python

# Check that falcon rebuilds an output if it is deleted.

import time
import os

makefile = '''
{
  "rules":
    [
    {
      "inputs": [ "source1", "source2" ],
      "outputs": [ "output" ],
      "cmd": "cat source1 > output && cat source2 >> output"
    }
    ]
}
'''

def run(test):
  test.create_makefile(makefile)
  test.write_file("source1", "1")
  test.write_file("source2", "2")
  test.start()

  # Build a first time to generate output
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')

  # Delete source1
  time.sleep(1)
  os.remove("source1")
  test.expect_watchman_trigger("source1")
  assert(set(["source1", "output"]) == set(test.get_dirty_targets()))

  # Build again. This should fail.
  try:
    test.build()
    # Unreachable, the build should fail because a source file is missing.
    assert(false)
  except:
    pass

  # This should generate an error log
  test.expect_error_log()
