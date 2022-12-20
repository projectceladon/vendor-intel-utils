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
 * @file   ga_param_shared.cpp
 *
 * @brief  Implementation of parameter memory shared classes
 *
 */

#include "ga-param-shared.h"

#define LOG_PREFIX "ga-param-shared: "

/**
* @brief ga_param_shared class constructor
*
* @param pid  process id. create new param shared memory named with process id.
* @param desired_access type of access to a file mapping object (FILE_MAP_READ/FILE_MAP_WRITE)
*/
ga_param_shared::ga_param_shared(uint64_t pid, uint32_t desired_access)
                : map_file_handle_(nullptr)
                , shared_param_(nullptr)
{
  std::string mem_named;

  mem_named = std::string("GA_Param_Shared_") + std::to_string(pid);
  if (!create_param_shared_mem(mem_named, desired_access)) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to create shared memory for parameters!\n");
    return;
  }
}

/**
* @brief ga_param_shared class destructor
*
*/
ga_param_shared::~ga_param_shared()
{
  if (shared_param_) {
    UnmapViewOfFile(shared_param_);
    shared_param_ = nullptr;
  }
  if (map_file_handle_) {
    CloseHandle(map_file_handle_);
    map_file_handle_ = nullptr;
  }
}

/**
* @brief Is params shared memory valid
*
* @return Params shared memory validation
*
*/
bool ga_param_shared::is_valid() const
{
  // Both ga_root_path and config_pathname need set to be valid.
  return !get_ga_root_path().empty() && !get_config_pathname().empty();
}

/**
* @brief Set params shared data to shared memory
*
* @param params  Params shared data
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::set_param_shared(const param_shared_s& params)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set param shared data!\n");
    return false;
  }

  strcpy_s(shared_param_->config_pathname, _countof(shared_param_->config_pathname), params.config_pathname);
  strcpy_s(shared_param_->ga_root_path, _countof(shared_param_->ga_root_path), params.ga_root_path);
  strcpy_s(shared_param_->game_dir, _countof(shared_param_->game_dir), params.game_dir);
  strcpy_s(shared_param_->game_exe, _countof(shared_param_->game_exe), params.game_exe);
  strcpy_s(shared_param_->game_argv, _countof(shared_param_->game_argv), params.game_argv);
  strcpy_s(shared_param_->hook_type, _countof(shared_param_->hook_type), params.hook_type);
  strcpy_s(shared_param_->codec_format, _countof(shared_param_->codec_format), params.codec_format);
  strcpy_s(shared_param_->server_peer_id, _countof(shared_param_->server_peer_id), params.server_peer_id);
  strcpy_s(shared_param_->client_peer_id, _countof(shared_param_->client_peer_id), params.client_peer_id);
  strcpy_s(shared_param_->logfile, _countof(shared_param_->logfile), params.logfile);
  strcpy_s(shared_param_->video_bitrate, _countof(shared_param_->video_bitrate), params.video_bitrate);
  shared_param_->loglevel = params.loglevel;
  shared_param_->luid = params.luid;
  shared_param_->enable_tcae = params.enable_tcae;
  shared_param_->enable_present = params.enable_present;
  shared_param_->width = params.width;
  shared_param_->height = params.height;
  shared_param_->encode_width = params.encode_width;
  shared_param_->encode_height = params.encode_height;

  return true;
}

/**
* @brief Set config pathname to shared memory
*
* @param config_pathname  Config file pathname
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::set_config_pathname(const std::string config_pathname)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set config pathname!\n");
    return false;
  }

  strcpy_s(shared_param_->config_pathname, _countof(shared_param_->config_pathname), config_pathname.c_str());

  return true;
}

/**
* @brief Get config pathname from shared memory
*
* @return config file pathname
*
*/
std::string ga_param_shared::get_config_pathname() const
{
  return shared_param_ ? std::string(shared_param_->config_pathname) : std::string();
}

/**
* @brief Set ga root path to shared memory
*
* @param ga_root_path  ga root path
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::set_ga_root_path(const std::string root_path)
{
  if (shared_param_ == nullptr) {
      ga_logger(Severity::ERR, LOG_PREFIX "Failed to set ga root path!\n");
      return false;
  }

  strcpy_s(shared_param_->ga_root_path, _countof(shared_param_->ga_root_path), root_path.c_str());

  return true;
}

/**
* @brief Get ga root path from shared memory
*
* @return ga root path
*
*/
std::string ga_param_shared::get_ga_root_path() const
{
  return shared_param_ ? std::string(shared_param_->ga_root_path) : std::string();
}

/**
* @brief Set game directory to shared memory
*
* @param game_dir  game directory
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::set_game_dir(const std::string game_dir)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set game directory!\n");
    return false;
  }

  strcpy_s(shared_param_->game_dir, _countof(shared_param_->game_dir), game_dir.c_str());

  return true;
}

/**
* @brief Get game directory from shared memory
*
* @return game directory
*
*/
std::string ga_param_shared::get_game_dir() const
{
  return shared_param_ ? std::string(shared_param_->game_dir) : std::string();
}

/**
* @brief Set game executable to shared memory
*
* @param game_exe  game executable
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::set_game_exe(const std::string game_exe)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set game executable!\n");
    return false;
  }

  strcpy_s(shared_param_->game_exe, _countof(shared_param_->game_exe), game_exe.c_str());

  return true;
}

/**
* @brief Get game executable from shared memory
*
* @return game executable
*
*/
std::string ga_param_shared::get_game_exe() const
{
  return shared_param_ ? std::string(shared_param_->game_exe) : std::string();
}

/**
* @brief Set game arguments to shared memory
*
* @param game_argv  game arguments
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::set_game_argv(const std::string game_argv)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set game arguments!\n");
    return false;
  }

  strcpy_s(shared_param_->game_argv, _countof(shared_param_->game_argv), game_argv.c_str());

  return true;
}

/**
* @brief Get game arguments from shared memory
*
* @return game arguments
*
*/
std::string ga_param_shared::get_game_argv() const
{
    return shared_param_ ? std::string(shared_param_->game_argv) : std::string();
}

/**
 * @brief Set hook type to shared memory
 *
 * @param hook_type  hook type
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_hook_type(const std::string hook_type)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set hook type!\n");
    return false;
  }

  strcpy_s(shared_param_->hook_type, _countof(shared_param_->hook_type), hook_type.c_str());

  return true;
}

/**
 * @brief Get hook type from shared memory
 *
 * @return hook type
 *
 */
std::string ga_param_shared::get_hook_type() const
{
  return shared_param_ ? std::string(shared_param_->hook_type) : std::string();
}

/**
 * @brief Set codec format to shared memory
 *
 * @param codec_format  codec format
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_codec_format(const std::string codec_format)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set codec format!\n");
    return false;
  }

  strcpy_s(shared_param_->codec_format, _countof(shared_param_->codec_format), codec_format.c_str());

  return true;
}

/**
 * @brief Get codec format from shared memory
 *
 * @return codec format
 *
 */
std::string ga_param_shared::get_codec_format() const
{
  return shared_param_ ? std::string(shared_param_->codec_format) : std::string();
}

/**
 * @brief Set server peer id to shared memory
 *
 * @param server_id  server peer id
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_server_peer_id(const std::string server_id)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set server peer id!\n");
    return false;
  }

  strcpy_s(shared_param_->server_peer_id, _countof(shared_param_->server_peer_id), server_id.c_str());

  return true;
}

/**
 * @brief Get server peer id from shared memory
 *
 * @return server peer id
 *
 */
std::string ga_param_shared::get_server_peer_id() const
{
  return shared_param_ ? std::string(shared_param_->server_peer_id) : std::string();
}

/**
 * @brief Set client peer id to shared memory
 *
 * @param client_id  client peer id
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_client_peer_id(const std::string client_id)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set client peer id!\n");
    return false;
  }

  strcpy_s(shared_param_->client_peer_id, _countof(shared_param_->client_peer_id), client_id.c_str());

  return true;
}

/**
 * @brief Get client peer id from shared memory
 *
 * @return client peer id
 *
 */
std::string ga_param_shared::get_client_peer_id() const
{
  return shared_param_ ? std::string(shared_param_->client_peer_id) : std::string();
}

/**
* @brief Set logfile name to shared memory
*
* @param logfile  logfile name
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::set_logfile(const std::string logfile)
{
    if (shared_param_ == nullptr) {
        ga_logger(Severity::ERR, LOG_PREFIX "Failed to set logfile!\n");
        return false;
    }

    strcpy_s(shared_param_->logfile, _countof(shared_param_->logfile), logfile.c_str());

    return true;
}

/**
* @brief Get logfile name from shared memory
*
* @return logfile name
*
*/
std::string ga_param_shared::get_logfile() const
{
    return shared_param_ ? std::string(shared_param_->logfile) : std::string();
}

/**
 * @brief Set GA log level to shared memory
 *
 * @param level  GA log level
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_loglevel(const Severity level)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set GA log level!\n");
    return false;
  }

  shared_param_->loglevel = level;

  return true;
}

/**
 * @brief Get GA log level from shared memory
 *
 * @return GA log level
 *
 */
Severity ga_param_shared::get_loglevel() const
{
  return shared_param_ ? shared_param_->loglevel : Severity::INFO;
}

/**
 * @brief Set adapter LUID to shared memory
 *
 * @param luid  adapter LUID
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_luid(const LUID luid)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set LUID!\n");
    return false;
  }

  shared_param_->luid = luid;

  return true;
}

/**
 * @brief Get adapter LUID from shared memory
 *
 * @return Adapter LUID
 *
 */
LUID ga_param_shared::get_luid() const
{
  return shared_param_ ? shared_param_->luid : LUID();
}

/**
 * @brief Set TCAE enable flag to shared memory
 *
 * @param enable  Enable or disable TCAE
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_tcae(const bool enable)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set TCAE enable flag!\n");
    return false;
  }

  shared_param_->enable_tcae = enable;

  return true;
}

/**
 * @brief Get TCAE enable flag from shared memory
 *
 * @return TCAE enable flag
 *
 */
bool ga_param_shared::get_tcae() const
{
    return shared_param_ ? shared_param_->enable_tcae : false;
}

/**
 * @brief Set present enable flag to shared memory
 *
 * @param enable  Enable or disable present
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_present(const bool enable)
{
    if (shared_param_ == nullptr) {
        ga_logger(Severity::ERR, LOG_PREFIX "Failed to set present enable flag!\n");
        return false;
    }

    shared_param_->enable_present = enable;

    return true;
}

/**
 * @brief Get present enable flag from shared memory
 *
 * @return present enable flag
 *
 */
bool ga_param_shared::get_present() const
{
  return shared_param_ ? shared_param_->enable_present : false;
}

/**
 * @brief Set resolution width to shared memory
 *
 * @param width  Resolution width
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_width(const int width)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set resolution width!\n");
    return false;
  }

  shared_param_->width = width;

  return true;
}

/**
 * @brief Get resolution width from shared memory
 *
 * @return Resolution width
 *
 */
int ga_param_shared::get_width() const
{
  return shared_param_ ? shared_param_->width : 0;
}

/**
 * @brief Set resolution height to shared memory
 *
 * @param width  Resolution height
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_height(const int height)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set resolution height!\n");
    return false;
  }

  shared_param_->height = height;

  return true;
}

/**
 * @brief Get resolution height from shared memory
 *
 * @return Resolution height
 *
 */
int ga_param_shared::get_height() const
{
  return shared_param_ ? shared_param_->height : 0;
}

/**
 * @brief Set video bitrate to shared memory
 *
 * @param video_bitrate Video bitrate
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_video_bitrate(const std::string video_bitrate)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set video bitrate!\n");
    return false;
  }

  strcpy_s(shared_param_->video_bitrate, _countof(shared_param_->video_bitrate), video_bitrate.c_str());

  return true;
}

/**
 * @brief Get video bitrate from shared memory
 *
 * @return Video bitrate
 *
 */
std::string ga_param_shared::get_video_bitrate() const
{
  return shared_param_ ? std::string(shared_param_->video_bitrate) : std::string();
}

/**
* @brief Create param named shared memory
*
* @param named          named of the shared memory
* @param desired_access type of access to a file mapping object (FILE_MAP_READ|FILE_MAP_WRITE)
*
* @return success return true, otherwise false
*
*/
bool ga_param_shared::create_param_shared_mem(const std::string named, uint32_t desired_access)
{
  uint32_t mem_size = sizeof(param_shared_s);  // size of mem to hold param_shared_s
  map_file_handle_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, mem_size, named.c_str());
  if (map_file_handle_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Could not create file mapping object (%d)\n", GetLastError());
    return false;
  }

  shared_param_ = (param_shared_s*)MapViewOfFile(map_file_handle_, desired_access, 0, 0, mem_size);
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Could not map view file (%d)\n", GetLastError());
    CloseHandle(map_file_handle_);
    return false;
  }

  return true;
}

/**
 * @brief Set encode width to shared memory
 *
 * @param encode_width  Encode width
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_encode_width(const int encode_width)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set encode width!\n");
    return false;
  }

  shared_param_->encode_width = encode_width;

  return true;
}

/**
 * @brief Get encode width from shared memory
 *
 * @return Encode width
 *
 */
int ga_param_shared::get_encode_width() const
{
  return shared_param_ ? shared_param_->encode_width : 0;
}

/**
 * @brief Set encode height to shared memory
 *
 * @param encode_height  Encode height
 *
 * @return success return true, otherwise false
 *
 */
bool ga_param_shared::set_encode_height(const int encode_height)
{
  if (shared_param_ == nullptr) {
    ga_logger(Severity::ERR, LOG_PREFIX "Failed to set encode height!\n");
    return false;
  }

  shared_param_->encode_height = encode_height;

  return true;
}

/**
 * @brief Get encode height from shared memory
 *
 * @return Encode height
 *
 */
int ga_param_shared::get_encode_height() const
{
  return shared_param_ ? shared_param_->encode_height : 0;
}
