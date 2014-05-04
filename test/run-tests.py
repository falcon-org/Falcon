#!/usr/bin/env python

import os
import sys
import traceback
import glob
from FalconTest import FalconTest

if __name__ == "__main__":
  """Run all the modules named Test*.py in the test/ directory.
  All these modules are supposed to provide a run() function which should throw
  on failure."""

  cur_dir = os.path.dirname(os.path.realpath(__file__))
  test_modules = sorted(glob.glob(cur_dir + "/Test*.py"))

  num_failed = 0

  for mod_path in test_modules:
    mod_file = os.path.split(mod_path)[1]
    mod_name = os.path.splitext(mod_file)[0]
    mod = __import__(mod_name)

    success = True
    test = FalconTest()
    try:
      test.prepare()
      mod.run(test)
      test.finish()
    except:
      f = open(test.get_error_log_file(), 'a')
      traceback.print_exc(file=f)
      f.close()
      success = False
      num_failed += 1

    if success:
      print '\033[92m' + "PASS" + '\033[0m',
    else:
      print '\033[91m' + "FAIL" +  '\033[0m',
    print mod_name, test.get_working_directory()

  if num_failed == 0:
    print '\033[92m' + "All tests passed" + '\033[0m',
  else:
    print '\033[91m' + str(num_failed) + " tests failed" +  '\033[0m',
