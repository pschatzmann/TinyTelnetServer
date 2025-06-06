/***
 * @file test.ino
 * @brief Just a simple sketch for the desktop that replys pong for the ping command.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
#include "TinyTelnetServer.h"
#include "WiFi.h"

// port 23 needs root privileges on Linux: so we use a different port
const int port = 9023;
WiFiServer wifi(port);
TinyTelnetServer<WiFiServer, WiFiClient> server(wifi);

void login() {
  WiFi.begin("dummy","dummy");
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  WiFi.setSleep(false);
  Serial.print("Connect with telnet ");
  Serial.print(WiFi.localIP());
  Serial.print(" ");
  Serial.println(port);
}

// Callback function for the led command
bool ping(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters, Print& out,
         TinySerialServer* self) {
  out.println(">pong");
  out.println();
  return true;
}


void setup() {
  Serial.begin(115200);
  // setup logger
  TinyTelnetLogger.begin(Serial, TinyTelnetLogLevel::Info);

  // login to Wifi
  login();

  // register a commands
  server.addCommand("ping", ping, ": (no parameters) - just replys with pong");
  server.addCommand("exit", TinyTelnetServer<WiFiServer, WiFiClient>::cmd_bye,": synonym to bye");

  // start server
  server.begin();
}

void loop() { server.processCommand(); }
