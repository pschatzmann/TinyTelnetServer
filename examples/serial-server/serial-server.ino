/**
 * @file serial-server.ino
 * @brief Example for TinySerialServer.
 * This example demonstrates how to create a simple serial server. It includes a command to control an LED.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
#include "TinySerialServer.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

TinySerialServer server(Serial);

// Callback function for the led command
bool led(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters, Print& out,
         TinySerialServer* self) {
  if (parameters.size() != 1) {
    out.println(">led Error: Invalid number of parameters");
    return false;
  }
  if (!parameters[0].equals("on") && !parameters[0].equals("off")) {
    out.println(
        ">led Error: parameter value is not valid. It must be on or off");
    return false;
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, parameters[0] == "on" ? HIGH : LOW);
  out.println(">led: OK\n");
  return true;
}

void setup() {
  Serial.begin(115200);
  // setup logger
  TinyTelnetLogger.begin(Serial, TinyTelnetLogLevel::Info);

  // register a command
  server.addCommand("led", led, "(on|off)");

  // start server
  server.begin();
}

void loop() { server.processCommand(); }
