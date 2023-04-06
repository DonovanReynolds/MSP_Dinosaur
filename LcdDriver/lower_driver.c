// This code was ported from TI's sample code. See Copyright notice at the bottom of this file.

#include <Grlib/grlib/grlib.h>
#include <LcdDriver/lower_driver.h>
#include "msp430fr6989.h"
#include <stdint.h>


// Configure the pins
void HAL_LCD_PortInit(void);
// Configure eUSCI for SPI operation
void HAL_LCD_SpiInit(void);


void HAL_LCD_PortInit(void)
{
    /////////////////////////////////////
    // Configuring the SPI pins
    /////////////////////////////////////

    // Configure UCB0CLK/P1.4 pin to serial clock
    P1SEL0 |= BIT6 | BIT4;
    // Configure UCB0SIMO/P1.6 pin to SIMO

    // OK to ignore UCB0STE/P1.5 since we'll connect the display's enable bit to low (enabled all the time)
    // OK to ignore UCB0SOMI/P1.7 since the display doesn't give back any data

    ///////////////////////////////////////////////
    // Configuring the display's other pins
    ///////////////////////////////////////////////
    // Set reset pin as output
    P9DIR |= BIT4;
    // Set the data/command pin as output
    P2DIR |= BIT3;
    // Set the chip select pin as output
    P2DIR |= BIT5;
    return;
}

void HAL_LCD_SpiInit(void)
{
    //////////////////////////
    // SPI configuration
    //////////////////////////

    // Put eUSCI in reset state and set all fields in the register to 0
    UCB0CTLW0 = UCSWRST;

    // Fields that need to be nonzero are changed below

    // Set clock phase to "capture on 1st edge, change on following edge"
    UCB0CTLW0 |= UCCKPH;
    // Set clock polarity to "inactive low"
    UCB0CTLW0 &= ~UCCKPL;
    // Set data order to "transmit MSB first"
    UCB0CTLW0 |= UCMSB;
    // Set data size to 8-bit
    UCB0CTLW0 &= ~UC7BIT;
    // Set MCU to "SPI master"
    UCB0CTLW0 |= UCMST;
    // Set SPI to "3-pin SPI" (we won't use eUSCI's chip select)
    UCB0CTLW0 &= ~UCMODE_3;
    // Set module to synchronous mode
    UCB0CTLW0 |= UCSYNC;
    // Set clock to SMCLK
    UCB0CTLW0 |= UCSSEL_2;

    // Configure the clock divider (SMCLK is from DCO at 8 MHz; run SPI at 8 MHz using SMCLK)
    UCB0BRW = 1;

    // Exit the reset state at the end of the configuration
    UCB0CTLW0 &= ~UCSWRST;

    // Set CS' (chip select) bit to 0 (display always enabled)
    P2OUT &= ~BIT5;
    // Set DC' bit to 0 (assume data)
    P2OUT &= ~BIT3;
    //*/

    return;
}


//*****************************************************************************
// Writes a command to the CFAF128128B-0145T.  This function implements the basic SPI
// interface to the LCD display.
//*****************************************************************************
void HAL_LCD_writeCommand(uint8_t command)
{
    // For command, set the DC' bit to low before transmission
    P2OUT &= ~BIT3;

    // Wait as long as the module is busy
    while (UCB0STATW & UCBUSY);

    // Transmit data
    UCB0TXBUF = command;

    // Set DC' bit back to high
    P2OUT |= BIT3;
}


//*****************************************************************************
// Writes a data to the CFAF128128B-0145T.  This function implements the basic SPI
// interface to the LCD display.
//*****************************************************************************
void HAL_LCD_writeData(uint8_t data)
{
    // Wait as long as the module is busy
    while (UCB0STATW & UCBUSY);

    // Transmit data
    UCB0TXBUF = data;
}








/* --COPYRIGHT--,BSD
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

