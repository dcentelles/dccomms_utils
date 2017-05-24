/*
 * main.cpp
 *
 *  Created on: 22 oct. 2016
 *      Author: Diego Centelles Beltran
 */

#include <iostream>
#include <dccomms/Utils.h>
#include <cstdio>
#include <dccomms_utils/USBLStream.h>
#include <dccomms_utils/EvologicsBridge.h>

#include  <cstdio>
#include  <sys/types.h>
#include  <signal.h>

using namespace std;
using namespace dccomms;
using namespace dccomms_utils;

EvologicsBridge * comms;
USBLStream * stream ;

void SIGINT_handler (int sig)
{
        printf("Received %d signal\nClosing device socket...\n",sig);
        comms->Stop();
        printf("Device closed.\n");
        fflush(stdout);
        comms->FlushLog();
        stream->FlushLog();
        Utils::Sleep(2000);
        printf("Log messages flushed.\n");

        exit(0);
}

void setSignals()
{
    if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
         printf("SIGINT install error\n");
         exit(1);
    }
}

int main(int argc, char ** argv)
{
    int baudrate = atoi(argv[1]);
    const char * devicedir = argv[2];

    setSignals();

    stream = new USBLStream(devicedir);
    comms = new EvologicsBridge(stream, baudrate, DataLinkFrame::fcsType::crc16);

    comms->SetLogLevel(cpplogging::Loggable::debug);
    comms->SetNamespace("operator");
    comms->SetLogName("OperatorBridge");
    stream->SetLogName(comms->GetLogName()+":USBLStream");
    comms->SetEndOfCmd("\n");

    comms->SetLocalAddr(1);
    comms->SetRemoteAddr(3);
    comms->SetClusterSize(atoi(argv[3]));

    comms->FlushLogOn(Loggable::LogLevel::info);
    comms->LogToFile("usbl_comms_bridge_log");

    stream->FlushLogOn(Loggable::LogLevel::info);
    stream->LogToFile("usbl_comms_bridge_device_log");

    comms->Start();
    while(1)
    {
            Utils::Sleep(1000);
    }
}

