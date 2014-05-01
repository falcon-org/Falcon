/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_TEST_H_
# define FALCON_TEST_H_

# include <string>
# include <vector>
# include <ostream>

namespace falcon {

/*!
 * @class Test
 * @brief Interface
 * This is the interface every test must implement */
class Test {
public:
  Test(std::string comment, std::string error)
    : success_(false), commentMessage_(comment), errorMessage_(error) {}
  /* Prepare the test (can open socket, ...)
   * Will be called before runTest() */
  virtual void prepareTest() = 0;

  /* Run the test, if there are timdeout to check, ensure your test is inside
   * this function */
  virtual void runTest()     = 0;

  /* Close the test (close opened sockets, ...)
   * Will be called after runTest() */
  virtual void closeTest() = 0;

  /* return True if the test passed, else false */
  bool success() const { return success_; }
  /* return the description message (if any) */
  std::string const& getCommentMessage() const { return commentMessage_; }
  /* return the error message (if any) */
  std::string const& getErrorMessage() const { return errorMessage_; }

protected:
  void setSuccess(bool b) { success_ = b; }
  void setCommentMessage(std::string const c) { commentMessage_ = c; }
  void setErrorMessage(std::string const e) { errorMessage_ = e; }
private:
  bool success_;
  std::string commentMessage_;
  std::string errorMessage_;
};

/*!
 * @class TestSuite
 * @brief Collection of Tests
 * This class manages a collection of tests and knows how to run them and how to
 * print them */
class TestSuite {
public:
  TestSuite(std::string title);

  /* Add a test */
  void add(Test* t);

  /* run the test suite:
   *  -1- on every Test
   *    -a- call prepareTest
   *    -b- call runTest
   *    -c- call closeTest
   *    -d- if the test did not succeed, then save the Test for print */
  void run();

  /* return the number of tests which failed (0 on success) */
  unsigned fails() const;

  /* JsonPretty printer:
   * Print it in a format readable by Arcanist Unit module */
  void printJsonOutput(std::ostream& oss) const;

  /* Classic Pretty Printer */
  void printStandardOutput(std::ostream& oss) const;

private:
  std::string title_;
  std::vector<falcon::Test*> tests_;
  unsigned passed_;
  unsigned failed_;

  std::vector<falcon::Test*> errors_;
};

}

#endif /* !FALCON_TEST_H_ */
