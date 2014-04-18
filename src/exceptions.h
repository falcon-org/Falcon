/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_JSON_EXCEPTIONS_H_
# define FALCON_JSON_EXCEPTIONS_H_

# include <cerrno>
# include <cstring>

# include <exception>
# include <string>
# include <sstream>

# define THROW_ERROR_CODE(code)                               \
  do {                                                        \
    throw Exception (__func__, __FILE__, __LINE__, code); \
  } while (0)

# define THROW_ERROR(code, message)                                    \
  do {                                                                 \
    throw Exception (__func__, __FILE__, __LINE__, code, message); \
  } while (0)

# if defined (DEBUG)
#  define THROW_FORWARD_ERROR(e)                         \
    do {                                                 \
      throw Exception (e.getErrorMessage (),         \
                           __func__, __FILE__, __LINE__, \
                           e.getCode ());                \
    } while (0)
# else
#  define THROW_FORWARD_ERROR(e)                         \
   THROW_ERROR (e.getCode (), e.getMessage ())
#endif

namespace falcon
{

/*! @class Exception
 *
 * It allows us to follow the call trace and report
 * easily and in the most understable way error reporting and handleling.
 *
 * See the constructors' comments for more information.
 *
 * @code
 * void func1 (void)
 * {
 *   trow Exception (__func__, __FILE__, __LINE__,
 *                   EINVAL, "bad args...");
 * }
 *
 * void func2 (void)
 * {
 *   try {
 *     func1 ();
 *   } catch (Exception& e) {
 *     throw Exception (e.getErrorMessage (),
 *                      __func__, __FILE__, __LINE__,
 *                      e.getCode ());
 *   }
 * }
 *
 * void main (void)
 * {
 *   try {
 *     func2 ();
 *   } catch (Exception& e) {
 *     std::clog << e.getErrorMessage () << std::endl;
 *   }
 * }
 * @endcode
 *
 * The output should be:
 *
 * @code
 *  $ [func1][main.cc:3][Invalid argument] bad args...
 *  $ [func2][main.cc:10][Invalid argument] Invalid argument
 * @endcode */
class Exception: public std::exception
{
  public:
    /*! @brief Constructor without exception forwarding.
     *
     * This constructor will build the Exception without any previous
     * message (i.e. without previous exception message).
     *
     * @param func function name (use the gcc macro \_\_func\_\_)
     * @param file file name (use \_\_FILE\_\_)
     * @param line line number (use \_\_LINE\_\_)
     * @param code is the error code (use errno code, see man 2 errno) */
    Exception (char const* func,
               char const* file, int const line,
               int const code)
      : _have_upper (false), _upper (""), _function (func)
      , _file (file), _line (line)
      , _code (code), _message (strerror (code))
    {
    } ;

    /*! @brief Constructor without exception forwarding.
     *
     * This constructor will build the Exception without any previous
     * message (i.e. without previous exception message).
     *
     * @param func function name (use the gcc macro \_\_func\_\_)
     * @param file file name (use \_\_FILE\_\_)
     * @param line line number (use \_\_LINE\_\_)
     * @param code is the error code (use errno code, see man 2 errno)
     * @param message a message string */
    Exception (char const* func,
               char const* file, int const line,
               int const code, char const* message)
      : _have_upper (false), _upper (""), _function (func)
      , _file (file), _line (line)
      , _code (code), _message (message)
    {
    };

    /*! @brief Constructor with exception forwarding.
     *
     * This constructor will build the Exception with a previous
     * message (i.e. with previous exception message).
     *
     * @param upper the result of the previous exception
     * @param func function name (use the gcc macro \_\_func\_\_)
     * @param file file name (use \_\_FILE\_\_)
     * @param line line number (use \_\_LINE\_\_)
     * @param code is the error code (use errno code, see man 2 errno) */
    Exception (std::string upper, char const* func,
               char const* file, int const line,
               int const code)
      : _have_upper (true), _upper (upper), _function (func)
      , _file (file), _line (line)
      , _code (code), _message ("")
    {
    } ;

    /*! @brief Constructor with exception forwarding.
     *
     * This constructor will build the Exception with a previous
     * message (i.e. with previous exception message).
     *
     * @param upper the result of the previous exception
     * @param func function name (use the gcc macro \_\_func\_\_)
     * @param file file name (use \_\_FILE\_\_)
     * @param line line number (use \_\_LINE\_\_)
     * @param code is the error code (use errno code, see man 2 errno)
     * @param message a message string */
    Exception (std::string upper, char const* func,
               char const* file, int const line,
               int const code, char const* message)
      : _have_upper (true), _upper (upper), _function (func)
      , _file (file), _line (line)
      , _code (code), _message (message)
    {
    };

    /*! @brief destructor */
    ~Exception (void) throw ()
    {
    };

    /*! @brief redifined the std::exception's method */
    char const* what (void) const throw ()
    {
      return this->getErrorMessage ().c_str ();
    };

    /*! @brief return the message string */
    std::string getErrorMessage (void) const throw ()
    {
      std::ostringstream oss;

      if (this->_have_upper)
      {
        oss << this->_upper << std::endl << "[FORWARD] ";
      }

#if defined (DEBUG)
      oss << "[" << this->_function << "]"
          << "[" << this->_file << ":" << this->_line << "]"
          << "[" << strerror (this->_code) << "] "
          << this->_message;
#else
      oss << "[" << strerror (this->_code) << "] "
          << this->_message;
#endif

      return oss.str ();
    };

    /*! @brief return the error code */
    int getCode (void) const throw ()
    {
      return this->_code;
    };

    /*! @brief return the message */
    char const* getMessage (void) const throw ()
    {
      return this->_message.c_str ();
    };
  private:
    bool _have_upper;
    std::string _upper;
    std::string _function;
    std::string _file;
    int _line;
    int _code;
    std::string _message;
};

}

#endif /* !FALCON_JSON_EXCEPTIONS_H_ */
