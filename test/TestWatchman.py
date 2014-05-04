#!/usr/bin/env python

import time
import subprocess

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

  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))

  test.build()

  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')

  time.sleep(1)
  test.write_file("source2", "3")

  # Wait for watchman to trigger the file and check that source2 and output are
  # dirty.
  test.expect_watchman_trigger("source2")
  assert(set(["source2", "output"]) == set(test.get_dirty_targets()))

  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '13')
