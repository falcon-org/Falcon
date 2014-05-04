#!/usr/bin/env python

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

  # test.gen_graphviz("graph0.png")

  # At the beginning, all the targets are dirty.
  assert(set(["source1", "source2", "output"]) == set(test.get_dirty_targets()))

  # Build. Everything should be up to date.
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')

  # test.gen_graphviz("graph1.png")

  # Stop the daemon, and write to source
  test.shutdown()
  test.write_file("source2", "3")

  # Restart the daemon, source2 and output should be dirty
  test.start()

  # test.gen_graphviz("graph2.png")

  # TODO: this test is failing. get_dirty_targets() returns an empty list...
  assert(set(["source2", "output"]) == set(test.get_dirty_targets()))

  # Build. Everything should be up to date and output should have the new
  # content.
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '13')
