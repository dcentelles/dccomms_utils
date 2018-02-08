/*
 * GironaStream.cpp
 *
 *  Created on: 16 nov. 2016
 *      Author: centelld
 */

#include <cstdint>
#include <dccomms_utils/S100Stream.h>
#include <string>

namespace dccomms_utils {

S100Stream::S100Stream(std::string serialportname,
                       SerialPortStream::BaudRate baudrate, int maxBaudrate)
    : SerialPortStream(serialportname.c_str(), baudrate) {
  _maxBaudrate = maxBaudrate;
  init();
}

S100Stream::~S100Stream() {}

void S100Stream::init() {
  if (_maxBaudrate > 0)
    _byteTransmissionTime = 1000. / (_maxBaudrate / 8.);

  _maxTrunkSize = 64;
}

int S100Stream::_Recv(void *dbuf, int n, bool block) {
  return Read((unsigned char *)dbuf, n, (unsigned int)block);
}

void S100Stream::LogConfig() {

  Log->info("baudrate: {} ; byte transmission time: {} ; frame trunk size: {}",
            _maxBaudrate, _byteTransmissionTime, _maxTrunkSize);
}

void S100Stream::WritePacket(const PacketPtr &dlf) {
  auto buffer = dlf->GetBuffer();
  auto fs = dlf->GetPacketSize();

  auto ptr = buffer;
  auto maxPtr = buffer + fs;

  unsigned int _frameTransmissionTime = ceil(fs * _byteTransmissionTime);
  //  Log->debug("TX {}->{}: estimated frame transmission time: {} ms (FS:
  //  {}).",
  //             dlf->GetSrcDir(), dlf->GetDesDir(), _frameTransmissionTime,
  //             fs);

  unsigned int _trunkTransmissionTime;

  _trunkTransmissionTime = ceil(_maxTrunkSize * _byteTransmissionTime);

  while (ptr + _maxTrunkSize < maxPtr) {
    Log->debug("Sending trunk of {} bytes... ({} ms)", _maxTrunkSize,
               _trunkTransmissionTime);
    Write(ptr, _maxTrunkSize);
    std::this_thread::sleep_for(
        std::chrono::milliseconds(_trunkTransmissionTime));
    ptr += _maxTrunkSize;
  }

  uint8_t endOfPacket[2] = {0xd, 0xa};
  unsigned long left = maxPtr - ptr;
  if (left > 0) {
    _trunkTransmissionTime = ceil((left + 2) * _byteTransmissionTime);
    Log->debug("Sending trunk of {} bytes and end of packet... ({} ms)", left,
               _trunkTransmissionTime);
    Write(ptr, left);
  } else {
    _trunkTransmissionTime = ceil(2 * _byteTransmissionTime);
    Log->debug("Sending end of packet... ({} ms)", _trunkTransmissionTime);
  }
  Write(endOfPacket, 2);
  std::this_thread::sleep_for(
      std::chrono::milliseconds(_trunkTransmissionTime));
}

} /* namespace merbots */
