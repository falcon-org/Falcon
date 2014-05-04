#!/usr/bin/env python

# A basic test that checks if we can start and stop the daemon.

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
  test.start()
