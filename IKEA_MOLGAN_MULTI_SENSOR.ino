#define MY_RADIO_NRF24
#define MY_BAUD_RATE                (9600)

#define MY_WITH_LEDS_BLINKING_INVERSE
#define MY_DEFAULT_TX_LED_PIN       (A5)
#define MY_DEBUG 

#include <SPI.h>
#include <MySensors.h> 
#include <DallasTemperature.h>
#include <OneWire.h>
#include <BH1750.h>
#include <Wire.h> 

/* MySensors multisensor
 
 Batterylevel
 Temperature
 Motion
 Light 

*/

//********** CONFIG **********************************

    #define NODE_ID     111  // ID of node
    #define TEMP_ID     1    // ID of TEMP
    #define PIR_ID      2    // ID of PIR
    #define LIGHTSEN_ID    3    // ID of LDR
    #define DIMMER_ID   4    // ID of Dimmer
    #define LIGHT_ID 5    // ID of Lightsensor
    
    #define PIR_PIN       3     // Pin of PIR
    #define ONE_WIRE_BUS  4     // Pin where dallase sensor is connected 
    #define LED_PIN       5     // OUTPUT TO LED
    
    unsigned long SLEEP_TIME = 30000; // Sleep time between reads
    
    int BATTERY_SENSE_PIN = A0; 

//****************************************************

int batteryPcnt;
int oldBatteryPcnt;
float lastTemp;
float lastTemperature[1];
int lastPIR = 2;
int lastPIR2 = 2;
int lastLightLevel;
int dimLevel = 100;

boolean doSleep = true;   //  If the light is on sleep mode is deactivated
boolean debug = false;     //  Give feedback to serial what is going on

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
BH1750 lightSensor;

MyMessage TEMPmsg(TEMP_ID, V_TEMP);
MyMessage PIRmsg(PIR_ID, V_TRIPPED);
MyMessage LIGHTSENmsg(LIGHTSEN_ID, V_LIGHT_LEVEL);
MyMessage DIMMERmsg(DIMMER_ID, V_DIMMER);
MyMessage LIGHTmsg(LIGHT_ID, V_LIGHT);

void setup() {
  analogReference(INTERNAL);    // set the ADC reference to 1.1V
  sensors.begin();                      // Startup up the OneWire library
  sensors.setWaitForConversion(false);  // requestTemperatures() will not block current thread
  lightSensor.begin();                  // Startup the BH1750
  request(DIMMER_ID, V_DIMMER);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}

void presentation() {
  // Register all sensors to gateway
  sendSketchInfo("Molgan", "1.0"); 
  present(TEMP_ID, S_TEMP);
  present(PIR_ID, S_MOTION); 
  present(LIGHTSEN_ID, S_LIGHT_LEVEL);
  present(DIMMER_ID, S_DIMMER);
  }

void loop() {
int PIR = readPIR();
lightControl(PIR);
  
  readBATTERY();
  if (PIR == 0) {
   readDALLAS();
   readLIGHT();
   sleep(PIR_PIN-2, CHANGE, SLEEP_TIME);
  }
}

// *** LIGHT CONTROL ****************************************************
void lightControl(int PIR){
  if (PIR == 1) {
    Serial.println("PIR is HIGH");
    if (PIR != lastPIR2){
      request(DIMMER_ID, V_DIMMER);
      wait(500);
      lastPIR2 = PIR;
      Serial.print("Het ingestelde dimnivo is: ");
      Serial.println((int)(dimLevel / 100. * 255) );
      analogWrite( LED_PIN, (int)(dimLevel / 100. * 255) );
    }
  }
  else {
    Serial.println("The PIR is LOW so set light to zero");
    analogWrite (LED_PIN,0);
    lastPIR2 = PIR;
  }
}

// *** PIR SENSOR ****************************************************

int readPIR() {
  int PIR = digitalRead(PIR_PIN);
  if (PIR != lastPIR) {
    lastPIR = PIR;
    send(PIRmsg.set(PIR == HIGH ? 1 : 0));
  }
  if (debug) {
    Serial.print("Motion: ");
    if (PIR == HIGH) {
      Serial.println("true");
      } 
     else {
      Serial.println("false");
      }
    }
  return PIR;
  }

// *** DALLAS SENSOR ************************************************

void readDALLAS(){
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0); // Fetch and round temperature to one decimal
    if (temperature != lastTemp && temperature != -127.00 && temperature != 85.00) {
       send(TEMPmsg.set(temperature,1)); // Send in the new temperature
       lastTemp=temperature; // Save new temperatures for next compare
    }
    if (debug){
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println("C");
      }
  }

// *** LIGHT SENSOR  *************************************************

void readLIGHT() {
  uint16_t lightLevel = lightSensor.readLightLevel(); // Get Lux value
   if (lightLevel != lastLightLevel) {                // If value of LDR has changed
      send(LIGHTSENmsg.set(lightLevel));            // Send value of LDR to gateway
      lastLightLevel = lightLevel;
      }
   if (debug){
      Serial.print("Light: ");
      Serial.print(lightLevel);
      Serial.println("%");
      }
  }

// *** BATTERY SENSOR **********************************************

void readBATTERY(){
    int sensorValue = analogRead(BATTERY_SENSE_PIN);
    Serial.print (" De ruwe analoge waarde is: ");
    Serial.println (sensorValue);
    float batteryV  = sensorValue * 0.004398827; 
    int batteryPcnt = sensorValue / 10; 
    if (oldBatteryPcnt != batteryPcnt) {   // If battery percentage has changed
      sendBatteryLevel(batteryPcnt);  // Send battery percentage to gateway
      oldBatteryPcnt = batteryPcnt;
      }
    if (debug){
        Serial.print("Battery: ");
        Serial.print(batteryPcnt);
        Serial.println("%");
        Serial.print("Battery Voltage: ");
        Serial.println(batteryV);
        }
  }

void receive(const MyMessage &message)
{
    if (message.type == V_LIGHT || message.type == V_DIMMER) {
        //  Retrieve the power or dim level from the incoming request message
        int requestedLevel = atoi( message.data );
        dimLevel =  requestedLevel > 100 ? 100 : requestedLevel;
        dimLevel =  requestedLevel < 4   ? 0   : requestedLevel;
        if (debug){
          Serial.print ("Het ontvangen dimLevel is: ");
          Serial.println (dimLevel);
        }
        analogWrite( LED_PIN, (int)(dimLevel / 100. * 255) );
    }
}

