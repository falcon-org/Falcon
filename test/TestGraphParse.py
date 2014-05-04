#!/usr/bin/env python

# A basic test that checks that the graph is correctly parsed and the
# dependencies properly constructed.

makefile = '''
{
  "rules":
    [
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
  test.start()

  assert(test.get_inputs_of("source1") == [])
  assert(test.get_inputs_of("source2") == [])
  assert(test.get_inputs_of("source3") == [])
  assert(test.get_inputs_of("source4") == [])
  assert(set(test.get_inputs_of("node1")) == set(["source1", "source2"]))
  assert(set(test.get_inputs_of("node2")) == set(["source3", "source4"]))
  assert(set(test.get_inputs_of("output")) == set(["node1", "node2"]))
  assert(set(test.get_outputs_of("source1")) == set(["node1"]))
  assert(set(test.get_outputs_of("source2")) == set(["node1"]))
  assert(set(test.get_outputs_of("source3")) == set(["node2"]))
  assert(set(test.get_outputs_of("source4")) == set(["node2"]))
  assert(set(test.get_outputs_of("node1")) == set(["output"]))
  assert(set(test.get_outputs_of("node2")) == set(["output"]))
  assert(test.get_outputs_of("output") == [])
