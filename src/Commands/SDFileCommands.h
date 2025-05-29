#pragma once
#include <SD.h>

#include "TinySerialServer.h"

namespace telnet {

// Current directory
static String current_dir = "/";  // Default to root directory
static int max_file_length = 60;

/**
 * @brief Class providing SD card file commands for TinyTelnetServer
 *
 * Implements common file operations:
 * - ls: List directory contents
 * - cat: Display file contents
 * - mv: Move/rename files
 * - rm: Remove files/directories
 * - mkdir: Create directories
 * - cp: Copy files
 * - df: Show disk space information
 * - touch: Create empty files or update timestamps
 * - write: Write text to files
 * - head: Display first lines of a file
 * - pwd: print current directopy
 * - cd: change directory
 *
 * The SD API does nto support cd and pwd, so we simulate this functionality.
 *
 * @copyright GPLv3
 */

class SDFileCommands {
 public:
  SDFileCommands() = default;
  /// Default constructor
  SDFileCommands(TinySerialServer& server) { addCommands(server); }

  /// Defines the maximum length for file names
  void setMaxFileLength(int len) {
    max_file_length = len;
  }

  /**
   *
   * @brief Register file commands with the server
   *
   * @param server The TinySerialServer to register commands with
   */
  void addCommands(TinySerialServer& server) {
    server.addCommand("ls", cmd_ls, "[DIRECTORY]");
    server.addCommand("cat", cmd_cat, "FLENAME");
    server.addCommand("mv", cmd_mv, "SOURCE DESTINATION");
    server.addCommand("cp", cmd_cp, "SOURCE DESTINATION");
    server.addCommand("rm", cmd_rm, "FILENAME");
    server.addCommand("mkdir", cmd_mkdir, "DIRECTORY_NAME");
    server.addCommand("df", cmd_df);
    server.addCommand("touch", cmd_touch, "FILENAME");
    server.addCommand("write", cmd_write, "FILENAME TEXT");
    server.addCommand("head", cmd_head, "[-n lines] FILENAME");
    server.addCommand("cd", cmd_cd, "DIRECTORY");
    server.addCommand("pwd", cmd_pwd);
  }

  /**
   * @brief Create empty files or update timestamps
   */
  static bool cmd_touch(telnet::Str& cmd,
                        telnet::Vector<telnet::Str> parameters, Print& out,
                        TinySerialServer* self) {
    if (parameters.size() == 0 || parameters[0].length() == 0) {
      out.println("Usage: touch <filename>");
      return false;
    }

    String filename = resolveName(parameters[0].c_str());

    if (SD.exists(filename.c_str())) {
      // File exists, open and close to update timestamp
      File file = SD.open(filename.c_str(), FILE_WRITE);
      if (!file) {
        out.print("Error: Could not update file: ");
        out.println(filename);
        return false;
      }
      file.close();
      out.print("Updated timestamp on: ");
      out.println(filename.c_str());
    } else {
      // Create new empty file
      File file = SD.open(filename.c_str(), FILE_WRITE);
      if (!file) {
        out.print("Error: Could not create file: ");
        out.println(filename.c_str());
        out.println();
        return false;
      }
      file.close();
      out.print("Created empty file: ");
      out.println(filename.c_str());
    }
    out.println();

    return true;
  }

  /**
   * @brief Write text to a file
   */
  static bool cmd_write(telnet::Str& cmd,
                        telnet::Vector<telnet::Str> parameters, Print& out,
                        TinySerialServer* self) {
    if (parameters.size() < 2 || parameters[0].length() == 0) {
      out.println("Usage: write <filename> <text>");
      out.println();
      return false;
    }

    // resolve file name
    String fstr = resolveName(parameters[0].c_str());
    const char* filename = fstr.c_str();

    // Open file for writing (overwrites existing content)
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
      out.print("Error: Could not open file for writing: ");
      out.println(filename);
      out.println();
      return false;
    }

    // Write each parameter as a separate line
    for (size_t i = 1; i < parameters.size(); i++) {
      file.println(parameters[i].c_str());
    }

    file.close();
    out.print("Written to: ");
    out.println(filename);
    out.println();
    return true;
  }

  /**
   * @brief Show first N lines of a file
   */
  static bool cmd_head(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                       Print& out, TinySerialServer* self) {
    if (parameters.size() != 1 || parameters[0].length() == 0) {
      out.println("Usage: head [-n lines] <filename>");
      return false;
    }

    int numLines = 10;  // Default

    // resolve file name
    String fstr = resolveName(parameters[0].c_str());
    const char* filename = fstr.c_str();

    // Check for -n parameter
    if (parameters.size() > 2 && parameters[0] == "-n" &&
        parameters[1].length() > 0) {
      numLines = atoi(parameters[1].c_str());
      filename = parameters[2].c_str();
    }

    if (!SD.exists(filename)) {
      out.print("Error: File not found: ");
      out.println(filename);
      return false;
    }

    File file = SD.open(filename);
    if (!file) {
      out.print("Error: Could not open file: ");
      out.println(filename);
      return false;
    }

    if (file.isDirectory()) {
      out.print(filename);
      out.println(" is a directory");
      file.close();
      return false;
    }

    out.print("First ");
    out.print(numLines);
    out.print(" lines of ");
    out.println(filename);

    // Read and display first N lines
    int lineCount = 0;
    char buffer[128];
    int bufIndex = 0;

    while (file.available() && lineCount < numLines) {
      char c = file.read();
      if (c == '\n' || bufIndex >= sizeof(buffer) - 1) {
        buffer[bufIndex] = '\0';
        out.println(buffer);
        bufIndex = 0;
        if (c == '\n') lineCount++;
      } else if (c != '\r') {
        buffer[bufIndex++] = c;
      }
    }

    // Output any partial line at the end
    if (bufIndex > 0) {
      buffer[bufIndex] = '\0';
      out.println(buffer);
    }

    file.close();
    out.println("*** END ***");
    out.println();

    return true;
  }

  /**
   * @brief Create a new directory
   */
  static bool cmd_mkdir(telnet::Str& cmd,
                        telnet::Vector<telnet::Str> parameters, Print& out,
                        TinySerialServer* self) {
    if (parameters.size() != 1 || parameters[0].length() == 0) {
      out.println("Usage: mkdir <directory_name>");
      out.println();
      return false;
    }

    // resolve directory name
    String fstr = resolveName(parameters[0].c_str());
    const char* dirName = fstr.c_str();

    if (SD.exists(dirName)) {
      out.print("Error: ");
      out.print(dirName);
      out.println(" already exists");
      out.println();
      return false;
    }

    if (SD.mkdir(dirName)) {
      out.print("Created directory: ");
      out.println(dirName);
      out.println();
      return true;
    } else {
      out.print("Error: Failed to create directory ");
      out.println(dirName);
      out.println();
      return false;
    }
  }

  /**
   * @brief Copy a file
   */
  static bool cmd_cp(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Print& out, TinySerialServer* self) {
    if (parameters.size() != 2 || parameters[0].length() == 0 ||
        parameters[1].length() == 0) {
      out.println("Usage: cp <source> <destination>");
      out.println();
      return false;
    }

    // resolve directory name
    String src_str = resolveName(parameters[0].c_str());
    String dst_str = resolveName(parameters[1].c_str());
    const char* source = src_str.c_str();
    const char* destination = dst_str.c_str();

    if (!SD.exists(source)) {
      out.print("Error: Source file not found: ");
      out.println(source);
      out.println();
      return false;
    }

    File sourceFile = SD.open(source);
    if (!sourceFile) {
      out.print("Error: Could not open source file: ");
      out.println(source);
      out.println();
      return false;
    }

    if (sourceFile.isDirectory()) {
      out.println("Error: Cannot copy directories (use cp -r for that)");
      sourceFile.close();
      out.println();
      return false;
    }

    File destFile = SD.open(destination, FILE_WRITE);
    if (!destFile) {
      out.print("Error: Could not create destination file: ");
      out.println(destination);
      out.println();
      sourceFile.close();
      return false;
    }

    // Copy file contents
    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];

    while (sourceFile.available()) {
      size_t bytesRead = sourceFile.read(buffer, bufferSize);
      if (bytesRead > 0) {
        destFile.write(buffer, bytesRead);
      }
    }

    sourceFile.close();
    destFile.close();

    out.print("Copied '");
    out.print(source);
    out.print("' to '");
    out.print(destination);
    out.println("'");
    out.println();
    return true;
  }

  /**
   * @brief Show SD card space information
   */
  static bool cmd_df(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Print& out, TinySerialServer* self) {
    // Get total and used space
    long long unsigned int totalBytes = SD.totalBytes();
    long long unsigned int usedBytes = SD.usedBytes();
    long long unsigned int freeBytes = totalBytes - usedBytes;

    float totalGB = totalBytes / (1024.0 * 1024.0 * 1024.0);
    float usedGB = usedBytes / (1024.0 * 1024.0 * 1024.0);
    float freeGB = freeBytes / (1024.0 * 1024.0 * 1024.0);

    out.println("SD Card Space Information");

    char buffer[64];

    snprintf(buffer, sizeof(buffer), "Total Space: %.2f GB (%llu bytes)",
             totalGB, totalBytes);
    out.println(buffer);

    snprintf(buffer, sizeof(buffer), "Used Space:  %.2f GB (%lld bytes)",
             usedGB, usedBytes);
    out.println(buffer);

    snprintf(buffer, sizeof(buffer), "Free Space:  %.2f GB (%lld bytes)",
             freeGB, freeBytes);
    out.println(buffer);

    float usedPercent = (usedBytes * 100.0) / totalBytes;
    snprintf(buffer, sizeof(buffer), "Used: %.1f%%", usedPercent);
    out.println(buffer);
    out.println();

    return true;
  }

  /**
   * @brief List files in a directory
   */
  static bool cmd_ls(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Print& out, TinySerialServer* self) {
    // Default to root directory if no path is specified
    String path_str = current_dir;
    const char* path = path_str.c_str();
    if (parameters.size() == 1 && parameters[0].length() > 0) {
      path_str = resolveName(parameters[0].c_str());
      path = path_str.c_str();
    }

    // Open directory
    File dir = SD.open(path);
    if (!dir) {
      out.print("Error: Could not open directory ");
      out.println(path);
      out.println();
      return false;
    }

    if (!dir.isDirectory()) {
      out.print(path);
      out.println(" is not a directory");
      dir.close();
      out.println();
      return false;
    }

    // List files
    out.print("Directory listing of: ");
    out.println(path);
    out.println();
    out.print("Name");
    for (int j=0;j<max_file_length - 4;j++) {
      out.print(" ");
    }
    out.println("Type      Size");
    

    int start = 0;

    File entry;
    while (entry = dir.openNextFile()) {
      if (StrView(entry.name()).startsWith(path)) {
        start = strlen(path);
        if (entry.name()[start] == '/') 
          start++;
      }
      // Skip hidden files
      if (entry.name()[start] == '.') {
        continue;  
      }
      // Print filename with padding
      const char* name = entry.name() + start;
      out.print(name);

      // Padding
      int nameLen = strlen(name);
      for (int i = nameLen; i < max_file_length; i++) {
        out.print(" ");
      }

      // Print size or <DIR>
      if (entry.isDirectory()) {
        out.println("<DIR>");
      } else {
        // Print file size with padding
        out.print("      ");
        char sizeStr[16];
        snprintf(sizeStr, sizeof(sizeStr), "%8lu", entry.size());
        out.println(sizeStr);
      }

      entry.close();
    }

    dir.close();
    out.println("*** END ***");
    out.println();
    return true;
  }

  /**
   * @brief Display contents of a file
   */
  static bool cmd_cat(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                      Print& out, TinySerialServer* self) {
    // Require a filename parameter
    if (parameters.size() != 1 || parameters[0].length() == 0) {
      out.println("Usage: cat <filename>");
      out.println();

      return false;
    }

    // resolve file name
    String fstr = resolveName(parameters[0].c_str());
    const char* filename = fstr.c_str();

    // Check if file exists
    if (!SD.exists(filename)) {
      out.print("Error: File not found: ");
      out.println(filename);
      out.println();
      return false;
    }

    // Open file
    File file = SD.open(filename);
    if (!file) {
      out.print("Error: Could not open file ");
      out.println(filename);
      out.println();
      return false;
    }

    // Check if it's a directory
    if (file.isDirectory()) {
      out.print(filename);
      out.println(" is a directory");
      file.close();
      out.println();
      return false;
    }

    // Read and display the file
    out.print("File: ");
    out.println(filename);

    // Read file contents
    while (file.available()) {
      char buffer[64];
      size_t bytesRead = file.readBytes(buffer, sizeof(buffer) - 1);

      if (bytesRead > 0) {
        buffer[bytesRead] = '\0';  // Null-terminate the string
        out.write(buffer, bytesRead);
      }
    }

    file.close();
    out.println("*** END ***");
    out.println();
    return true;
  }

  /**
   * @brief Move/rename a file
   */
  static bool cmd_mv(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Print& out, TinySerialServer* self) {
    // Check parameters
    if (parameters.size() != 2 || parameters[0].length() == 0 ||
        parameters[1].length() == 0) {
      out.println("Usage: mv <source> <destination>");
      out.println();
      return false;
    }

    // resolve directory name
    String src_str = resolveName(parameters[0].c_str());
    String dst_str = resolveName(parameters[1].c_str());
    const char* source = src_str.c_str();
    const char* destination = dst_str.c_str();

    // Check if source file exists
    if (!SD.exists(source)) {
      out.print("Error: Source file not found: ");
      out.println(source);
      out.println();
      return false;
    }

    // Check if destination already exists
    if (SD.exists(destination)) {
      out.print("Error: Destination already exists: ");
      out.println(destination);
      out.println();
      return false;
    }

    // Open source file
    File sourceFile = SD.open(source);
    if (!sourceFile) {
      out.print("Error: Could not open source file: ");
      out.println(source);
      out.println();
      return false;
    }

    // Don't try to move directories
    if (sourceFile.isDirectory()) {
      out.println("Error: Moving directories is not supported");
      sourceFile.close();
      out.println();
      return false;
    }

    // Create destination file
    File destFile = SD.open(destination, FILE_WRITE);
    if (!destFile) {
      out.print("Error: Could not create destination file: ");
      out.println(destination);
      sourceFile.close();
      out.println();
      return false;
    }

    // Copy contents
    while (sourceFile.available()) {
      const size_t bufferSize = 512;
      uint8_t buffer[bufferSize];
      size_t bytesRead = sourceFile.read(buffer, bufferSize);
      if (bytesRead > 0) {
        destFile.write(buffer, bytesRead);
      }
    }

    // Close files
    sourceFile.close();
    destFile.close();

    // Remove source file
    if (SD.remove(source)) {
      out.print("Moved '");
      out.print(source);
      out.print("' to '");
      out.print(destination);
      out.println("'");
      out.println();
      return true;
    } else {
      out.println("Error: File copied but could not remove source file");
      out.println();
      return false;
    }
  }

  /**
   * @brief Remove a file or directory
   */
  static bool cmd_rm(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Print& out, TinySerialServer* self) {
    // Check for recursive flag
    bool recursive = false;
    int fileIndex = 0;

    if (parameters.size() > 0 && parameters[0] == "-r") {
      recursive = true;
      fileIndex = 1;
    }

    // Check parameters
    if (parameters.size() <= fileIndex || parameters[fileIndex].length() == 0) {
      out.println("Usage: rm [-r] <filename>");
      out.println();
      return false;
    }

    /// resolve file name
    String fstr = resolveName(parameters[fileIndex].c_str());
    const char* filename = fstr.c_str();

    // Check if file exists
    if (!SD.exists(filename)) {
      out.print("Error: File not found: ");
      out.println(filename);
      out.println();
      return false;
    }

    // Check if it's a directory
    File file = SD.open(filename);
    bool isDir = file.isDirectory();
    file.close();

    if (isDir && !recursive) {
      out.println("Error: Cannot remove directory without -r flag");
      out.println();
      return false;
    }

    if (isDir && recursive) {
      // Remove directory recursively
      if (!removeDirectory(filename, out)) {
        out.print("Error: Failed to remove directory: ");
        out.println(filename);
        out.println();
        return false;
      }
    } else {
      // Remove file
      if (!SD.remove(filename)) {
        out.print("Error: Failed to remove file: ");
        out.println(filename);
        out.println();
        return false;
      }
    }

    out.print("Removed ");
    out.println(filename);
    out.println();
    return true;
  }

  static bool cmd_pwd(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                      Print& out, TinySerialServer* self) {
    // check parameters
    if (parameters.size() > 0) {
      out.println("Usage: pwd (no parameters expected)");
      out.println();
      return false;
    }

    // Print current working directory
    out.println(current_dir.c_str());
    out.println();
    return true;
  }

  static bool cmd_cd(telnet::Str& cmd, telnet::Vector<telnet::Str> parameters,
                     Print& out, TinySerialServer* self) {
    // check parameters
    if (parameters.size() == 0 || parameters[0].length() == 0) {
      out.println("Usage: cd <pathnname>");
      out.println();
      return false;
    }

    // Change current working directory
    String tmp_str = resolveName(parameters[0].c_str());
    const char* file = tmp_str.c_str();

    File dir = SD.open(file);
    if (!dir) {
      out.print("Error: Could not change directory to ");
      out.println(file);
      out.println();
      return false;
    }
    if (!dir.isDirectory()) {
      out.print("Error: ");
      out.print(file);
      out.println(" is not a directory");
      dir.close();
      out.println();
      return false;
    }
    dir.close();
    out.print("Changed directory to: ");
    out.println(file);
    current_dir = file;
    out.println();
    return true;
  }

 protected:
  /// Resolve relative path name
  static String resolveName(const char* path) {
    if (String(path).startsWith("/")) {
      return path;
    }
    if (String(path) == ".") return current_dir;
    if (String(path).startsWith("./")) {
      path += 2;  // Skip "./"
      return current_dir + (current_dir.endsWith("/") ? "" : "/") + path;
    }
    if (String(path).startsWith("../")) {
      String parent = dirUp(current_dir.c_str());
      path += 3;  // Skip "../"
      return parent + (parent.endsWith("/") ? "" : "/") + path;
    }
    if (String(path) == "..") return dirUp(current_dir.c_str());
    return current_dir + (current_dir.endsWith("/") ? "" : "/") + path;
  }

  /// Move one path level up
  /// Returns the parent directory of the current directory
  static String dirUp(const char* current) {
    String result;
    String current_dir(current);
    int lastSlash = current_dir.lastIndexOf("/");
    if (lastSlash > 0) {
      result = current_dir.substring(0, lastSlash);
    } else {
      result = "/";
    }
    return result;
  }
  /**
   * @brief Remove a directory and all its contents recursively
   *
   * @param dirPath Path to directory
   * @param out Output stream for messages
   * @return true if successful
   */
  static bool removeDirectory(const char* dirPath, Print& out) {
    File dir = SD.open(dirPath);
    if (!dir) {
      return false;
    }

    if (!dir.isDirectory()) {
      dir.close();
      return SD.remove(dirPath);
    }

    // Remove all files in directory
    File file;
    while (file = dir.openNextFile()) {
      char filePath[100];  // Adjust size as needed

      // Construct full path
      strcpy(filePath, dirPath);

      // Add slash if needed
      if (dirPath[strlen(dirPath) - 1] != '/') {
        strcat(filePath, "/");
      }

      strcat(filePath, file.name());

      if (file.isDirectory()) {
        // Recursively remove subdirectory
        file.close();
        if (!removeDirectory(filePath, out)) {
          dir.close();
          return false;
        }
      } else {
        // Remove file
        file.close();
        if (!SD.remove(filePath)) {
          out.print("Failed to remove: ");
          out.println(filePath);
          dir.close();
          return false;
        }
      }
    }

    dir.close();

    // Finally remove the empty directory
    return SD.rmdir(dirPath);
  }
};

}  // namespace telnet