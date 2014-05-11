#!/usr/bin/env python

import os
import sys
import traceback
import glob
import argparse
from FalconTest import FalconTest

def log_traceback(file, print_stack):
  f = open(file, 'a')
  traceback.print_exc(file=f)
  f.close()
  if print_stack:
    traceback.print_exc()

def run_test(test, mod, print_stack):
  error_log = test.get_error_log_file()
  try:
    test.prepare()
  except:
    log_traceback(error_log, print_stack)
    return False
  ret = True
  try:
    mod.run(test)
  except:
    log_traceback(error_log, print_stack)
    ret = False
  try:
    test.finish()
  except:
    log_traceback(error_log, print_stack)
    ret = False
  return ret

if __name__ == "__main__":
  """Run all the modules named Test*.py in the test/ directory.
  All these modules are supposed to provide a run() function which should throw
  on failure."""

  parser = argparse.ArgumentParser(description="Falcon tests.")
  parser.add_argument('-t', '--tests', metavar='<test name>', nargs='+',
      help="Define the list of tests to run")
  parser.add_argument('-s', '--stack', action="store_true",
      help="Print the stack trace if a test fails")

  args = parser.parse_args(sys.argv[1:])

  cur_dir = os.path.dirname(os.path.realpath(__file__))
  test_modules = sorted(glob.glob(cur_dir + "/Test*.py"))

  mods = []
  if args.tests:
    mods = args.tests
  else:
    for mod_path in test_modules:
      mod_file = os.path.split(mod_path)[1]
      mod_name = os.path.splitext(mod_file)[0]
      mods.append(mod_name)

  num_failed = 0

  for mod_name in mods:
    mod = __import__(mod_name)
    test = FalconTest()
    if not run_test(test, mod, args.stack):
      num_failed += 1
      print '\033[91m' + "FAIL" +  '\033[0m',
    else:
      print '\033[92m' + "PASS" + '\033[0m',
    print mod_name, test.get_working_directory()

  if num_failed == 0:
    print '\033[92m' + "All tests passed" + '\033[0m',
  else:
    print '\033[91m' + str(num_failed) + " tests failed" +  '\033[0m',
