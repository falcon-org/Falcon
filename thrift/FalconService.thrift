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

  /* Mark the given target as dirty. */
  void setDirty(1:string target) throws(1:TargetNotFound e)

  /* Stop the daemon. Will interrupt the current build, if any. */
  void shutdown()
}
