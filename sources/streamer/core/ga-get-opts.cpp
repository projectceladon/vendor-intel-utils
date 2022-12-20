// Copyright (C) 2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

/**
  * @file   ga_get_opts.cpp
  *
  * @brief  Implementation of command line argument parser classes
  *
  */

#include "ga-get-opts.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

/**
 * @brief ga_get_opts class constructor
 *
 * @param argc Reference to a number of arguments given to the application
 * @param argv Reference to arguments given to the application
 *
 * @note @p argc and @p argv application variables will be updated by
 *       the parse() method.
 */
ga_get_opts::ga_get_opts(int& argc, const char**& argv)
           : argc_(argc), argv_(argv)
           , argc_ref_(argc), argv_ref_(argv)
{
  // get application process full pathname
  char app_pathname[1024]{};
  GetModuleFileName(NULL, app_pathname, sizeof(app_pathname));

  // obtain application path and name
  std::string pathname = std::string(app_pathname);
  size_t path_end = pathname.find_last_of("/\\");
  if (path_end != std::string::npos) {
    app_path_ = pathname.substr(0, path_end);
    app_name_ = pathname.substr(path_end + 1);
  }
  else {
    app_path_ = "";
    app_name_ = pathname;
  }
}

/**
 * @brief ga_get_opts class destructor
 */
ga_get_opts::~ga_get_opts()
{
  opt_list_.clear();
}

/**
 * @brief Returns application path
 *
 * Method returns application path.
 *
 * @return Application path
 */
const std::string& ga_get_opts::app_path() const
{
  return app_path_;
}

/**
 * @brief Returns application name
 *
 * Method returns application name.
 *
 * @return Application name
 */
const std::string& ga_get_opts::app_name() const
{
  return app_name_;
}

/**
 * @brief Returns command line string
 *
 * Method returns command line string.
 *
 * @return Command line string
 */
std::string ga_get_opts::command_line() const
{
  std::string cmd_line = app_name_;

  for (int i = 1; i < argc_; ++i) {
    cmd_line += " " + std::string(argv_[i]);
  }

  return cmd_line;
}

/**
 * @brief Adds new option to command line argument parser
 *
 * Method adds new option to command line argument parser.
 *
 * @note Ownership is not passed to ga_get_opts class.
 *
 * @param opt Option to add
 */
void ga_get_opts::add_option(option& opt)
{
  opt_list_.push_back(&opt);
}

/**
 * @brief Parse application command line
 *
 * Method parses application command line. It checks all the arguments
 * from command line until it finds first argument that is not an option
 * (does not start from '-' sign). If an option is found than it is
 * marked as valid. If an option contain an argument it is stored in
 * an option and it is checked for correctness.
 *
 * @note @p argc and @p argv application variables are set to point to
 *       the first argument that is not an option.
 */
void ga_get_opts::parse()
{
  int i = 1;
  // iterate through given cli arguments
  // index 0 is application name so skip it
  for (; i < argc_; ++i) {
    const char* ptr = argv_[i];

    // check if it is an option
    if (ptr[0] == '-') {
      // it is an option
      if (strlen(ptr) == 1) {
        std::cerr << "Option name not given! Please use minus width letter, e.g. -h, not just -" << std::endl;
        throw "ga_get_opts: Option name not given!!!";
      }

      // iterate through all known options and compare
      option_list::iterator it = opt_list_.begin();
      for (; it != opt_list_.end(); ++it) {
        if (ptr[1] == '-') {
          // it is a long form of an option name
          if (strlen(ptr) > 2 && (*it)->long_name() == &ptr[2]) {
            // option found
            break;
          }
        }
        else {
          // it is a short form of an option name
          if (strlen(ptr) != 2) {
            std::cerr << "More than one character given for a short form of an option: " << ptr << "!!!" << std::endl;
            throw "ga_get_opts: More than one character given for a short form of an option!!!";
          }

          if ((*it)->short_name() == ptr[1]) {
            // option found
            break;
          }
        }
      }

      if (it == opt_list_.end()) {
        // option not found
        std::cerr << "Option: " << ptr << " not supported!!!" << std::endl;
        throw"ga_get_opts: Option not supported";
      }

      // option found
      if ((*it)->valid()) {
        // option given more than once
        std::cerr << "Option: " << ptr << " can be specified only once!!!" << std::endl;
        throw "ga_get_opts: Option can be specified only once!!!";
      }

      option::ArgumentType arg_type = (*it)->argument_type();
      if (arg_type != option::ArgumentType::ARGUMENT_NO) {
        // check if argument is present
        bool found = false;

        if (i + 1 != argc_) {
          const char* next_ptr = argv_[i + 1];

          if (strlen(next_ptr)) {
            // argument present
            if (arg_type == option::ArgumentType::ARGUMENT_OPTIONAL)
              found = (next_ptr[0] != '-');
            else
              found = true; // arg_type == option::ArgumentType::ARGUMENT_MANDATORY
          }
        }

        if (found) {
          // set argument in an option
          (*it)->argument(argv_[i + 1]);

          // valid the argument
          if (!(*it)->argument_check()) {
            std::cerr << "Bad argument given: \"" << argv_[i + 1] << "\" for an option: '" << ptr << "'!!!" << std::endl;
            throw "ga_get_opts: Bad argument given for an option!!!";
          }

          // increment cli argument index
          ++i;
        }
        else {
          if (arg_type == option::ArgumentType::ARGUMENT_MANDATORY) {
            std::cerr << "Mandatory argument not specified for option: '" << ptr << "'!!!" << std::endl;
            throw "ga_get_opts: Mandatory argument not specified for option!!!";
          }
        }
      }
      // set option as valid
      (*it)->valid(true);
    }
    else {
      // option part is over
      break;
    }
  }

  // update application data
  argc_ref_ -= i;
  argv_ref_ = &argv_[i];
}

/**
 * @brief Prints usage screen
 *
 * Method prints usage screen.
 *
 * @param additional_params Parameters that are allowed by the
 *                          application and are not an option
 * @param desc_indent Indentation of option description in usage screen
 * @param args_width  Space needed to write option argument in usage screen
 */
void ga_get_opts::usage(const std::string& additional_params /*= ""*/,
                        unsigned int desc_indent /*= DESC_INDENT*/,
                        unsigned int args_width /*= ARGS_WIDTH*/)
{
  std::cout << "Usage:" << std::endl;
  usage_cli(additional_params);

  std::cout << std::endl;

  std::cout << "Options:" << std::endl;
  usage_options(desc_indent, args_width);
}

/**
 * @brief Prints one line usage info
 *
 * Method prints one line usage info with short form of options.
 *
 * @note If an option contains only long name version it is not
 *       printed here.
 *
 * @param additional_params Parameters that are allowed by the
 *                          application and are not an option
 */
void ga_get_opts::usage_cli(const std::string& additional_params) const
{
  std::cout << "  " << app_name_;

  for (option_list::const_iterator it = opt_list_.begin(); it != opt_list_.end(); ++it) {
    char short_name = (*it)->short_name();

    if (short_name == 0)
      // No short name for this option
      continue;

    // print option name
    std::cout << " -" << short_name;

    // print option argument
    switch ((*it)->argument_type()) {
    case option::ArgumentType::ARGUMENT_NO:
      break;

    case option::ArgumentType::ARGUMENT_OPTIONAL:
      std::cout << " [" << (*it)->argument_name() << "]";
      break;

    case option::ArgumentType::ARGUMENT_MANDATORY:
      std::cout << " " << (*it)->argument_name();
      break;

    default:
      std::cerr << "Unknown argument type: " << (int)(*it)->argument_type() << std::endl;
      throw "ga_get_opts: Unknown argument type!!!";
    }
  }

  // print additional elements in application cli
  if (additional_params != "")
    std::cout << " " << additional_params;

  std::cout << std::endl;
}

/**
 * @brief Prints options description
 *
 * Method prints descriptions for all supported options.
 *
 * @param desc_indent Indentation of option description in usage screen
 * @param args_width  Space needed to write option argument in usage screen
 */
void ga_get_opts::usage_options(unsigned int desc_indent, unsigned int args_width) const
{
  for (option_list::const_iterator it = opt_list_.begin(); it != opt_list_.end(); ++it) {
    std::cout << "  ";

    // print short name of an option
    if (char short_name = (*it)->short_name())
      std::cout << "-" << short_name << ", ";
    else
      std::cout << "    ";

    // print long name of an option
    const std::string long_name = (*it)->long_name();
    if (long_name != "")
      std::cout << "--" << std::setfill(' ') << std::setw(desc_indent - args_width - 8) << std::left << long_name;
    else
      std::cout << std::setfill(' ') << std::setw(desc_indent - args_width - 6) << " ";

    // print option argument
    std::string arg_str;
    switch ((*it)->argument_type()) {
    case option::ArgumentType::ARGUMENT_NO:
      break;

    case option::ArgumentType::ARGUMENT_OPTIONAL:
      arg_str = "[";
      arg_str += (*it)->argument_name();
      arg_str += "]";
      break;

    case option::ArgumentType::ARGUMENT_MANDATORY:
      arg_str = (*it)->argument_name();
      break;

    default:
      std::cerr << "Unknown argument type: " << (int)(*it)->argument_type() << std::endl;
      throw "ga_get_opts: Unknown argument type!!!";
    }
    std::cout << std::setfill(' ') << std::setw(args_width) << arg_str;

    // print description text
    const std::string& description = (*it)->description();
    size_t idx = 0;
    do {
      size_t new_idx = description.find_first_of("\n", idx);
      std::string descline = description.substr(idx, new_idx - idx);

      if (!idx)
        std::cout << descline << std::endl;
      else
        std::cout << std::setfill(' ') << std::setw(desc_indent) << " " << descline << std::endl;

      if (new_idx != std::string::npos)
        idx = new_idx + 1;
      else
        idx = new_idx;

    } while (idx != std::string::npos);
  }
}

/**
 * @brief ga_get_opts::option class constructor
 *
 * @param short_name  Option letter code to be used in short form
 * @param long_name   Option string name to be used in long form
 * @param arg_type    Specifies if argument is needed for that option
 * @param description String that describe option in 'usage' screen
 */
ga_get_opts::option::option(char short_name,
                            const std::string& long_name,
                            ArgumentType arg_type,
                            const std::string& description)
                    : short_name_(short_name)
                    , long_name_(long_name)
                    , argument_type_(arg_type)
                    , description_(description)
                    , valid_(false)
{
}

/**
 * @brief ga_get_opts::option class destructor
 */
ga_get_opts::option::~option()
{
}

/**
 * @brief Returns short name identifier of the option
 *
 * Method returns short name identifier of the option.
 *
 * @return Short name identifier of the option
 */
char ga_get_opts::option::short_name() const
{
  return short_name_;
}

/**
 * @brief Returns long name identifier of the option
 *
 * Method returns long name identifier of the option.
 *
 * @return Long name identifier of the option
 */
const std::string& ga_get_opts::option::long_name() const
{
  return long_name_;
}

/**
 * @brief Returns information if option argument is expected
 *
 * Method returns information if option argument is expected.
 *
 * @return Information if option argument is expected
 */
ga_get_opts::option::ArgumentType ga_get_opts::option::argument_type() const
{
  return argument_type_;
}

/**
 * @brief Returns option description string
 *
 * Method returns option description string.
 *
 * @return Option description string
 */
const std::string& ga_get_opts::option::description() const
{
  return description_;
}

/**
 * @brief Returns true if option was found in application command line
 *
 * Method returns true if option was found in application command line.
 *
 * @return true if option was found in application command line
 */
bool ga_get_opts::option::valid() const
{
  return valid_;
}

/**
 * @brief Returns option argument
 *
 * Method returns pointer to option argument or 0 if argument was not specified.
 *
 * @return Pointer to option argument or 0 if argument was not specified
 */
const std::string& ga_get_opts::option::argument() const
{
  return argument_;
}

/**
 * @brief Returns argument name that will be presented in 'usage' screen
 *
 * Method returns argument name that will be presented in 'usage' screen.
 *
 * @return Argument name that will be presented in 'usage' screen
 */
const char* ga_get_opts::option::argument_name() const
{
  return "<ARG>";
}

/**
 * @brief Marks an option as found in command line
 *
 * Method marks an option as found in application command line.
 *
 * @param flag @b true means that option was found
 */
void ga_get_opts::option::valid(bool flag)
{
  valid_ = flag;
}

/**
 * @brief Sets argument assigned to an option
 *
 * Method sets argument assigned to an option.
 *
 * @param value Argument string value
 */
void ga_get_opts::option::argument(const std::string& value)
{
  argument_ = value;
}

/**
 * @brief Verifies argument correctness
 *
 * Method verifies option argument correctness. Can be used by
 * child classes to validate and convert given argument.
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool ga_get_opts::option::argument_check()
{
  return true;
}

/**
 * @brief option_boolean class constructor
 *
 * @param short_name  Option letter code to be used in short form
 * @param long_name   Option string name to be used in long form
 * @param arg_type    Specifies if argument is needed for that option
 * @param description String that describe option in 'usage' screen
 */
option_boolean::option_boolean(char short_name,
                              const std::string& long_name,
                              ArgumentType arg_type,
                              const std::string& description)
              : ga_get_opts::option(short_name, long_name, arg_type, description)
{
}

/**
 * @brief option_boolean class destructor
 */
option_boolean::~option_boolean()
{
}

/**
 * @brief Returns boolean option
 *
 * Method returns boolean option.
 *
 * @return boolean
 */
bool option_boolean::boolean() const
{
  return boolean_;
}

/**
 * @brief Verifies boolean argument correctness
 *
 * Method verifies boolean option argument correctness.
 * Validate and convert given argument to boolean.
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_boolean::argument_check(const std::string& arg)
{
  // Lambda for compare strings in lower cases
  auto is_equal = [](const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(),
      b.begin(), b.end(),
      [](char ca, char cb) {
        return std::tolower(ca) == std::tolower(cb);
      });
  };

  if (is_equal(arg, "true")
    || is_equal(arg, "1")
    || is_equal(arg, "y")
    || is_equal(arg, "yes")
    || is_equal(arg, "enabled")
    || is_equal(arg, "enable")
    || is_equal(arg, "on")) {
    boolean_ = true;
  }
  else if (is_equal(arg, "false")
    || is_equal(arg, "0")
    || is_equal(arg, "n")
    || is_equal(arg, "no")
    || is_equal(arg, "disabled")
    || is_equal(arg, "disable")
    || is_equal(arg, "off")) {
    boolean_ = false;
  }
  else {
    std::cerr << "boolean parsing failed. Unknown boolean flag: " << arg << ". Use '-h' for a list of correct values." << std::endl;
    return false;
  }

  return true;
}

/**
 * @brief Verifies boolean argument correctness
 *
 * Method verifies boolean option argument correctness.
 * Validate and convert given argument to boolean.
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_boolean::argument_check()
{
  return argument_check(argument());
}

/**
 * @brief option_level class constructor
 *
 * @param short_name  Option letter code to be used in short form
 * @param long_name   Option string name to be used in long form
 * @param arg_type    Specifies if argument is needed for that option
 * @param description String that describe option in 'usage' screen
 */
option_loglevel::option_loglevel(char short_name,
                                const std::string& long_name,
                                ArgumentType arg_type,
                                const std::string& description)
                : ga_get_opts::option(short_name, long_name, arg_type, description)
{
}

/**
 * @brief option_loglevel class destructor
 */
option_loglevel::~option_loglevel()
{
}

/**
 * @brief Returns argument name that will be presented in 'usage' screen
 *
 * Method returns argument name that will be presented in 'usage' screen.
 *
 * @return Argument name that will be presented in 'usage' screen
 */
const char* option_loglevel::argument_name() const
{
  return "LEVEL";
}

/**
 * @brief Returns option log level
 *
 * Method returns severity log level.
 *
 * @return Severity log level
 */
Severity option_loglevel::log_level() const
{
  return loglevel_;
}

/**
 * @brief Verifies log level argument correctness
 *
 * Method verifies log level option argument correctness.
 * Validate and convert given argument to Severity log level.
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_loglevel::argument_check(const std::string& arg)
{
  if (arg == "error") {
    loglevel_ = Severity::ERR;
  }
  else if (arg == "warning") {
    loglevel_ = Severity::WARNING;
  }
  else if (arg == "info") {
    loglevel_ = Severity::INFO;
  }
  else if (arg == "debug") {
    loglevel_ = Severity::DBG;
  }
  else {
    std::cerr << "Log level parsing failed. Unknown level: " << arg << ". Use '-h' for a list of correct values." << std::endl;
    return false;
  }

  return true;
}

/**
 * @brief Verifies log level argument correctness
 *
 * Method verifies log level option argument correctness.
 * Validate and convert given argument to Severity log level.
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_loglevel::argument_check()
{
  return argument_check(argument());
}

/**
 * @brief option_luid class constructor
 *
 * @param short_name  Option letter code to be used in short form
 * @param long_name   Option string name to be used in long form
 * @param arg_type    Specifies if argument is needed for that option
 * @param description String that describe option in 'usage' screen
 */
option_luid::option_luid(char short_name,
                         const std::string& long_name,
                         ArgumentType arg_type,
                         const std::string& description)
            : ga_get_opts::option(short_name, long_name, arg_type, description)
{
}

/**
 * @brief option_luid class destructor
 */
option_luid::~option_luid()
{
}

/**
 * @brief Returns argument name that will be presented in 'usage' screen
 *
 * Method returns argument name that will be presented in 'usage' screen.
 *
 * @return Argument name that will be presented in 'usage' screen
 */
const char* option_luid::argument_name() const
{
  return "\"HI LO\"";
}

/**
 * @brief Returns option adapter LUID
 *
 * Method returns adapter LUID.
 *
 * @return Adapter LUID
 */
LUID option_luid::luid() const
{
  return luid_;
}

/**
 * @brief Verifies adapter LUID argument correctness
 *
 * Method verifies adapter LUID option argument correctness.
 * Validate and convert given argument to adapter LUID.
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_luid::argument_check(const std::string& arg)
{
  std::vector<unsigned long> luid;
  std::stringstream ss(arg);
  std::string f;
  while (ss >> f) {
    try {
      luid.push_back(std::stoul(f, nullptr, 0));
    }
    catch (std::exception &ex) {
      std::cerr << ex.what() << ": " << f << std::endl;
      return false;
    }
  }

  if (luid.size() < 2) {
    std::cerr << "LUID parsing failed. Incorrect LUID argument: \"" << arg << "\". Use '-h' for list correct format." << std::endl;
    return false;
  }

  luid_.HighPart = luid[0];
  luid_.LowPart = luid[1];

  return true;
}

/**
 * @brief Verifies adapter LUID argument correctness
 *
 * Method verifies adapter LUID option argument correctness.
 * Validate and convert given argument to adapter LUID.
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_luid::argument_check()
{
  return argument_check(argument());
}

/**
 * @brief option_resolution class constructor
 *
 * @param short_name  Option letter code to be used in short form
 * @param long_name   Option string name to be used in long form
 * @param arg_type    Specifies if argument is needed for that option
 * @param description String that describe option in 'usage' screen
 */
option_resolution::option_resolution(char short_name,
                                     const std::string& long_name,
                                     ArgumentType arg_type,
                                     const std::string& description)
                 : ga_get_opts::option(short_name, long_name, arg_type, description)
{
}

/**
 * @brief option_resolution class destructor
 */
option_resolution::~option_resolution()
{
}

/**
 * @brief Returns argument name that will be presented in 'usage' screen
 *
 * Method returns argument name that will be presented in 'usage' screen.
 *
 * @return Argument name that will be presented in 'usage' screen
 */
const char* option_resolution::argument_name() const
{
  return "WxH";
}

/**
 * @brief Returns option resolution width
 *
 * Method returns resolution width.
 *
 * @return Resolution width
 */
int option_resolution::width() const
{
  return width_;
}

/**
 * @brief Returns option resolution height
 *
 * Method returns resolution height.
 *
 * @return Resolution height
 */
int option_resolution::height() const
{
  return height_;
}

/**
 * @brief Verifies resolution argument correctness
 *
 * Method verifies resolution option argument correctness.
 * Validate and convert given argument to resolution (width,height).
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_resolution::argument_check(const std::string& arg)
{
  std::stringstream stream(arg);
  uint16_t w = 0;
  stream >> w;
  char c = 0;
  stream >> c; // ignored
  uint16_t h = 0;
  stream >> h;

  if (stream.fail() || std::tolower(c) != 'x') {
    std::cerr << "Resolution parsing failed. Expected format: [WIDTHxHEIGHT] actual data: " << arg << std::endl;
    return false;
  }
  width_ = w;
  height_ = h;

  return true;
}

/**
 * @brief Verifies resolution argument correctness
 *
 * Method verifies resolution option argument correctness.
 * Validate and convert given argument to resolution (width,height).
 *
 * @return Verification status
 * @retval true if option argument is correct
 * @retval false otherwise
 */
bool option_resolution::argument_check()
{
  return argument_check(argument());
}
