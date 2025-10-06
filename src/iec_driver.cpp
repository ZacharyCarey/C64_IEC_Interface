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

constexpr IOPin ClkPin{CLK_GPIO_Port, CLK_Pin};
constexpr IOPin DataPin{DATA_GPIO_Port, DATA_Pin};
constexpr IOPin AtnPin{ATN_GPIO_Port, ATN_Pin};
constexpr IOPin ResetPin{Reset_GPIO_Port, Reset_Pin};
constexpr IOPin SrqPin{SRQ_GPIO_Port, SRQ_Pin};

TIM_TypeDef* htim = TIM2;
void TIM2_Init()
{
	// Enable clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// Timer config
	htim->CR1 &= ~TIM_CR1_CEN; // Ensure timer is disabled before configuring
	htim->CR1 &= ~TIM_CR1_DIR; // Count up
	htim->PSC = 0; // Set prescaler
	htim->ARR = 2253; // timer maximum, 93.9us
	htim->CCR3 = 1644; // compare value, 68.5us
	htim->CCR4 = 1056; // compare value, 44us

	// Set PWM mode for channel 3
	htim->CCMR2 &= ~TIM_CCMR2_OC3M_Msk;
	htim->CCMR2 |= (TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2); // 111 - PWM mode 2

	// Set PWM mode for channel 4
	htim->CCMR2 &= ~TIM_CCMR2_OC4M_Msk;
	htim->CCMR2 |= (TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2); // 110 - PWM mode 1

	htim->CCER &= ~(TIM_CCER_CC3P | TIM_CCER_CC4P); // Select output polarity to 0 - active high
	htim->CCER |= (TIM_CCER_CC3E | TIM_CCER_CC4E); // Enable outputs of channel 3 and 4

	// configure GPIO
	//__HAL_RCC_GPIOB_CLK_ENABLE();
	//GPIO_InitTypeDef GPIO_InitStruct = {0};
	/**TIM2 GPIO Configuration
	PB10     ------> TIM2_CH3
	PB11     ------> TIM2_CH4
	*/
	/*GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);*/
	__HAL_AFIO_REMAP_TIM2_PARTIAL_2();

	// enable timer
	htim->CR1 |= TIM_CR1_CEN;
}

const uint32_t ClkChannel = GPIO_CRH_CNF10_1;
const uint32_t DataChannel = GPIO_CRH_CNF11_1;
void TIM2_EnableChannels(uint32_t channel_flags)
{
	GPIOB->CRH |= channel_flags;
}

void TIM2_DisableChannels(uint32_t channel_flags)
{
	GPIOB->CRH &= ~channel_flags;
}

void delayMicroseconds(uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&htim7, 0);
	while (__HAL_TIM_GET_COUNTER(&htim7) < us);
}

/*IECResult*/void timeoutWait(IOPin waitPin, bool waitForSignal/*, uint32_t timeout*/)
{
	//uint32_t time = 0;
	bool state;

	while(1)
	{
		state = waitPin.read();
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
	TIM2_Init();
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
		ClkPin.write(false);
		ClkPin.pinMode_Output();

		// Wait for listener to receive attention
		timeoutWait(DataPin, false);
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
	timeoutWait(DataPin, true);
	ClkPin.write(true);
	ClkPin.pinMode_Input();
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
	ClkPin.write(true);

	// Wait for listener to be ready
	timeoutWait(DataPin, true); // Must wait indefinitely

	if(signalEOI) {
		// get eoi acknowledge
		timeoutWait(DataPin, false);

		// Wait for listener to end eoi acknowledge
		timeoutWait(DataPin, true);

		// Prepare for transmission
		delayMicroseconds(TIMING_EOI_DELAY);
	}
	else
	{
		// Clock must go true within 200us to indicate no EOI
		delayMicroseconds(TIMING_NO_EOI);
	}

	// Switch pins to be timer controlled. Should automatically pull CLK low.
	ClkPin.write(false);
	DataPin.write(true);
	DataPin.pinMode_Output();
	htim->CCR4 = 60000; // By default leave data disabled
	htim->CNT = 0;
	TIM2_EnableChannels(ClkChannel | DataChannel);
	//HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_SET);

	// Send bits, LSB first
	for (int i = 0; i < 8; i++)
	{
		// Load data bit into timer
		// 1056 = ~44us
		// 60k is larger than timer max value, just to prevent data pin from being pulled low
		htim->CCR4 = (data & 1) ? 60000 : 1056;

		// Wait for timer to pass the data threshold
		while(htim->CNT <= 1056);

		// Wait for timer rollover
		while(htim->CNT >= 1056);

		data >>= 1;
	}

	// Disables timer and returns control to normal GPIO operations
	// Should return to DATA pin as floating input, and CLK pulled low
	TIM2_DisableChannels(ClkChannel | DataChannel);
	DataPin.pinMode_Input();
	//HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);

	// End transmission, give time for receiver to indicate busy state
	// TODO if DATA pin doesnt immediately go high after transmission (i.e. before)
	// receiver acknowledge) then there was an error
	delayMicroseconds(1000);

	// Wait for receivers to be ready again
	timeoutWait(DataPin, false); // Receiver has 1000 us to indicate receive OK
}
