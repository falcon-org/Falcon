#!/usr/bin/env python

import time

# In this test, source2 is an implicit dependency of output.
# We do a first build, check that it is a new dependency, and is not dirty. Then
# we modify it, check that it becomes dirty, build again, and check it is up to
# date.

# Note: this test relies on watchman to notify that the file becomes dirty when
# we modify it.

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

def run(test):
  test.create_makefile(makefile)
  test.write_file("source1", "1")
  test.write_file("source2", "2") # implicit dependency
  test.start()

  # At the beginning, all the targets are dirty, source2 is unknown.
  assert(set(["source1", "output"]) == set(test.get_dirty_targets()))

  # Build. Everything should be up to date.
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')

  # Now, falcon knows about the implicit dependency
  assert(set(["source1", "source2"]) == set(test.get_inputs_of("output")))
  assert(set(["output"]) == set(test.get_outputs_of("source2")))

  # Modify the implicit dependency.
  test.write_file("source2", "3")
  # Watchman should trigger the file.
  test.expect_watchman_trigger("source2")
  assert(set(["source2", "output"]) == set(test.get_dirty_targets()))

  # Build. Everything should be up to date and output should have the new
  # content.
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '13')
