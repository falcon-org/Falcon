#!/usr/bin/env python

# A basic test that checks that we can build a simple makefile.

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

  # You can debug the current state of the graph by creating a png image:
  # test.gen_graphviz("graph1.png")

  # Before we build, every source file should be dirty
  dirty = test.get_dirty_sources()
  assert("source1" in dirty)
  assert("source2" in dirty)
  assert("source3" in dirty)
  assert("source4" in dirty)

  test.build()

  # After we build, nothing should be dirty
  dirty = test.get_dirty_sources()
  assert(not dirty)

  # check the content of all generated files
  assert(test.get_file_content('node1') == '12')
  assert(test.get_file_content('node2') == '34')
  assert(test.get_file_content('output') == '1234')
