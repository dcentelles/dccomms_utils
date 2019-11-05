/*
 * EvologicsStream.h
 *
 *  Created on: 21 nov. 2016
 *      Author: centelld
 */

#ifndef MERBOTS_LIB_INCLUDE_MERBOTS_WFSSTREAM_H_
#define MERBOTS_LIB_INCLUDE_MERBOTS_WFSSTREAM_H_

#include <cpplogging/Logger.h>
#include <dccomms/SerialPortStream.h>
#include <dccomms_utils/Constants.h>
#include <functional>
#include <queue>
#include <string>

namespace dccomms_utils {
using namespace cpplogging;
using namespace dccomms;

#define NANOMODEM_BES_LENGTH 1
#define NANOMODEM_BUFFER_SIZE 9000
#define NANOMODEM_NOTIFICATION_MAX_SIZE 200

// virtual inheritance:
// http://www.cprogramming.com/tutorial/virtual_inheritance.html
class NanoModemStream : public SerialPortStream, public virtual Logger {
public:
  NanoModemStream(std::string serialportname = "/dev/ttyUSB0",
                  BaudRate = BaudRate::BAUD_9600);
  virtual ~NanoModemStream();
  virtual int ReadData(void *dbuf, int n, bool block = true);

  typedef std::function<void(const std::string &notification)> f_notification;

  virtual void SetNotificationReceivedCallback(f_notification);
  virtual void WritePacket(const PacketPtr &dlf);
  virtual void LogConfig();

protected:
  void init();
  f_notification notificationReceivedCallback;
  void _InitNotificationsFilter();
  virtual int _Recv(void *, int n, bool block = true);
  // To filter notifications from input:
  char buffer[NANOMODEM_BUFFER_SIZE];
  char *notifHeader, *notifQueue, *notifHeaderCurPtr, *notifHeaderEndPtr,
      *notifQueueCurPtr, *notifQueueEndPtr;
  uint8_t *data, *cdata;
  int ndata;
  char lastNotification[NANOMODEM_NOTIFICATION_MAX_SIZE];
  int maxNotifLength, notifLength;
  char bes[NANOMODEM_BES_LENGTH + 1] = "#";
  int _maxTrunkSize;
  int _maxBaudrate;
  uint64_t _byteTransmissionTimeNanos;
  uint64_t _baseTransmissionTimeNanos;
  char *notificationPayload;
  char notification[NANOMODEM_NOTIFICATION_MAX_SIZE];

  void _WritePacketManualFlowControl(const PacketPtr &dlf);
};

} // namespace dccomms_utils

#endif /* MERBOTS_LIB_INCLUDE_MERBOTS_WFSSTREAM_H_ */
