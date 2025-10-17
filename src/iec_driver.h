#ifndef IEC_DRIVER_H
#define IEC_DRIVER_H

#include "GpioPin.h"

class IEC
{
public:
	IEC(){}
	IEC(GpioPin reset, GpioPin atn, GpioPin clk, GpioPin data);

	// Interrupts the bus (ATN line) to start the transmission or releases the but as the end of a command
	void command(bool start);

	// Returns the bus back to idle
	void end();

	// Sends a reset signal to soft reset all devices on the bus
	void reset();

	// Sends a single byte and can signal EOI on the last byte
	void send(uint8_t data, bool signalEOI);

	// Checks if CBM is sending an attention message. If this is the case,
	// the message is received.
	// ATNCheck checkATN(ATNCmd& cmd, char deviceNumber);

	// bool checkReset();

	// sendEmptyStream();
	// receive();
	// turnAround;

private:
	GpioPin ATN;
	GpioPin RESET;
	GpioPin CLK;
	GpioPin DATA;
	//GpioPin SRQ;
};

#endif
