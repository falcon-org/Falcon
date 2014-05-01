/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "test.h"
#include <iostream>
#include <cassert>

namespace falcon {

TestSuite::TestSuite(std::string title)
  : title_(title), tests_(), passed_(0), failed_(0), errors_()
{}

void TestSuite::add(Test* t) {
  assert(t != nullptr);
  tests_.push_back(t);
}

void TestSuite::run() {
  /* check the run function has never been called before */
  assert((passed_ == 0) && (failed_ == 0));
  assert(errors_.empty());

  for (auto it = tests_.begin(); it != tests_.end(); it++) {
    (*it)->prepareTest();
    (*it)->runTest();
    (*it)->closeTest();
    if ((*it)->success()) {
      passed_++;
    } else {
      errors_.push_back(*it);
      failed_++;
    }
  }
}

unsigned TestSuite::fails() const { return failed_; }

/* In the case of JSon output we need to escape double-quote characters */
static std::string escape(std::string const& s) {
  std::size_t n = s.size();
  std::string escaped;

  for (std::size_t i = 0; i < n; ++i) {
    if (s[i] == '\"') {
      escaped += '\\';
    }
    escaped += s[i];
  }

  return escaped;
}

void TestSuite::printJsonOutput(std::ostream& oss) const {
  std::string status(failed_ ? "false" : "true");

  oss << "{" << std::endl
      << "  \"title\" : \"" << escape(title_) << "\"," << std::endl
      << "  \"success\": " << status << "," << std::endl
      << "  \"passed\" : " <<  passed_;
  if (failed_) {
    oss << "," << std::endl
        << "  \"failed\" : " <<  failed_ << "," << std::endl
        << "  \"infos\"  : [" << std::endl;

    unsigned counter = 0;
    for (auto it = errors_.cbegin(); it != errors_.cend(); it++) {
      oss << "    { \"input\" : \"" << escape((*it)->getCommentMessage())
          <<      "\"," << std::endl
          << "      \"error\" : \"" + escape((*it)->getErrorMessage()) + "\"}";
      if (++counter < errors_.size()) {
        oss << "," << std::endl;
      } else {
        oss << std::endl;
      }
    }
    oss << "  ]" << std::endl;
  } else {
    oss << std::endl;
  }
  oss << "}" << std::endl;
}

void TestSuite::printStandardOutput(std::ostream& oss) const {
  oss << " [" << title_ << "]" << std::endl;
  oss << "   Tests:     " << passed_ + failed_ << std::endl;
  oss << "   Succeeded: " << passed_ << std::endl;
  oss << "   Failed:    " << failed_ << std::endl;

  for (auto it = errors_.cbegin(); it != errors_.cend(); it++) {
    oss << "   [TEST]" << std::endl;
    oss << "     comment: " <<(*it)->getCommentMessage() << std::endl;
    oss << "     error:   " << (*it)->getErrorMessage() << std::endl;
  }
  oss << std::endl;
}
}
