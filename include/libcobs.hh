#ifndef COBS_HPP
#define COBS_HPP

#include <cstdlib>
#include <cstdint>
#include <cstring>

/*! Macro for declaration of struct to insert specific packed data into 
 *  the union of the COBS class. The resulting struct will be packed
 *  X is the struct typedefinition and Y is the body of the struct.
 */
#define COBS_DEFINE_STRUCT(X, Y) struct X Y __attribute__((packed)); typedef struct X X;

/*! This union combines the data to send and a buffer to send the data. The struct
 * is accessed through the data field.
 */
template <class STRUCT>
union Packet {
  STRUCT data;                          /**< The struct or the data field (like an array) */
  uint8_t buffer[sizeof(STRUCT)]; /**< The buffer is the char array equivalent of the data */
};

/*! Read Callback: this is the prototype of function that should return a char at the time.
 *  It has 2 arguments. The first, in the form of a size_t is the current index in the
 * reading, the second is a payload (arbitrary content) in the form a void pointer.
 */
typedef uint8_t (*COBS_read_callback)(void *);
/*! Write Calback: this is the prototype of function that should accept one char at the
 *  time. The callback has 3 arguments: the index of currently read char, the char itself,
 *  and a payload (arbitrary content) in the form a void pointer.
 */
typedef void (*COBS_write_callback)(const uint8_t, void *);

/*! COBS Class
 *  The COBS class is an abstraction of the Consistent Overhead Byte Stuffing, an algorithm
 *  for encoding data byte, in such a way the receiver should not handle necessarely the
 *  dimension of the packet (the packet is ended when it contains a 0x00). The class is
 *  abstracted over a template that contains the actual data.
 */
template <class STRUCT>
class COBS {
  
  union Packet<STRUCT> packet; /**< The actual package */
  uint8_t outbound[sizeof(STRUCT) + 1];    /**< The container for the COBS encoded package */

  COBS_read_callback read_clbk;
  COBS_write_callback write_clbk;
  void * payload;
  
  void stuff(void);   /**< The encoding function */
  void unstuff(void); /**< The decoding function */
  
  public:
  /*! The class constructor */
  COBS() : read_clbk(nullptr), write_clbk(nullptr), payload(nullptr) {};
  
  COBS(const COBS_read_callback & read) : 
    read_clbk(read), write_clbk(nullptr), payload(nullptr) {};

  COBS(const COBS_write_callback & write) : 
    read_clbk(nullptr), write_clbk(write), payload(nullptr) {};
  
  COBS(const COBS_read_callback & read, const COBS_write_callback & write) : 
    read_clbk(read), write_clbk(write), payload(nullptr) {};
  
  COBS(const COBS_read_callback & read, void * payload_ptr) : 
    read_clbk(read), write_clbk(nullptr), payload(payload_ptr) {};

  COBS(const COBS_write_callback & write, void * payload_ptr) : 
    read_clbk(nullptr), write_clbk(write), payload(payload_ptr) {};
  
  COBS(const COBS_read_callback & read, const COBS_write_callback & write, void * payload_ptr) : 
    read_clbk(read), write_clbk(write), payload(payload_ptr) {};

  /*! The class destructor */
  ~COBS() {}
  
  /*! Return the current COBS encoded package */
  const uint8_t * get_packet() const { 
    return outbound; 
  }
  
  /*! Get the size of the handled data */
  const size_t size() const { return sizeof(STRUCT); } 
  
  /*! Read from the transmission channel the COBS package */
  bool read(STRUCT & data);
  /*! Write in the transmission channel the COBS package */
  void write(const STRUCT & data);
};

#define FinishBlock(X) (*code_ptr = (X), code_ptr = dst++, code = 0x01)
template <class STRUCT>
void COBS<STRUCT>::stuff(void) {
  memset(outbound, 0, size() + 1);
  
  const uint8_t* end = packet.buffer + size();
  const uint8_t* ptr = packet.buffer;
  uint8_t * dst = outbound;
  
  uint8_t * code_ptr = dst++;
  uint8_t code = 0x01;

  while (ptr < end) {
    if (*ptr == 0)
      FinishBlock(code);
    else {
      *(dst)++ = *ptr;
      if (++code == 0xFF)
        FinishBlock(code);
    }
    ptr++;
  }
  FinishBlock(code);
}

template <class STRUCT>
void COBS<STRUCT>::unstuff(void) {
  const uint8_t* end = outbound + size() + 1;
  uint8_t * ptr = outbound;
  uint8_t * dst = packet.buffer;
  
  while (ptr < end) {
    int code = *ptr++;
    for (int i = 1; ptr < end && i < code; i++)
      *dst++ = *ptr++;
    if (code < 0xFF)
      *dst++ = 0;
  }
}

template <class STRUCT>
bool COBS<STRUCT>::read(STRUCT & data) {
  if (!read_clbk) { return false; }

  memset(packet.buffer, 0x00, size());
  memset(outbound, 0x00, size() + 1);

  size_t i = 0;
  uint8_t c;
  while((c = read_clbk(payload)) != 0x00) {
    outbound[i++] = c;
    if (i >= (size() + 1))
      break;
  }
  unstuff();
  memcpy(&data, &(packet.data), size());
  return (i == (size() + 1));
}

template <class STRUCT>
void COBS<STRUCT>::write(const STRUCT & data) {
  if (!write_clbk) { return; }

  memcpy(&(packet.data), &data, size());
  memset(outbound, 0x00, size() + 1);
  
  stuff();
  size_t i = 0;
  while(outbound[i] != 0x00) {
    write_clbk(outbound[i++], payload);
    if (i >= (size() + 1))
      break;
  }
}

#endif /* COBS_HPP */