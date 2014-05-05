#!/usr/bin/env python

# Start falcon, modify some files and expect the hashes to change.

import time

makefile = '''
{
  "rules":
    [
    {
      "inputs": [ "output" ],
      "outputs": [ "all" ]
    },
    {
      "inputs": [ "node1", "node2" ],
      "outputs": [ "output" ],
      "cmd": "cat node1 > output && cat node2 >> output"
    },
    {
      "inputs": [ "source1", "source2" ],
      "outputs": [ "node1" ],
      "cmd": "cat source1 > node1 && cat source2 >> node1"
    },
    {
      "inputs": [ "source3", "source4" ],
      "outputs": [ "node2" ],
      "cmd": "cat source3 > node2 && cat source4 >> node2"
    }
    ]
}
'''

def run(test):
  test.create_makefile(makefile)
  test.write_file("source1", "1")
  test.write_file("source2", "2")
  test.write_file("source3", "3")
  test.write_file("source4", "4")

  test.start()
  test.build()

  hash_source1 = test.get_hash_of("source1")
  hash_source2 = test.get_hash_of("source2")
  hash_source3 = test.get_hash_of("source3")
  hash_source4 = test.get_hash_of("source4")
  hash_node1   = test.get_hash_of("node1")
  hash_node2   = test.get_hash_of("node2")
  hash_output  = test.get_hash_of("output")
  hash_all = test.get_hash_of("all")

  # Modify source2. Watchman should trigger the file to be dirty.
  time.sleep(1)
  test.write_file("source2", "3")
  test.expect_watchman_trigger("source2")
  assert(set(["source2", "node1", "output", "all"])
      == set(test.get_dirty_targets()))

  # Expect the hash of source2, node1, output, all to be changed
  tmp_hash_source2   = test.get_hash_of("source2")
  tmp_hash_node1     = test.get_hash_of("node1")
  tmp_hash_output    = test.get_hash_of("output")
  tmp_hash_all       = test.get_hash_of("all")
  assert(tmp_hash_source2 != hash_source2)
  assert(tmp_hash_node1   != hash_node1)
  assert(tmp_hash_output  != hash_output)
  assert(tmp_hash_all     != hash_all)
  hash_source2 = tmp_hash_source2
  hash_node1   = tmp_hash_node1
  hash_output  = tmp_hash_output
  hash_all     = tmp_hash_all

  # Expect the hash of source1, source3, source4, node2 to be unchanged
  assert(test.get_hash_of("source1") == hash_source1)
  assert(test.get_hash_of("source3") == hash_source3)
  assert(test.get_hash_of("source4") == hash_source4)
  assert(test.get_hash_of("node2") == hash_node2)

  # Modify source1. Watchman should trigger the file to be dirty.
  time.sleep(1)
  test.write_file("source1", "5")
  test.expect_watchman_trigger("source1")
  assert(set(["source1", "source2", "node1", "output", "all"])
      == set(test.get_dirty_targets()))

  # Expect the hash of source1, node1, output, all to be changed
  tmp_hash_source1   = test.get_hash_of("source1")
  tmp_hash_node1     = test.get_hash_of("node1")
  tmp_hash_output    = test.get_hash_of("output")
  tmp_hash_all       = test.get_hash_of("all")
  assert(tmp_hash_source1 != hash_source1)
  assert(tmp_hash_node1   != hash_node1)
  assert(tmp_hash_output  != hash_output)
  assert(tmp_hash_all     != hash_all)
  hash_source1 = tmp_hash_source1
  hash_node1   = tmp_hash_node1
  hash_output  = tmp_hash_output
  hash_all     = tmp_hash_all

  # Expect the hash of source2, source3, source4, node2 to be unchanged
  assert(test.get_hash_of("source2") == hash_source2)
  assert(test.get_hash_of("source3") == hash_source3)
  assert(test.get_hash_of("source4") == hash_source4)
  assert(test.get_hash_of("node2") == hash_node2)

  # Modify source3. Watchman should trigger the file to be dirty.
  time.sleep(1)
  test.write_file("source3", "6")
  test.expect_watchman_trigger("source3")
  assert(set(["source1", "source2", "source3", "node1", "node2", "output",
    "all"]) == set(test.get_dirty_targets()))

  # Expect the hash of source3, node2, output, all to be changed
  tmp_hash_source3   = test.get_hash_of("source3")
  tmp_hash_node2     = test.get_hash_of("node2")
  tmp_hash_output    = test.get_hash_of("output")
  tmp_hash_all       = test.get_hash_of("all")
  assert(tmp_hash_source3 != hash_source3)
  assert(tmp_hash_node2   != hash_node2)
  assert(tmp_hash_output  != hash_output)
  assert(tmp_hash_all     != hash_all)
  hash_source3 = tmp_hash_source3
  hash_node2   = tmp_hash_node2
  hash_output  = tmp_hash_output
  hash_all     = tmp_hash_all

  # Expect the hash of source1, source2, source4, node1 to be unchanged
  assert(test.get_hash_of("source1") == hash_source1)
  assert(test.get_hash_of("source2") == hash_source2)
  assert(test.get_hash_of("source4") == hash_source4)
  assert(test.get_hash_of("node1") == hash_node1)
