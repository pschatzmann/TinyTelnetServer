#pragma once
#include "Client.h"
#include "TinyTelnetServerConfig.h"
#include "utils/Logger.h"
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
    addCommand("help", cmd_help);
    addCommand("close", cmd_close);
    addCommand("bye", cmd_close);
    addCommand("exit", cmd_close);
    addCommand("end", cmd_close);
  }
  /// Start the server
  bool begin() {
    p_server->begin(port);
    is_active = true;
    return is_active;
  }

  /// Stop the server
  void end() {
    is_active = false;
    for (auto& client : clients) {
      client.stop();
      client.end();
    }
    clients.clear();
    if (p_server) {
      p_server->stop();
    }
  }

  /// Add a new command
  void addCommand(const char* cmd,
                  bool (*cb)(telnet::Str& cmd,
                             telnet::Vector<telnet::Str> parameters,
                             Client& out, TinyTelnetServer* self),
                  const char* parameter_help = "") {
    Command command;
    command.cmd = cmd;
    command.callback = cb;
    command.parameter_help = parameter_help;
    commands.push_back(command);
  }

  /// proccess the next command: call in loop()
  bool processCommand() {
    if (!is_active) return false;
    // reconnect
    connectClients();

    // process all clients
    for (auto& client : clients) {
      if (client.connected()) {
        // process the new client with standard functionality
        if (client.available() > 3) {
          TELNET_LOGI("available: %d bytes", client.available());
          char input[max_input_buffer_size];
          int len = readLine(client, input, max_input_buffer_size);
          // process command codes
          int start = parseTelnetCommands(input, len, client);
          TELNET_LOGD("len: %d - start: %d", len, start);
          // end if all telnet commands are processed
          if (start == len) return true;

          // process user command
          return processCommand(input + start, client);
        } else {
          // no data available
          delay(no_connect_delay);
        }
      }
    }
    return false;
  }

  /// provide number of clients
  int count(){ return clients.size();} 

  /// provide number of active clients
  int countActive(){ 
    int count = 0;
    for (auto& client : clients) {
      if (client.connected()) {
        count++;
      }
    }
    return count;
  }


 protected:
  Server* p_server = nullptr;
  telnet::Vector<Client> clients;
  int no_connect_delay = NO_CONNECT_DELAY_MS;
  int max_input_buffer_size = MAX_INPUT_BUFFER_SIZE;
  int port = 23;
  bool is_active = false;
  const char SE = 240;
  const char SB = 250;
  const char IAC = 255;
  const char DO = 253;
  const char DONT = 254;
  const char WILL = 251;
  const char WONT = 252;
  const char SUPPRESS_GA = 3;
  const char STATUS = 5;
  const char LINEMODE = 34;
  int active_clients = 0;

  /// TinyTelnetServer command
  struct Command {
    /// command string
    const char* cmd;
    /// example/information for parameters
    const char* parameter_help = "";
    /// callback function
    bool (*callback)(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Client& out, TinyTelnetServer* self);
  };

  telnet::Vector<Command> commands;

  /// help callback
  static bool cmd_help(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Client& out, TinyTelnetServer* self) {
    out.println("\nAvailable commands:");
    for (auto& command : self->commands) {
      out.print(command.cmd);
      out.print(command.parameter_help);
      out.print("\t");
    }
    out.println("\n");

    return true;
  }

  /// close callback
  static bool cmd_close(telnet::Str& cmd,
                        telnet::Vector<telnet::Str> parameters, Client& out,
                        TinyTelnetServer* self) {
    out.println("Bye");
    out.stop();
    return true;
  }

  /// Acccepts new clients 
  void connectClients() {
    auto tmp = p_server->accept();
    if (tmp.connected()) {
      tmp.setTimeout(CLIENT_TIMEOUT_MS);
      addClient(tmp);
      TELNET_LOGI("%s", "New client connected");
    }

    // log change of active clients
    if (active_clients != countActive()) {
      active_clients = countActive();
      TELNET_LOGI("active clients: %d", active_clients);
    }
  }

  /// Adds a new client to the list of clients
  void addClient(Client& client) {
    for (auto& c : clients) {
      if (!c.connected()) {
        c = client;
        return;
      }
    }
    clients.push_back(client);
  }

  /// Parse and process the telnet commands 
  int parseTelnetCommands(char* cmds, int len, Client& client) {
    TELNET_LOGD("parseTelnetCommands: %d", len);
    int start = 0;
    char cmd[80];
    while (cmds[start] == IAC) {
      int len = 3;
      if (cmds[start + 1] == SB) {
        // subnegotiation
        TELNET_LOGD("---> subnegotiation %d", start + 1);
        int end = start + 2;
        while (cmds[end] != SE && end < len) {
          end++;
        }
        assert(cmds[end] == SE);
        len = end - start;
      }
      strncpy(cmd, cmds + start, len);
      processTelnetCommand(cmd, len, client);
      start += len;
    }

    return start;
  }

  // Telnet options:
  // 0	BINARY	Binary transmission
  // 1	ECHO	Remote echo
  // 3	SUPPRESS-GA	Suppress "Go Ahead"
  // 24	TERMINAL-TYPE	Terminal type (e.g., xterm)
  // 31	NAWS	Negotiate About Window Size
  // 32	TERMINAL-SPEED	Terminal speed info
  // 33	REMOTE-FLOW-CONTROL	Flow control settings
  // 34	LINEMODE	Line-oriented mode
  // 36	ENVIRONMENT	Send environment variables

  /// Converts the telnet command to a string
  const char* controlStr(int cmd) {
    if (cmd == 253) return "DO";
    if (cmd == 254) return "DONT";
    if (cmd == 251) return "WILL";
    if (cmd == 252) return "WONT";
    if (cmd == 250) return "SB";  // Start subnegotiation

    static char str[30];
    snprintf(str, sizeof(str), "Unknown (%d)", cmd);
    return str;
  }

  /// Process telnet protocol command
  void processTelnetCommand(char* cmd, int len, Client& client) {
    assert(cmd[0] == IAC);
    TELNET_LOGD("telnet cmd:%s %d (len=%d)", controlStr(cmd[1]), cmd[2], len);
    // confirm all do request
    if (cmd[1] == DO) {
      // DO -> WILL or WONT
      cmd[1] = WILL;
      if (cmd[2] == STATUS) {
        // suppress go ahead
        cmd[1] = WONT;
      }
      client.write(cmd, len);
      TELNET_LOGD("-> reply:%s %d", controlStr(cmd[1]), cmd[2]);
    } else if (cmd[1] == WILL) {
      // WILL -> DO or DONT
      cmd[1] = DONT;
      if (cmd[2] == SUPPRESS_GA || cmd[2] == LINEMODE) {
        // accept line mode negotiation
        cmd[1] = DO;
      }
      client.write(cmd, len);
      TELNET_LOGD("-> reply:%s %d", controlStr(cmd[1]), cmd[2]);
    } else if (cmd[1] == SB && cmd[2] == LINEMODE) {
      // Acknowledges only MODE_EDIT accepted
      char tmp[7] = {IAC, SB, 34, 1, 01, IAC, SE};
      TELNET_LOGD("-> reply %d (len=%d)", tmp[2], sizeof(tmp));
      client.write(tmp, sizeof(tmp));
      client.println("> Welcome to TinyTelnetServer");
    }
  }

  /// Reads a line delimited by '\n' from the Stream
  int readLine(Stream& in, char* str, int max) {
    memset(str, 0, max);
    int index = 0;
    if (in.available() > 0) {
      index = in.readBytesUntil('\n', str, max);
      str[index - 1] = '\0';  // null termination character
    }
    return index - 1;
  }

  /// Processes the command and returns the result output via Client
  bool processCommand(const char* input, Client& result) {
    telnet::Str cmd;
    telnet::Vector<telnet::Str> parameters;
    parseCommand(input, cmd, parameters);
    if (cmd.isEmpty()) {
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
      /// remove quotes
      if (par.startsWith("\"") && par.endsWith("\"")) {
        par.substr(par, 1, par.length());
        par.trim();
      }
      TELNET_LOGI("- par: '%s'", par.c_str());
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
                      Client& result) {
    for (auto& command : commands) {
      if (cmd.equalsIgnoreCase(command.cmd)) {
        TELNET_LOGI("Command: '%s'", cmd.c_str());
        for (auto& parameter : parameters) {
          TELNET_LOGI("- Parameter: '%s'", parameter.c_str());
        }
        return command.callback(cmd, parameters, result, this);
      }
    }

    if (cmd.c_str() != nullptr && isalpha(cmd.c_str()[0])) {
      char str[160];
      snprintf(str, sizeof(str), "Invalid command: '%s'", cmd.c_str());
      result.print(str);
      result.println("- type 'help' for a list of commands");
      TELNET_LOGE("%s", str);
    } else {
      int start = parseTelnetCommands((char*)cmd.c_str(), cmd.length(), result);
      TELNET_LOGE("Not Processed: %s", cmd.c_str()+start);
    }

    return false;
  }
};
