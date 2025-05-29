#pragma once
#include "AudioTools.h"
#include "TinySerialServer.h"
#include "Utils/Logger.h"
#include "Utils/Str.h"

namespace telnet {

static int max_input_files = 0;
static Vector<Str> input_files;
static Vector<const char*> input_files_refs;

/**
 * @brief Class providing KA-Radio control commands for TinyTelnetServer using
 * the AudioTools AudioPlayer: The Audio player supports multiple audio sources:
 * - files
 * - urls
 * - ftp
 * - etc
 *
 * Implements KA-Radio CLI commands with proper prefixes:
 * - cli.play: Start a station by id
 * - cli.start: Start playback of current station or resume playback
 * - cli.stop: Stop playback
 * - cli.vol: Get or set volume
 * - cli.vol+: Increase volume
 * - cli.vol-: Decrease volume
 * - cli.list: List available stations
 * - cli.next: Move to next preset
 * - cli.prev: Move to previous preset
 * - cli.info: Show current station info
 * - cli.instant: Immediately play a station
 *   - cli.url: set url
 *   - cli.path: set path
 *   - cli.port: set port
 * - sys.version: Get firmware version
 * - sys.boot: Reboot the system
 *
 * @copyright GPLv3
 */
class KARadioCommands {
 public:
  /// Empty constructor: you need to call addCommands() and setAudioPlayer()
  /// yourself
  KARadioCommands() = default;
  /// Constructor which registers the commands to the server and adds the player
  KARadioCommands(TinySerialServer& server, AudioPlayer& player) {
    addCommands(server);
    setAudioPlayer(player);
  }

  /// Defines a maximum number of files: 0 = unlimited
  void setMaxInputFiles(int max) { max_input_files = max; }

  /// Assigns the AudioPlayer to be used
  void setAudioPlayer(AudioPlayer& player) { this->p_player = &player; }

  AudioPlayer& audioPlayer() { return *this->p_player; }

  /**
   * @brief Register commands with the server
   *
   * @param server The TinySerialServer to register commands with
   */
  void addCommands(TinySerialServer& server) {
    // Register CLI commands
    const char* no_parameters = ": no parameters";
    server.addCommand("cli.start", cmd_play, no_parameters);
    server.addCommand("cli.play", cmd_play, ": play(\"no\")");
    server.addCommand("cli.stop", cmd_stop), no_parameters;
    server.addCommand("cli.vol", cmd_volume, ": cli.vol[(\"0-254\")]");
    server.addCommand("cli.vol+", cmd_volup, no_parameters);
    server.addCommand("cli.vol-", cmd_voldown, no_parameters);
    server.addCommand("cli.list", cmd_list, ": cli.list[(\"no\")]");
    server.addCommand("cli.next", cmd_next, no_parameters);
    server.addCommand("cli.prev", cmd_prev, no_parameters);
    server.addCommand("cli.info", cmd_info, no_parameters);
    // server.addCommand("cli.instant", cmd_instant);
    //  server.addCommand("cli.name", cmd_name);
    //  server.addCommand("cli.url", cmd_url);
    //  server.addCommand("cli.path", cmd_path);
    //  server.addCommand("cli.port", cmd_port);

    // Register SYS commands
    server.addCommand("sys.version", cmd_version, no_parameters);

    // Store reference to this instance in the server
    server.setReference(this);
    server.setErrorCallback(cmd_error);
  }

  /**
   * Enumerating files might be too slow to be useful, so we provide
   * an alternative interface to made all audio files or urls available.
   * A good way to structure things it to provide a list of directories.
   * We store a copy of the provided name.
   */
  void addAudio(const char* name) { input_files.push_back(Str(name)); }

  /**
   * Enumerating files might be too slow to be useful, so we provide
   * an alternative interface to made all audio files or urls available.
   * A good way to structure things it to provide a list of directories.
   * We just store a pointer to the provided name, so you need to make sure
   * that the provided name is valid as long as the server is running.
   */
  void addAudioRefs(const char* name) { input_files_refs.push_back(name); }

  /**
   * @brief Error handler
   */
  static bool cmd_error(telnet::Str& cmd,
                        telnet::Vector<telnet::Str> parameters, Print& out,
                        TinySerialServer* self) {
    out.println("##CMD_ERROR#");
    return true;
  }

  /**
   * @brief Start playback of a station
   */
  static bool cmd_play(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    AudioPlayer& player = commands->audioPlayer();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    // Format: cli.play [id]
    if (parameters.size() == 1) {
      // Send command to KA-Radio
      int idx = parameters[0].toInt();
      TELNET_LOGI("Setting index to %d", idx);
      player.setIndex(idx);
      player.play();
    } else {
      // Resume playback
      player.play();
    }

    out.println();
    printPlaying(*commands, player, out);
    return true;
  }

  /**
   * @brief Stop playback
   */
  static bool cmd_stop(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    AudioPlayer& player = commands->audioPlayer();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    player.stop();
    out.println();
    out.println("##CLI.STOPPED#");
    return true;
  }

  /**
   * @brief Get or set volume
   */
  static bool cmd_volume(telnet::Str& cmd,
                         telnet::Vector<telnet::Str> parameters, Print& out,
                         TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    AudioPlayer& player = commands->audioPlayer();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    // Format: cli.vol [0-254]
    if (parameters.size() == 1) {
      // Set volume
      int volume = atoi(parameters[0].c_str());
      if (volume > 254) volume = 254;
      if (volume < 0 ) volume = 0;
      player.setVolume(static_cast<float>(volume) / 254.0);
    }
    
    // Get current volume
    int volume = player.volume() * 254.0;
    out.println();
    printVolume(commands->audioPlayer(), out);

    return true;
  }

  /**
   * @brief Increase volume
   */
  static bool cmd_volup(telnet::Str& cmd,
                        telnet::Vector<telnet::Str> parameters, Print& out,
                        TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    AudioPlayer& player = commands->audioPlayer();
    if (commands == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    float volume = player.volume();
    volume += 0.05;
    if (volume > 1.0) volume = 1.0;
    player.setVolume(volume);

    int ivolume = player.volume() * 254.0;
    out.println();
    printVolume(commands->audioPlayer(), out);
    return true;
  }

  /**
   * @brief Decrease volume
   */
  static bool cmd_voldown(telnet::Str& cmd,
                          telnet::Vector<telnet::Str> parameters, Print& out,
                          TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    AudioPlayer& player = commands->audioPlayer();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    float volume = player.volume();
    ;
    volume -= 0.05;
    if (volume < 0.0) volume = 0.0;
    player.setVolume(volume);

    int ivolume = player.volume() * 254.0;
    out.println();
    printVolume(commands->audioPlayer(), out);
    return true;
  }

  /**
   * @brief List stations
   */
  static bool cmd_list(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    AudioPlayer& player = commands->audioPlayer();
    AudioSource& source = player.audioSource();
    int original_idx = source.index();

    // Check if we're listing a specific index
    int specific_index = -1;
    if (parameters.size() > 0) {
      specific_index = parameters[0].toInt();
      TELNET_LOGI("Requested specific item index: %d", specific_index);
      // Convert from 1-based (user visible) to 0-based (internal)
      specific_index = specific_index > 0 ? specific_index - 1 : -1;
    }

    out.println();
    out.println("##CLI.LIST#");

    // List items based on priority
    if (input_files.size() > 0) {
      listFromInputFiles(out, specific_index);
    } else if (input_files_refs.size() > 0) {
      listFromInputFileRefs(out, specific_index);
    } else {
      listFromAudioSource(source, out, specific_index);
    }

    out.println("##CLI.LIST#");
    out.println();

    // Restore original position
    source.setIndex(original_idx);
    return true;
  }

  /**
   * @brief Switch to next station
   */
  static bool cmd_next(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    AudioPlayer& player = commands->audioPlayer();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    player.next();
    out.println();
    printPlaying(*commands, player, out);
    return true;
  }

  /**
   * @brief Switch to previous station
   */
  static bool cmd_prev(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    AudioPlayer& player = commands->audioPlayer();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    player.previous();
    out.println();
    printPlaying(*commands, player, out);
    return true;
  }

  // /**
  //  * @brief Immediately play a station
  //  */
  // static bool cmd_instant(telnet::Str& cmd,
  //                         telnet::Vector<telnet::Str> parameters, Print& out,
  //                         TinySerialServer* self) {
  //   KARadioCommands* commands = (KARadioCommands*)self->getReference();
  //   if (commands == nullptr || commands->p_player == nullptr) {
  //     TELNET_LOGE("%s", "KA-Radio communication not initialized");
  //     return false;
  //   }
  //   AudioPlayer& player = commands->audioPlayer();
  //   Str& name = commands->name;
  //   Str& url = commands->url;
  //   Str& path = commands->path;
  //   int& port = commands->port;

  //   if (parameters.size() == 1) {
  //     player.setPath(parameters[0].c_str());
  //   } else {
  //     Str result;
  //     if (port == 443)
  //       result.add("https://");
  //     else
  //       result.add("http://");
  //     result.add(url.c_str());
  //     result.add(path.c_str());
  //     TELNET_LOGE("instant: %s", url.c_str());
  //     player.setPath(result.c_str());
  //   }

  //   player.play();

  //   out.println();
  //   printPlaying(*commands, player, out);

  //   return true;
  // }

  /**
   * @brief Show current radio info
   */
  static bool cmd_info(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    int volume = commands->audioPlayer().volume() * 254.0;
    out.println();
    printPlaying(*commands, commands->audioPlayer(), out);
    return true;
  }

  // /**
  //  * @brief Get or set name
  //  */
  // static bool cmd_name(telnet::Str& cmd, telnet::Vector<telnet::Str>
  // parameters,
  //                      Print& out, TinySerialServer* self) {
  //   KARadioCommands* commands = (KARadioCommands*)self->getReference();
  //   if (commands == nullptr || commands->p_player == nullptr) {
  //     TELNET_LOGE("%s", "KA-Radio communication not initialized");
  //     return false;
  //   }

  //   Str& name = commands->name;
  //   // Format: cli.url [url]
  //   if (parameters.size() == 1) {
  //     // Set URL
  //     name = parameters[0].c_str();
  //   }

  //   out.println();
  //   out.print("##CLI.NAMESET#: ");
  //   out.println(name.c_str());

  //   return true;
  // }

  // /**
  //  * @brief Get or set url
  //  */
  // static bool cmd_url(telnet::Str& cmd, telnet::Vector<telnet::Str>
  // parameters,
  //                     Print& out, TinySerialServer* self) {
  //   KARadioCommands* commands = (KARadioCommands*)self->getReference();
  //   if (commands == nullptr || commands->p_player == nullptr) {
  //     TELNET_LOGE("%s", "KA-Radio communication not initialized");
  //     return false;
  //   }

  //   // Format: cli.path [path]
  //   Str& url = commands->url;
  //   if (parameters.size() == 1) {
  //     url = parameters[0].c_str();
  //   }

  //   out.println();
  //   out.print("##CLI.URLSET#: ");
  //   out.println(url.c_str());

  //   return true;
  // }

  // /**
  //  * @brief Get or set path
  //  */
  // static bool cmd_path(telnet::Str& cmd, telnet::Vector<telnet::Str>
  // parameters,
  //                      Print& out, TinySerialServer* self) {
  //   KARadioCommands* commands = (KARadioCommands*)self->getReference();
  //   if (commands == nullptr || commands->p_player == nullptr) {
  //     TELNET_LOGE("%s", "KA-Radio communication not initialized");
  //     return false;
  //   }

  //   // Format: cli.path [path]
  //   Str& path = commands->path;
  //   if (parameters.size() == 1) {
  //     path = parameters[0].c_str();
  //   }

  //   out.println();
  //   out.print("##CLI.PATHSET#: ");
  //   out.println(path.c_str());

  //   return true;
  // }

  // /**
  //  * @brief Get or set port
  //  */
  // static bool cmd_port(telnet::Str& cmd, telnet::Vector<telnet::Str>
  // parameters,
  //                      Print& out, TinySerialServer* self) {
  //   KARadioCommands* commands = (KARadioCommands*)self->getReference();
  //   if (commands == nullptr || commands->p_player == nullptr) {
  //     TELNET_LOGE("%s", "KA-Radio communication not initialized");
  //     return false;
  //   }

  //   // Format: cli.port [number]
  //   int& port = commands->port;
  //   if (parameters.size() == 1) {
  //     // Set port
  //     port = parameters[0].toInt();
  //     TELNET_LOGE("port %d", port);
  //   }

  //   out.println();
  //   out.print("##CLI.PORTSET#: ");
  //   out.println(port);

  //   return true;
  // }

  /**
   * @brief Get firmware version
   */
  static bool cmd_version(telnet::Str& cmd,
                          telnet::Vector<telnet::Str> parameters, Print& out,
                          TinySerialServer* self) {
    KARadioCommands* commands = (KARadioCommands*)self->getReference();
    if (commands == nullptr || commands->p_player == nullptr) {
      TELNET_LOGE("%s", "KA-Radio communication not initialized");
      return false;
    }

    out.println();
    out.println("Release: 2.4, Revision: 0, KaRadio32");
    return true;
  }

 protected:
  AudioPlayer* p_player = nullptr;
  Str name;
  Str url;
  Str path;
  int port;

  /// prints the urlset, portset and pathset
  static void printURL(KARadioCommands& commands, AudioPlayer& player,
                       Print& out) {
    const char* source_str = player.audioSource().toStr();
    int& result_port = commands.port;
    Str& result_path = commands.path;
    Str& result_url = commands.url;
    Str& result_name = commands.name;
    if (StrView(source_str).startsWith("http")) {
      Url url{source_str};
      result_port = url.port();
      result_path = "";
      result_url = source_str;
      result_name = source_str;
    } else {
      result_port = 0;
      result_path = source_str;
      result_url = "";
      result_name = source_str;
    }
    out.print("##CLI.URLSET#: ");
    out.println(result_url.c_str());
    out.print("##CLI.PORTSET#: ");
    out.println(result_port);
    out.print("##CLI.PATHSET#: ");
    out.println(result_path.c_str());
  }

  static void printVolume(AudioPlayer& player, Print& out) {
    int ivolume = player.volume() * 254.0;
    out.print("##CLI.VOL#:");
    out.print(ivolume);
    out.println();
  }

  static void printPlaying(KARadioCommands& commands, AudioPlayer& player,
                           Print& out) {
    printURL(commands, player, out);
    printVolume(player, out);
    out.println(player.isActive() ? "##CLI.PLAYING#" : "CLI.STOPPED");
  }

  /// print file name w/o extension
  static void printKey(Print& out, const char* name) {
      Str key{name};
    int start = key.lastIndexOf("/");
    if (start > 0) {
      int end = key.lastIndexOf(".");
      if (end < 0) end = key.length();
      key.substr(name, start + 1, end);
      out.print(key.c_str());
    } else {
      out.print(name);
    }
  }

  /**
   * @brief Print a single list item in the standard format
   *
   * @param out Output stream
   * @param index Item index (1-based for display)
   * @param name Item name/path
   */
  static void printListItem(Print& out, int index, const char* name) {
    out.print("#CLI.LISTINFO#: ");
    out.print(index);
    out.print(", ");
    printKey(out, name);
    out.print(", ");
    out.print(name);
    out.println();
  }

  /**
   * @brief List stations from input_files
   *
   * @param out Output stream
   * @param specific_index Index to show (-1 for all)
   */
  static void listFromInputFiles(Print& out, int specific_index) {
    if (specific_index > 0) {
      printListItem(out, specific_index, input_files[specific_index-1].c_str());
      return;
    }

    for (int i = 0; i < input_files.size(); i++) {
      printListItem(out, i + 1, input_files[i].c_str());

      // Check max limit
      if (max_input_files > 0 && i >= max_input_files - 1) {
        TELNET_LOGI("max limit reached: %d", max_input_files);
        break;
      }
    }
  }

  /**
   * @brief List stations from input_files_refs
   *
   * @param out Output stream
   * @param specific_index Index to show (-1 for all)
   */
  static void listFromInputFileRefs(Print& out, int specific_index) {
    if (specific_index > 0) {
      printListItem(out, specific_index, input_files_refs[specific_index-1]);
      return;
    }

    for (int i = 0; i < input_files_refs.size(); i++) {
      printListItem(out, i + 1, input_files_refs[i]);

      // Check max limit
      if (max_input_files > 0 && i >= max_input_files - 1) {
        TELNET_LOGI("max limit reached: %d", max_input_files);
        break;
      }
    }
  }

  /**
   * @brief List stations from AudioSource
   *
   * @param source The AudioSource to list from
   * @param out Output stream
   * @param specific_index Index to show (-1 for all)
   */
  static void listFromAudioSource(AudioSource& source, Print& out,
                                  int specific_index) {
    // If requesting a specific index, try to jump directly to it
    if (specific_index >= 0) {
      TELNET_LOGI("Item at index %d", specific_index);
      if (source.setIndex(specific_index)) {
        printListItem(out, specific_index + 1, source.toStr());
      } else {
        TELNET_LOGW("No item found at index %d", specific_index + 1);
      }
      return;
    }

    // Otherwise list all items
    int idx = 0;
    int count = 0;
    while (source.setIndex(idx)) {
      printListItem(out, idx + 1, source.toStr());
      count++;

      // Check max limit
      if (max_input_files > 0 && count >= max_input_files) {
        TELNET_LOGI("max limit reached: %d", max_input_files);
        break;
      }

      idx++;
    }
  }
};

}  // namespace telnet