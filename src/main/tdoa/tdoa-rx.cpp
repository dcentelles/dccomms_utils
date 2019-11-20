#include <cpputils/Object.h>
#include <cpputils/SignalManager.h>
#include <cpputils/Timer.h>
#include <cstdio>
#include <cstring>
#include <dccomms/Arduino.h>
#include <dccomms/DataLinkFrame.h>

#include <cstdio>
#include <cxxopts.hpp>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace cpputils;

LoggerPtr Log = cpplogging::CreateLogger("tdoa-rx");

int main(int argc, char **argv) {
  std::string ac_modemPort, rf_modemPort;
  uint32_t ac_portBaudrate = 9600, rf_portBaudrate = 9600;
  std::string dccommsId;
  std::string logLevelStr, logFile;
  bool flush = false, syncLog = false, hwFlowControlEnabled = false;
  Log->Info("TDoA-RX");
  try {
    cxxopts::Options options("dccomms_utils/s100_bridge",
                             " - command line options");
    options.add_options()("C,flow-control-enabled",
                          "the flow control by hw is enabled in the modem",
                          cxxopts::value<bool>(hwFlowControlEnabled))(
        "F,flush-log", "flush log", cxxopts::value<bool>(flush))(
        "s,sync-log", "sync-log (default: false -> async.)",
        cxxopts::value<bool>(syncLog))(
        "f,log-file", "File to save the log",
        cxxopts::value<std::string>(logFile)->default_value("")->implicit_value(
            "bridge_log"))("ac-modem-port", "AC Modem's serial port",
                           cxxopts::value<std::string>(ac_modemPort)
                               ->default_value("/dev/ttyUSB0"))(
        "rf-modem-port", "RF Modem's serial port",
        cxxopts::value<std::string>(rf_modemPort)
            ->default_value("/dev/ttyUSB1"))(
        "ac-baud-rate", "AC Serial port baudrate (default: 9600)",
        cxxopts::value<uint32_t>(ac_portBaudrate))(
        "rf-baud-rate", "RF Serial port baudrate (default: 9600)",
        cxxopts::value<uint32_t>(rf_portBaudrate))(
        "l,log-level", "log level: critical,debug,err,info,off,trace,warn",
        cxxopts::value<std::string>(logLevelStr)->default_value("info"))(
        "help", "Print help");

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

  } catch (const cxxopts::OptionException &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  Log->Info("AC: port: {} ; baudrate: {} ; hw.FlowC: {}", ac_modemPort,
            ac_portBaudrate, hwFlowControlEnabled);
  Log->Info("RF: port: {} ; baudrate: {} ; hw.FlowC: {}", rf_modemPort,
            rf_portBaudrate, hwFlowControlEnabled);

  LogLevel logLevel = cpplogging::GetLevelFromString(logLevelStr);
  auto ac_baudrate = SerialPortStream::BaudRateFromUInt(ac_portBaudrate);
  auto rf_baudrate = SerialPortStream::BaudRateFromUInt(rf_portBaudrate);

  auto logFormatter = std::make_shared<spdlog::pattern_formatter>("%T.%F %v");
  Log->SetLogFormatter(logFormatter);

  if (logFile != "") {
    Log->LogToFile(logFile);
  }
  if (flush) {
    Log->FlushLogOn(info);
    Log->Info("Flush log on info");
  }
  if (!syncLog) {
    Log->SetAsyncMode();
    Log->Info("Async. log");
  }
Log->LogToConsole(true);
auto ac_stream =
    CreateObject<SerialPortStream>(ac_modemPort.c_str(), ac_baudrate);

auto rf_stream =
    CreateObject<SerialPortStream>(rf_modemPort.c_str(), rf_baudrate);

//  Arduino arduino = Arduino::FindArduino(Arduino::BAUD_115200,
//                                        "Hello, are you TX?\n",
//                                        "Yes, I'm TX");

//  auto rf_stream = cpputils::Ptr<Arduino>(&arduino);

//ac_stream->Open();
rf_stream->Open();
//  if(!arduino.IsOpen())
//  {
//      Log->Error("arduino not found");
//  }
  auto checksumType = DataLinkFrame::fcsType::crc16;
  auto pb = CreateObject<DataLinkFramePacketBuilder>(checksumType);
  auto pkt = pb->Create();

  int cont = 0;
  char message[100];
  int br = 5;
  message[1] = 0;
  ac_stream->FlushInput();
  rf_stream->FlushInput();
  char ac_pre[50] = "#B0021";
  size_t ac_pre_len = std::strlen(ac_pre);
  uint8_t rf_pre[50] = {'1', 0xd, 0xa};
  size_t rf_pre_len = 3;
  cpputils::Timer td;
  uint elapsed;
  std::thread main([&]() {
    while (1) {
      rf_stream->FlushIO();
      //rf_stream->Read(message, rf_pre_len);
      rf_stream >> pkt;
      //ac_stream->FlushIO();

      td.Reset();
      //      message[rf_pre_len] = 0;
      //      Log->Info("RF: {} -- {}", message,
      //                ac_stream->Available());
      message[1] = 0;
      if(pkt->PacketIsOk())
      {
          Log->Info("RF RX");
          //      ac_stream->WaitFor((const uint8_t *)ac_pre, ac_pre_len);
          //      int res = ac_stream->Read(message, 3);
//          message[1] = 0;
//          elapsed = td.Elapsed();
//          Log->Info("message: {} ; TDoA: {}", message, elapsed);

          //      ac_stream->WaitFor((const uint8_t *)ac_pre, ac_pre_len);
          //      int res = ac_stream->Read(message, 3);
          //      message[1] = 0;
          //      elapsed = td.Elapsed();
          //      Log->Info("message: {} ; TDoA: {}", message, elapsed);
      }else
      {
          Log->Warn("RF ERR");

      }
    }
  });

  std::thread aux([&]() {
    while (1) {
      //      Log->Info("Cont: {}", cont);
      std::this_thread::sleep_for(std::chrono::seconds(8));
    }
  });

  SignalManager::SetLastCallback(SIGINT, [&](int sig) {
    printf("Received %d signal\nClosing device socket...\n", sig);
    printf("Device closed.\n");
    fflush(stdout);
    Log->FlushLog();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    printf("Log messages flushed.\n");

    exit(0);
  });
  main.join();
  aux.join();
}

// rf_stream->Read(message, rf_pre_len);
// ac_stream->FlushIO();
// td.Reset();
// message[rf_pre_len] = 0;
// Log->Info("RF: {} -- {}", message,
//          ac_stream->Available());
////ac_stream->WaitFor((const uint8_t *)ac_pre, ac_pre_len);
// int res = ac_stream->Read(message, ac_pre_len + 3);
// message[ac_pre_len + 1] = 0;
// elapsed = td.Elapsed();
// Log->Info("message: {} ; TDoA: {}", message, elapsed);
