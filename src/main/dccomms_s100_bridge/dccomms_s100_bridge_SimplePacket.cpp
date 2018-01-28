/*
 * main.cpp
 *
 *  Created on: 22 oct. 2016
 *      Author: Diego Centelles Beltran
 */

#include <cstdio>
#include <dccomms/CommsBridge.h>
#include <dccomms/Utils.h>
#include <dccomms_packets/SimplePacket.h>
#include <dccomms_utils/S100Stream.h>
#include <iostream>

#include <cstdio>
#include <cxxopts.hpp>
#include <signal.h>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace dccomms_packets;
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
  uint32_t modemBitrate = 115200, txPacketSize = 20, rxPacketSize = 20;
  std::string dccommsId;
  std::string logLevelStr;
  Log->Info("S100 Bridge");
  Log->LogToFile("s100_comms_main_log");
  try {
    cxxopts::Options options("dccomms_utils/s100_bridge",
                             " - command line options");
    options.add_options()(
        "p,modem-port", "Modem's serial port",
        cxxopts::value<std::string>(modemPort)->default_value("/dev/ttyUSB0"))(
        "l,log-level", "log level: critical,debug,err,info,off,trace,warn",
        cxxopts::value<std::string>(logLevelStr)->default_value("info"))(
        "b, modem-bitrate", "maximum bitrate",
        cxxopts::value<uint32_t>(modemBitrate))("help", "Print help")(
        "dccomms-id", "dccomms id for bridge",
        cxxopts::value<std::string>(dccommsId)->default_value("s100"))(
        "tx-packet-size", "transmitted SimplePacket size in bytes",
        cxxopts::value<uint32_t>(txPacketSize))(
        "rx-packet-size", "received SimplePacket size in bytes",
        cxxopts::value<uint32_t>(rxPacketSize));

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

  PacketBuilderPtr txpb = CreateObject<SimplePacketBuilder>(0, FCS::CRC16);
  auto emptyPacket = txpb->Create();
  auto emptyPacketSize = emptyPacket->GetPacketSize();

  auto payloadSize = txPacketSize - emptyPacketSize;
  txpb = CreateObject<SimplePacketBuilder>(payloadSize, FCS::CRC16);
  auto testPacket = txpb->Create();
  Log->Info("Transmitted packet size: {}", testPacket->GetPacketSize());
  payloadSize = rxPacketSize - emptyPacketSize;
  PacketBuilderPtr rxpb =
      CreateObject<SimplePacketBuilder>(payloadSize, FCS::CRC16);
  testPacket = rxpb->Create();
  Log->Info("Receiving packet size: {}", testPacket->GetPacketSize());

  bridge = new CommsBridge(stream, txpb, rxpb, 0);

  bridge->SetLogLevel(logLevel);
  bridge->SetCommsDeviceId(dccommsId);
  bridge->SetLogName("S100Bridge");
  stream->SetLogName(bridge->GetLogName() + ":S100Stream");
  stream->SetLogLevel(info);

  bridge->FlushLogOn(info);
  bridge->LogToFile("s100_comms_bridge_log");

  stream->FlushLogOn(info);
  stream->LogToFile("s100_comms_bridge_device_log");

  bridge->SetReceivedPacketWithoutErrorsCb([](PacketPtr pkt) {
    Log->Info("Received packet of {} bytes", pkt->GetPacketSize());
  });
  bridge->SetReceivedPacketWithErrorsCb([](PacketPtr pkt) {
    Log->Warn("Received packet with errors ({} bytes)", pkt->GetPacketSize());
  });
  bridge->SetTransmitingPacketCb([](PacketPtr pkt) {
    Log->Info("Transmitting packet of {} bytes", pkt->GetPacketSize());
  });

  bridge->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
