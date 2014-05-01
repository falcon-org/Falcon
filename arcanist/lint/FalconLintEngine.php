<?php

/* The falcon lint engne:
 *
 * Will load linter for Text, CPP and python */

final class FalconLintEngine extends ArcanistLintEngine {
  public function buildLinters() {
    $paths = $this->getPaths();

    $text_linter = new ArcanistTextLinter();
    $cpp_linter = new ArcanistCppcheckLinter();
    $py_linter = new ArcanistCppcheckLinter();
    $cpp_license_linter = new FalconCPPLicenseLinter();

    foreach ($paths as $key => $path) {
      if (!$this->pathExists($path)) {
        unset($paths[$key]);
      }
    }

    foreach ($paths as $path) {
      if (preg_match('@^externals/@', $path)) {
        continue;
      }

      if (preg_match('/\.(cpp|c)$/', $path)) {
        $cpp_linter->addPath($path);
        $cpp_license_linter->addPath($path);
        continue;
      }
      if (preg_match('/\.py$/', $path)) {
        $py_linter->addPath($path);
      }
      $text_linter->addPath($path);
    }

    return array(
      $cpp_linter,
      $py_linter,
      $text_linter,
      $cpp_license_linter,
    );
  }
}
