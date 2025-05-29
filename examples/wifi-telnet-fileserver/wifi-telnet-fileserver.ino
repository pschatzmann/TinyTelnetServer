/***
 * @file wifi-telnet-fileserver.ino
 * @brief This example demonstrates how to use the TinyTelnetServer library to
 * create a telnet server that allows file operations on an SD card. It includes
 * commands for listing files, displaying file contents, moving files, removing
 * files, creating directories, copying files, showing disk space information,
 * creating empty files or updating timestamps, writing text to files, and
 * displaying the first lines of a file.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
#include "SD.h"
#include "SPI.h"
#include "TinyTelnetServer.h"
#include "WiFi.h"
#include "Commands/SDFileCommands.h"

#define PIN_SD_CARD_CS 13
#define PIN_SD_CARD_MISO 2
#define PIN_SD_CARD_MOSI 15
#define PIN_SD_CARD_CLK 14

WiFiServer wifi(23);
TinyTelnetServer<WiFiServer, WiFiClient> server(wifi);
SDFileCommands sdFileCommands(server);
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

bool setupSD() {
  SPI.begin(PIN_SD_CARD_CLK, PIN_SD_CARD_MISO, PIN_SD_CARD_MOSI,
            PIN_SD_CARD_CS);
  if (!SD.begin(PIN_SD_CARD_CS)) {
    Serial.println("Card Mount Failed");
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  // setup logger
  TinyTelnetLogger.begin(Serial, TinyTelnetLogLevel::Info);

  // setup SD card
  setupSD();

  // login to Wifi
  login();

  // start server
  server.begin();
}

void loop() { server.processCommand(); }
