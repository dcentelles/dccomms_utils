/*
 * GironaStream.cpp
 *
 *  Created on: 16 nov. 2016
 *      Author: centelld
 */

#include <dccomms_utils/S100Stream.h>
#include <cstdint>
#include <string>

namespace dccomms_utils {

S100Stream::S100Stream(std::string serialportname, SerialPortStream::BaudRate baudrate, int maxBaudrate):
        SerialPortStream(serialportname.c_str(), baudrate)
{
    _maxBaudrate = maxBaudrate;
	init();
}

S100Stream::~S100Stream() {

}

void S100Stream::init()
{
    if(_maxBaudrate > 0)
        _byteTransmissionTime =  1000./( _maxBaudrate / 8.);

    _maxTrunkSize = 100;
    Log->debug("baudrate: {} ; byte transmission time: {} ; frame trunk size: {}",
               _maxBaudrate,
               _byteTransmissionTime,
               _maxTrunkSize
               );
}

int S100Stream::_Recv(void * dbuf, int n, bool block)
{
	return Read((unsigned char *) dbuf, n, (unsigned int)block);
}

ICommsLink& S100Stream::operator<< (const DataLinkFramePtr & dlf)
{
    auto buffer = dlf->GetFrameBuffer ();
    auto fs = dlf->GetFrameSize ();

    auto ptr = buffer;
    auto maxPtr = buffer + fs;

    unsigned int _frameTransmissionTime = ceil(fs * _byteTransmissionTime);
    Log->debug("TX {}->{}: estimated frame transmission time: {} ms (FS: {}).",
               dlf->GetSrcDir (),
               dlf->GetDesDir (),
               _frameTransmissionTime, fs);

    unsigned int _trunkTransmissionTime;

    _trunkTransmissionTime = ceil(_maxTrunkSize * _byteTransmissionTime);

    while(ptr + _maxTrunkSize < maxPtr)
    {
        Log->debug("Sending trunk of {} bytes... ({} ms)",
                   _maxTrunkSize,
                   _trunkTransmissionTime);
        Write(ptr, _maxTrunkSize);
        std::this_thread::sleep_for(std::chrono::milliseconds(_trunkTransmissionTime));
        ptr += _maxTrunkSize;
    }
    unsigned long left = maxPtr - ptr;
    if(left > 0)
    {
        _trunkTransmissionTime = ceil(left * _byteTransmissionTime);
        Log->debug("Sending trunk of {} bytes... ({} ms)",
                   left,
                   _trunkTransmissionTime);
        Write(ptr, left);
        std::this_thread::sleep_for(std::chrono::milliseconds(_trunkTransmissionTime));
    }


    return *this;
}

} /* namespace merbots */
