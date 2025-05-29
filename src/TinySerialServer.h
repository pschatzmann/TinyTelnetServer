#pragma once
#include "Utils/Logger.h"
#include "Utils/Str.h"
#include "Utils/Vector.h"

namespace telnet {

/**
 * @brief A simple serial server for Arduino. Call the addCommand method to
 * register your commands.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */

class TinySerialServer {
 public:
  /// Enpty constructor
  TinySerialServer() = default;
  /// Default constructor that expects a Stream reference
  TinySerialServer(Stream& stream) { setStream(stream); }

  /// Defines the stream to be used
  void setStream(Stream& stream) { p_stream = &stream; }

  /// Start the server
  virtual bool begin() {
    is_active = true;
    return is_active;
  }

  /// Stop the server
  virtual void end() { is_active = false; }

  /// Add a new command
  virtual void addCommand(const char* cmd,
                          bool (*cb)(telnet::Str& cmd,
                                     telnet::Vector<telnet::Str> parameters,
                                     Print& out, TinySerialServer* self),
                          const char* parameter_help = "") {
    Command command;
    command.cmd = cmd;
    command.callback = cb;
    command.parameter_help = parameter_help;
    commands.push_back(command);
  }

  /// proccess the next command: call in loop()
  virtual bool processCommand() {
    if (!is_active) return false;
    Stream& stream = *p_stream;
    TELNET_LOGI("available: %d bytes", stream.available());
    char input[max_input_buffer_size];
    int len = readLine(stream, input, max_input_buffer_size);
    return processCommand(input, stream);
  }

  /// Defines the input buffer size: default is MAX_INPUT_BUFFER_SIZE (256)
  void setMaxInputBufferSize(int size) { max_input_buffer_size = size; }

  /// Defines a reference object which can be used in the callback
  void setReference(void* reference) { p_reference = reference; }

  /// Returns the reference object which can be used in the callback
  void* getReference() { return p_reference; }

  /// Defines an error callback
  void setErrorCallback(bool (*cb)(telnet::Str& cmd,
                                   telnet::Vector<telnet::Str> parameters,
                                   Print& out, TinySerialServer* self)) {
    error_callback = cb;
  }

 protected:
  int max_input_buffer_size = MAX_INPUT_BUFFER_SIZE;
  Stream* p_stream = nullptr;
  bool is_active = false;
  void* p_reference = nullptr;
  bool (*error_callback)(telnet::Str& cmd,
                         telnet::Vector<telnet::Str> parameters, Print& out,
                         TinySerialServer* self) = nullptr;

  /// TinySerialServer command
  struct Command {
    /// command string
    const char* cmd;
    /// example/information for parameters
    const char* parameter_help = "";
    /// callback function
    bool (*callback)(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Print& out, TinySerialServer* self);
  };

  telnet::Vector<Command> commands;

  /// Finds a command by name
  Command* findCommand(const char* cmd) {
    for (auto& command : commands) {
      if (strcmp(command.cmd, cmd) == 0) {
        return &command;
      }
    }
    return nullptr;
  }

  /// help callback
  static bool cmd_help(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    if (parameters.size() == 0) {
      out.println("\nAvailable commands:");
      for (auto& command : self->commands) {
        if (isAscii(command.cmd[0])) {
          out.print(command.cmd);
          out.print("\t");
        }
      }
      out.println("\n");
    } else {
      const char* help_cmd = parameters[0].c_str();
      Command* command = self->findCommand(help_cmd);
      if (command != nullptr && !StrView(command->parameter_help).isEmpty()) {
        out.print(">Command: ");
        out.print(command->cmd);
        out.print(" ");
        out.println(command->parameter_help);
      } else {
        out.print(">Command: ");
        out.print(command->cmd);
        out.println(": No help available");
      }
      out.println();
    }

    return true;
  }

  /// Reads a line delimited by '\n' from the Stream
  int readLine(Stream& in, char* str, int max) {
    memset(str, 0, max);
    int index = 0;
    if (in.available() > 0) {
      index = in.readBytesUntil('\n', str, max);
      str[index - 1] = '\0';  // null termination character
      // special logic for Windows line endings
      if (str[index - 2] == '\r') {
        str[index - 2] = '\0';  // remove carriage return if present
        index--;
      }
    }
    return index - 1;
  }

  /// Processes the command and returns the result output via Client
  virtual bool processCommand(const char* input, Print& result) {
    telnet::Str cmd;
    telnet::Vector<telnet::Str> parameters;
    parseCommand(input, cmd, parameters);
    if (cmd.isEmpty()) {
      return false;
    }
    bool ok = processCommand(cmd, parameters, result);
    if (!ok && error_callback != nullptr) {
      error_callback(cmd, parameters, result, this);
    }
    return ok;
  }

  /// Parses the command and parameters: syntax: cmd(param1,param2,...) or cmd
  /// par1 par2 ...
  bool parseCommand(const char* input, telnet::Str& cmd,
                    telnet::Vector<telnet::Str>& parameters) {
    cmd = input;
    char delimiter;
    int offset;
    int end;
    // check if function or list syntax
    int pos = cmd.indexOf('(');
    if (pos != -1) {
      delimiter = ',';
      offset = 1;
      end = cmd.indexOf(')');
      if (end == -1) {
        return false;
      }
    } else {
      pos = cmd.indexOf(' ');
      delimiter = ' ';
      offset = 0;
      end = cmd.length();
    }

    // determine cmd
    cmd.substr(input, 0, pos);
    telnet::Str tail = "";
    assert(tail.isEmpty());
    // determine parameters
    if (pos > 0) tail.substr(input, pos + offset, end);
    telnet::Str par = "";
    TELNET_LOGI("cmd: '%s'", cmd.c_str());

    while (!tail.isEmpty()) {
      tail.trim();
      split(tail, par, tail, delimiter);
      par.trim();
      TELNET_LOGI("- par: '%s'", par.c_str());
      parameters.push_back(par);
    }
    return true;
  }

  // / Splits the string into head and tail at the first comma
  void split(telnet::Str& str, telnet::Str& head, telnet::Str& tail,
             char sep = ',') {

    // Use single quotes for aruments with spaces
    int pos = str.indexOf(sep);
    int start = 0;
    if (str.startsWith("'")){
      pos = str.indexOf("'", 1);
      start = 1;  // Skip the first quote
    }
    if (str.startsWith("\"")){
      pos = str.indexOf("\"", 1);
      start = 1;  // Skip the first quote
    }

    if (pos == -1) {
      head = str;
      tail = "";
    } else {
      head.substr(str, start, pos);
      tail.substr(str, pos + 1, str.length());
    }
  }

  /// process the command
  bool processCommand(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                      Print& result) {
    for (auto& command : commands) {
      if (cmd.equalsIgnoreCase(command.cmd)) {
        TELNET_LOGI("Command: '%s'", cmd.c_str());
        for (auto& parameter : parameters) {
          TELNET_LOGI("- Parameter: '%s'", parameter.c_str());
        }
        return command.callback(cmd, parameters, result, this);
      }
    }
    return processCommandUndefined(cmd, parameters, result);
  }

  /// Handle undefined commands
  virtual bool processCommandUndefined(telnet::Str& cmd,
                                       telnet::Vector<telnet::Str> parameters,
                                       Print& result) {
    char str[160];
    snprintf(str, sizeof(str), "Invalid command: '%s'", cmd.c_str());
    result.print(str);
    result.println("- type 'help' for a list of commands");
    result.println();
    TELNET_LOGE("%s", str);

    return false;
  }
};

}  // namespace telnet