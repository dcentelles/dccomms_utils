/*
 * main.cpp
 *
 *  Created on: 22 oct. 2016
 *      Author: Diego Centelles Beltran
 */

#include <cstdio>
#include <dccomms/Utils.h>
#include <dccomms_utils/EvologicsBridge.h>
#include <dccomms_utils/GironaStream.h>
#include <iostream>

#include <cstdio>
#include <signal.h>
#include <sys/types.h>

using namespace std;
using namespace dccomms;

using namespace dccomms_utils;
EvologicsBridge *comms;
GironaStream *stream;

void SIGINT_handler(int sig) {
  printf("Received %d signal\nClosing device...\n", sig);
  // comms->Stop();
  printf("Device closed.\n");
  fflush(stdout);
  // comms->FlushLog();
  // stream->FlushLog();
  Utils::Sleep(2000);
  printf("Log messages flushed.\n");

  exit(0);
}

void setSignals() {
  if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
    printf("SIGINT install error\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  int maxDataRate = std::stoi(argv[1]);
  const char *serialportname = argv[2];
  int localAddr = std::stoi(argv[3]);
  int remoteAddr = std::stoi(argv[4]);

  setSignals();

  stream = new GironaStream(serialportname, SerialPortStream::BAUD_19200);
  comms =
      new EvologicsBridge(stream, maxDataRate, DataLinkFrame::fcsType::crc16);

  comms->SetLogLevel(cpplogging::LogLevel::debug);
  comms->SetCommsDeviceId("camera");
  comms->SetLogName("ROVBridge");
  stream->SetLogName(comms->GetLogName() + ":ROVStream");
  comms->SetEndOfCmd("\r");

  comms->SetLocalAddr(localAddr);   // 2
  comms->SetRemoteAddr(remoteAddr); // 1

  std::cout << "Local add: " << localAddr << endl;
  std::cout << "Remote add: " << remoteAddr << endl;
  comms->SetClusterSize(atoi(argv[3]));

  comms->FlushLogOn(cpplogging::LogLevel::info);
  comms->LogToFile("rov_comms_bridge_log");

  stream->FlushLogOn(cpplogging::LogLevel::info);
  stream->LogToFile("rov_comms_bridge_device_log");

  comms->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
