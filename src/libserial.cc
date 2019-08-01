#include "libserial.hh"

namespace am {

const std::map<std::size_t, std::size_t> Serial::baudrates = {
  { 0, B0 }, { 50, B50 }, { 75, B75 }, { 110, B110 }, { 134, B134 },
  { 150, B150 }, { 200, B200 }, { 300, B300 },
  { 600, B600 }, { 1200, B1200 }, { 1800, B1800 }, 
  { 2400, B2400 }, { 4800, B4800 }, { 9600, B9600 },
  { 19200, B19200 }, { 38400, B38400 }, { 57600, B57600 },
  { 115200, B115200 }, { 230400, B230400 }, { 460800, B460800 }
};

char Serial::error_string[LIBSERIAL_ERRORSTRING_LENGTH] = {0};

std::size_t Serial::get_baud(std::size_t value) {
  try {
    return Serial::baudrates.at(value);
  } catch (std::out_of_range) {
    std::size_t new_value = 0;
    for (auto it = Serial::baudrates.begin(); it != Serial::baudrates.end(); ++it) {
      int new_delta = std::abs(static_cast<int>(it->first) - static_cast<int>(value));
      int old_delta = std::abs(static_cast<int>(new_value) - static_cast<int>(value));
      if (new_delta <= old_delta)
        new_value = it->first;
    }
    std::cerr << "The requested key is invalid, using nearest: B" << new_value << std::endl;
    return Serial::baudrates.at(new_value);
  }
}

bool Serial::file_exists(const std::string & filename) {
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}

void Serial::raise_c_exception(const std::string & error_string) {
  memset(Serial::error_string, 0, LIBSERIAL_ERRORSTRING_LENGTH);
  snprintf(Serial::error_string, LIBSERIAL_ERRORSTRING_LENGTH, 
          "[ERROR.%d] Error while %s: %s", errno, error_string.c_str(), strerror(errno));
  throw std::runtime_error(Serial::error_string);
}


void Serial::open() {
  if (_serial_port > 0) return;

  _serial_port = ::open(_filename.c_str(), O_RDWR);
  if (_serial_port < 0) Serial::raise_c_exception("opening serial port");

  struct termios tty;
  memset(&tty, 0, sizeof(tty));

  // Read in existing settings, and handle any error
  if(tcgetattr(_serial_port, &tty) != 0)
    Serial::raise_c_exception("reading port settings");

  tty.c_cflag &= ~PARENB;        // Clear parity bit, disabling parity (most common)
  tty.c_cflag &= ~CSTOPB;        // Clear stop field, only one stop bit used in communication (most common)
  tty.c_cflag |= CS8;            // 8 bits per byte (most common)
  tty.c_cflag &= ~CRTSCTS;       // Disable RTS/CTS hardware flow control (most common)
  tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
  tty.c_lflag &= ~ICANON;        // Turn off canonical mode
  tty.c_lflag &= ~ECHO;          // Disable echo
  tty.c_lflag &= ~ECHOE;         // Disable erasure
  tty.c_lflag &= ~ECHONL;        // Disable new-line echo
  tty.c_lflag &= ~ISIG;          // Disable interpretation of INTR, QUIT and SUSP
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);                          // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
  tty.c_oflag &= ~OPOST;         // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR;         // Prevent conversion of newline to carriage return/line feed
  tty.c_cc[VTIME] = 250;
  tty.c_cc[VMIN] = 0;

  cfsetispeed(&tty, _baudrate);
  cfsetospeed(&tty, _baudrate);
  if (tcsetattr(_serial_port, TCSANOW, &tty) != 0)
    Serial::raise_c_exception("writing port settings");

  if (flock(_serial_port, LOCK_EX | LOCK_NB) == -1) 
    throw std::runtime_error("Serial port " + _filename + "is already locked by another process");

  flush();
}

void Serial::close() {
  flock(_serial_port, LOCK_UN | LOCK_NB);
  ::close(_serial_port);
  _serial_port = 0;
}

void Serial::flush() {
  sleep(1);
  tcflush(_serial_port, TCIOFLUSH);
  sleep(1);
}

uint8_t Serial::read() {
  uint8_t value;
  if (read(&value, 1) == 1)
    return value;
  throw std::runtime_error("Cannot read from serial");
}

std::size_t Serial::read(uint8_t * buffer, std::size_t size) {
  if (_serial_port == 0)
    throw std::runtime_error("Read called before opening the serial port");

  if (!buffer)
    throw std::invalid_argument("Invalid buffer for serial read");

  // memset(buffer, 0, size);
  std::size_t read_bytes = ::read(_serial_port, buffer, size);
  if (read_bytes < 0)
    Serial::raise_c_exception("reading serial port");
  return read_bytes;
}

std::size_t Serial::write(uint8_t byte) {
  return write(&byte, 1);
}

std::size_t Serial::write(const uint8_t * buffer, std::size_t size) {
  if (_serial_port == 0)
    throw std::runtime_error("Read called before opening the serial port");
  if (!buffer)
    throw std::invalid_argument("Invalid buffer for serial write");

  std::size_t write_bytes = ::write(_serial_port, buffer, size);
  if (write_bytes < 0)
    Serial::raise_c_exception("writing serial port");
  return write_bytes; 
}

} // libserial namespace