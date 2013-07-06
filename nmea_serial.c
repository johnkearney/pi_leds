//
// Code based heavily on http://www.raspberry-projects.com/pi/programming-in-c/uart-serial-port/using-the-uart
//

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define SERIAL_DEVICE "/dev/ttyUSB0"
#define NMEA_MAX 82

int parse_sentence(const char* str);
int setup_uart();

int main() {
    int fd = setup_uart();
    if (fd == -1) {
        return 1;
    }

    //----- TX BYTES -----
    //unsigned char tx_buffer[NMEA_MAX];
    //memset(tx_buffer, 0, NMEA_MAX);

    //const char* data = "$JKMSG,*\r\n";
    //memcpy(tx_buffer, data, sizeof(data));
    
    //int count = write(fd, tx_buffer, sizeof(data));
    //if (count < 0) {
    //    printf("UART TX error\n");
    //}

    for (;;) {
        //----- CHECK FOR ANY RX BYTES -----
        // Read up to 255 characters from the port if they are there
        unsigned char rx_buffer[NMEA_MAX];
        int rx_length = read(fd, rx_buffer, NMEA_MAX);
        if (rx_length < 0) {
            //An error occured (will occur if there are no bytes)
        } else if (rx_length == 0) {
            //No data waiting
        } else {
            //Bytes received
            rx_buffer[rx_length] = '\0';
            puts(rx_buffer);
            //printf("%i bytes read : %s\n", rx_length, rx_buffer);
        }
    }
    

    //----- CLOSE THE UART -----
    close(fd);

    return 0;
}

int setup_uart() {
    //-------------------------
    //----- SETUP USART 0 -----
    //-------------------------
    //At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
    //
    //OPEN THE UART
    //The flags (defined in fcntl.h):
    //  Access modes (use 1 of these):
    //      O_RDONLY - Open for reading only.
    //      O_RDWR - Open for reading and writing.
    //      O_WRONLY - Open for writing only.
    //
    //  O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
    //                                          if there is no input immediately available (instead of blocking). Likewise, write requests can also return
    //                                          immediately with a failure status if the output can't be written immediately.
    //
    //  O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
    //int fd = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);      //Open in non blocking read/write mode
    int fd = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY);      //Open in non blocking read/write mode
    if (fd == -1) {
        printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
        return -1;
    }
    
    struct termios options;
    if (tcgetattr(fd, &options) != 0) {
        printf("error %d from tcgetattr", errno);
        close(fd);
        return -1;
    }
    options.c_cflag = B4800 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR | ICRNL;
    options.c_oflag = 0;
    options.c_lflag = 0;
    if (tcflush(fd, TCIFLUSH) != 0) {
        printf("error %d from tcflush", errno);
        close(fd);
        return -1;
    }
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        printf("error %d from tcsetattr", errno);
        close(fd);
        return -1;
    }

    return fd;
}

int parse_sentence(const char* str) {
  // sendable messages
  //
  // $IIDBT Depth below transducer
  // $IIVHW Speed t. Water
  // $IIMTW Water Temperature
  // $IIVLW Total / Trip Mileage
  // $IIMWV Wind angle & Wind speed
  // $IIHDM Heading compass
  // $IIRMC Speed over ground, Course over ground, Latitude, Longitude,
     //UTC time, Date
  // Optional an additional $IIGLL sentence can be sent for position information
  // $IIRSA Rudder Position
  // $STALK special SeaTalk datagram
  // $SNBSE special system configuration datagram

  // messages types seen so far
  //
  // IIVHW - Speed t. Water           - $IIVHW,287.,T,288.,M,,,,
  // IIHDM - Heading magnetics        - $IIHDM,289.,M
  // IIHDT - Heading true             - $IIHDT,286.,T
  // IIHSC - Command heading to steer - $IIHSC,289.,T,290.,M



  return 0;
}

