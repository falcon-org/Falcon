import json
import os
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
    os.chdir(self._test_dir)
    os.mkdir(self._falcon_log_dir)

    # Make sure the daemon is shut down. Do not display any error.
    FNULL = open(os.devnull, 'w')
    subprocess.call([self._falcon_client, "--stop"], stdout=FNULL, stderr=FNULL)

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
    subprocess.call([self._falcon_client, "--stop"])

  def restart(self):
    """Start the falcon daemon"""
    subprocess.call([self._falcon_client, "--restart",
                      self._falcon_log_level, self._falcon_log_dir])

  def start(self):
    """Start the falcon daemon"""
    subprocess.call([self._falcon_client, "--start",
                      self._falcon_log_level, self._falcon_log_dir])

  def build(self):
    """Trigger a build"""
    subprocess.call([self._falcon_client, "--build"])

  def write_file(self, name, content):
    """Write to a file"""
    f = open(self._test_dir + "/" + name, 'w')
    f.write(content)
    f.close()

  def gen_graphviz(self, name):
    """Generate a graphviz image"""
    data = subprocess.check_output([self._falcon_client, "--get-graphviz"])
    proc = subprocess.Popen(['dot', '-Tpng'],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    png = proc.communicate(data)[0]
    f = open(name, 'w')
    f.write(png)
    f.close()

  def get_dirty_sources(self):
    """Get the list of currently dirty sources"""
    data = subprocess.check_output([self._falcon_client, "--get-dirty-sources"])
    return json.loads(data)

  def get_dirty_targets(self):
    """Get the list of currently dirty targets"""
    data = subprocess.check_output([self._falcon_client, "--get-dirty-targets"])
    return json.loads(data)

  def get_inputs_of(self, target):
    """Get the list of inputs of a given targets"""
    data = subprocess.check_output([self._falcon_client,
                                    "--get-inputs-of",
                                    target])
    return json.loads(data)

  def get_outputs_of(self, target):
    """Get the list of outputs of a given targets"""
    data = subprocess.check_output([self._falcon_client,
                                    "--get-outputs-of",
                                    target])
    return json.loads(data)
