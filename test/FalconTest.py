import time
import json
import os
import os.path
import subprocess
import tempfile

class FalconTest:
  """Test engine. Provides methods for communicating with the falcon daemon"""

  def __init__(self):
    self._test_folder = os.path.dirname(os.path.realpath(__file__))
    self._falcon_client = self._test_folder + "/../clients/python/falcon.py"
    self._test_dir = tempfile.mkdtemp()
    self._falcon_log_dir = self._test_dir + "/logs"
    self._falcon_log_level = "0"
    self._error_log = self._test_dir + "/error.log"
    self._expect_error_log = False
    os.chdir(self._test_dir)
    os.mkdir(self._falcon_log_dir)

  def ensure_shutdown(self, expect_running=False):
    # Try to shut down any existing daemon nicely.
    try:
      pid = subprocess.check_output(["pgrep", "falcon"])
    except subprocess.CalledProcessError:
      return
    r = subprocess.call([self._falcon_client, "--stop"])
    if expect_running:
      assert(r == 0)
    if r == 0:
      # Now that we sent the stop command, wait for it to stop
      tries = 0
      while tries < 50:
        try:
          pid = subprocess.check_output(["pgrep", "falcon"])
        except subprocess.CalledProcessError:
          return
        tries += 1
        time.sleep(0.1)
      # If we reach here, we could not stop the daemon, kill it...
      # If we expected the daemon to be running, not being able to stop it
      # cleanly is an error.
      assert(not expect_running)
      subprocess.call("kill -9 " + pid, shell=True)

  def prepare(self):
    self.ensure_shutdown()
    # Kill watchman (should have been stopped by the daemon).
    subprocess.call("pkill watchman", shell=True)

  def finish(self):
    self.ensure_shutdown()
    # Kill watchman (should have been stopped by the daemon).
    subprocess.call("pkill watchman", shell=True)
    # check that there were no errors
    has_error_log = os.path.isfile(self._falcon_log_dir + "/falcon.ERROR")
    if self._expect_error_log:
      assert(has_error_log)
    else:
      assert(not has_error_log)

  def expect_error_log(self):
    self._expect_error_log = True

  def get_working_directory(self):
    return self._test_dir

  def get_error_log_file(self):
    return self._error_log

  def create_makefile(self, data):
    f = open(self._test_dir + "/makefile.json", 'w')
    f.write(data)
    f.close()

  def shutdown(self):
    """Shutdown the falcon daemon"""
    self.ensure_shutdown(True)

  def restart(self):
    """Restart the falcon daemon"""
    FNULL = open(os.devnull, 'w')
    r = subprocess.call([self._falcon_client, "--restart",
                         self._falcon_log_level, self._falcon_log_dir],
                         stdout=FNULL)
    assert(r == 0)

  def start(self):
    """Start the falcon daemon"""
    FNULL = open(os.devnull, 'w')
    r = subprocess.call([self._falcon_client, "--start",
                         self._falcon_log_level, self._falcon_log_dir],
                         stdout=FNULL)
    assert(r == 0)

  def build(self):
    """Trigger a build.."""
    # TODO: for now we pipe the output to stdout. We will parse the output
    # later.
    FNULL = open(os.devnull, 'w')
    return subprocess.call([self._falcon_client, "--no-start", "--build"],
                           stdout=FNULL)

  def write_file(self, name, content):
    """Write to a file"""
    f = open(self._test_dir + "/" + name, 'w')
    f.write(content)
    f.close()

  def get_file_content(self, file):
    """Get the content of a file """
    return ''.join(open(file, 'r').readlines())

  def get_pid(self):
    pid = subprocess.check_output([self._falcon_client, "--pid"])
    return pid

  def gen_graphviz(self, name):
    """Generate a graphviz image"""
    data = subprocess.check_output([self._falcon_client, "--no-start",
                                   "--get-graphviz"])
    proc = subprocess.Popen(['dot', '-Tpng'],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    png = proc.communicate(data)[0]
    f = open(name, 'w')
    f.write(png)
    f.close()

  def get_dirty_sources(self):
    """Get the list of currently dirty sources"""
    data = subprocess.check_output([self._falcon_client, "--no-start",
                                   "--get-dirty-sources"])
    return json.loads(data)

  def get_dirty_targets(self):
    """Get the list of currently dirty targets"""
    data = subprocess.check_output([self._falcon_client, "--no-start",
                                   "--get-dirty-targets"])
    return json.loads(data)

  def get_inputs_of(self, target):
    """Get the list of inputs of a given targets"""
    data = subprocess.check_output([self._falcon_client, "--no-start",
                                    "--get-inputs-of", target])
    return json.loads(data)

  def get_outputs_of(self, target):
    """Get the list of outputs of a given targets"""
    data = subprocess.check_output([self._falcon_client, "--no-start",
                                    "--get-outputs-of", target])
    return json.loads(data)

  def get_hash_of(self, target):
    """Get the hash of a given target"""
    data = subprocess.check_output([self._falcon_client, "--no-start",
                                    "--get-hash-of", target])
    return data

  def expect_watchman_trigger(self, target):
    """Wait for the given target to be dirty because of a watchman trigger.
    This will call get_dirty_targets several times until the target is dirty.
    This asserts in case a timeout expires."""
    tries = 0
    while tries < 50:
      if target in self.get_dirty_targets():
        return
      time.sleep(0.1)
      tries += 1
    # timeout
    assert(False)
