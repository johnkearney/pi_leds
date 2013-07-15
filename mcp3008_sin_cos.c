#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define BLOCK_SIZE (4*1024)

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

volatile unsigned* setup_io();
int readAdc(volatile unsigned *gpio, int adcnum, int clockpin, int mosipin, int misopin, int cspin);

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(gpio,g) *((gpio)+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(gpio,g) *((gpio)+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(gpio,g,a) *((gpio)+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET(gpio,pin) *((gpio)+7) = 1<<(pin)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR(gpio,pin) *((gpio)+10) = 1<<(pin) // clears bits which are 1 ignores bits which are 0
#define GPIO_READ(gpio, pin) (*((gpio)+13) & (1 << (pin)) != 0)

#define SPICLK  (18)
#define SPIMISO (23)
#define SPIMOSI (24)
#define SPICS   (25)

int main(int argc, char **argv) {
  // Set up gpi pointer for direct register access
  volatile unsigned *gpio = setup_io();

  // set up the SPI interface pins
  INP_GPIO(gpio, SPIMOSI);
  OUT_GPIO(gpio, SPIMOSI);
  INP_GPIO(gpio, SPIMISO);
  INP_GPIO(gpio, SPICLK);
  OUT_GPIO(gpio, SPICLK);
  INP_GPIO(gpio, SPICS);
  OUT_GPIO(gpio, SPICS);

  for (;;) {
    int sin_raw = readadc(gpio, 1, SPICLK, SPIMOSI, SPIMISO, SPICS);
    int cos_raw = readadc(gpio, 1, SPICLK, SPIMOSI, SPIMISO, SPICS);
    printf("%d %d\n", sin_raw, cos_raw);
    sleep(1);
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
int readAdc(volatile unsigned *gpio, int adcnum, int clockpin, int mosipin, int misopin, int cspin) {
  int i;

  if ((adcnum > 7) || (adcnum < 0)) {
    return -1;
  }
  GPIO_SET(gpio, cspin);
  //GPIO.output(cspin, True)

  GPIO_CLR(gpio, clockpin);
  //GPIO.output(clockpin, False)  // start clock low
  GPIO_CLR(gpio, cspin);
  //GPIO.output(cspin, False)     // bring CS low

  // start bit + single-ended bit, we only need to send 5 bits here
  int commandout = (adcnum | 0x18) << 3;
  for (i=0; i<5; i++) {
    if (commandout & 0x80) {
      GPIO_SET(gpio, mosipin);
      //GPIO.output(mosipin, True)
    } else {
      GPIO_CLR(gpio, mosipin);
      //GPIO.output(mosipin, False)
    }
    commandout <<= 1
    GPIO_SET(gpio, clockpin);
    //GPIO.output(clockpin, True)
    GPIO_CLR(gpio, clockpin);
    //GPIO.output(clockpin, False)
  }

  int adcout = 0;
  // read in one empty bit, one null bit and 10 ADC bits
  for (i=0; i<12; i++) {
    GPIO_SET(gpio, clockpin);
    //GPIO.output(clockpin, True)
    GPIO_CLR(gpio, clockpin);
    //GPIO.output(clockpin, False)
    adcout <<= 1;
    //if (GPIO.input(misopin)) {
    if (GPIO_READ(gpio, misopin)) {
      adcout |= 0x1;
    }
  }

  GPIO_SET(gpio, cspin);
  //GPIO.output(cspin, True)
  
  adcout >>= 1;       // first bit is 'null' so drop it
  return adcout;
}

