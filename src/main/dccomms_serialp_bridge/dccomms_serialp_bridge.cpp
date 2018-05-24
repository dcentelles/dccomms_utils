#include <cpputils/SignalManager.h>
#include <cstdio>
#include <dccomms/CommsBridge.h>
#include <dccomms/DataLinkFrame.h>
#include <dccomms/Utils.h>
#include <dccomms/SerialPortStream.h>
#include <iostream>

#include <cstdio>
#include <cxxopts.hpp>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace cpputils;

LoggerPtr Log = cpplogging::CreateLogger("SerialPortBridgeMain");
CommsBridge *bridge;
SerialPortStream *stream;

int main(int argc, char **argv) {
  std::string modemPort;
  std::string dccommsId;
  std::string logLevelStr, logFile;
  uint32_t portBaudrate = 9600;
  bool flush = false, asyncLog = true, hwFlowControlEnabled = false;
  Log->Info("SerialPort Bridge");
  try {
    cxxopts::Options options("dccomms_utils/serialp_bridge",
                             " - command line options");
    options.add_options()
        ("C,flow-control-enabled", "the flow control by hw is enabled in the modem", cxxopts::value<bool>(hwFlowControlEnabled))
        ("F,flush-log", "flush log", cxxopts::value<bool>(flush))
        ("a,async-log", "async-log", cxxopts::value<bool>(asyncLog))
        ("f,log-file", "File to save the log", cxxopts::value<std::string>(logFile)->default_value("")->implicit_value("example2_log"))
        ("p,modem-port", "Modem's serial port", cxxopts::value<std::string>(modemPort)->default_value("/dev/ttyUSB0"))
        ("b, baud-rate", "Serial port baudrate (default: 9600)", cxxopts::value<uint32_t>(portBaudrate))
        ("l,log-level", "log level: critical,debug,err,info,off,trace,warn", cxxopts::value<std::string>(logLevelStr)->default_value("info"))
        ("help", "Print help")
        ("dccomms-id", "dccomms id for bridge", cxxopts::value<std::string>(dccommsId)->default_value("s100"));

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

  } catch (const cxxopts::OptionException &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  Log->Info("dccommsId: {} ; port: {} ; bit-rate: {} ; hw.FlowC: {}", dccommsId, modemPort,
            hwFlowControlEnabled);

  LogLevel logLevel = cpplogging::GetLevelFromString(logLevelStr);
  stream = new SerialPortStream(modemPort, portBaudrate);
  stream->SetHwFlowControl(hwFlowControlEnabled);

  PacketBuilderPtr pb = std::make_shared<DataLinkFramePacketBuilder>(
      DataLinkFrame::fcsType::crc16);

  bridge = new CommsBridge(stream, pb, pb, 0);

  bridge->SetLogLevel(logLevel);
  bridge->SetCommsDeviceId(dccommsId);
  bridge->SetLogName("SerialPortBridge");
  stream->SetLogName(bridge->GetLogName() + ":SerialPort");
  stream->SetLogLevel(info);

  auto logFormatter = std::make_shared<spdlog::pattern_formatter>("%T.%F %v");
  stream->SetLogFormatter(logFormatter);
  bridge->SetLogFormatter(logFormatter);
  Log->SetLogFormatter(logFormatter);

  if (logFile != "") {
    Log->LogToFile(logFile);
    stream->LogToFile(logFile + "_stream");
    bridge->LogToFile(logFile + "_bridge");
  }
  if (flush) {
    Log->FlushLogOn(info);
    Log->Info("Flush log on info");
  }
  if (asyncLog) {
    Log->SetAsyncMode();
    Log->Info("Async. log");
  }

  bridge->SetReceivedPacketWithoutErrorsCb(
      [](PacketPtr pkt) { Log->Info("RX {}", pkt->GetPacketSize()); });
  bridge->SetReceivedPacketWithErrorsCb(
      [](PacketPtr pkt) { Log->Warn("ERR {}", pkt->GetPacketSize()); });
  bridge->SetTransmitingPacketCb(
      [](PacketPtr pkt) { Log->Info("TX {}", pkt->GetPacketSize()); });

  SignalManager::SetLastCallback(SIGINT, [&](int sig) {
    printf("Received %d signal\nClosing device socket...\n", sig);
    //bridge->Stop();
    printf("Device closed.\n");
    fflush(stdout);
    bridge->FlushLog();
    stream->FlushLog();
    Log->FlushLog();
    Utils::Sleep(2000);
    printf("Log messages flushed.\n");
    exit(0);
  });

  bridge->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
