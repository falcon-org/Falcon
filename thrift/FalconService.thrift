namespace cpp Test

service FalconService {
  /* Get the pid of the Falcon daemon. */
  i64 getPid()

  /* Trigger a build. */
  void startBuild()

  /* Stop the daemon. Will interrupt the current build if any. */
  void shutdown();
}
