#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define BLOCK_SIZE (4*1024)

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

volatile unsigned* setup_io();
unsigned readAdc(unsigned adcnum);

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(gpio,g) *((gpio)+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(gpio,g) *((gpio)+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(gpio,g,a) *((gpio)+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET(gpio,pin) *((gpio)+7) = 1<<(pin); usleep(1) // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR(gpio,pin) *((gpio)+10) = 1<<(pin); usleep(1) // clears bits which are 1 ignores bits which are 0
#define GPIO_READ(gpio, pin) (((*((gpio)+13)) & (1 << (pin))) != 0)

#define SPICLK  (18)
#define SPIMISO (23)
#define SPIMOSI (24)
#define SPICS   (25)

volatile unsigned *gpio;

int main(int argc, char **argv) {
  unsigned sin_raw, cos_raw;

  // Set up gpi pointer for direct register access
  gpio = setup_io();

  // set up the SPI interface pins
  INP_GPIO(gpio, SPIMOSI);
  OUT_GPIO(gpio, SPIMOSI);
  INP_GPIO(gpio, SPIMISO);
  INP_GPIO(gpio, SPICLK);
  OUT_GPIO(gpio, SPICLK);
  INP_GPIO(gpio, SPICS);
  OUT_GPIO(gpio, SPICS);

  for (;;) {
    sin_raw = readAdc(1);
    cos_raw = readAdc(2);
    printf("%d %d\n", sin_raw, cos_raw);
    usleep(200);
  }

  return 0;
}

// Set up a memory regions to access GPIO
volatile unsigned* setup_io() {
   int mem_fd;

   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   void* gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   return (volatile unsigned *)gpio_map;
}

// read SPI data from MCP3008 chip, 8 possible adc's (0 thru 7)
unsigned readAdc(unsigned adcnum) {
  int i;

  if ((adcnum > 7) || (adcnum < 0)) {
    return -1;
  }
  GPIO_SET(gpio, SPICS);
  GPIO_CLR(gpio, SPICLK);
  GPIO_CLR(gpio, SPICS);

  // start bit + single-ended bit, we only need to send 5 bits here
  unsigned commandout = adcnum;
  commandout |= 0x18;  // start bit + single-ended bit
  commandout <<= 3;    // we only need to send 5 bits here

  // unsigned commandout = (adcnum | 0x18) << 3;
  for (i=0; i<5; i++) {
    if (commandout & 0x80) {
      GPIO_SET(gpio, SPIMOSI);
    } else {
      GPIO_CLR(gpio, SPIMOSI);
    }
    commandout <<= 1;
    GPIO_SET(gpio, SPICLK);
    GPIO_CLR(gpio, SPICLK);
  }

  unsigned adcout = 0;
  // read in one empty bit, one null bit and 10 ADC bits
  for (i=0; i<12; i++) {
    GPIO_SET(gpio, SPICLK);
    GPIO_CLR(gpio, SPICLK);
    adcout <<= 1;
    if (GPIO_READ(gpio, SPIMISO)) {
      adcout |= 0x1;
    }
  }

  GPIO_SET(gpio, SPICS);
  
  adcout >>= 1;       // first bit is 'null' so drop it
  return adcout;
}

