# TinyTelnetServer

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

A lightweight Arduino library for creating custom Command Line Interfaces (CLI) via serial and telnet servers. This library makes it easy to add remote management capabilities to your microcontroller projects.

## Overview

TinyTelnetServer provides a flexible framework to:
- Create a telnet server for remote command execution
- Build a serial CLI for local control
- Define and register your own custom commands
- Access your device through standard telnet clients

By default, microcontrollers don't support telnet commands - this library fills that gap with minimal resource usage.

## Features

- **Dual interface**: Works with both telnet and serial connections
- **Custom commands**: Register your own functions as CLI commands
- **Network support**: Works with both WiFi and Ethernet
- **Predefined commands**: Ready-to-use file and audio control commands
- **Extensible**: Easy to add new command sets
- **Lightweight**: Optimized for resource-constrained devices

## Installation

### Arduino IDE

1. Open Arduino IDE
2. Navigate to Sketch > Include Library > Manage Libraries
3. Search for "TinyTelnetServer"
4. Click Install

### Manual Installation

```bash
cd ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/TinyTelnetServer.git

```

## Quick Start

### Basic Telnet Server with WiFi

```cpp
#include <WiFi.h>
#include "TinyTelnetServer.h"

const char* ssid = "your-ssid";
const char* password = "your-password";

WiFiServer server(23);  // Telnet standard port
TinyTelnetServer<WiFiServer, WiFiClient> telnetServer(server);

// Example command function
bool led_command(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters, Print& out, TinySerialServer* self) {
  if (parameters.size() > 0) {
    if (parameters[0] == "on") {
      digitalWrite(LED_BUILTIN, HIGH);
      out.println("LED turned ON");
    } else if (parameters[0] == "off") {
      digitalWrite(LED_BUILTIN, LOW);
      out.println("LED turned OFF");
    }
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
  
  // Register commands
  telnetServer.addCommand("led", led_command, "led [on|off] - Control the LED");
  
  // Start the server
  telnetServer.begin();
  Serial.println("Telnet server started");
}

void loop() {
  telnetServer.processCommand();
}
```

## Examples

The library includes several example sketches:

- **[WiFi Telnet Server](/examples/wifi-telnet-server/wifi-telnet-server.ino)**: Basic telnet server using WiFi
- **[Ethernet Telnet Server](/examples/ethernet-telnet-server/ethernet-telnet-server.ino)**: Basic telnet server using Ethernet
- **[Serial Server](/examples/serial-server/serial-server.ino)**: Command-line interface over Serial
- **[WiFi Telnet File Server](/examples/wifi-telnet-fileserver/wifi-telnet-fileserver.ino)**: File management commands over telnet

## Predefined Command Sets

The library includes ready-to-use command implementations:

- **[KARadioCommands](https://pschatzmann.github.io/TinyTelnetServer/html/classtelnet_1_1_k_a_radio_commands.html)**: Control internet radio playback
- **[SDFileCommands](https://pschatzmann.github.io/TinyTelnetServer/html/classtelnet_1_1_s_d_file_commands.html)**: File operations (ls, cat, mv, rm, mkdir, etc.)

## Documentation

Comprehensive documentation is available:

- **[TinyTelnetServer](https://pschatzmann.github.io/TinyTelnetServer/html/class_tiny_telnet_server.html)**: Telnet server implementation
- **[TinySerialServer](https://pschatzmann.github.io/TinyTelnetServer/html/class_tiny_serial_server.html)**: Serial interface implementation
- **[Complete API Documentation](https://pschatzmann.github.io/TinyTelnetServer/html/namespaces.html)**: Full library reference

## Creating Custom Commands

Creating your own commands is straightforward:

```cpp
// Command function signature
bool my_command(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters, Print& out, TinySerialServer* self) {
  // Command implementation
  out.println("Hello from my custom command!");
  
  // Access parameters if provided
  if (parameters.size() > 0) {
    out.print("Parameter: ");
    out.println(parameters[0]);
  }
  
  return true; // Return true for success, false for failure
}

// In setup()
telnetServer.addCommand("hello", my_command, "hello [name] - Greet a user");
```

## Support

Before opening issues, please:

1. Read the [documentation](https://pschatzmann.github.io/TinyTelnetServer/html/index.html) thoroughly
2. Check existing [issues](https://github.com/pschatzmann/TinyTelnetServer/issues) and [discussions](https://github.com/pschatzmann/TinyTelnetServer/discussions)

When reporting bugs, include:
- The specific sketch/scenario you're using
- Your hardware details
- Software versions (Arduino IDE, board package, etc.)
- Clear steps to reproduce the issue

For questions and discussions, please use the GitHub Discussions section rather than opening issues.

## Sponsor This Project

If you find this library helpful, consider supporting its development:

- [Buy me a coffee](https://www.buymeacoffee.com/philschatzh)
- [PayPal](https://paypal.me/pschatzmann?country.x=CH&locale.x=en_US)

## License

This project is licensed under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0).


