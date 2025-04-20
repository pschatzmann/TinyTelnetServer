#pragma once
#include "../TinyTelnetServerConfig.h"

/// Log levels for the TelnetServer
enum class TelnetLogLevel { Debug, Info, Warning, Error };

namespace telnet {

/**
 * @brief Logger for the TelnetServer
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class TelnetLoggerClass {
 public:
  TelnetLoggerClass() { resize(MAX_LOG_MSG_SIZE); }
  ~TelnetLoggerClass() { if (msg != nullptr) delete[] msg; }

  /// Define the logger: Set the logger to a specific log level
  bool begin(Print* print, TelnetLogLevel level) {
    this->print = print;
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
  void log(TelnetLogLevel level, const char* ctx, const char* fmt, ...) {
    if (level < logLevel) return;
    print->print(logLevelStr[(int)level]);
    print->print(": [");
    print->print(ctx);
    print->print("]: ");
    va_list args;
    va_start(args, fmt);
    snprintf(msg, msgLen, fmt, args);
    print->print(msg);
    va_end(args);
  }

 protected:
  char* msg = nullptr;
  int msgLen;
  TelnetLogLevel logLevel = TelnetLogLevel::Warning;
  Print* print = &Serial;
  const char* logLevelStr[4] = {"DEBUG", "INFO", "WARN", "ERROR"};
};

}  // namespace telnet

static telnet::TelnetLoggerClass TelnetLogger;

#if defined(ESP32) && USE_ESP32_LOGGER
static const char* TELNET_TAG = "TinyTelnetServer   ";
#define TELNET_LOGD(fmt, ...) LOGD(TELNET_TAG, fmt, __VA_ARGS__)
#define TELNET_LOGI(fmt, ...) LOGI(TELNET_TAG, fmt, __VA_ARGS__)
#define TELNET_LOGW(fmt, ...) LOGW(TELNET_TAG, fmt, __VA_ARGS__)
#define TELNET_LOGE(fmt, ...) LOGE(TELNET_TAG, fmt, __VA_ARGS__)
#else
#define TELNET_LOGD(fmt, ...) TelnetLogger.log(TelnetLogLevel::Debug, __func__, fmt, __VA_ARGS__)
#define TELNET_LOGI(fmt, ...) TelnetLogger.log(TelnetLogLevel::Info, __func__, fmt, __VA_ARGS__)
#define TELNET_LOGW(fmt, ...) TelnetLogger.log(TelnetLogLevel::Warning, __func__, fmt, __VA_ARGS__)
#define TELNET_LOGE(fmt, ...) TelnetLogger.log(TelnetLogLevel::Error, __func__, fmt, __VA_ARGS__)
#endif
