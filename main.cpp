#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <atomic>
#include <thread>

#include "libcobs.hh"
#include "libserial.hh"
#include "libshmem.hh"

#define SERIAL_UNLOCK 0x4F

COBS_DEFINE_STRUCT(pedals_t, {
  float clutch;
  float brake;
  float accelerator;
});

typedef struct options_t {
  std::string serial_file;
  std::string shmem_file;
} options_t;

uint8_t serial_read(void * serial_ptr);
void process_args(options_t& options, int argc, char** argv);
void print_greetings(const options_t & options);
void stop_requested();

static std::atomic<bool> stop(false);

//    ___       _   _            ___             _           
//   / _ \ _ __| |_(_)___ _ _   | _ \__ _ _ _ __(_)_ _  __ _ 
//  | (_) | '_ \  _| / _ \ ' \  |  _/ _` | '_(_-< | ' \/ _` |
//   \___/| .__/\__|_\___/_||_| |_| \__,_|_| /__/_|_||_\__, |
//        |_|                                          |___/ 
void print_help() {
  std::cout <<
    " --input filename\n"
    " -i filename\n"
    "                     Set the input file: serial port, it will be set to 115200 bps)\n"
    " --output filename\n"
    " -o filename\n"
    "                     Set the output file: shared memory, it is relative to the posix shared memory dir)\n";
    exit(1);
}

void print_greetings(const options_t& options) {
  std::cout << "Starting:" << std::endl;
  std::cout << "          Serial file: " << options.serial_file << std::endl;
  std::cout << "   Shared Memory file: " << options.shmem_file << std::endl;
  std::cout << "press a key to terminate, AFTER stopping simulation" << std::endl;
}

void process_args(options_t& options, int argc, char** argv) {
  const char * const short_opts = "i:o:h";
  const option long_opts[] = {
    {"input", required_argument, nullptr, 'i'},
    {"output", required_argument, nullptr, 'o'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, no_argument, nullptr, 0}
  };

  while(!stop) {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    if (opt == -1) break;

    switch (opt) {
      case 'h':
      case '?':
        print_help();
        break;
      case 'i':
        options.serial_file = std::string(optarg);
        break;
      case 'o':
        options.shmem_file = std::string(optarg);
        break;  
    }
  }
}



//    ___                       ___ _ _    _   
//   / __|___ _ __  _ __  ___  / __| | |__| |__
//  | (__/ _ \ '  \| '  \(_-< | (__| | '_ \ / /
//   \___\___/_|_|_|_|_|_/__/  \___|_|_.__/_\_\
                                            
uint8_t serial_read(void * serial_ptr) {
  return reinterpret_cast<am::Serial *>(serial_ptr)->read();
}

//   __  __      _      
//  |  \/  |__ _(_)_ _  
//  | |\/| / _` | | ' \ 
//  |_|  |_\__,_|_|_||_|

void stop_requested() {
  std::cin.get();
  stop.store(true);
}


int main(int argc, char** argv) {
  options_t options;
  pedals_t data = { 0 };

  process_args(options, argc, argv);

  std::thread stop_question(stop_requested);

  am::Serial serial(options.serial_file, 115200);
  am::SharedMemory<float, 3> shmem(options.shmem_file);
  COBS<pedals_t> cobs_receiver(serial_read, reinterpret_cast<void*>(&serial));
  
  serial.open();
  print_greetings(options);

  serial.write(SERIAL_UNLOCK);
  while (!stop.load()) {
    if (!cobs_receiver.read(data)) {
      std::cerr << "Invalid reading, skipping" << std::endl;
      continue;
    }
    // std::cout << data.accelerator << ", " << data.brake << ", " << data.clutch << std::endl;
    shmem[0] = data.accelerator;
    shmem[1] = data.brake;
    shmem[2] = data.clutch;
  }

  stop_question.join();
  return 0;
}
