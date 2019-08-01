#pragma once

#include <string>
#include <map>
#include <stdexcept>
#include <iostream>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#define LIBSERIAL_ERRORSTRING_LENGTH 512

namespace am {

class Serial {
  static const std::map<std::size_t, std::size_t> baudrates;
  static char error_string[LIBSERIAL_ERRORSTRING_LENGTH];
  
  std::string _filename;
  std::size_t _baudrate;
  int _serial_port;

  static std::size_t get_baud(std::size_t value);
  static bool file_exists(const std::string & filename);
  static void raise_c_exception(const std::string & error_string);


  public:
  Serial(const std::string & filename, const std::size_t baudrate) : _serial_port(0) {
    if (!Serial::file_exists(filename)) {
      throw std::invalid_argument("The file '" + filename + "' does not exists");
    }
    _filename = filename;
    _baudrate = Serial::get_baud(baudrate);
  }

  ~Serial() {
    if (_serial_port)
      close();
  }

  void open();
  void close();
  void flush();
  uint8_t read();
  std::size_t read(uint8_t * buffer, std::size_t size);
  std::size_t write(uint8_t byte);
  std::size_t write(const uint8_t * buffer, std::size_t size);
};

}