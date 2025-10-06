A Commodore64 IEC interface designed to interface with an old dot matrix printer. Hopefully the project will be later expanded to support sending data to the C64 as well.

The interface uses an STM32 microprocessor to read commands from UART then send that data over the IEC interface.

The code is built using the STM32CubeIDE. The file Debug/C64_IEC_Interface.bin is what gets uploaded to the STM32.

At the moment this project targets an STM32F100RB, also known as the STM32VLDiscovery board.