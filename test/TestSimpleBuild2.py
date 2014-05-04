#!/usr/bin/env python

import time

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

  # At the beginning, all the targets are dirty.
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))

  # Build. Everything should be up to date.
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')

  # test.gen_graphviz("graph1.png")

  # Stop the daemon, and write to source
  test.shutdown()
  # We need to wait because stat() is not very precise... We can remove that
  # once falcon uses hashes to compare files.
  time.sleep(1)
  test.write_file("source2", "3")

  # We need to wait because when we require watchman to trigger a file, the
  # since option is set to 'currentTime - 1' : if the file has just been
  # created, watchman will set the file dirty after we finish to start and
  # build.
  time.sleep(1)

  # Restart the daemon, source2 and output should be dirty
  test.start()

  assert(set(["source2", "output"]) == set(test.get_dirty_targets()))

  # Build. Everything should be up to date and output should have the new
  # content.
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '13')
