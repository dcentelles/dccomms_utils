/*
 * EvologicsStream.cpp
 *
 *  Created on: 21 nov. 2016
 *      Author: centelld
 */

#include <cstring>
#include <dccomms_utils/NanoModemStream.h>

namespace dccomms_utils {

NanoModemStream::NanoModemStream(std::string serialportname,
                                 SerialPortStream::BaudRate baudrate)
    : SerialPortStream(serialportname.c_str(), baudrate) {
  init();
}

NanoModemStream::~NanoModemStream() {}

void NanoModemStream::init() {
  notificationReceivedCallback = [](const std::string &s) {};
  _InitNotificationsFilter();
  SetHwFlowControl(false);
  _byteTransmissionTimeNanos = 0.2 * 1e9;

  _maxTrunkSize = 7;
}

void NanoModemStream::_WritePacketManualFlowControl(const PacketPtr &dlf) {
  auto buffer = dlf->GetBuffer();
  auto fs = dlf->GetPacketSize();

  auto ptr = buffer;
  auto maxPtr = buffer + fs;

  unsigned int _trunkTransmissionTime;

  _trunkTransmissionTime = ceil(_maxTrunkSize * _byteTransmissionTimeNanos +
                                _baseTransmissionTimeNanos);

  while (ptr + _maxTrunkSize < maxPtr) {
    Log->debug("Sending trunk of {} bytes... ({} ms)", _maxTrunkSize,
               _trunkTransmissionTime);
    Write("$B", 2);
    WriteUint8('0' + _maxTrunkSize);
    Write(ptr, _maxTrunkSize);
    std::this_thread::sleep_for(
        std::chrono::nanoseconds(_trunkTransmissionTime));
    ptr += _maxTrunkSize;
  }

  unsigned long left = maxPtr - ptr;
  if (left > 0) {
    _trunkTransmissionTime =
        ceil(left * _byteTransmissionTimeNanos + _baseTransmissionTimeNanos);
    Log->debug("Sending trunk of {} bytes and end of packet... ({} ms)", left,
               _trunkTransmissionTime);
    Write("$B", 2);
    WriteUint8('0' + left);
    Write(ptr, left);
  }
  std::this_thread::sleep_for(std::chrono::nanoseconds(_trunkTransmissionTime));
}

int NanoModemStream::_Recv(void *dbuf, int n, bool block) {
  return Read((unsigned char *)dbuf, n, (unsigned int)block);
}

void NanoModemStream::LogConfig() {

  Log->info("baudrate: {} ; byte transmission time: {} ; frame trunk size: {}",
            _maxBaudrate, _byteTransmissionTimeNanos, _maxTrunkSize);
}

void NanoModemStream::WritePacket(const PacketPtr &dlf) {
  _WritePacketManualFlowControl(dlf);
}

void NanoModemStream::SetNotificationReceivedCallback(f_notification func) {
  notificationReceivedCallback = func;
}

void NanoModemStream::_InitNotificationsFilter() {
  // to filter notifications:
  memcpy(buffer, bes, NANOMODEM_BES_LENGTH);
  notifHeader = buffer;
  notifHeaderCurPtr = notifHeader;
  notifHeaderEndPtr = notifHeader + NANOMODEM_BES_LENGTH;

  notifQueue = notifHeaderEndPtr;

  strcpy(notifQueue, "\r\n");
  notifQueueCurPtr = notifQueue;
  notifQueueEndPtr = notifQueueCurPtr + strlen(notifQueue);

  data = (uint8_t *)notifQueueEndPtr;
  cdata = data;

  ndata = 0;

  notifLength = 0;
  maxNotifLength = NANOMODEM_NOTIFICATION_MAX_SIZE;
}

// This method filters and logs any received notification from the modem
int NanoModemStream::ReadData(void *dbuf, int n, bool block) {
  int reqn;
  if (ndata < n) {
    reqn = n - ndata;
  } else {
    reqn = 1;
  }
  int res = _Recv(cdata, reqn, block);

  int nreturn; // the number of valid bytes that will be returned (nreturn <=
               // ret)

  uint8_t *firstNoValid = data;
  uint8_t *noData = cdata + res;
  while (cdata < noData) {
    // if we have not read a notification header...
    if (notifHeaderCurPtr < notifHeaderEndPtr) {
      if (*cdata != *notifHeaderCurPtr) {
        notifHeaderCurPtr = notifHeader;
        firstNoValid = cdata + 1;
      } else {
        notifHeaderCurPtr++;
      }
      cdata++;
    }
    // else, a notification has been found, and we will discard
    // all bytes until the notification's queue reception
    else if (notifQueueCurPtr < notifQueueEndPtr) {
      if (notifLength <= maxNotifLength) {
        if (*cdata != *notifQueueCurPtr) {
          notifQueueCurPtr = notifQueue;
        } else {
          notifQueueCurPtr++;
        }
        notifLength++;
      } else {
        notifLength = 0;
        notifHeaderCurPtr = notifHeader;
        notifQueueCurPtr = notifQueue;

        firstNoValid = cdata + 1;
      }
      cdata++;
    } else {
      notifLength = 0;
      notifHeaderCurPtr = notifHeader;
      notifQueueCurPtr = notifQueue;

      // all data from firstNoValid to cdata is a notification.
      int notifsize = cdata - firstNoValid;

      // save and log the nofification, and call callback
      memcpy(lastNotification, firstNoValid, notifsize);
      lastNotification[notifsize - 2] = 0;
      Log->info("notification received from modem: {}", lastNotification);
      notificationReceivedCallback(std::string(lastNotification));

      // copy the following bytes upon the notif ptr
      int endData = noData - cdata;
      memcpy(firstNoValid, cdata, endData);

      // reset cdata to notif ptr (firstNoValid)
      cdata = firstNoValid;
      noData = cdata + endData;
    }
  }
  ndata = firstNoValid > data ? firstNoValid - data : 0;
  nreturn = n <= ndata ? n : ndata;

  if (nreturn > 0) {
    memcpy(dbuf, data, nreturn);
    uint8_t *end = data + nreturn;

    if (firstNoValid == end) {
      cdata = data;
      ndata = 0;
    } else if (firstNoValid > end) // there are valid bytes that will not be
                                   // returned yet (n < ret)
    {
      memcpy(data, end, firstNoValid - end);
      cdata = end;
      ndata = cdata - data;
    } else // cdata < end (Impossible)
    {
      Log->critical("this message should not be shown, else there is a bug");
    }
  }

  // return number of valid bytes (after filter) copied in dbuf
  return nreturn;
}
} // namespace dccomms_utils
