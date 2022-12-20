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
 * @file   ga-get-opts.h
 *
 * @brief  Declaration of command line argument parser classes
 *
 */

#ifndef __GA_GET_OPTS_H__
#define __GA_GET_OPTS_H__

#include "ga-common.h"  // For enum Severity
#include <deque>

/**
 * @brief Command line arguments parser
 *
 * ga_get_opts class is responsible for parsing options specified by
 * the user in command line while running the application. The
 * class contains a list of supported options. Options are determined
 * by its short or long identifier. Each option can have optional or
 * mandatory argument. ga_get_opts class is also responsible for dumping
 * 'usage' screen of the application.
 */
class  ga_get_opts
{
public:
  /**
   * @brief Command line option data
   *
   * ga_get_opts::option class is responsible for actions connected to
   * one option supported by an application. Option is determined
   * by its short or long identifier. It can have optional or
   * mandatory argument.
   */
  class option
  {
  public:
    friend class ga_get_opts;

    /**
     * @brief Option argument types
     */
    enum class ArgumentType {
      ARGUMENT_NO,          /**< @brief No argument for this option */
      ARGUMENT_OPTIONAL,    /**< @brief Argument is optional */
      ARGUMENT_MANDATORY    /**< @brief Argument is mandatory */
    };

    EXPORT option(char short_name,
                  const std::string& long_name,
                  ArgumentType arg_type,
                  const std::string& description);

    virtual EXPORT ~option();

    option() = delete;
    void* operator new(std::size_t) = delete;

    EXPORT char short_name() const;
    EXPORT const std::string& long_name() const;
    EXPORT ArgumentType argument_type() const;
    EXPORT const std::string& description() const;
    EXPORT bool valid() const;
    EXPORT const std::string& argument() const;

    virtual EXPORT const char* argument_name() const;

  private:
    const char short_name_;             /**< @brief Option letter code to be used in short form */
    const std::string long_name_;       /**< @brief Option string name to be used in long form */
    const ArgumentType argument_type_;  /**< @brief Specifies if argument is needed for that option */
    const std::string description_;     /**< @brief String that describe option in 'usage' screen */
    std::string argument_;              /**< @brief If argument is present it will be stored here */
    bool valid_ = false;                /**< @brief Flag that specifies if option was found cli */

    void valid(bool flag);
    void argument(const std::string& value);

    virtual bool argument_check();
  };

public:
  EXPORT ga_get_opts(int& argc, const char**& argv);
  EXPORT ~ga_get_opts();

  ga_get_opts() = delete;
  void* operator new(std::size_t) = delete;

  EXPORT const std::string& app_path() const;
  EXPORT const std::string& app_name() const;
  EXPORT std::string command_line() const;
  EXPORT void add_option(option& opt);
  EXPORT void parse();
  EXPORT void usage(const std::string& additional_params = "",
                    unsigned int desc_indent = DESC_INDENT,
                    unsigned int args_width = ARGS_WIDTH);

private:
  typedef std::deque<option*> option_list;

  static const unsigned int DESC_INDENT = 35; /**< @brief Indentation of option description in usage screen */
  static const unsigned int ARGS_WIDTH = 8;   /**< @brief Space needed to write option argument in usage screen */

  std::string app_path_;    /**< @brief Application path string */
  std::string app_name_;    /**< @brief Application name string */
  const int argc_;          /**< @brief Local copy of original arguments number */
  const char** const argv_; /**< @brief Local copy of original start of arguments table */
  int& argc_ref_;           /**< @brief Reference of application arguments number */
  const char** & argv_ref_; /**< @brief Reference to application start of arguments table */
  option_list opt_list_;    /**< @brief The list of options allowed for that application */

  void usage_cli(const std::string& additional_params) const;
  void usage_options(unsigned int desc_indent, unsigned int args_width) const;
};

/**
 * @brief Command line option boolean
 *
 * option_boolean class is responsible for actions connected to
 * boolean option supported by an application.
 *
 */
class  option_boolean : public ga_get_opts::option
{
public:
  EXPORT option_boolean(char short_name,
                const std::string& long_name,
                ArgumentType arg_type,
                const std::string& description);

  EXPORT ~option_boolean();

  option_boolean() = delete;
  void* operator new(std::size_t) = delete;

  EXPORT bool boolean() const;

private:
  bool boolean_ = false;  /**< @brief boolean option flag */

  virtual bool argument_check(const std::string& arg);
  virtual bool argument_check();
};

/**
 * @brief Command line option severity log level
 *
 * option_loglevel class is responsible for actions connected to
 * log level supported by an application.
 *
 */
class  option_loglevel : public ga_get_opts::option
{
public:
  EXPORT option_loglevel(char short_name,
                        const std::string& long_name,
                        ArgumentType arg_type,
                        const std::string& description);

  EXPORT ~option_loglevel();

  option_loglevel() = delete;
  void* operator new(std::size_t) = delete;

  virtual EXPORT const char* argument_name() const;
  EXPORT Severity log_level() const;

private:
  Severity loglevel_ = Severity::INFO;  /**< @brief GA log severity level (error, warning, info, debug) */

  virtual bool argument_check(const std::string& arg);
  virtual bool argument_check();
};

/**
 * @brief Command line option adapter LUID
 *
 * option_luid class is responsible for actions connected to
 * adapter LUID supported by an application.
 *
 */
class  option_luid : public ga_get_opts::option
{
public:
  EXPORT option_luid(char short_name,
                    const std::string& long_name,
                    ArgumentType arg_type,
                    const std::string& description);

  EXPORT ~option_luid();

  option_luid() = delete;
  void* operator new(std::size_t) = delete;

  virtual EXPORT const char* argument_name() const;
  EXPORT LUID luid() const;

private:
  LUID luid_ = {};  /**< @brief Adapter LUID */

  virtual bool argument_check(const std::string& arg);
  virtual bool argument_check();
};

/**
 * @brief Command line option resolution (width x height)
 *
 * option_resolution class is responsible for actions connected to
 * resolution supported by an application.
 *
 */
class  option_resolution : public ga_get_opts::option
{
public:
  EXPORT option_resolution(char short_name,
                          const std::string& long_name,
                          ArgumentType arg_type,
                          const std::string& description);

  EXPORT ~option_resolution();

  option_resolution() = delete;
  void* operator new(std::size_t) = delete;

  virtual EXPORT const char* argument_name() const;
  EXPORT int width() const;
  EXPORT int height() const;

private:
  int width_ = 0;  /**< @brief Resolution width */
  int height_ = 0; /**< @brief Resolution height */

  virtual bool argument_check(const std::string& arg);
  virtual bool argument_check();
};

#endif  // __GA_GET_OPTS_H__
