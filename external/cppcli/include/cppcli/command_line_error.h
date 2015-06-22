// Author(s): Jeroen van der Wulp, Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file command_line_error.h
/// \brief Exception class for use in command line library

#ifndef CLI_COMMAND_LINE_ERROR_H
#define CLI_COMMAND_LINE_ERROR_H

#include <stdexcept>
#include <string>

/**
 * \brief Exception class for errors raised by the command-line parser.
 **/
class command_line_error : public std::runtime_error
{
private:
  std::string m_name;
public:
  command_line_error(const std::string& name, const std::string& message) throw()
    : std::runtime_error(message), m_name(name)
  {}
  virtual const char* what() const throw()
  {
    return (m_name + ": " + std::runtime_error::what() + "\n"
            "Try '" + m_name + " --help' for more information.").c_str();
  }
  virtual ~command_line_error() throw()
  {};
};

#endif
