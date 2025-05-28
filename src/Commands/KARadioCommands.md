Implements KA-Radio CLI commands with proper prefixes. This document gives the supported commands and the corresponding original output of KA-Radio.

## Android remote command

KaRadio Remote Control by Vassilis Serasidis on google Play is an easy and fast WiFi remote control for your AudioTools AudioPlayer using the KA-Radio API.

With this android app you can select the station or file you want to play, set the volume.

See https://play.google.com/store/apps/details?id=com.serasidis.karadio.rc


## Supported Commands

  - cli.play: Start a Station by id  e.g. cli.play("0")

```
      ##CLI.NAMESET#: 0 lafabrik
      ##CLI.URLSET#: http://stream.lafabrik.ch
      ##CLI.PATHSET#: /lafabrik
      ##CLI.PORTSET#: 80
      ##CLI.OVOLSET#: 0
      ##CLI.VOL#: 70
      ##CLI.ICY0#:  La Fabrik
      ##CLI.ICY3#:  http:\/\/www.lafabrik.ch\/
      ##CLI.ICY4#:  pop
      ##CLI.ICY5#:  256
      ##CLI.ICY7#:  channels=2;samplerate=44100;bitrate=256
      ##CLI.META#: Radio La Fabrik - Retrouvez-nous sur www.lafabrik.ch
      ##CLI.PLAYING#
```
  - cli.stop: Stop playback
```
      ##CLI.STOPPED# from cli stop
```
  - cli.vol: Get or set volume (cli.vol("100"))

```   
      ##CLI.VOL#: 100
```
  - cli.vol+: Increase volume

```
      ##CLI.VOL#: 100
```

  - cli.vol-: Decrease volume

```
      ##CLI.VOL#: 100
```

  - cli.list: List available stations

```
      ##CLI.LIST#
      #CLI.LISTINFO#:   0: RTL2, streaming.radio.rtl.fr:80/rtl2-1-44-128
      #CLI.LISTINFO#:   1: RTL2, streaming.radio.rtl.fr:80/rtl2-1-44-128
      ##CLI.LIST#
```

  
  - cli.next: Move to next preset
  
```
      ##CLI.NAMESET#: 2 freedom
      ##CLI.URLSET#: http://freedomice.streamakaci.com
      ##CLI.PATHSET#: /freedom.mp3
      ##CLI.PORTSET#: 80
      ##CLI.OVOLSET#: 0
      ##CLI.STOPPED# from header1
      ##CLI.VOL#: 70
      ##CLI.ICY0#:  FREEDOM
      ##CLI.ICY3#:  http:\/\/www.audiorealm.com
      ##CLI.ICY4#:  Various
      ##CLI.ICY5#:  96
      ##CLI.ICY6#:  My station description
      ##CLI.ICY7#:  ice-samplerate=44100;ice-bitrate=96;ice-channels=2
      ##CLI.META#: 
      ##CLI.PLAYING#
```

  - cli.prev: Move to previous preset

```
      ##CLI.NAMESET#: 2 freedom
      ##CLI.URLSET#: http://freedomice.streamakaci.com
      ##CLI.PATHSET#: /freedom.mp3
      ##CLI.PORTSET#: 80
      ##CLI.OVOLSET#: 0
      ##CLI.STOPPED# from header1
      ##CLI.VOL#: 70
      ##CLI.ICY0#:  FREEDOM
      ##CLI.ICY3#:  http:\/\/www.audiorealm.com
      ##CLI.ICY4#:  Various
      ##CLI.ICY5#:  96
      ##CLI.ICY6#:  My station description
      ##CLI.ICY7#:  ice-samplerate=44100;ice-bitrate=96;ice-channels=2
      ##CLI.META#: 
      ##CLI.PLAYING#
```


  - cli.instant: Immediately play a station

```
      ##CLI.STOPPED# from cli instantplay
      ##CLI.ICY0#:  FREEDOM
      ##CLI.ICY3#:  http:\/\/www.audiorealm.com
      ##CLI.ICY4#:  Various
      ##CLI.ICY5#:  96
      ##CLI.ICY6#:  My station description
      ##CLI.ICY7#:  ice-samplerate=44100;ice-bitrate=96;ice-channels=2
      ##CLI.META#: 
      ##CLI.PLAYING#
```

  - cli.name("name")

```
      ##CLI.NAMESET#: 92 ALBUMROCK.MIAMI
```

  - cli.url("url")		: the name or ip of the station to instant play

```
      ##CLI.URLSET#: uk4.internet-radio.com
```

  - cli.path("/path")	: the path of the station to instant play

```
      ##CLI.PATHSET#: /
```
  
  - cli.port("xxxx")	: the port number of the station to instant play

```
      ##CLI.PORTSET#: 8081
```


  - cli.info: Show current station info

```
      ##CLI.INFO#
      ##SYS.DATE#: 2025-04-24T09:32:09+00:00
      ##CLI.NAMESET#: 2 freedom
      ##CLI.ICY0#:  FREEDOM
      ##CLI.ICY3#:  http:\/\/www.audiorealm.com
      ##CLI.ICY4#:  Various
      ##CLI.ICY5#:  96
      ##CLI.ICY6#:  My station description
      ##CLI.ICY7#:  ice-samplerate=44100;ice-bitrate=96;ice-channels=2
      ##CLI.META#: 
      ##CLI.VOL#: 70
      ##CLI.PLAYING#
```

  - sys.version: Get firmware version
    
```
      nl
      Release: 2.4, Revision: 0, KaRadio32
```

  - sys.boot: Reboot the system
 

Out implementations does not provide

- CLI.OVOLSET
- CLI.ICYn
- CLI.META
- SYS.DATE

Error

    ##CMD_ERROR#
