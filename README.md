# TinyTelnetServer

A simple Arduino library to easily build your custom Command Line Interface (CLI) via a serial and telnet server.

By default a microcontroller does not support any telnet commands. Therefore I implemented
a flexible functionality to register the commands that you want to provide.


### Some Simple Examples

For __telnet__, I am providing an example that uses [WiFi.h](/examples/wifi-telnet-server/wifi-telnet-server.ino) and one that relies on the [Ethernet.h](/examples/ethernet-telnet-server/ethernet-telnet-server.ino)

For the __serial server__ I am providing [an example](examples/serial-server/serial-server.ino) that uses Serial.

I am registering the led function: you can execute _help_ to provide the list of all supported commands and _led(on)_ or _led(off)_ to switch the led on and off.


### Predefined Command Handlers

As examples, I am providing some predefined command handler implementations 

- KARadioCommands
- SDFileCommands


### Documentation

- [Class Docoumentation](https://pschatzmann.github.io/TinyTelnetServer/html/class_tiny_telnet_server.html)

### Support

I spent a lot of time to provide a comprehensive and complete documentation. So please read the documentation first and check the issues and discussions before posting any new ones on Github.

Open __issues only for bugs__ and if it is not a bug, use a discussion: Provide enough information about

- the selected scenario/sketch
- what exactly you are trying to do
- your hardware
- your software versions
- what exactly your problem is
- to enable others to understand and reproduce your issue.

Finally, __don't send me any e-mails__ or post questions on my personal website!


### Installation

You can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with

```
cd  ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/TinyTelnetServer.git
```

### Sponsor Me

This software is totally free, but you can make me happy by rewarding me with a treat

- [Buy me a coffee](https://www.buymeacoffee.com/philschatzh)
- [Paypal me](https://paypal.me/pschatzmann?country.x=CH&locale.x=en_US)

