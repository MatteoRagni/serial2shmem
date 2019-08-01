#pragma once

#include <string>
#include <stdexcept>

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

#include <fcntl.h> 
#include <sys/mman.h> 
#include <unistd.h>

#define SHMEM_ERRORSTRING_LENGTH 512

namespace am {

  typedef int shmem_file_t;

  template <typename T, const std::size_t size>
  class SharedMemory {
    std::string _filename;
    shmem_file_t _file_handle;
    T * _data_handle;

    void raise_c_exception(const std::string & error_string) {
      char error_cstring[SHMEM_ERRORSTRING_LENGTH];
      memset(error_cstring, 0, SHMEM_ERRORSTRING_LENGTH);
      snprintf(error_cstring, SHMEM_ERRORSTRING_LENGTH, 
              "[ERROR.%d] Error while %s: %s", errno, error_string.c_str(), ::strerror(errno));
      throw std::runtime_error(error_cstring);
    }

    public:
    SharedMemory(const std::string & filename) : _filename(filename), _file_handle(-1), _data_handle((T*)MAP_FAILED) {
      _file_handle = shm_open(filename.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
      if (_file_handle < 0)
        SharedMemory<T, size>::raise_c_exception("opening shared memory");
      if (ftruncate(fd, sizeof(T) * size) != 0)
        SharedMemory<T, size>::raise_c_exception("resizing shared memory");
      _data_handle = (T*)mmap(0, sizeof(T) * size, PROT_READ | PROT_WRITE, MAP_SHARED, _file_handle, 0);
      if (_data_handle == MAP_FAILED)
        SharedMemory<T, size>::raise_c_exception("mapping shared memory");
    }

    ~SharedMemory() {
      if (_data_handle != (T*)MAP_FAILED)
        munmap(_data_handle, sizeof(T) * size);
      if (_file_handle >= 0)
        close(_file_handle);
      shm_unlink(_filename.c_str());
    }

    T& operator[](std::size_t idx)  {
      return _data_handle[idx % size];
    }

    T operator[](std::size_t idx) const {
      return _data_handle[idx % size];
    }
  };

}