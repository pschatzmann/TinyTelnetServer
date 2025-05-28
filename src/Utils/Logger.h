#pragma once
#include "../TinyTelnetServerConfig.h"
#include "Arduino.h"
#include <stdarg.h>

/// Log levels for the TelnetServer
enum class TinyTelnetLogLevel { Debug, Info, Warning, Error };

namespace telnet {

/**
 * @brief Logger for the TelnetServer
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class Logger {
 public:
 Logger() { resize(MAX_LOG_MSG_SIZE); }
  ~Logger() { if (msg != nullptr) delete[] msg; }

  /// Define the logger: Set the logger to a specific log level
  bool begin(Print& print, TinyTelnetLogLevel level) {
    this->p_print = &print;
    this->logLevel = level;
    return true;
  }

  /// Set the max log message length
  void resize(int maxMsgSize) {
    msgLen = maxMsgSize;
    if (msg != nullptr) {
      delete[] msg;
    }
    msg = new char[msgLen];
    memset(msg, 0, msgLen);
  }

  /// print log message
  void log(TinyTelnetLogLevel level, const char* ctx, const char* fmt, ...) {
    if (level < logLevel) return;
    p_print->print(logLevelStr[(int)level]);
    p_print->print(" [");
    p_print->print(ctx);
    p_print->print("]: ");

    memset(msg, 0, msgLen);
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, msgLen, fmt, args);
    p_print->println(msg);
    va_end(args);
  }

 protected:
  char* msg = nullptr;
  int msgLen;
  TinyTelnetLogLevel logLevel = TinyTelnetLogLevel::Warning;
  Print* p_print = &Serial;
  const char* logLevelStr[4] = {"DEBUG", "INFO", "WARN", "ERROR"};
};

}  // namespace telnet

static telnet::Logger TinyTelnetLogger;

#if defined(ESP32) && USE_ESP32_LOGGER
static const char* TELNET_TAG = "TinyTelnetServer   ";
#define TELNET_LOGD(fmt, ...) LOGD(TELNET_TAG, fmt, __VA_ARGS__)
#define TELNET_LOGI(fmt, ...) LOGI(TELNET_TAG, fmt, __VA_ARGS__)
#define TELNET_LOGW(fmt, ...) LOGW(TELNET_TAG, fmt, __VA_ARGS__)
#define TELNET_LOGE(fmt, ...) LOGE(TELNET_TAG, fmt, __VA_ARGS__)
#else
#define TELNET_LOGD(fmt, ...) TinyTelnetLogger.log(TinyTelnetLogLevel::Debug, __PRETTY_FUNCTION__, fmt, __VA_ARGS__)
#define TELNET_LOGI(fmt, ...) TinyTelnetLogger.log(TinyTelnetLogLevel::Info, __PRETTY_FUNCTION__, fmt, __VA_ARGS__)
#define TELNET_LOGW(fmt, ...) TinyTelnetLogger.log(TinyTelnetLogLevel::Warning, __PRETTY_FUNCTION__, fmt, __VA_ARGS__)
#define TELNET_LOGE(fmt, ...) TinyTelnetLogger.log(TinyTelnetLogLevel::Error, __PRETTY_FUNCTION__, fmt, __VA_ARGS__)
#endif
