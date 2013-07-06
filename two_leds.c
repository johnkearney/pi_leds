//
// How to access GPIO registers from C-code on the Raspberry-Pi
// Based on code from: http://elinux.org/Rpi_Low-level_peripherals#GPIO_Driving_Example_.28C.29
//

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define BLOCK_SIZE (4*1024)

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

volatile unsigned* setup_io();

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(gpio,g) *((gpio)+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(gpio,g) *((gpio)+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(gpio,g,a) *((gpio)+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET(gpio,pin) *((gpio)+7) = 1<<(pin)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR(gpio,pin) *((gpio)+10) = 1<<(pin) // clears bits which are 1 ignores bits which are 0

int main(int argc, char **argv) {
  int rep;

  // Set up gpi pointer for direct register access
  volatile unsigned *gpio = setup_io();

  // Switch GPIO 17, 22 to output mode
  INP_GPIO(gpio, 17); // must use INP_GPIO before we can use OUT_GPIO
  OUT_GPIO(gpio, 17);
  INP_GPIO(gpio, 22); // must use INP_GPIO before we can use OUT_GPIO
  OUT_GPIO(gpio, 22);

  for (rep=0; rep<10; rep++) {
    GPIO_SET(gpio, 17);
    sleep(1);
    GPIO_SET(gpio, 22);
    GPIO_CLR(gpio, 17);
    sleep(1);
    GPIO_CLR(gpio, 22);
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

