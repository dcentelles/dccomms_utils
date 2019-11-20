#include <cpputils/Object.h>
#include <cpputils/SignalManager.h>
#include <cpputils/Timer.h>
#include <cstdio>
#include <cstring>
#include <dccomms/SerialPortStream.h>
#include <dccomms/DataLinkFrame.h>

#include <cstdio>
#include <cxxopts.hpp>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace cpputils;

int main(int argc, char **argv) {
    LoggerPtr Log = cpplogging::CreateLogger("TDoA-tx");
    LoggerPtr CsvLog = cpplogging::CreateLogger("CSV");
    CsvLog->SetAsyncMode();
    std::string ac_modemPort, rf_modemPort;
    uint32_t ac_portBaudrate = 9600, rf_portBaudrate = 115200;
    std::string dccommsId;
    std::string logLevelStr, logFile;
    uint32_t samples = 10;
    std::string csvfile = "data.csv";
    bool flush = false, syncLog = false, hwFlowControlEnabled = false;
    Log->Info("TDoA-tx");
    try {
        cxxopts::Options options("dccomms_utils/tdoa-tx",
                                 " - command line options");
        options.add_options()
            ("C,flow-control-enabled", "the flow control by hw is enabled in the modem", cxxopts::value<bool>(hwFlowControlEnabled))
            ("F,flush-log", "flush log", cxxopts::value<bool>(flush))
            ("s,sync-log", "sync-log (default: false -> async.)", cxxopts::value<bool>(syncLog))
            ("f,log-file", "File to save the log",cxxopts::value<std::string>(logFile)->default_value("")->implicit_value("bridge_log"))
            ("csv", "sample files", cxxopts::value<std::string>(csvfile))
            ("ac-modem-port", "AC Modem's serial port",cxxopts::value<std::string>(ac_modemPort)->default_value("/dev/ttyUSB0"))
            ("ac-baud-rate", "AC Serial port baudrate (default: 9600)",cxxopts::value<uint32_t>(ac_portBaudrate))
            ("rf-modem-port", "RF Modem's serial port",cxxopts::value<std::string>(rf_modemPort)->default_value("/dev/ttyACM0"))
            ("rf-baud-rate", "RF Serial port baudrate (default: 115200)",cxxopts::value<uint32_t>(rf_portBaudrate))
            ("samples", "default 10", cxxopts::value<uint32_t>(samples))
            ("l,log-level", "log level: critical,debug,err,info,off,trace,warn",cxxopts::value<std::string>(logLevelStr)->default_value("info"))
            ("help", "Print help");

        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help({""}) << std::endl;
            exit(0);
        }

    } catch (const cxxopts::OptionException &e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    Log->Info("AC port: {} ; AC baudrate: {} ; RF port: {} ; RF baudrate: {}", ac_modemPort,
              ac_portBaudrate, rf_modemPort, rf_portBaudrate);

    auto ac_baudrate = SerialPortStream::BaudRateFromUInt(ac_portBaudrate);
    auto rf_baudrate = SerialPortStream::BaudRateFromUInt(rf_portBaudrate);

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

    auto ac0_stream = CreateObject<SerialPortStream>(ac_modemPort, ac_baudrate);
    auto rf_stream = CreateObject<SerialPortStream>(rf_modemPort, rf_baudrate);

    ac0_stream->Open();
    rf_stream->Open();
    CsvLog->LogToFile(csvfile);
    CsvLog->LogToConsole(true);
    Log->SetLogFormatter(std::make_shared<spdlog::pattern_formatter>("%T.%F %v"));
    Log->LogToConsole(true);
    CsvLog->SetLogFormatter(std::make_shared<spdlog::pattern_formatter>("%T.%F %v"));

    auto checksumType = DataLinkFrame::fcsType::crc16;
    auto pb = CreateObject<DataLinkFramePacketBuilder>(checksumType);
    auto pkt = pb->Create();
    uint8_t * rf_count_hl = pkt->GetPayloadBuffer();
    uint8_t * rf_count_h = rf_count_hl;
    uint8_t * rf_count_l = rf_count_hl+1;

    uint8_t count_hl[2];
    uint8_t * count_h = count_hl;
    uint8_t * count_l = count_hl+1;

    char message[100];
    message[1] = 0;
    ac0_stream->FlushInput();
    cpputils::TimerNanos td;
    uint16_t count = 0;
    int first_its = 1;
    uint16_t max = samples + first_its;
    std::thread main([&]() {
        while (count <= max) {
            ac0_stream->FlushIO();
            rf_stream->FlushIO();

            *rf_count_h = count >> 8;
            *rf_count_l = count & 0x00ff;

            //SEND RF MESSAGE
            pkt->PayloadUpdated(2);
            rf_stream << pkt;

            //SEND ACOUSTIC MESSAGE
            ac0_stream << "$B2";
            *count_h = count >> 8;
            *count_l = count & 0x00ff;
            ac0_stream->Write(&count_hl, 2);
            if(first_its < count)
                CsvLog->Info("{}", count);
            else
                Log->Info("SYNC: {}", count);
            count++;

            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
        Log->Info("END");
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
}
