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


#define CLK_ModeMask 0xC000
//#define CLK_ModeBit(X) ((X) << 14)
#define CLK_ModeBit 14
#define DAT_ModeMask 0x3000
//#define DAT_ModeBit(X) ((X) << 12)
#define DAT_ModeBit 12
#define MODE_Input 0
#define MODE_Output 1
#define MODE_Timer 2
#define SET_MODE(PORT, BIT, MODE) ((PORT)->MODER = ((PORT)->MODER & ~(3 << (BIT))) | ((MODE) << (BIT)))
#define PIN_HIGH(PORT, PIN) ((PORT)->BSRR |= (PIN))
#define PIN_LOW(PORT, PIN) ((PORT)->BSRR |= (PIN) << 16)

// TODO use later when reading or during idle bus
bool checkATN = false;

// Wrap pins in this helpful struct
struct IOPin
{
	GPIO_TypeDef* port;
    uint16_t pin;

    inline void pinMode_Input() const
    {
    	// TODO optimize this function?
		GPIO_InitTypeDef GPIO_InitStruct = {0};
		GPIO_InitStruct.Pin = pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(port, &GPIO_InitStruct);
    }

    inline void pinMode_Output() const
    {
    	// TODO optimize this function?
		GPIO_InitTypeDef GPIO_InitStruct = {0};
		GPIO_InitStruct.Pin = pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(port, &GPIO_InitStruct);
    }

    // false = pull pin LOW (logical high)
    // true = pull pin HIGH (logical low)
    inline void write(bool state) const
    {
    	// TODO optimize?
    	HAL_GPIO_WritePin(port, pin, (GPIO_PinState)state);
    }

    // false = pin state LOW (logical high)
    // true = pin state HIGH (logical low)
    inline bool read() const
    {
    	return HAL_GPIO_ReadPin(port, pin);
    }
};

//constexpr IOPin ClkPin{CLK_GPIO_Port, CLK_Pin};
//constexpr IOPin DataPin{DAT_GPIO_Port, DAT_Pin};
constexpr IOPin AtnPin{ATN_GPIO_Port, ATN_Pin};
constexpr IOPin ResetPin{RESET_GPIO_Port, RESET_Pin};
//constexpr IOPin SrqPin{SRQ_GPIO_Port, SRQ_Pin};

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

/*const uint32_t ClkChannel = GPIO_MODER_MODE7_1;
const uint32_t DataChannel = GPIO_MODER_MODE6_1;
void TIM_EnableChannels(uint32_t channel_flags)
{
	GPIOA->MODER |= channel_flags;
}

void TIM_DisableChannels(uint32_t channel_flags)
{
	GPIOA->MODER &= ~channel_flags;
}*/

void delayMicroseconds(uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&htim7, 0);
	while (__HAL_TIM_GET_COUNTER(&htim7) < us);
}

/*IECResult*/void timeoutWaitData(bool waitForSignal/*, uint32_t timeout*/)
{
	//uint32_t time = 0;
	bool state;

	while(1)
	{
		//state = waitPin.read();
		state = (DAT_GPIO_Port->IDR & DAT_Pin) != 0;
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

void iec_init()
{
	TIM_Init();
}

void iec_command(bool start)
{
	if (start)
	{
		delayMicroseconds(TIMING_CMD_DELAY_AFTER_BYTE);

		// Initiate ATN
		AtnPin.write(false);
		AtnPin.pinMode_Output();
		delayMicroseconds(TIMING_CMD_START_DELAY);
		PIN_LOW(CLK_GPIO_Port, CLK_Pin); //ClkPin.write(false);
		SET_MODE(CLK_GPIO_Port, CLK_ModeBit, MODE_Output);//ClkPin.pinMode_Output();

		// Wait for listener to receive attention
		timeoutWaitData(false);
	}
	else
	{
		delayMicroseconds(TIMING_CMD_END);
		AtnPin.write(true);
		AtnPin.pinMode_Input();
	}
}

void iec_end()
{
	// Wait for receivers to indicate no listeners
	timeoutWaitData(true);
	PIN_HIGH(CLK_GPIO_Port, CLK_Pin); // ClkPin.write(true);
	SET_MODE(CLK_GPIO_Port, CLK_ModeBit, MODE_Input); //ClkPin.pinMode_Input();
}

void iec_reset()
{
	ResetPin.write(false);
	ResetPin.pinMode_Output();
	HAL_Delay(1000);
	ResetPin.write(true);
	ResetPin.pinMode_Input();
	HAL_Delay(3000); // Give time for device to reset
}

void iec_send(uint8_t data, bool signalEOI)
{
	delayMicroseconds(TIMING_BETWEEN_BYTES);

	// Indicate we are ready to send data
	PIN_HIGH(CLK_GPIO_Port, CLK_Pin); //ClkPin.write(true);

	// Wait for listener to be ready
	timeoutWaitData(true); // Must wait indefinitely

	if(signalEOI) {
		// get eoi acknowledge
		timeoutWaitData(false);

		// Wait for listener to end eoi acknowledge
		timeoutWaitData(true);

		// Prepare for transmission
		delayMicroseconds(TIMING_EOI_DELAY);
	}
	else
	{
		// Clock must go true within 200us to indicate no EOI
		delayMicroseconds(TIMING_NO_EOI);
	}

	// Switch pins to be timer controlled. Should automatically pull CLK low.
	PIN_LOW(CLK_GPIO_Port, CLK_Pin); //ClkPin.write(false);
	PIN_HIGH(DAT_GPIO_Port, DAT_Pin); //DataPin.write(true);
	SET_MODE(DAT_GPIO_Port, DAT_ModeBit, MODE_Output); //DataPin.pinMode_Output();
	htim->CCR1 = 60000; // By default leave data disabled
	htim->CNT = 0;
	//TIM_EnableChannels(ClkChannel | DataChannel);
	SET_MODE(CLK_GPIO_Port, CLK_ModeBit, MODE_Timer);
	SET_MODE(DAT_GPIO_Port, DAT_ModeBit, MODE_Timer);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_SET);

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
	//TIM_DisableChannels(ClkChannel | DataChannel);
	SET_MODE(CLK_GPIO_Port, CLK_ModeBit, MODE_Output);
	SET_MODE(DAT_GPIO_Port, DAT_ModeBit, MODE_Input);;//DataPin.pinMode_Input();
	//HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);

	// End transmission, give time for receiver to indicate busy state
	// TODO if DATA pin doesnt immediately go high after transmission (i.e. before)
	// receiver acknowledge) then there was an error
	delayMicroseconds(1000);

	// Wait for receivers to be ready again
	timeoutWaitData(false); // Receiver has 1000 us to indicate receive OK
}
