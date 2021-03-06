/**
 * 
 * Ulfius Framework example program
 * 
 * This example program describes the main features 
 * that are available in a callback function
 * 
 * Copyright 2015 Nicolas Mora <mail@babelouest.org>
 * 
 * License MIT
 *
 */

#include <string.h>
#include <jansson.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h> 
#include <unistd.h>

#include "gpio_sun7i.h"
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <rrd.h> 
 

extern "C" {
  #include "../lib/ulfius/src/ulfius.h"
}

// #include <RF24/RF24.h>

using namespace std;

#define PORT 8080
#define PREFIX "/node"
#define PREFIX "/node"
#define PREFIXJSON "/raspjson"
#define PREFIXCOOKIE "/testcookie"


const int min_payload_size = 4;
const int max_payload_size = 32;
const int payload_size_increments_by = 1;
int next_payload_size = min_payload_size;
char b[max_payload_size+1]; // +1 to allow room for a terminating NULL char
unsigned long started_time_at;

bool sending;
int relay_status;
int timeoutCount;

typedef struct {
         uint8_t nodeID;  // nodeID
         int temp;  // Sensor value
         int supplyV; // Supply voltage
         int lux;
         int soil_humidity;
         int voltage;
         int relay;
         float tmin;
         float tmax;
         int lmin;
         int lmax;
         time_t update;
} Payload;
 
Payload r45, r71, r95;


// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 8Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

// OrangePI
RF24 radio(SUNXI_GPA(3), SUNXI_GPC(3), "/dev/spidev0.0");


/**
 * callback functions declaration
 */
int callback_get_node (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data);


static unsigned long long epoch;

unsigned int millis(void)
{
struct timeval now;
unsigned long long ms;

gettimeofday(&now, NULL);

ms = (now.tv_sec * 1000000 + now.tv_usec) / 1000 ;

return ((uint32_t) (ms - epoch ));
}


int decodeMsg(int pos) {
  int p = pos*2-1; 
  short res = ((b[p+1] & 0xff) << 8) | ((b[p] & 0xff));
  return res;
}

int decodeMsgChar(char *buf, int pos) {
  int p;
  p = pos*2-1; 
  return ((buf[p+1] & 0xff) << 8) | ((buf[p] & 0xff));
}


/**
 * decode a u_map into a string
 */
int print_map(char to_return[100], const struct _u_map * map) {
  char line[100];

  const char **keys, * value;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      value = u_map_get(map, keys[i]);
      len = snprintf(NULL, 0, "key is %s, length is %zu, value is %.*s", keys[i], u_map_get_length(map, keys[i]), (int)u_map_get_length(map, keys[i]), value);
      snprintf(line, (len+1), "key is %s, length is %zu, value is %.*s", keys[i], u_map_get_length(map, keys[i]), (int)u_map_get_length(map, keys[i]), value);
      if (to_return != NULL) {
        len = strlen(to_return) + strlen(line) + 1;
        if (strlen(to_return) > 0) {
          strcat(to_return, "\n");
        }
      } else {
        to_return[0] = 0;
      }
      strcat(to_return, line);
    }
    return 0;
  } else {
    return 1;
  }
}


void printMsg(int len) {
      uint8_t nodeID = (b[0] & 0xff); 
      // Spew it
      y_log_message(Y_LOG_LEVEL_DEBUG, "Got response size=%i",len);
      y_log_message(Y_LOG_LEVEL_DEBUG, "Received a packet from Node: %d", nodeID);
      y_log_message(Y_LOG_LEVEL_DEBUG, "Temp: %d", decodeMsg(1));
      y_log_message(Y_LOG_LEVEL_DEBUG, "Lux: %d", decodeMsg(3));
      y_log_message(Y_LOG_LEVEL_DEBUG, "TX Millivolts: %d", decodeMsg(2));       
      y_log_message(Y_LOG_LEVEL_DEBUG, "Soil humidity: %d", decodeMsg(4));       
      y_log_message(Y_LOG_LEVEL_DEBUG, "Battery level: %d", decodeMsg(5));  
      y_log_message(Y_LOG_LEVEL_DEBUG, "Relay status: %d", decodeMsg(6)); 
}

void decodeMsgPayload(uint8_t node, Payload *rx) {
  rx->update  = time(NULL);  
  rx->nodeID  = node;
  rx->temp    = decodeMsg(1);
  rx->supplyV = decodeMsg(2);
  rx->lux     = decodeMsg(3);
  rx->soil_humidity = decodeMsg(4);
  rx->voltage = decodeMsg(5);
  rx->relay   = decodeMsg(6); 
}

int setupRF24(const uint64_t readPipe, const uint64_t writePipe) {
  // Print preamble:
  //printf("RF24/examples/pingtest/\n");

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
  radio.openWritingPipe(writePipe);
  radio.openReadingPipe(1,readPipe);
  //radio.printDetails();  
  return 0;
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
     y_log_message(Y_LOG_LEVEL_ERROR, "Timeout receiving RF24"); 
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


void sendOverRadio(int valore) {
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  char bb[13] = {0};

  sending = true;
  radio.stopListening();
  sleep(2);

  y_log_message(Y_LOG_LEVEL_DEBUG, "Sending value: %d", valore); 

  setupRF24(0xF0F0F0F0F3LL, 0xF0F0F0F0E2LL); 
  //power on radio 
  radio.powerUp();
  
  bb[0] = 95; // nodeID
  bb[11] = valore &0xFF;
  bb[12] = (valore >> 8) &0xFF;

  // Send to hub  
  while ( ! radio.write(&bb, sizeof(bb)) && ! timeout ) {
        if (millis() - started_waiting_at > 10000 )
          timeout = true;
  }

  if (timeout) y_log_message(Y_LOG_LEVEL_ERROR, "Timeout !"); 

  y_log_message(Y_LOG_LEVEL_DEBUG, "Sent !"); 

  setupRF24(0xF0F0F0F0E2LL, 0xF0F0F0F0E3LL); 
  sleep(1);
  radio.startListening();
  sending = false;
  sleep(1);
}

int writeRrd(Payload * rx) {

/*
rrd archive
  DS:temp:GAUGE:120:-20:60 
  DS:lux:GAUGE:120:0:100 
  DS:txmv:GAUGE:120:0:10 
  DS:level:GAUGE:120:0:10 
  DS:hum:GAUGE:120:0:1000 
*/ 
  int len, result;
  char buffer[255]; 
  char *updateparams[] = { 
        "rrdupdate", 
        "/home/fabio/rrdtool/nostradomus.rrd", 
        buffer, 
        NULL 
  };

  len = snprintf(NULL, 0, "%d:%4.2f:%d:%5.3f:%5.3f:%d", 
    (int)rx->update, 
    (rx->temp/100.0),
    rx->lux,
    (rx->supplyV/1000.0), 
    (rx->voltage/1000.0),
    rx->soil_humidity
     ); 
  
  snprintf(buffer, (len+1), "%d:%4.2f:%d:%5.3f:%5.3f:%d", 
    (int)rx->update, 
    (rx->temp/100.0),
    rx->lux,
    (rx->supplyV/1000.0), 
    (rx->voltage/1000.0),
    rx->soil_humidity
     ); 
 
  rrd_clear_error();
  result = rrd_update(3, updateparams);

  y_log_message(Y_LOG_LEVEL_INFO, "Writing rrd with node %d, updateparams[2]: %s", rx->nodeID, updateparams[2]);
  
  return result;
}


int calcMinMax(Payload * rx) {
/*
We need to fix these errors:

(process:14530): GLib-GObject-CRITICAL **: g_object_get_qdata: assertion 'G_IS_OBJECT (object)' failed

(process:14530): GLib-GObject-CRITICAL **: g_object_replace_qdata: assertion 'G_IS_OBJECT (object)' failed

*/

  int pcount,result;
  
  int xsize, ysize;
  double ymin,ymax;
  
  char **calcpr=NULL;

  char *rrdargs[] = {
    "rrdgraph",
    "x",
    "-s", "end-18h",
    "DEF:temp=/home/fabio/rrdtool/nostradomus.rrd:temp:AVERAGE",
    "DEF:lux=/home/fabio/rrdtool/nostradomus.rrd:lux:AVERAGE",     
    "VDEF:vmin=temp,MINIMUM",
    "VDEF:vmax=temp,MAXIMUM",
    "VDEF:lmin=lux,MINIMUM",
    "VDEF:lmax=lux,MAXIMUM",
    "PRINT:vmin:%lf",
    "PRINT:vmax:%lf",
    "PRINT:lmin:%lf",
    "PRINT:lmax:%lf",    
    NULL
  };
  
  for (pcount = 0; (rrdargs[pcount]); pcount++);

  rrd_clear_error();    
  result = rrd_graph(pcount, rrdargs, &calcpr, &xsize, &ysize, NULL, &ymin, &ymax);
  
  uint i=0;
  if (calcpr) {
    for (i=0; calcpr[i]; i++) {
        if (i==0) rx->tmin = atof(calcpr[0]);
        if (i==1) rx->tmax = atof(calcpr[1]);
        if (i==2) rx->lmin = atoi(calcpr[2]);
        if (i==3) rx->lmax = atoi(calcpr[3]); 
        free(calcpr[i]);
    }
    free(calcpr);
  }
  if (i<3) {
    y_log_message(Y_LOG_LEVEL_INFO, "Error no data for min & max !");
    result = -1;
  } 
  return result;
}


void loop() {
    int len = readRF24();
    if (len==-1) timeoutCount++;
    char bb[max_payload_size+1] = {0};
    //printMsg(len);


    uint8_t nodeID = (b[0] & 0xff); 

    if (nodeID>44) y_log_message(Y_LOG_LEVEL_DEBUG, "Receiving from node: %d", nodeID); 

    if ((len>0) && (nodeID==45)) decodeMsgPayload(45, &r45);
    if ((len>0) && (nodeID==71)) decodeMsgPayload(71, &r71);
    if ((len>0) && (nodeID==95)) decodeMsgPayload(95, &r95);

    b[len] = 0;
    strncpy(b, bb, sizeof(bb));

    // delay(1000);
    sleep(1);  
    if (millis() - started_time_at > 60000 ) {
      writeRrd(&r71);
      started_time_at = millis();
    }

}


int main (int argc, char **argv) {
  setupRF24(0xF0F0F0F0E2LL, 0xF0F0F0F0E3LL); 
  radio.startListening();
  sending = false;
  relay_status = 0;
  started_time_at = millis();
  timeoutCount = 0;

  int ret;
  
  // Set the framework port number
  struct _u_instance instance;
  
  y_init_logs("rest_server", Y_LOG_MODE_CONSOLE, Y_LOG_LEVEL_INFO, NULL, "Starting rest_server");
  
  if (ulfius_init_instance(&instance, PORT, NULL) != U_OK) {
    y_log_message(Y_LOG_LEVEL_ERROR, "Error ulfius_init_instance, abort");
    return(1);
  }
  
  u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  u_map_put(instance.default_headers, "content-type", "application/json");
  
  // Maximum body size sent by the client is 1 Kb
  instance.max_post_body_size = 1024;
  
  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/multiple/:multiple/:multiple/:not_multiple", NULL, NULL, NULL, &callback_get_node, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/:foo", NULL, NULL, NULL, &callback_get_node, NULL);

  
  // default_endpoint declaration
  ulfius_set_default_endpoint(&instance, NULL, NULL, NULL, &callback_default, NULL);
  
  // Start the framework
  // Open an http connection
  ret = ulfius_start_framework(&instance);
  
  if (ret == U_OK) {
    y_log_message(Y_LOG_LEVEL_INFO, "Start %sframework on port %d", ((argc == 4 && strcmp("-secure", argv[1]) == 0)?"secure ":""), instance.port);
    
    // Wait for the user to press <enter> on the console to quit the application
    //getchar();
    while (1)
    {  
      if (relay_status==1111) {
         sendOverRadio(1111);
         relay_status = 2222;
      } else if (relay_status==3333) {
         sendOverRadio(3333);
         relay_status = 0;
      } else {  
        if (!sending) {
          if (timeoutCount>5) {
            timeoutCount = 0;
            radio.stopListening(); 
            setupRF24(0xF0F0F0F0E2LL, 0xF0F0F0F0E3LL); 
            radio.startListening();          
          }
          loop();
        }
      }
    } 
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "Error starting framework");
  }
  y_log_message(Y_LOG_LEVEL_INFO, "End framework");
  
  
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance); 
  
  y_close_logs(); 

  return 0;
}



/**
 * Callback function that put "Hello World!" and all the data sent by the client in the response as string (http method, url, params, cookies, headers, post, json, and user specific data in the response
 */
int callback_get_node (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct _u_map map;
  //char url_params[100];
  const char *node, *cmd;
  Payload * rx;

  // print_map(url_params, request->map_url);

  map = * request->map_url;
  node = u_map_get(&map, "foo");
  cmd  = u_map_get(&map, "switch");

  if (atoi(node)==45) {
    rx = &r45; 
  } else if (atoi(node)==71) {
    rx = &r71; 
  } else if (atoi(node)==95) {
    rx = &r95;   
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "Node not valid: %s", node);
    response->status = 404;
    return U_ERROR_PARAMS;
  }


  // convert update time to date time format
  char s[64];
  struct tm *tm = localtime(&rx->update);
  strftime(s, sizeof(s), "%c", tm);

  // y_log_message(Y_LOG_LEVEL_DEBUG, "Url params %s", url_params);
  y_log_message(Y_LOG_LEVEL_DEBUG, "Http url %s", request->http_url);
  y_log_message(Y_LOG_LEVEL_DEBUG, "get value from key node: %s", node);
  y_log_message(Y_LOG_LEVEL_DEBUG, "get value from key node: %s", cmd);

  response->json_body = json_object();

//  ulfius_add_header_to_response(response, "content-type", "application/json");

  json_object_set_new(response->json_body, "update", json_string(s));

  if (cmd) {
    if (atoi(cmd)==0) {
      relay_status = 3333;
      json_object_set_new(response->json_body, "relay", json_string("off"));
    } else if (atoi(cmd)==1) {
      relay_status = 1111;
      json_object_set_new(response->json_body, "relay", json_string("on"));
    }       
    response->status = 200; 
    return U_OK;
  }

  json_object_set_new(response->json_body, "temperature", json_real(rx->temp/100.0));
  json_object_set_new(response->json_body, "Lux", json_integer(rx->lux));
  json_object_set_new(response->json_body, "Tx_millivolts", json_real(rx->supplyV/1000.0));
  json_object_set_new(response->json_body, "humidity", json_integer(rx->soil_humidity));
  json_object_set_new(response->json_body, "Solar_volts", json_real(rx->voltage/1000.0));  
  json_object_set_new(response->json_body, "Relay_status", json_integer(rx->relay));

  if ((atoi(node)==71) && (calcMinMax(rx)!=-1)) {
    json_object_set_new(response->json_body, "Tmin", json_real(rx->tmin));     
    json_object_set_new(response->json_body, "Tmax", json_real(rx->tmax));    
    json_object_set_new(response->json_body, "Lmin", json_integer(rx->lmin));     
    json_object_set_new(response->json_body, "Lmax", json_integer(rx->lmax));       
  };

// Da completare
  if ((atoi(node)==45)) {
    json_object_set_new(response->json_body, "Tmin", json_real(19.5));     
    json_object_set_new(response->json_body, "Tmax", json_real(24.2));    
    json_object_set_new(response->json_body, "Lmin", json_integer(0));     
    json_object_set_new(response->json_body, "Lmax", json_integer(60));       
  };


  response->status = 200;
  return U_OK;
}


/**
 * Default callback function called if no endpoint has a match
 */
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
  response->string_body = strdup("Page not found, do what you want");
  response->status = 404;
  return U_OK;
}
