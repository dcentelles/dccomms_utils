/*
 * main.cpp
 *
 *  Created on: 22 oct. 2016
 *      Author: Diego Centelles Beltran
 */

#include <cstdio>
#include <dccomms/CommsBridge.h>
#include <dccomms/DataLinkFrame.h>
#include <dccomms/Utils.h>
#include <dccomms_utils/S100Stream.h>
#include <iostream>

#include <cstdio>
#include <signal.h>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace dccomms_utils;

CommsBridge *bridge;
S100Stream *stream;

void SIGINT_handler(int sig) {
  printf("Received %d signal\nClosing device socket...\n", sig);
  printf("Device closed.\n");
  fflush(stdout);
  bridge->FlushLog();
  stream->FlushLog();
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
  std::string modemPort;
  int modemBaudrate;
  std::string ns;

  modemBaudrate = atoi(argv[1]);
  modemPort = argv[2];
  ns = argv[3];

  setSignals();

  auto portBaudrate = SerialPortStream::BAUD_2400;
  stream = new S100Stream(modemPort, portBaudrate, modemBaudrate);

  PacketBuilderPtr pb = std::make_shared<DataLinkFramePacketBuilder>(
      DataLinkFrame::fcsType::crc16);

  bridge = new CommsBridge(stream, pb, pb, 0);

  bridge->SetLogLevel(cpplogging::LogLevel::debug);
  bridge->SetCommsDeviceId(ns);
  bridge->SetLogName("S100Bridge");
  stream->SetLogName(bridge->GetLogName() + ":S100Stream");

  bridge->FlushLogOn(cpplogging::LogLevel::info);
  bridge->LogToFile("s100_comms_bridge_log");

  stream->FlushLogOn(cpplogging::LogLevel::info);
  stream->LogToFile("s100_comms_bridge_device_log");

  bridge->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
