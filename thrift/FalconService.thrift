enum FalconStatus {
  BUILDING,
  IDLE
}

enum StartBuildResult {
  OK,         /* The build was properly started. */
  BUSY        /* A build is already ongoing. */
}

exception TargetNotFound {}

service FalconService {
  /* Get the pid of the Falcon daemon. */
  i64 getPid()

  /* Start a build. */
  StartBuildResult startBuild()

  /* Get the current status of the daemon. */
  FalconStatus getStatus()

  /* Interrupt the current build, if any. */
  void interruptBuild()

  /* Retrieve the list of source files that are dirty. */
  set<string> getDirtySources()

  /* Retrieve the list of targets that are dirty, including source files. */
  set<string> getDirtyTargets()

  /* Retrieve the list of targets that are inputs of the given target. */
  set<string> getInputsOf(1:string target) throws(1:TargetNotFound e)

  /* Retrieve the list of targets that are outputs of the given target. */
  set<string> getOutputsOf(1:string target) throws(1:TargetNotFound e)

  /* Get the hash of a target. */
  string getHashOf(1:string target) throws(1:TargetNotFound e)

  /* Mark the given target as dirty. */
  void setDirty(1:string target) throws(1:TargetNotFound e)

  /* Stop the daemon. Will interrupt the current build, if any. */
  void shutdown()

  /* Return a graphviz representation of the graph. */
  string getGraphviz()
}
