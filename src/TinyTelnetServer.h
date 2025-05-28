#pragma once
#include "Client.h"
#include "TinySerialServer.h"
#include "TinyTelnetServerConfig.h"

namespace telnet {

/**
 * @brief A simple telnet server for Arduino. Call the addCommand method to
 * register your commands.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
template <class Server, class Client>
class TinyTelnetServer : public TinySerialServer {
 public:
  TinyTelnetServer(Server& server) {
    p_server = &server;
    // register help command
    addCommand("help", cmd_help);
    addCommand("bye", cmd_bye, ": (no parameters) - Closes the session");
  }
  /// Start the server
  bool begin() override {
    p_server->begin();
    is_active = true;
    return is_active;
  }

  /// End the processing: warning: this method does not call end on the server because this is 
  /// not available for all server implementations
  void end() override {
    is_active = false;
    for (auto& client : clients) {
      client.stop();
    }
    clients.clear();
    // Commented out because not available for EthernetServer!
    // if (p_server) {
    //   p_server->end();
    // }
  }

  /// proccess the next command: call in loop()
  bool processCommand() override {
    if (!is_active) return false;
    
    // reconnect
    connectClients();

    // process all clients
    for (Client& client : clients) {
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
  int count() { return clients.size(); }

  /// provide number of active clients
  int countActive() {
    int count = 0;
    for (auto& client : clients) {
      if (client.connected()) {
        count++;
      }
    }
    return count;
  }

  /// close callback: you can register it with addCommand under different names
  static bool cmd_bye(telnet::Str& cmd,
                        telnet::Vector<telnet::Str> parameters, Print& out,
                        TinySerialServer* self) {
    Client& client = (Client&)out;
    client.println("Bye");
    client.stop();
    return true;
  }

 protected:
  Server* p_server = nullptr;
  telnet::Vector<Client> clients;
  int no_connect_delay = NO_CONNECT_DELAY_MS;
  int port = 23;
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

  bool processCommand(const char* input, Client& result) {
    return TinySerialServer::processCommand(input, (Print&)result);
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
  int parseTelnetCommands(char* cmds, int len, Print& client) {
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
  void processTelnetCommand(char* cmd, int len, Print& client) {
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

  /// Handle undefined commands
  virtual bool processCommandUndefined(telnet::Str& cmd,
                                       telnet::Vector<telnet::Str> parameters,
                                       Print& result) {
    if (cmd.c_str() != nullptr && isalpha(cmd.c_str()[0])) {
      char str[160];
      snprintf(str, sizeof(str), "Invalid command: '%s'", cmd.c_str());
      result.print(str);
      result.println("- type 'help' for a list of commands");
      TELNET_LOGE("%s", str);
    } else {
      int start = parseTelnetCommands((char*)cmd.c_str(), cmd.length(), result);
      TELNET_LOGE("Not Processed: %s", cmd.c_str() + start);
    }

    return false;
  }
};

}