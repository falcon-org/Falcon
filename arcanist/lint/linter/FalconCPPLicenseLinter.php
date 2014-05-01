<?php

final class FalconCPPLicenseLinter extends ArcanistLinter {

  const LINT_NO_LICENSE = 1;

  private function getLicense() {
   return "/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */";
  }

  public function getLinterPriority() {
    return 0.8;
  }

  public function getLintNameMap() {
    return array (
      self::LINT_NO_LICENSE => pht('No license'),
    );
  }

  public function getLinterName() {
    return 'FalconLicenseLinter';
  }

  public function lintPath($path) {
    if (!strlen($this->getData($path))) {
      return; // if the file is empty, then nothing to do
    }

    $fileHeader = substr($this->getData($path), 0, strlen($this->getLicense()));
    if ($fileHeader != $this->getLicense()) {
      $this->raiseLintAtPath(self::LINT_NO_LICENSE,
                             "license does not correspond");
    }
  }
}
