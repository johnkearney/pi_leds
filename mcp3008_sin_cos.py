#!/usr/bin/env python
#
# Interface code for MCP3008
# based on git://gist.github.com/3151375.git 
#

import time
import os
import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)
DEBUG = 1

# read SPI data from MCP3008 chip, 8 possible adc's (0 thru 7)
def readadc(adcnum, clockpin, mosipin, misopin, cspin):
        if ((adcnum > 7) or (adcnum < 0)):
                return -1
        GPIO.output(cspin, True)

        GPIO.output(clockpin, False)  # start clock low
        GPIO.output(cspin, False)     # bring CS low

        commandout = adcnum
        commandout |= 0x18  # start bit + single-ended bit
        commandout <<= 3    # we only need to send 5 bits here
        for i in range(5):
                if (commandout & 0x80):
                        GPIO.output(mosipin, True)
                else:
                        GPIO.output(mosipin, False)
                commandout <<= 1
                GPIO.output(clockpin, True)
                GPIO.output(clockpin, False)

        adcout = 0
        # read in one empty bit, one null bit and 10 ADC bits
        for i in range(12):
                GPIO.output(clockpin, True)
                GPIO.output(clockpin, False)
                adcout <<= 1
                if (GPIO.input(misopin)):
                        adcout |= 0x1

        GPIO.output(cspin, True)
        
        adcout >>= 1       # first bit is 'null' so drop it
        return adcout

# change these as desired - they're the pins connected from the
# SPI port on the ADC to the Cobbler
SPICLK = 18
SPIMISO = 23
SPIMOSI = 24
SPICS = 25

# set up the SPI interface pins
GPIO.setup(SPIMOSI, GPIO.OUT)
GPIO.setup(SPIMISO, GPIO.IN)
GPIO.setup(SPICLK, GPIO.OUT)
GPIO.setup(SPICS, GPIO.OUT)

while True:
        # read the analog pins

        # 10k trim pot connected to adc #0
        trim_pot = readadc(0, SPICLK, SPIMOSI, SPIMISO, SPICS)

        # sin connected to adc #1
        sin_raw = readadc(1, SPICLK, SPIMOSI, SPIMISO, SPICS)

        # cos connected to adc #2
        cos_raw = readadc(2, SPICLK, SPIMOSI, SPIMISO, SPICS)

        #print "trim_pot:", trim_pot
        #print "sin_raw:", sin_raw
        #print "cos_raw:", cos_raw
        print sin_raw, cos_raw

        # hang out and do nothing for a half second
        time.sleep(0.05)

