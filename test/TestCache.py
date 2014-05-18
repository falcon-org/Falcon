#!/usr/bin/env python

import time

makefile = '''
{
  "rules":
    [
    {
      "inputs": [ "source1" ],
      "outputs": [ "output" ],
      "cmd": "cat source1 > output && cat source2 >> output && echo 'output: source1 source2' > deps",
      "depfile": "deps"
    }
    ]
}
'''

def set_version_1(test):
  test.write_file("source1", "1")
  test.write_file("source2", "2")

def set_version_2(test):
  test.write_file("source1", "2")
  test.write_file("source2", "3")

def run(test):
  test.create_makefile(makefile)

  set_version_1(test)
  test.start()
  assert(set(["source1", "output"]) == set(test.get_dirty_targets()))
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')
  assert(set(["source1", "source2"]) == set(test.get_inputs_of("output")))
  assert(set(["output"]) == set(test.get_outputs_of("source2")))

  set_version_2(test)
  test.expect_watchman_trigger("source1")
  test.expect_watchman_trigger("source2")
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '23')

  set_version_1(test)
  test.expect_watchman_trigger("source1")
  test.expect_watchman_trigger("source2")
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))
  # Build and check we retrieve output from the cache
  data = test.build()
  assert(len(data['cmds']) == 1)
  assert(data['cmds'][0] == { 'cache' : 'output' })
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')

  set_version_2(test)
  test.expect_watchman_trigger("source1")
  test.expect_watchman_trigger("source2")
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))
  # Build and check we retrieve output from the cache
  data = test.build()
  assert(len(data['cmds']) == 1)
  assert(data['cmds'][0] == { 'cache' : 'output' })
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '23')
