#include <Ethernet.h>
#include <SPI.h>
#include "TinyTelnetServer.h"

#ifndef LED_BUILTIN
#  define LED_BUILTIN 22
#endif

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {192, 168, 1, 7};
byte gateway[] = {192, 168, 1, 1};
byte subnet[] = {255, 255, 255, 0};

EthernetServer eth(23);  // Telnet listens on port 23
TinyTelnetServer<EthernetServer, EthernetClient> server(eth);

void login() {
  // start the Ethernet connection
  Ethernet.begin(mac, ip, gateway, subnet);
  Serial.println("Ethernet connected");
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());
}

// callback function for the led command
bool led(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters, EthernetClient& out,
         TinyTelnetServer<EthernetServer, EthernetClient>* self) {
  if (parameters.size() != 1) {
    out.println("led Error: Invalid number of parameters");
    return false;
  }
  if (parameters[0] != "on" && parameters[0] != "off") {
    out.println(
        "led Error: parameter value is not valid. It must be on or off");
    return false;
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, parameters[0] == "on" ? HIGH : LOW);
  out.print("led: OK");
  return true;
}

void setup() {
  Serial.begin(115200);
  // setup logger
  TinyTelnetLogger.begin(Serial, TinyTelnetLogLevel::Info);

  // login to Ethernet
  login();

  // register the led command
  server.addCommand("led", led, "(on|off)");

  // start server
  server.begin();
}

void loop() { server.processCommand(); }
