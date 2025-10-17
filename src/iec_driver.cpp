#include "iec_driver.h"
#include "main.h"
#include <stdint.h>

// IEC protocol timing consts (in microseconds):
#define TIMING_CMD_DELAY_AFTER_BYTE 90
#define TIMING_CMD_START_DELAY 17
#define TIMING_CMD_END 40 // After last byte of command is sent, wait this long before releasing the ATN line
#define TIMING_BETWEEN_BYTES 200 // delay time between bytes to allow receiver to signal busy
#define TIMING_NO_EOI 45 // Timing delay after receiver indicated ready to wait before signaling no EOI
#define TIMING_EOI 400 // Timing delay after receiver indicated ready to wait before signaling EOI
#define TIMING_EOI_DELAY 62 // TODO temporary. Time to wait after EOI acknowledge. Should use timers to ensure proper timing
#define TIMING_SEND_DELAY 45 // Small delay between bits
#define TIMING_BIT_DELAY 25 // Time to wait after setting the data bit before pulsing clock
#define TIMING_BIT 80 // Clock cycle to use while sending bits

// TODO use later when reading or during idle bus
bool checkATN = false;

#define TIM_PERIOD 7887
#define TIM_PRESCALER 0
#define TIM_CLK_PULSE 5754
#define TIM_DATA_PULSE 3696

TIM_TypeDef* htim = TIM3;

// NOTE: This function is for channel 1 and 2 only
void TIM_ConfigChannelPWM(uint32_t channel, uint32_t mode)
{
	htim->CCER &= ~((TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NP | TIM_CCER_CC1NE) << channel);
	htim->CCER |= (TIM_OCPOLARITY_HIGH) << channel;

	htim->CCMR1 &= ~((TIM_CCMR1_OC1M | TIM_CCMR1_CC1S | TIM_CCMR1_OC1FE | TIM_CCMR1_OC1PE) << (channel * 2));
	htim->CCMR1 |= (mode) << (channel * 2); // PWM mode
	htim->CCMR1 |= TIM_OCFAST_DISABLE << (channel * 2);
}

void TIM_Init()
{
	htim->CR1 &= ~TIM_CR1_CEN; // Ensure timer is disabled before configuring
	__HAL_RCC_TIM3_CLK_ENABLE();

	htim->CR1 &= ~(TIM_CR1_DIR | TIM_CR1_CMS | TIM_CR1_CKD | TIM_CR1_ARPE);
	htim->CR1 |= TIM_COUNTERMODE_UP;
	htim->CR1 |= TIM_CLOCKDIVISION_DIV1;
	htim->CR1 |= TIM_AUTORELOAD_PRELOAD_DISABLE;
	htim->CR1 |= TIM_CR1_URS;

	htim->ARR = TIM_PERIOD;
	htim->PSC = TIM_PRESCALER;

	/* Generate an update event to reload the Prescaler
	 and the repetition counter (only for advanced timer) value immediately */
	htim->EGR = TIM_EGR_UG;

	// Config master sync
	htim->SMCR &= ~(TIM_SMCR_SMS | TIM_SMCR_TS | TIM_SMCR_ETF | TIM_SMCR_ETPS | TIM_SMCR_ECE | TIM_SMCR_ETP | TIM_SMCR_MSM);
	htim->SMCR |= TIM_MASTERSLAVEMODE_DISABLE;
	htim->CR2 &= ~(TIM_CR2_MMS);
	htim->CR2 |= TIM_TRGO_RESET;

	// Configure outputs
	TIM_ConfigChannelPWM(TIM_CHANNEL_1, TIM_OCMODE_PWM1);
	htim->CCR1 = TIM_DATA_PULSE;

	TIM_ConfigChannelPWM(TIM_CHANNEL_2, TIM_OCMODE_PWM2);
	htim->CCR2 = TIM_CLK_PULSE;

	/**TIM3 GPIO Configuration
	PA6     ------> TIM3_CH1
	PA7     ------> TIM3_CH2
	*/

	// Enable channel 1 and 2
	htim->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
	htim->CR1 |= TIM_CR1_CEN; // enable timer
}

void delayMicroseconds(uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&htim7, 0);
	while (__HAL_TIM_GET_COUNTER(&htim7) < us);
}

/*IECResult*/void timeoutWait(GpioPin* pin, bool waitForSignal/*, uint32_t timeout*/)
{
	//uint32_t time = 0;
	bool state;

	while(1)
	{
		state = pin->read();
		if (state == waitForSignal)
		{
			return;
		}
	}

	/*while((timeout == 0) || (time < timeout)) {
		// Check for Attention inerrupt
		if (!checkATN && m_atnPin.read())
		{
			return IECResult::AttentionInterrupt;
		}

		// Check the waiting condition:
		state = waitBit->read();

		if (state == waitForSignal) {
			return IECResult::Success;
		}

		//delayMicroseconds(5); // The aim is to make the loop at least 3 us
		//time += 5;
	}

	return IECResult::Timeout;*/
}

IEC::IEC(GpioPin reset, GpioPin atn, GpioPin clk, GpioPin data)
{
	this->RESET = reset;
	this->ATN = atn;
	this->CLK = clk;
	this->DATA = data;

	const int nPins = 4;
	GpioPin* pins[nPins] = { &RESET, &ATN, &CLK, &DATA };
	for (int i = 0; i < nPins; i++)
	{
		pins[i]->setPull(GpioPull::None);
		pins[i]->setMode(GpioMode::Input);
		pins[i]->setOpenDrain(true);
		pins[i]->setAlternateFunction(0);
		pins[i]->setSpeed(GpioSpeed::Low);
	}

	TIM_Init();
	CLK.setAlternateFunction(2);
	DATA.setAlternateFunction(2);
}

void IEC::command(bool start)
{
	if (start)
	{
		delayMicroseconds(TIMING_CMD_DELAY_AFTER_BYTE);

		// Initiate ATN
		ATN.writeLow();
		ATN.setMode(GpioMode::Output);
		delayMicroseconds(TIMING_CMD_START_DELAY);
		CLK.writeLow();
		CLK.setMode(GpioMode::Output);

		// Wait for listener to receive attention
		timeoutWait(&DATA, false);
	}
	else
	{
		delayMicroseconds(TIMING_CMD_END);
		ATN.writeHigh();
		ATN.setMode(GpioMode::Input);
	}
}

void IEC::end()
{
	// Wait for receivers to indicate no listeners
	timeoutWait(&DATA, true);
	CLK.writeHigh();
	CLK.setMode(GpioMode::Input);
}

void IEC::reset()
{
	RESET.writeLow();
	RESET.setMode(GpioMode::Output);
	HAL_Delay(1000);
	RESET.writeHigh();
	RESET.setMode(GpioMode::Input);
	HAL_Delay(3000); // Give time for device to reset
}

void IEC::send(uint8_t data, bool signalEOI)
{
	delayMicroseconds(TIMING_BETWEEN_BYTES);

	// Indicate we are ready to send data
	CLK.writeHigh();

	// Wait for listener to be ready
	timeoutWait(&DATA, true); // Must wait indefinitely

	if(signalEOI) {
		// get eoi acknowledge
		timeoutWait(&DATA, false);

		// Wait for listener to end eoi acknowledge
		timeoutWait(&DATA, true);

		// Prepare for transmission
		delayMicroseconds(TIMING_EOI_DELAY);
	}
	else
	{
		// Clock must go true within 200us to indicate no EOI
		delayMicroseconds(TIMING_NO_EOI);
	}

	// Switch pins to be timer controlled. Should automatically pull CLK low.
	CLK.writeLow();
	DATA.writeHigh();
	DATA.setMode(GpioMode::Output);
	htim->CCR1 = 60000; // By default leave data disabled
	htim->CNT = 0;
	CLK.setMode(GpioMode::AlternateFunction);
	DATA.setMode(GpioMode::AlternateFunction);

	// Send bits, LSB first
	for (int i = 0; i < 8; i++)
	{
		// Load data bit into timer
		// 3696 = ~44us
		// 60k is larger than timer max value, just to prevent data pin from being pulled low
		htim->CCR1 = (data & 1) ? 60000 : TIM_DATA_PULSE;

		// Wait for timer to pass the data threshold
		while(htim->CNT <= TIM_DATA_PULSE);

		// Wait for timer rollover
		while(htim->CNT >= TIM_DATA_PULSE);

		data >>= 1;
	}

	// Disables timer and returns control to normal GPIO operations
	// Should return to DATA pin as floating input, and CLK pulled low
	CLK.setMode(GpioMode::Output);
	DATA.setMode(GpioMode::Input);

	// End transmission, give time for receiver to indicate busy state
	// TODO if DATA pin doesnt immediately go high after transmission (i.e. before)
	// receiver acknowledge) then there was an error
	delayMicroseconds(1000);

	// Wait for receivers to be ready again
	timeoutWait(&DATA, false); // Receiver has 1000 us to indicate receive OK
}
