#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address
byte ip[] = { 10, 0, 0, 31 }; // ip in lan
byte gateway[] = { 10, 0, 0, 2 }; // internet access via router
byte subnet[] = { 255, 0, 0, 0 }; //subnet mask
EthernetServer server(80); //server port

String readString; 

int pin = 9;
bool lightState = false;

//////////////////////

void setup(){

  pinMode(pin, OUTPUT); //pin selected to control
  digitalWrite(pin, HIGH);    // set pin 5 high
  delay(1000);
  digitalWrite(pin, LOW);    // set pin 5 high
  //start Ethernet
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  server.begin();
  //enable serial data print 
  Serial.begin(9600);
  Serial.println("Start..."); 
}

void loop(){
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (readString.length() < 100) {

          //store characters to string 
          readString += c; 
          //Serial.print(c);
        } 

        //if HTTP request has ended
        if (c == '\n') {

          ///////////////
          Serial.println(readString); //print to serial monitor for debuging 

          client.println("HTTP/1.1 200 OK"); //send new page
          client.println("Content-Type: text/html");
          client.println();

          client.println("<HTML>");
          client.println("<HEAD>");
          client.println("<TITLE>Arduino GET test page</TITLE>");
          client.println("</HEAD>");
          client.println("<BODY>");

          client.println("<H1>Simple Arduino button</H1>");
          
          client.println("<a href=\"/?on\">ON</a>"); 
          client.println("<a href=\"/?off\">OFF</a>"); 

          client.println("</BODY>");
          client.println("</HTML>");

          delay(1);
          //stopping client
          client.stop();

          ///////////////////// control arduino pin
          if(readString.indexOf("on") > 0)//checks for on
          {

            lightState = true;            

          } 
          if(readString.indexOf("off") > 0)//checks for off
          {

            lightState = false;

          }
 if (lightState) {
    Serial.println(readString.indexOf("on"));
    digitalWrite(pin, HIGH);    // set pin 5 high
    Serial.println("Led On");
    //delay(1000);
  } else {
    Serial.println(readString.indexOf("off"));
    digitalWrite(pin, LOW);    // set pin 5 low
    Serial.println("Led Off");
    //delay(1000);
  }
          //clearing string for next read
          readString="";

        }
      }
    }
    
  }

} 
