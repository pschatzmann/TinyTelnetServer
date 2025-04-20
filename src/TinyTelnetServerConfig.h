#pragma once

/// The maximum number of characters for a command
#ifndef MAX_INPUT_BUFFER_SIZE
#  define MAX_INPUT_BUFFER_SIZE 256
#endif

/// The delay in ms between two connection attempts
#ifndef NO_CONNECT_DELAY_MS
#  define NO_CONNECT_DELAY_MS 20
#endif

/// PSRAM support
#ifndef USE_ALLOCATOR
#  define USE_ALLOCATOR true
#endif

/// For ESP32 only: use ESP32 logger
#ifndef USE_ESP32_LOGGER
#  define USE_ESP32_LOGGER false
#endif

/// The maximum size of the log message for the standard logger
#ifndef MAX_LOG_MSG_SIZE
#  define MAX_LOG_MSG_SIZE 160
#endif