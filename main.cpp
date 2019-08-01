#include <iostream>
#include "libserial.hh"
#include "libshmem.hh"

#define SERIAL_UNLOCK 0x4E

typedef struct __attribute__ ((packed)) pedals_t {
  float clutch;
  float brake;
  float accelerator;
} pedals_t;

typedef union packet_t {
  pedals_t pedals;
  uint8_t buffer[sizeof(pedals)];
} packet_t;


int main() {
  packet_t packet;
  memset(packet.buffer, 0, sizeof(pedals_t));
  
  am::Serial serial("/dev/ttyS3", 115200);
  am::SharedMemory<float, 3> shmem("/serial_bus");
  serial.open();
  serial.write(SERIAL_UNLOCK);
  usleep(100);
  
  while (true) {
    std::size_t read_bytes = serial.read(packet.buffer, sizeof(pedals_t));
    if (read_bytes != sizeof(pedals_t))
      std::cerr << "Wrong number of bytes read" << std::endl;
    
    shmem[0] = packet.pedals.accelerator;
    shmem[1] = packet.pedals.brake;
    shmem[2] = packet.pedals.clutch;
  }
  return 0;
}
