/*
 * GironaStream.h
 *
 *  Created on: 16 nov. 2016
 *      Author: centelld
 */

#ifndef INCLUDE_MERBOTS_GIRONASTREAM_H_
#define INCLUDE_MERBOTS_GIRONASTREAM_H_

#include <dccomms/SerialPortStream.h>
#include <dccomms_utils/Constants.h>
#include <dccomms_utils/WFSStream.h>
#include <regex>
#include <string>

namespace dccomms_utils {

using namespace dccomms;

class S100Stream : public SerialPortStream, public WFSStream {
public:
  S100Stream(std::string serialportname = "/dev/ttyUSB0",
             BaudRate = BaudRate::BAUD_19200, int maxBaudrate = 2400);
  virtual ~S100Stream();

  virtual void WritePacket(const PacketPtr &dlf);

  virtual void LogConfig();

private:
  void init();
  virtual int _Recv(void *dbuf, int n, bool block = true);

  char *notificationPayload;
  char notification[MAX_NOTIFICATION_LENGTH];
  int _maxBaudrate;
  double _byteTransmissionTime;
  int _maxTrunkSize;
};

} /* namespace merbots */

#endif /* INCLUDE_MERBOTS_GIRONASTREAM_H_ */
