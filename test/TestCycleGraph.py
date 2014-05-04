#!/usr/bin/env python

# A basic test that checks that the graph is correctly parsed and the
# dependencies properly constructed.

makefile = '''
{
  "rules":
  [
    {
      "inputs": [ "input0" ],
      "outputs": [ "input1" ]
    },
    {
      "inputs": [ "input1" ],
      "outputs": [ "input2" ]
    },
    {
      "inputs": [ "input2" ],
      "outputs": [ "input3" ],
      "cmd" : "echo r2"
    },
    {
      "inputs": [ "input2" ],
      "outputs": [ "input4" ],
      "cmd" : "echo r3"
    },
    {
      "inputs": [ "input4" ],
      "outputs": [ "input0" ],
      "cmd" : "echo r2"
    }

  ]
}
'''

def run(test):
  test.expect_error_log()
  test.create_makefile(makefile)
  try:
    test.start()
  except AssertionError:
    return 0
