#include <SPI.h>
#include <Mirf.h>
#include <MirfHardwareSpiDriver.h>
#include <OneWire.h>
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip

#define DEBUG false
#define myNodeID 95     // node ID 

#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10
#define POWER_PIN 8
#define RELAY_PIN 9 

#define DYNPD       0x1C
#define FEATURE     0x1D
#define DPL_P5      5
#define DPL_P4      4
#define DPL_P3      3
#define DPL_P2      2
#define DPL_P1      1
#define DPL_P0      0
#define EN_DPL      2

int PA0, PA1, PA2;

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance

DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature

typedef struct {
         byte nodeID;  // nodeID
         int temp;  // Sensor value
         int supplyV; // Supply voltage
         int lux;
         int soil_humidity;
         int voltage;
         int temp2;
         int mbar;
         int temp3;
         int altitude;
} Payload;
 
Payload rx;

byte RADDR[] = {0xf3, 0xf0, 0xf0, 0xf0, 0xf0};
byte TADDR[] = {0xe2, 0xf0, 0xf0, 0xf0, 0xf0};

byte RADDR2[] = {0xe3, 0xf0, 0xf0, 0xf0, 0xf0};

bool relay_status; 
byte node;
int temp, millivolts, lux, soil_hum, temp2, battery;
unsigned long started_sending_at, last_sent;


void configMirf(byte rx_addr[5]) {
//  Mirf.cePin = PA7;             //attiny 85-84
//  Mirf.csnPin = PA3;  
  Mirf.cePin = 7;             //ce pin on Mega 2560, REMOVE THIS LINE IF YOU ARE USING AN UNO
  Mirf.csnPin = 8;            //csn pin on Mega 2560, REMOVE THIS LINE IF YOU ARE USING AN UNO
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  
  Mirf.channel = 0x5a; // Channel number = 90. Range 0 - 127 or 0 - 84 in the US.
  Mirf.configRegister(RF_SETUP,0x26); 

  Mirf.setRADDR(rx_addr);  
  Mirf.setTADDR(TADDR);
  Mirf.config();
  
  // Enable dynamic payload 
  Mirf.configRegister( FEATURE, 1<<EN_DPL ); 
  Mirf.configRegister( DYNPD, 1<<DPL_P0 | 1<<DPL_P1 | 1<<DPL_P2 | 1<<DPL_P3 | 1<<DPL_P4 | 1<<DPL_P5 );  
}

void setup(){

  Serial.begin(9600);
  configMirf(RADDR);

  delay(100);
  // Print out register readinds for important settings
  uint8_t rf_ch, rf_setup = 0;
  byte tx_addr[5];
  byte rx_addr[5];
  
  Mirf.readRegister(RF_CH, &rf_ch,sizeof(rf_ch));
  Mirf.readRegister(RF_SETUP, &rf_setup, sizeof(rf_setup));
  Mirf.readRegister(TX_ADDR, tx_addr, sizeof(tx_addr));
  Mirf.readRegister(RX_ADDR_P1, rx_addr, sizeof(rx_addr));

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN,LOW);
         
  pinMode(POWER_PIN, OUTPUT);  
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC         
                  

  Serial.println("---------------------");     
  Serial.print("RF_CH :0x");
  Serial.println(rf_ch,HEX);  
  Serial.print("RF_SETUP :0x");
  Serial.println(rf_setup,HEX);  
  Serial.print("TX_ADDR :");
  for ( int i=0;i<5;i++ ) {  // Loop 5 times, print in HEX
    Serial.print( tx_addr[i], HEX);
  }
  Serial.println();
  Serial.print("RX_ADDR :");
  for ( int i=0;i<5;i++ ) {  // Loop 5 times, print in HEX
    Serial.print( rx_addr[i], HEX);
  }
  Serial.println("");
  Serial.println("---------------------");
  delay(1000);      // For serial debug to read the init config output

  relay_status = false;
  started_sending_at = millis();
  last_sent = millis();
}

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------
long readVcc() {
   long result;
   // Read 1.1V reference against Vcc
   #if defined(__AVR_ATtiny84__) 
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
   #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega328
   #endif 
   delay(2); // Wait for Vref to settle
   ADCSRA |= _BV(ADSC); // Convert
   while (bit_is_set(ADCSRA,ADSC));
   result = ADCL;
   result |= ADCH<<8;
   result = 1126400L / result; // Back-calculate Vcc in mV
   return result;
} 
//########################################################################################################################


void readSensor() {
   double celsius;
   bitClear(PRR, PRADC); // power up the ADC
   ADCSRA |= bit(ADEN); // enable the ADC
   digitalWrite(POWER_PIN,HIGH);
   delay(100);
   sensors.begin(); //start up temp sensor
   sensors.requestTemperatures(); // Get the temperature
   celsius=sensors.getTempCByIndex(0)*100; // Read first sensor and convert to integer, reversed at receiving end
  
    //Read Lux  
    delay(10);
    int sensorValue = analogRead(PA2); //PA0 rev.11, PA2 rev. 13
    int lightLevel = 100-(1023-sensorValue)/10.23;
    delay(10);
     
/* Read Voltage solar panel */
    unsigned int ADCValue;
    double Voltage;
    double Vcc;

    Vcc = readVcc();
    ADCValue = analogRead(PA0); //PA2 rev.11, PA0 rev. 13
    Voltage = (ADCValue / 1023.0) * Vcc;
    Voltage *= 0.128; // fattore di correzione e resistenze 
      
    rx.nodeID = myNodeID;
    rx.temp=(int)celsius;
    rx.supplyV = (int)readVcc();
    rx.lux = lightLevel;
    rx.soil_humidity = analogRead(PA1); 
    rx.voltage = (int)Voltage;
         
    digitalWrite(POWER_PIN,LOW);
    ADCSRA &= ~ bit(ADEN); // disable the ADC
    bitSet(PRR, PRADC); // power down the ADC
}

void sensorData() {
    delay(100);
    rx.nodeID = myNodeID;
    rx.temp=0;
    rx.supplyV = 0;
    rx.lux = 0;
    rx.soil_humidity = 0; 
    rx.voltage = 0;
}

void sendData() {

    int temp2 = 0;
    if (relay_status) temp2 = 2222;
    rx.temp2 = temp2; 
    //readSensor();
    sensorData();
    Mirf.payload = sizeof (rx);
    Mirf.send((byte *) &rx); 
    while( Mirf.isSending() ) {
      delay(1);
    }

}


void loop(){
  if ((millis() - started_sending_at > 10000 )) {
      Serial.print("sending data...");
      configMirf(RADDR2);
      sendData();
      configMirf(RADDR);
      Serial.println("sent");
      started_sending_at = millis();
  }
  if(!Mirf.isSending() && Mirf.dataReady()) {
                     
    rx = *(Payload*) &rx;
    Mirf.payload = sizeof (rx);
    Mirf.getData((byte *) &rx);
   
    node = rx.nodeID;
    temp = rx.temp;
    millivolts = rx.supplyV;
    lux = rx.lux;
    soil_hum = rx.soil_humidity;
    temp2 = rx.temp2;
    battery = rx.voltage;

    if (node>40) {  
    
       Serial.print("@NodeId=");
       Serial.print(node);
       Serial.print("|Temp=");
       Serial.print((float)temp/100,2);
       Serial.print("|Lux=");
       Serial.print(lux);
       Serial.print("|Millivolts=");
       Serial.print((float)millivolts/1000,2);
       Serial.print("|Soil=");
       Serial.print(soil_hum); 
       Serial.print("|Battery=");
       Serial.print((float)battery/1000,2);       
       Serial.print("|Temp2=");
       Serial.println((float)temp2/100,2);   
  
       if ((node==95) && (temp2==1111)) relay_status = true;
       if ((node==95) && (temp2==3333)) relay_status = false;
    }
    if ((relay_status) && (temp2==1111)) {
      Serial.println("Temp2=1111 - relay ON");
      digitalWrite(RELAY_PIN,HIGH);
    }
    if ((!relay_status) && (temp2==3333)) {  
      Serial.println("Temp2=3333 - relay OFF");
      digitalWrite(RELAY_PIN,LOW);
    }
    delay(10); 
    Mirf.flushRx();
    delay(10); 
    sendData();
  // reset data
    node = 0;
    temp2 = 0; 
  }  

    
  delay(1000);
}
