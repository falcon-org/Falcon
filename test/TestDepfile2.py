#!/usr/bin/env python

# Check that on startup, falcon can reload a depfile that was built in a
# previous launch of the daemon.

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

  # Start the daemon a first time, and check that it loaded the depfile and
  # knows about source2.
  test.start()
  assert(set(["source1", "output"]) == set(test.get_dirty_targets()))
  test.build()
  assert(test.get_dirty_targets() == [])
  assert(test.get_file_content('output') == '12')
  assert(set(["source1", "source2"]) == set(test.get_inputs_of("output")))
  assert(set(["output"]) == set(test.get_outputs_of("source2")))

  # Stop the daemon.
  test.shutdown()

  # Restart the daemon. It should parse the depfile again and know about
  # source2. Everything should be up to date.
  test.start()

  assert(set(["source1", "source2"]) == set(test.get_inputs_of("output")))
  assert(set(["output"]) == set(test.get_outputs_of("source2")))
  assert(test.get_dirty_targets() == [])
