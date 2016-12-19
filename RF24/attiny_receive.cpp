/**
 * Example RF Radio
 *
 * This is an example of how to use the RF24 class on RPi, communicating to an Arduino running
 * the GettingStarted sketch.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>  


#include "gpio_sun7i.h"
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
 
using namespace std;


void error( char *msg ) {
    perror(  msg );
    exit(1);
}

#define RF_SETUP 0x26 

using namespace std;

const int min_payload_size = 4;
const int max_payload_size = 32;
const int payload_size_increments_by = 1;
int next_payload_size = min_payload_size;
char b[max_payload_size+1]; // +1 to allow room for a terminating NULL char
unsigned long repeat_at;

//
// Hardware configuration
//

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 1Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_1MHZ);

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ);

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 8Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

//RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_256KHZ);

// OrangePI
RF24 radio(SUNXI_GPA(3), SUNXI_GPC(3), "/dev/spidev0.0"); 

// Radio pipe addresses for the 2 nodes to communicate.
//const uint8_t pipes[][6] = {"1Node","2Node"};
// const uint64_t pipes[2] = {0xF0F0F0F0E3LL, 0xF0F0F0F0E2LL};

// In OrangePI non c'è millis()

static unsigned long long epoch ;

unsigned int millis(void)
{
struct timeval now;
unsigned long long ms;

gettimeofday(&now, NULL);

ms = (now.tv_sec * 1000000 + now.tv_usec) / 1000 ;

return ((uint32_t) (ms - epoch ));
}


int decodeMsg(int pos) {
  int p;
  p = pos*2-1; 
  return ((b[p+1] & 0xff) << 8) | ((b[p] & 0xff));
}


void printMsg(int nodeID, int len) {
      // Spew it
      printf("Got response size=%i \n\r",len);
      printf("Received a packet:\n");
      printf("From Node: %d \n", nodeID);
      printf("Temp: %d \n", decodeMsg(1));
      printf("Lux: %d \n", decodeMsg(3));
      printf("TX Millivolts: %d \n", decodeMsg(2));       
      printf("Soil humidity: %d \n", decodeMsg(4));       
      printf("Battery level: %d \n", decodeMsg(5));  
      printf("Relay status: %d \n", decodeMsg(6));            
      printf("--------------------\n");
}


int readRF24() {
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  int len = 0;
  while ( ! radio.available() && ! timeout ) {
        if (millis() - started_waiting_at > 10000 )
          timeout = true;
  }

  // Describe the results
  if ( timeout )
  {
     printf("Failed, response timed out.\n");
     return -1;
  }
  else
  {
    bool done = false;
    while (!done) {
      // Grab the response, compare, and send to debugging spew
      len = radio.getDynamicPayloadSize();
      radio.read( b, len );
      done = true;
    }
          // Update size for next time.
    next_payload_size += payload_size_increments_by;
    if ( next_payload_size > max_payload_size )
      next_payload_size = min_payload_size;
  }
  return len; 
}


int main(int argc, char* argv[]){
  repeat_at = millis();
  std::cout << "Parametri: " << argv[1] << ", " << argv[2] << std::endl;

  // Print preamble:
  printf("RF24/examples/pingtest/\n");

  // Setup and configure rf radio
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS); // Data rate = 250kbps
  radio.setPALevel(RF24_PA_MAX);   // Output power = Max
  radio.setChannel(90);            // Channel number = 90. Range 0 - 127 or 0 - 84 in the US.
  radio.enableDynamicPayloads();
  radio.setAutoAck(1);                     // Ensure autoACK is enabled
  radio.setRetries(2,15);                   // Optionally, increase the delay between retries & # of retries
  radio.setCRCLength(RF24_CRC_8);
  radio.openWritingPipe(0xF0F0F0F0E3LL);
  radio.openReadingPipe(1,0xF0F0F0F0E2LL);
  radio.printDetails();  
  radio.startListening();

  while (1)
  {
    int len = readRF24();
    int nodeID = (b[0] & 0xff); 
    if (len>0) {
      printf("Got response from node: %i, param1: %i\n\r",nodeID, atoi(argv[1]));
      if ((argc>1) && (atoi(argv[1])==nodeID)) printMsg(nodeID, len);
      if ((argc>2) && (atoi(argv[2])==nodeID)) printMsg(nodeID, len);
      if ((argc>3) && (atoi(argv[3])==nodeID)) printMsg(nodeID, len);
    }
    b[len] = 0;

    // delay(1000);
    sleep(1);  
  } 
  return 0;
}