#include <stdint.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// TODO temp delete
EXTERNC void TIM_Init();

EXTERNC void iec_init();

// Interrupts the bus (ATN line) to start the transmission or releases the but as the end of a command
EXTERNC void iec_command(bool start);

// Returns the bus back to idle
EXTERNC void iec_end();

// Sends a reset signal to soft reset all devices on the bus
EXTERNC void iec_reset();

// Sends a single byte and can signal EOI on the last byte
EXTERNC void iec_send(uint8_t data, bool signalEOI);

// Checks if CBM is sending an attention message. If this is the case,
// the message is received.
// ATNCheck checkATN(ATNCmd& cmd, char deviceNumber);

// bool checkReset();

// sendEmptyStream();
// receive();
// turnAround;

#undef EXTERNC
