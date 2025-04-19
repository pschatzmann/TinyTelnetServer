#include "TinyTelnetServer.h"
#include "WiFi.h"

#ifndef LED_BUILTIN
#  define LED_BUILTIN 22
#endif

WiFiServer wifi;
TinyTelnetServer<WiFiServer, WiFiClient> server(wifi);
const char* ssid = "SSID";
const char* password = "Password";

void login() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

bool led(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters, Print& out,
         TinyTelnetServer<WiFiServer, WiFiClient>* self) {
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
  // login to Wifi
  login();

  // register a command
  server.addCommand("led", led, "(on|off)");
}

void loop() {
  server.processCommand();
}
