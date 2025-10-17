#ifndef GPIO_PIN_H
#define GPIO_PIN_H

#include "stm32f4xx_hal.h"
#include "stdint.h"

enum class GpioMode : uint8_t
{
    Input = 0,
    Output = 1,
    AlternateFunction = 2,
    Analog = 3
};

enum class GpioSpeed : uint8_t
{
    Low = 0,
    Medium = 1,
    High = 2,
    VeryHigh = 3
};

enum class GpioPull : uint8_t
{
    None = 0,
    PullUp = 1,
    PullDown = 2
};

class GpioPin
{
public:
	GpioPin(){}
    GpioPin(GPIO_TypeDef* port, uint32_t pin);
    void setMode(GpioMode mode);
    void setOpenDrain(bool isOpenDrain);
    void setSpeed(GpioSpeed speed);
    void setPull(GpioPull pull);
    bool read();
    void writeHigh();
    void writeLow();
    void write(bool state);
    void setAlternateFunction(uint8_t af);

private:
    GPIO_TypeDef* port;
    uint32_t mask1;
    uint32_t mask2;
    uint32_t mask_afr;
    uint8_t afr_index;
    uint8_t afr_offset;
    uint8_t pin;
};

#endif
