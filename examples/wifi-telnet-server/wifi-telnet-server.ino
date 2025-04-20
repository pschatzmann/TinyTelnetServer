#include "TinyTelnetServer.h"
#include "WiFi.h"

#ifndef LED_BUILTIN
#  define LED_BUILTIN 22
#endif

WiFiServer wifi;
TinyTelnetServer<WiFiServer, WiFiClient> server(wifi);
const char* ssid = "ssid";
const char* password = "pwd";

void login() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  WiFi.setSleep(false);
  Serial.print("Connect with telnet ");
  Serial.println(WiFi.localIP());
}

// Callback function for the led command
bool led(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters, WiFiClient& out,
         TinyTelnetServer<WiFiServer, WiFiClient>* self) {
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

  // login to Wifi
  login();

  // register a command
  server.addCommand("led", led, "(on|off)");

  // start server
  server.begin();
}

void loop() {
  server.processCommand();
}
