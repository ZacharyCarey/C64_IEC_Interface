#include "GpioPin.h"

#define SET_BITS(REG, MASK, VALUE) ((REG) = ((REG) & ~(MASK)) | (VALUE))

GpioPin::GpioPin(GPIO_TypeDef* port, uint32_t pin)
{
    uint8_t pinNumber = 0;
    if ( pin != 0)
    {
        for(; pin > 0; pinNumber++)
        {
            pin >>= 1;
        }
        pinNumber--;
    }

    pinNumber &= 0xF; // Only 15 pins
    this->pin = pinNumber;
    this->port = port;
    this->mask1 = 0x1 << pinNumber;
    this->mask2 = 0x3 << (pinNumber * 2);

    if (pinNumber <= 7)
    {
        afr_index = 0;
        afr_offset = (pinNumber * 4);
        mask_afr = 0xF << afr_offset;
    } else {
        afr_index = 1;
        afr_offset = ((pinNumber - 8) * 4);
        mask_afr = 0xF << afr_offset;
    }
}

void GpioPin::setMode(GpioMode mode)
{
    SET_BITS(port->MODER, mask2, ((uint8_t)mode & 0x3) << (pin * 2));
}

void GpioPin::setOpenDrain(bool isOpenDrain)
{
    SET_BITS(port->OTYPER, mask1, ((uint8_t)isOpenDrain & 0x1) << pin);
}

void GpioPin::setSpeed(GpioSpeed speed)
{
    SET_BITS(port->OSPEEDR, mask2, ((uint8_t)speed & 0x3) << (pin * 2));
}

void GpioPin::setPull(GpioPull pull)
{
    SET_BITS(port->PUPDR, mask2, ((uint8_t)pull & 0x3) << (pin * 2));
}

bool GpioPin::read()
{
    return (port->IDR & mask1) != 0;
}

void GpioPin::writeHigh()
{
    port->BSRR = mask1;
}

void GpioPin::writeLow()
{
    port->BSRR = mask1 << 16;
}

void GpioPin::write(bool state)
{
    port->BSRR = mask1 << ((state & 0x1) * 16);
}

void GpioPin::setAlternateFunction(uint8_t af)
{
    SET_BITS(port->AFR[afr_index], mask_afr, (af & 0xF) << afr_offset);
}
