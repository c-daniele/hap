#include "ZZ_SimpleHomeAlarmProjectCredentials.h"

#include <ArduinoJson.h>
#include "ZZ_ConnectionManager.h"
#include "ZZ_PlantManagement.h"
#include "ZZ_CustomLoggerOutput.h"
#include <Logger.h>

using namespace SimpleHomeAlarmEsp32;

// WORKING

// The MQTT topic that this device should publish to
#define AWS_IOT_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define MAX_SILENCE_TIME 10
#define SYNC_WAIT_TIME      10

#define PIN_IN_PROX1                 23
#define PIN_IN_PROX2                 21

// PIN 4 and 15 utilize the ADC2, which is turned off when the WiFi is activated
// However, this problem only exists when you use the A/D converter (i.e. when you read analog values)
// In this case, both PIN 4 and 15 are used in digital model, so we can ignore the WiFi problem
#define PIN_IN_PROX3                 4
#define PIN_IN_PROX4                 15

#define PIN_IN_PROX5                 13

#define PIN_IN_PIR1                  33
#define PIN_IN_PIR2                  35 
#define PIN_IN_PIR3                  36

/*
When WiFi is activated, the following PINS can be used:
32, 33, 34, 35, 36, 39

The 13 is an ADC2 PIN, which cannot be used when WiFi is activated!!
The 27 is an ADC2 PIN, which cannot be used when WiFi is activated!!

*/



#define PIN_LED                      2

#define DEFAULT_SAMPLE_RATE          5

#define MQTT_OUTPUT_BUFFER           1024
#define MQTT_INPUT_BUFFER            1024

#define DEBUG

ConnectionManager cm;
BoardController bc;
time_t lastMessageTime;
//boolean armed = false;
boolean initialized = false;

//enum current_status_t {ALARM, NORMAL, TURNED_OFF, CONFIG_UPDATE};
enum device_status_t {NORMAL = 0, ALARM = 1, CONFIG_UPDATE = 2};
enum device_mode_t {SYNC = 0, OFF = 1, ARMED = 2, MONITOR = 3};

device_status_t currentStatus = NORMAL;
device_mode_t currentMode = SYNC;

Sensor s1(SimpleHomeAlarmEsp32::SensorType::PROXIMITY, SimpleHomeAlarmEsp32::SensorModel::Other, PIN_IN_PROX1, DEFAULT_SAMPLE_RATE, String("PROX_1"), String("Living Room Window-1"));
Sensor s2(SimpleHomeAlarmEsp32::SensorType::PROXIMITY, SimpleHomeAlarmEsp32::SensorModel::Other, PIN_IN_PROX2, DEFAULT_SAMPLE_RATE, String("PROX_2"), String("Living Room Window-2"));
Sensor s3(SimpleHomeAlarmEsp32::SensorType::PROXIMITY, SimpleHomeAlarmEsp32::SensorModel::Other, PIN_IN_PROX3, DEFAULT_SAMPLE_RATE, String("PROX_3"), String("Main Door"));
Sensor s4(SimpleHomeAlarmEsp32::SensorType::PROXIMITY, SimpleHomeAlarmEsp32::SensorModel::Other, PIN_IN_PROX4, DEFAULT_SAMPLE_RATE, String("PROX_4"), String("Study-Room Window"));
Sensor s5(SimpleHomeAlarmEsp32::SensorType::PROXIMITY, SimpleHomeAlarmEsp32::SensorModel::Other, PIN_IN_PROX5, DEFAULT_SAMPLE_RATE, String("PROX_5"), String("Bedroom Window"));
Sensor s6(SimpleHomeAlarmEsp32::SensorType::PIR, SimpleHomeAlarmEsp32::SensorModel::Pyronix, PIN_IN_PIR1, DEFAULT_SAMPLE_RATE, String("PIR_1"), String("Living Room PIR"));
Sensor s7(SimpleHomeAlarmEsp32::SensorType::PIR, SimpleHomeAlarmEsp32::SensorModel::Pyronix, PIN_IN_PIR2, DEFAULT_SAMPLE_RATE, String("PIR_2"), String("Study-Room PIR"));
Sensor s8(SimpleHomeAlarmEsp32::SensorType::PIR, SimpleHomeAlarmEsp32::SensorModel::Bentel, PIN_IN_PIR3, DEFAULT_SAMPLE_RATE, String("PIR_3"), String("Bedroom PIR"));


void messageReceivedCallback(String &topic, String &payload){
  Logger::notice("messageReceivedCallback()", String("incoming: " + topic + " - " + payload).c_str());

  // FIXME: it could be unsafe managing payload size this way
  DynamicJsonDocument doc(MQTT_INPUT_BUFFER);
  DeserializationError err = deserializeJson(doc, payload);

  if (!err){

    device_mode_t tmpNewMode = SYNC;

    if(doc["commands"]["alarmMode"] == "OFF"){
      tmpNewMode = OFF;
    } else if(doc["commands"]["alarmMode"] == "ARMED"){
      tmpNewMode = ARMED;
    } else if(doc["commands"]["alarmMode"] == "MONITOR"){
      tmpNewMode = MONITOR;
    }

    // SYNC the status if we are in a SYNC phase
    if(currentMode == SYNC){ 
      currentStatus = NORMAL;
    }

    // trick to make the message ACTION be sent in the main loop when sensors are enabled/disabled
    else{ 
      currentStatus = CONFIG_UPDATE;
    }

    currentMode = tmpNewMode;

    if(doc["sensors"] != nullptr){

      std::vector<Sensor*> sensors = bc.getAllSensors();

      //Logger::notice("messageReceivedCallback()", String("sensors size: " + doc["sensors"].size()).c_str());

      // iterate over all incoming sensor array within the Message Payload
      for (size_t i = 0; i < doc["sensors"].size(); i++){
       
        // iterate over all plant sensor in order to find a MATCH
        for(Sensor* s: sensors){

          String devCode = doc["sensors"][i]["deviceCode"];

          // if a match has found => set the ENABLED flag accordingly
          if(s->getCode().equals(devCode)){
            s->setEnabled(doc["sensors"][i]["enabled"]);
          }
        }
      } 
    }

/*
    // Get a reference to the root object
    JsonObject obj = doc.as<JsonObject>();

    // Loop through all the key-value pairs in obj
    for (JsonPair p : obj) {
      p.key(); // is a JsonString
      p.value(); // is a JsonVariant
    }
    */

  }

else{
  Logger::error("messageReceivedCallback()", String("deserializeJson() failed with code: " + String(err.c_str())).c_str());
}


  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup() {
  Serial.begin(115200);

  Logger::setLogLevel(Logger::NOTICE);
  //Logger::setLogLevel(Logger::WARNING);

  // Set a custom class for logging output.
  Logger::setOutputFunction(CustomLoggerOutput::customLogger);

  bc.init();
  bc.addSensor(&s1);
  bc.addSensor(&s2);
  bc.addSensor(&s3);
  bc.addSensor(&s4);
  bc.addSensor(&s5);
  bc.addSensor(&s6);
  bc.addSensor(&s7);
  bc.addSensor(&s8);

  cm.connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
  cm.connectToAWS(AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT, THINGNAME, AWS_IOT_SUBSCRIBE_TOPIC, &messageReceivedCallback);

  initialized = true;
  lastMessageTime = 0;

  Logger::notice("setup()", "Setup complete");
}

void serializeAndSend(std::vector<Sensor*> sensors
  , bool alarm
  //, bool changeStatusEvent
  , String workingMode
  , String action
  ){

  DynamicJsonDocument jsonEncoder(MQTT_OUTPUT_BUFFER);

  JsonObject rootObject = jsonEncoder.to<JsonObject>();    
  JsonObject generalStatus = rootObject.createNestedObject("status");
  rootObject["action"] = action;
  generalStatus["wifi_strength"] = WiFi.RSSI();
  generalStatus["alarm"] = alarm;
  generalStatus["alarmMode"] = workingMode;
  
  JsonArray parentArray = rootObject.createNestedArray("sensors");
  for(Sensor* s: sensors){
    s->getStatusSerialized(parentArray);
  }

  char jsonBuffer[measureJson(rootObject)+1];
  size_t n = serializeJson(rootObject, jsonBuffer, sizeof(jsonBuffer));

  cm.sendIotMessage(AWS_IOT_TOPIC, jsonBuffer, n);

  Logger::notice("serializeAndSend()", String("Sending IOT Message").c_str());
  Logger::notice("serializeAndSend()", String(jsonBuffer).c_str());

}

void loop() {

  time_t nowTime = now();

  /*
  Manage UNSYNC status
  at first boot, the plant has to be synchronized with the remote status
  in this case, we send a special action (SYNC) in order to request such synchronization from the remote handler
  */

 Logger::notice("loop()", (String("alarmMode = ") + currentMode + String("; Status = ") + currentStatus + String("; nowTime=") + String(nowTime) + String("; lastMessageTime=") + String(lastMessageTime)).c_str());

  if(cm.isWiFiConnected() && cm.isAwsConnected()){

    if(currentMode == SYNC){
      Logger::notice("loop()", String("alarmMode = SYNC").c_str());
      if (
        ((nowTime - lastMessageTime) > SYNC_WAIT_TIME)
        || (lastMessageTime == 0) // in the first loop => lastMessageTime is equal to zero
      ){
        Logger::notice("loop()", String("Sending SYNC Message").c_str());
        //serializeAndSend(bc.getAllSensors(), false, true, "SYNC", "SYNC");
        serializeAndSend(bc.getAllSensors(), false, "SYNC", "SYNC");
        lastMessageTime = nowTime;
        Logger::notice("loop()", String("SYNC Message SENT").c_str());
      }
    }


    else if((currentMode == ARMED) || (currentMode == MONITOR)){

      bc.refresh();

      std::vector<Sensor*> alarmedSensors = bc.getAlarmedSensors();

      // manage ARMED mode and ALARM Status
      if(alarmedSensors.size() > 0){

        Logger::notice("loop()", String("one or more ALARMED sensors found").c_str());

        serializeAndSend(alarmedSensors, true,  ((currentMode == ARMED) ? "ARMED" : "MONITOR"), "NOTIFY");
        currentStatus = ALARM;
      
        // update time offset
        lastMessageTime = nowTime;
      }

      // manage ARMED mode and NON Alarm Status (NORMAL or CONFIG_UPDATE)
      else{
        
        if(((nowTime - lastMessageTime) > MAX_SILENCE_TIME) || (currentStatus != NORMAL)){
          serializeAndSend(bc.getAllSensors(), false, ((currentMode == ARMED) ? "ARMED" : "MONITOR"), "NOTIFY");

          // update time offset
          lastMessageTime = nowTime;
        }

        currentStatus = NORMAL;
      }
    }

    /*
    manage NON-ARMED mode (CONFIG_UPDATE)
    if status is not yet updated, send a message to align the IOT Core and update it
    */
    else if(currentStatus == CONFIG_UPDATE){
      serializeAndSend(bc.getAllSensors(), false, "OFF", "NOTIFY");
      currentStatus = NORMAL;

      // update time offset
      lastMessageTime = nowTime;
    }
  }
  
  //the function client.loop() should be called regularly to allow the client to process incoming messages to send publish data and makes a refresh of the connection.
  if(!cm.keepAlive()){
    if(cm.wifiReconnect()){
      if(!cm.connectToAWS(AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT, THINGNAME, AWS_IOT_SUBSCRIBE_TOPIC, &messageReceivedCallback)){
        Logger::error("loop()", String("could not reconnect, restarting... ").c_str());
        ESP.restart();
      }
    }
    else{
      Logger::error("loop()", String("could not reconnect, restarting... ").c_str());
      ESP.restart();
    }
  }

  Logger::notice("loop()", String("Sleeping... ").c_str());

  delay(1000);

}


void loop_debug() {

  delay(1000);

  int prox1_val = digitalRead(PIN_IN_PROX1);
  int prox2_val = digitalRead(PIN_IN_PROX2);
  int prox3_val = digitalRead(PIN_IN_PROX3);
  int prox4_val = digitalRead(PIN_IN_PROX4);
  int prox5_val = digitalRead(PIN_IN_PROX5);

  int adcVal1 = 0;
  int adcVal2 = 0;
  int adcVal3 = 0;

  double pir1_val = debug_measurePir(PIN_IN_PIR1, REFERENCE_VOLTAGE, PERCENTAGE_RANGE_TOLERANCE, REFERENCE_RESISTOR_PIN33, &adcVal1);
  double pir2_val = debug_measurePir(PIN_IN_PIR2, REFERENCE_VOLTAGE, PERCENTAGE_RANGE_TOLERANCE, REFERENCE_RESISTOR_PIN35, &adcVal2);
  double pir3_val = debug_measurePir(PIN_IN_PIR3, REFERENCE_VOLTAGE, PERCENTAGE_RANGE_TOLERANCE, REFERENCE_RESISTOR_PIN36, &adcVal3);

  Serial.println("----------- NEW STATUS ------------------");
  Serial.printf("SENSOR[%d]: %d\n", PIN_IN_PROX1, prox1_val);
  Serial.printf("SENSOR[%d]: %d\n", PIN_IN_PROX2, prox2_val);
  Serial.printf("SENSOR[%d]: %d\n", PIN_IN_PROX3, prox3_val);
  Serial.printf("SENSOR[%d]: %d\n", PIN_IN_PROX4, prox4_val);
  Serial.printf("SENSOR[%d]: %d\n", PIN_IN_PROX5, prox5_val);

  if(pir1_val == __DBL_MAX__){
    Serial.printf("SENSOR[%d]: %d,  INF\n", PIN_IN_PIR1, adcVal1);
  }
  else{
    Serial.printf("SENSOR[%d]: %d,  %.6f\n", PIN_IN_PIR1, adcVal1, pir1_val);
  }

  if(pir2_val == __DBL_MAX__){
    Serial.printf("SENSOR[%d]: %d,  INF\n", PIN_IN_PIR2, adcVal2);
  }
  else{
    Serial.printf("SENSOR[%d]: %d,  %.6f\n", PIN_IN_PIR2, adcVal2, pir2_val);
  }

  if(pir3_val == __DBL_MAX__){
    Serial.printf("SENSOR[%d]: %d,  INF\n", PIN_IN_PIR3, adcVal3);
  }
  else{
    Serial.printf("SENSOR[%d]: %d,  %.6f\n", PIN_IN_PIR3, adcVal3, pir3_val);
  }
  
  Serial.println("-----------------------------------------");
  Serial.println();

  //delay(1000);

  //digitalWrite(PIN_LED, LOW);   // turn the LED off

  Serial.println(String("\n...sleeping..."));

}


double debug_measurePir(uint8_t p_inputPin, double p_referenceVoltage, double p_percRangeTolerance, double referenceResistor, int* adcRetVal){
  
  int adcVal = analogRead(p_inputPin);
  *adcRetVal = adcVal;

  // convert input signal to uint8
  int dacVal = map(adcVal, 0, 4095, 0, 255);

  // adjust scale
  double inputVoltage = adcVal / 4095.0 * p_referenceVoltage;

  double pir1ResistorValue = 0;

  // if input voltage is close to reference voltage => infinite resistor => normal situation => NO ALARM
  if ((inputVoltage <= (p_referenceVoltage * (1 + (p_percRangeTolerance / 100))) && (inputVoltage >= (p_referenceVoltage * (1 - (p_percRangeTolerance / 100)))))){
      pir1ResistorValue = __DBL_MAX__;
  }
  else{
      pir1ResistorValue = referenceResistor / ((p_referenceVoltage / inputVoltage) - 1);
  }

  return pir1ResistorValue;
}