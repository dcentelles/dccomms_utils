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
#include <cxxopts.hpp>
#include <signal.h>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace dccomms_utils;

LoggerPtr Log = cpplogging::CreateLogger("s100bridge");
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
  uint32_t modemBitrate = 600;
  std::string dccommsId;
  std::string logLevelStr, logFile;
  Log->Info("S100 Bridge");
  try {
    cxxopts::Options options("dccomms_utils/s100_bridge",
                             " - command line options");
    options.add_options()(
        "f,log-file", "File to save the log",
        cxxopts::value<std::string>(logFile)->default_value("")->implicit_value(
            "example2_log"))(
        "p,modem-port", "Modem's serial port",
        cxxopts::value<std::string>(modemPort)->default_value("/dev/ttyUSB0"))(
        "l,log-level", "log level: critical,debug,err,info,off,trace,warn",
        cxxopts::value<std::string>(logLevelStr)->default_value("info"))(
        "b, modem-bitrate", "maximum bitrate",
        cxxopts::value<uint32_t>(modemBitrate))("help", "Print help")(
        "dccomms-id", "dccomms id for bridge",
        cxxopts::value<std::string>(dccommsId)->default_value("s100"));

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

  } catch (const cxxopts::OptionException &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  Log->Info("dccommsId: {} ; port: {} ; bit-rate: {}", dccommsId, modemPort,
            modemBitrate);
  setSignals();
  LogLevel logLevel = cpplogging::GetLevelFromString(logLevelStr);
  auto portBaudrate = SerialPortStream::BAUD_2400;
  stream = new S100Stream(modemPort, portBaudrate, modemBitrate);

  PacketBuilderPtr pb = std::make_shared<DataLinkFramePacketBuilder>(
      DataLinkFrame::fcsType::crc16);

  bridge = new CommsBridge(stream, pb, pb, 0);

  bridge->SetLogLevel(logLevel);
  bridge->SetCommsDeviceId(dccommsId);
  bridge->SetLogName("S100Bridge");
  stream->SetLogName(bridge->GetLogName() + ":S100Stream");
  stream->SetLogLevel(info);

  auto logFormatter = std::make_shared<spdlog::pattern_formatter>("[%T.%F] %v");
  stream->SetLogFormatter(logFormatter);
  bridge->SetLogFormatter(logFormatter);
  Log->SetLogFormatter(logFormatter);

  if (logFile != "") {
    Log->LogToFile(logFile);
    stream->LogToFile(logFile + "_stream");
    bridge->LogToFile(logFile + "_bridge");
  }
  bridge->FlushLogOn(info);
  stream->FlushLogOn(info);
  Log->FlushLogOn(info);

  bridge->SetReceivedPacketWithoutErrorsCb([](PacketPtr pkt) {
    Log->Info("RX {} bytes", pkt->GetPacketSize());
  });
  bridge->SetReceivedPacketWithErrorsCb([](PacketPtr pkt) {
    Log->Warn("ERR {} bytes", pkt->GetPacketSize());
  });
  bridge->SetTransmitingPacketCb([](PacketPtr pkt) {
    Log->Info("TX {} bytes", pkt->GetPacketSize());
  });

  bridge->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
