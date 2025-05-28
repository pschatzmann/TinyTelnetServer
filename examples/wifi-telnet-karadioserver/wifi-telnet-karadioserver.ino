/***
 * @file wifi-telnet-karadio.ino
 * @brief This example demonstrates how to use this library to
 * create a ka-radio telnet server that allows you to use the use the
 * https://play.google.com/store/apps/details?id=com.serasidis.karadio.rc
 * Android app to control the AudioPlayer.
 *
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h"
#include "AudioTools/Disk/AudioSourceSD.h"  // or AudioSourceIdxSD.h
#include "SD.h"
#include "SPI.h"
#include "TinyTelnetServer.h"
#include "WiFi.h"
#include "Commands/KARadioCommands.h"

WiFiServer wifi(23);
TinyTelnetServer<WiFiServer, WiFiClient> server(wifi);
const char* startFilePath = "/";
const char* ext = "mp3";
AudioSourceSD source(startFilePath, ext, PIN_AUDIO_KIT_SD_CARD_CS);
AudioBoardStream i2s(AudioKitEs8388V1);
MP3DecoderHelix decoder;  // or change to MP3DecoderMAD
AudioPlayer player(source, i2s, decoder);
KARadioCommands sdFileCommands(server, player);
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

bool setupPlayer() {
  // setup output
  auto cfg = i2s.defaultConfig(TX_MODE);
  // sd_active is setting up SPI with the right SD pins by calling
  // SPI.begin(PIN_AUDIO_KIT_SD_CARD_CLK, PIN_AUDIO_KIT_SD_CARD_MISO,
  // PIN_AUDIO_KIT_SD_CARD_MOSI, PIN_AUDIO_KIT_SD_CARD_CS);
  cfg.sd_active = true;
  i2s.begin(cfg);

  // setup player
  player.setVolume(0.7);
  player.begin();
  player.setActive(false);
}

void setup() {
  Serial.begin(115200);
  // setup logger
  TinyTelnetLogger.begin(Serial, TinyTelnetLogLevel::Info);

  
  // start wifi for telnet
  login();

  setupPlayer();

  // start server
  server.begin();
}

void loop() {
  server.processCommand();
  player.copy();
}
