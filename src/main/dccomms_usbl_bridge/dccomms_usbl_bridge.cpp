/*
 * main.cpp
 *
 *  Created on: 22 oct. 2016
 *      Author: Diego Centelles Beltran
 */

#include <cstdio>
#include <dccomms/Utils.h>
#include <dccomms_utils/EvologicsBridge.h>
#include <dccomms_utils/USBLStream.h>
#include <iostream>

#include <cstdio>
#include <signal.h>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace dccomms_utils;

EvologicsBridge *bridge;
USBLStream *stream;

void SIGINT_handler(int sig) {
  printf("Received %d signal\nClosing device socket...\n", sig);
  bridge->Stop();
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
  int baudrate = atoi(argv[1]);
  const char *devicedir = argv[2];

  setSignals();

  stream = new USBLStream(devicedir);
  bridge = new EvologicsBridge(stream, baudrate);

  bridge->SetLogLevel(cpplogging::LogLevel::debug);
  bridge->SetCommsDeviceId("operator");
  bridge->SetLogName("OperatorBridge");
  stream->SetLogName(bridge->GetLogName() + ":USBLStream");
  bridge->SetEndOfCmd("\n");

  bridge->SetLocalAddr(1);
  bridge->SetRemoteAddr(3);
  bridge->SetClusterSize(atoi(argv[3]));

  bridge->LogToFile("usbl_comms_bridge_log");
  stream->LogToFile("usbl_comms_bridge_device_log");

  bridge->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
