// Author(s): Jeroen van der Wulp
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file command_line_interface_test.cpp

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include "cppcli/command_line_interface.h"
#include "cppcli/tool.h"
#include "cppcli/input_tool.h"
#include "cppcli/input_input_tool.h"
#include "cppcli/input_output_tool.h"
#include "cppcli/input_input_output_tool.h"

using namespace tools;

template < typename TypeName, bool b >
void string_to_type_test(std::string const& value)
{
  std::istringstream is(value);
  TypeName           s;

  EXPECT_TRUE((is >> s).fail() != b);
}

TEST(cppcli, border_invalid)
{
  interface_description test_interface("test", "TEST", "Kilroy", "[OPTIONS]... [PATH]", "whatis", "description");

  // Empty command line
  EXPECT_NO_THROW(command_line_parser(test_interface, ""));
  char    c = '\0';
  char*   pc = &c;
  EXPECT_NO_THROW(command_line_parser(test_interface, 0, &pc));
  wchar_t  w = L'\0';
  wchar_t* pw = &w;
  EXPECT_NO_THROW(command_line_parser(test_interface, 0, &pw));
}

TEST(cppcli, parsing)
{
  interface_description test_interface("test", "TEST", "Kilroy", "[OPTIONS]... [PATH]", "whatis", "description");

  // Valid option -h
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -v"));
  // Repeated options --help options
  EXPECT_THROW(command_line_parser(test_interface, "test --verbose -v -v"), std::runtime_error);
  // Invalid combination of short options
  EXPECT_THROW(command_line_parser(test_interface, "test -ve"), std::runtime_error);

  // Duplicate long option without argument
  EXPECT_THROW(test_interface.add_option("verbose","An option"), std::logic_error);
  // Duplicate long option with short option and without argument
  EXPECT_THROW(test_interface.add_option("verbose", "An option", 'h'), std::logic_error);
  // Duplicate long option with short option and with optional argument
  EXPECT_THROW(test_interface.add_option("verbose",make_mandatory_argument("STR"), "An option", 'v'), std::logic_error);
  // Duplicate long option with short option and with optional argument
  EXPECT_THROW(test_interface.add_option("verbose",make_optional_argument("STR", "XxXxX"), "An option", 'v'), std::logic_error);

  test_interface.add_option("mandatory", make_mandatory_argument("STR"), "option with mandatory argument", 'm');
  // Missing mandatory argument for option --mandatory
  EXPECT_THROW(command_line_parser(test_interface, "test --mandatory"), std::runtime_error);
  // Valid option with valid argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test --mandatory=test"));
  // Valid option with valid argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -m=test"));
  // Valid option with valid argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -m test"));
  // Valid short option v followed by option m with valid argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -vm=test"));

  test_interface.add_option("optional", make_optional_argument("STR", "*XxXxX*"), "option with optional argument", 'o');
  // Missing mandatory argument for option --mandatory
  EXPECT_NO_THROW(command_line_parser(test_interface, "test --optional"));
  // Valid option with valid argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test --optional=test"));
  // Valid option with valid argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -otest"));
  // Valid option without argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -o test"));
  // Valid short option v followed by option m with valid argument
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -vmtest"));
}

TEST(cppcli, conformance)
{
  interface_description test_interface("test", "TEST", "Kilroy", "[OPTIONS]... [PATH]", "whatis", "description");

  // Valid options -v, --verbose
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -v"));
  EXPECT_NO_THROW(command_line_parser(test_interface, "test --verbose"));
  // Valid options -q, --quiet
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -q"));
  EXPECT_NO_THROW(command_line_parser(test_interface, "test --quiet"));
  // Valid options -d, --debug
  EXPECT_NO_THROW(command_line_parser(test_interface, "test -d"));
  EXPECT_NO_THROW(command_line_parser(test_interface, "test --debug"));

  // Check conversion with wide characters
  wchar_t const* arguments[] = { L"test", L"--debug", L"--verbose=2" } ;

  EXPECT_NO_THROW(command_line_parser(test_interface, 2, arguments));
  EXPECT_THROW(command_line_parser(test_interface, 3, arguments), std::runtime_error);
}

inline std::string const& first_of(command_line_parser const& p, std::string const& option)
{
  return p.options.equal_range(option).first->second;
}

inline std::string const& last_of(command_line_parser const& p, std::string const& option)
{
  command_line_parser::option_map::const_iterator i(p.options.equal_range(option).second);

  return (--i)->second;
}

TEST(cppcli, result_browsing)
{
  interface_description test_interface("test", "TEST", "Kilroy", "[OPTIONS]... [PATH]", "whatis", "description");

  // disable check for duplicate options
  test_interface.add_option("cli-testing-no-duplicate-option-checking", "");

  {
    command_line_parser parser(test_interface, "test -v --debug -d --verbose");

    EXPECT_TRUE(parser.options.size() == 4);
    EXPECT_TRUE(parser.options.count("verbose") == 2);
    EXPECT_TRUE(first_of(parser, "verbose").empty());
    EXPECT_TRUE(last_of(parser, "verbose").empty());
    EXPECT_THROW(parser.option_argument("verbose"), std::logic_error);
    EXPECT_TRUE(parser.options.count("debug") == 2);
    EXPECT_TRUE(first_of(parser, "debug").empty());
    EXPECT_TRUE(last_of(parser, "debug").empty());
    EXPECT_TRUE(parser.options.count("quiet") == 0);
    EXPECT_TRUE(parser.arguments.size() == 0);
  }

  {
    command_line_parser parser(test_interface, "test /bin/ls -v \\or\\more:1234567890|,<>.:;[]}{+-_=~!@#$%^&*()");

    EXPECT_TRUE(parser.options.size() == 1);
    EXPECT_TRUE(first_of(parser, "verbose").empty());
    EXPECT_TRUE(parser.arguments.size() == 2);
    EXPECT_TRUE(parser.arguments[0] == "/bin/ls");
    EXPECT_TRUE(parser.arguments[1] == "\\or\\more:1234567890|,<>.:;[]}{+-_=~!@#$%^&*()");
  }

  test_interface.add_option("mandatory", make_mandatory_argument("STR"), "option with mandatory argument", 'm');
  test_interface.add_option("optional", make_optional_argument("STR", "4321"), "option with optional argument", 'o');

  {
    command_line_parser parser(test_interface, "test --mandatory=BLA --optional=1234 -vo -vmALB");

    EXPECT_TRUE(parser.options.size() == 6);
    EXPECT_TRUE(first_of(parser, "mandatory") != last_of(parser, "mandatory"));
    EXPECT_TRUE((first_of(parser, "mandatory") == "BLA" && last_of(parser, "mandatory") == "ALB") ||
                (first_of(parser, "mandatory") == "ALB" && last_of(parser, "mandatory") == "BLA"));
    EXPECT_TRUE((first_of(parser, "optional") == "4321" && last_of(parser, "optional") == "1234") ||
                (first_of(parser, "optional") == "1234" && last_of(parser, "optional") == "4321"));
    EXPECT_TRUE(parser.option_argument_as< int >("optional") == 1234 ||
                parser.option_argument_as< int >("optional") == 4321);
    EXPECT_TRUE(parser.arguments.size() == 0);
  }
  {
    command_line_parser parser(test_interface, "test -m BLA -o 1234");

    EXPECT_TRUE(first_of(parser, "mandatory") == "BLA");
    EXPECT_TRUE(parser.option_argument_as< int >("optional") == 4321);
    EXPECT_TRUE(parser.arguments.size() == 1);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
