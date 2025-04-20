#pragma once
#include "Print.h"
#include "TinyTelnetServerConfig.h"
#include "utils/Str.h"
#include "utils/Vector.h"


/**
 * @brief A simple telnet server for Arduino. Call the addCommand method to
 * register your commands.
 *
 * @author Phil Schatzmann
 */
template <class Server, class Client>
class TinyTelnetServer {
 public:
  TinyTelnetServer(Server& server, int port = 23) {
    p_server = &server;
    this->port = port;
    // register help command
    addCommand("help", help);
    addCommand("?", help);
  }
  /// Start the server
  bool begin() {
    is_active = p_server->begin(port);
    return is_active;
  }

  /// Stop the server
  void end() {
    is_active = false;
    if (p_client) {
      p_client->stop();
    }
    if (p_server) {
      p_server->stop();
    }
  }

  /// Add a new command
  void addCommand(const char* cmd,
                  bool (*cb)(telnet::Str& cmd,
                             telnet::Vector<telnet::Str> parameters, Print& out,
                             TinyTelnetServer* self),
                  const char* parameter_help = "") {
    Command command;
    command.cmd = cmd;
    command.cb = cb;
    command.parameter_help = parameter_help;
    commands.push_back(command);
  }

  /// proccess the next command: call in loop()
  bool processCommand() {
    if (!is_active) return false;

    auto client = p_server->accept();
    if (client.connected()) {
      p_client = &client;

      // process the new client with standard functionality
      if (client.available() != 0) {
        char input[max_input_buffer_size];
        int len = readLine(client, input, max_input_buffer_size);
        return processCommand(input, client);
      }

    } else {
      // give other tasks a chance
      delay(no_connect_delay);
    }
    return false;
  }

 protected:
  Server* p_server = nullptr;
  Client* p_client = nullptr;
  int no_connect_delay = NO_CONNECT_DELAY_MS;
  int max_input_buffer_size = MAX_INPUT_BUFFER_SIZE;
  int port = 23;
  bool is_active = false;

  struct Command {
    const char* cmd;
    const char* parameter_help = "";
    bool (*cb)(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
               Print& out, TinyTelnetServer* self);
  };

  telnet::Vector<Command> commands;

  /// help callback
  static bool help(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                   Print& out, TinyTelnetServer* self) {
    for (auto& command : self->commands) {
      out.print(command.cmd);
      out.println(command.parameter_help);
    }

    return true;
  }

  /// Reads a line delimited by '\n' from the Stream
  int readLine(Stream& in, char* str, int max) {
    int index = 0;
    if (in.available() > 0) {
      index = in.readBytesUntil('\n', str, max);
      str[index] = '\0';  // null termination character
    }
    return index;
  }

  /// Processes the command and returns the result output via Print
  bool processCommand(const char* input, Print& result) {
    telnet::Str cmd;
    telnet::Vector<telnet::Str> parameters;
    parseCommand(input, cmd, parameters);
    if (cmd.length() == 0) {
      return false;
    }
    return processCommand(cmd, parameters, result);
  }

  /// Parses the command and parameters: syntax: cmd(param1,param2,...) or cmd
  /// par1 par2 ...
  bool parseCommand(const char* input, telnet::Str& cmd,
                    telnet::Vector<telnet::Str>& parameters) {
    cmd = input;
    char delimiter;
    int offset;
    size_t end;
    // check if function or list syntax
    size_t pos = cmd.indexOf('(');
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
    telnet::Str tail;
    // determine parameters
    tail.substr(input, pos + offset, end - offset);
    telnet::Str par;

    while (!tail.isEmpty()) {
      tail.trim();
      split(tail, par, tail, delimiter);
      /// remove quotes
      if (par.startsWith("\"") && par.endsWith("\"")) {
        par.substr(par, 1, par.length() - 1);
        par.trim();
      }
      parameters.push_back(par);
    }
    return true;
  }

  // / Splits the string into head and tail at the first comma
  void split(telnet::Str& str, telnet::Str& head, telnet::Str& tail,
             char sep = ',') {
    int pos = str.indexOf(sep);
    if (pos == -1) {
      head = str;
      tail = "";
    } else {
      head.substr(str, 0, pos);
      tail.substr(str, pos + 1, str.length());
    }
  }

  /// process the command
  bool processCommand(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                      Print& result) {
    for (auto& command : commands) {
      if (cmd.equalsIgnoreCase(command.cmd)) {
        return command.cb(cmd, parameters, result, this);
      }
    }
    result.println("Unrecognized command: type 'help' for available commands.");

    return false;
  }
};

