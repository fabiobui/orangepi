/*ESP8266_arduino_Gio_08.ino
 *
 * ====== ESP8266 Demo ======
 *   Print out analog values
 * ==========================
 *
 * Change SSID and PASS to match your WiFi settings.
 * The IP address is displayed to soft serial upon successful connection.
 *
/*ESP8266_arduino_Gio_02.ino
 *
 * ====== ESP8266 Demo ======
 *   Print out time from start
 * ==========================
 *
 * Change SSID and PASS to match your WiFi settings.
 * The IP address is displayed to soft serial upon successful connection.
 *
 * Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=9734
 * Modificato da Carlo e Giorgio
 * Versione per Arduino Uno con SoftSerial
 */


#include <SoftwareSerial.h>
#include <RCSwitch.h>

#define RELAY 4
#define SWITCH_OPEN  5592512 
#define SWITCH_CLOSE 5592332 

SoftwareSerial ESP(11,12); // RX,TX collegati a ESP8266
RCSwitch mySwitch = RCSwitch();


enum {WIFI_ERROR_NONE=0, WIFI_ERROR_AT, WIFI_ERROR_RST, WIFI_ERROR_SSIDPWD, WIFI_ERROR_SERVER, WIFI_ERROR_UNKNOWN};

#define BUFFER_SIZE 128

#define SSID  "casami"   // change this to match your WiFi SSID
#define PASS  "quark123"  // change this to match your WiFi password
#define PORT  "8080"      // using port 8080 by default


boolean lightState, relayState = false;
long radioState;

char buffer[BUFFER_SIZE];
int led = 13; // Led Webserver ON

unsigned long time, t, t1, t2;


// callback for myThread
void niceCallback(){

  Serial.print("COOL! I'm running on: ");
  Serial.println(millis());
}



//----------------------------
void setup() {
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  relayState = false;
  mySwitch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2  
  digitalWrite(led, LOW);

  ESP.begin(9600);
  ESP.setTimeout(15000);
  
  Serial.begin(9600);
  Serial.println("ESP8266_arduino_Gio_02.ino");
  Serial.println("begin...");
  pinMode(led, OUTPUT); 
  while(1){
    byte err = setupWiFi();
    if(err){
      // error, print error code
      Serial.print("setup error:");
      Serial.println((int)err);
    }else{
      // success, print IP
      Serial.print("ip addr:");
      char *ip = getIP();
      if (ip) {
        Serial.println(ip);
        digitalWrite(led, HIGH);
        break;
      }
      else {
        Serial.println("none");
      }
      maxTimeout();
    }
    t1 = millis();
    t2 = millis();
  }
}
//------------------------------
bool maxTimeout(){
  // send AT command
  ESP.println("AT+CIPSTO=0");
  if(ESP.find("OK")){
    return true;
  }else{
    return false;
  }
}
//-----------------------------
char* getIP(){
  // send AT command
  ESP.println("AT+CIFSR");
  // the response from the module is:
  // AT+CIFSR\n\n
  // 192.168.x.x\n 
  // so read util \n three times
  ESP.readBytesUntil('\n', buffer, BUFFER_SIZE);  
  ESP.readBytesUntil('\n', buffer, BUFFER_SIZE);  
  ESP.readBytesUntil('\n', buffer, BUFFER_SIZE);  
  buffer[strlen(buffer)-1]=0;
  return buffer;
}
//----------------------------
void loop() {
  int ch_id, packet_len;
  char *pb;  
   
  time = millis(); 

  if (t1+100<time) {

    if (mySwitch.available()) {
      radioState = mySwitch.getReceivedValue();
      //Serial.print("Received ");
      //Serial.println( radioState );
  
      if (radioState == SWITCH_OPEN) {
       lightState = true;
      }
 
      if (radioState == SWITCH_CLOSE) {
       lightState = false;

      }
      mySwitch.resetAvailable();
    }

    t1 = millis();
  }    
  if (t2+500<time) {
    ESP.readBytesUntil('\n', buffer, BUFFER_SIZE);
    if(strncmp(buffer, "+IPD,", 5)==0) {             //Compara.
    // request: +IPD,ch,len:data
    sscanf(buffer+5, "%d,%d", &ch_id, &packet_len);
    if(packet_len > 0){
      // read serial until packet_len character received
      // start from :
        pb = buffer+5;
        while(*pb!=':') pb++;
        pb++;
        if (strncmp(pb, "GET /", 5) == 0) {
          if (strncmp(pb, "GET /?on", 8) == 0) {
            lightState = true;
          } 
          if (strncmp(pb, "GET /?off", 9) == 0) {
            lightState = false;
          }           
          serve_homepage(ch_id);
        }
      }
    }
  t2 = millis();
  }

  if ((lightState) && (!relayState)) {
    digitalWrite(RELAY, HIGH);
    Serial.println("Acceso");
    relayState = true;
  } else if ((!lightState) && (relayState)) {
    digitalWrite(RELAY, LOW);
    Serial.println("Spento");
    relayState = false;
  }

  //delay(500);
}

//----------------------------
void serve_homepage(int ch_id) {
  //String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n";
  String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: 20\r\n";
  String content="Hello ESP!!!: Sono attivo da: ";
  t = millis()/1000;
  content += t;
  content += "secondi!!!";
/* 
  // output the value of each analog input pin
  for (int analogChannel = 0; analogChannel < 2; analogChannel++) {
    int sensorReading = analogRead(analogChannel);
    content += "analog input ";
    content += analogChannel;
    content += " is ";
    content += sensorReading;
    content += "<br />\n";       
  } */
  
  header += "Content-Length:";
  header += (int)(content.length());
  header += "\r\n\r\n";
  ESP.print("AT+CIPSEND=");
  ESP.print(ch_id);
  ESP.print(",");
  ESP.println(header.length()+content.length());
  if (ESP.find(">")) {
    ESP.print(header);
    ESP.print(content);
    delay(20);
  }
  
  /*Serial.print("AT+CIPCLOSE=");
  Serial.println(ch_id);*/
}
//-------------------------------
byte setupWiFi() {
  ESP.println("AT");
  delay(500);
  if(!ESP.find("OK")) {
    delay(300);
    return WIFI_ERROR_AT;
  }
  //delay(1500); 
  // reset WiFi module
  ESP.println("AT+RST");
  delay(500);
  if(!ESP.find("ready")) {
    delay(300);
    return WIFI_ERROR_RST;
  }
  delay(500);
 
  // set mode 3
  ESP.print("AT+CWJAP=\"");
  ESP.print(SSID);
  ESP.print("\",\"");
  ESP.print(PASS);
  ESP.println("\"");
  delay(2000);
  if(!ESP.find("OK")) {
    delay(300);
    return WIFI_ERROR_SSIDPWD;
  }
  delay(500);
  // start server
  ESP.println("AT+CIPMUX=1");
  delay(500);
  if(!ESP.find("OK")){
    delay(200);
    return WIFI_ERROR_SERVER;
  }
  delay(500);
  ESP.print("AT+CIPSERVER=1,"); // turn on TCP service
  delay(500);
  ESP.println(PORT);
  delay(500);
  if(!ESP.find("OK")){
    delay(200);
    return WIFI_ERROR_SERVER;
  }
  return WIFI_ERROR_NONE;
}
