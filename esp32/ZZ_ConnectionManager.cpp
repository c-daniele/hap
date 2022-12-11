#include "ZZ_ConnectionManager.h"
#include <Logger.h>

// How many times we should attempt to connect to AWS
#define AWS_MAX_RECONNECT_TRIES 5
#define DEBUG

SimpleHomeAlarmEsp32::ConnectionManager::~ConnectionManager(){
  
}

SimpleHomeAlarmEsp32::ConnectionManager::ConnectionManager(){

  //wiFiClient = WiFiClientSecure();
  //mqttClient = MQTTClient(256);
}

boolean SimpleHomeAlarmEsp32::ConnectionManager::isWiFiConnected(){
  return WiFi.isConnected();
}

boolean SimpleHomeAlarmEsp32::ConnectionManager::isAwsConnected(){
  return mqttClient.connected();
}

boolean SimpleHomeAlarmEsp32::ConnectionManager::wifiReconnect(){
  
  WiFi.reconnect();

  // Only try 15 times to connect to the WiFi
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 15)
  {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
    retries++;
  }

  // If we still couldn't connect to the WiFi, go to deep sleep for a minute and try again.
  if (WiFi.status() != WL_CONNECTED){
    //esp_sleep_enable_timer_wakeup(1 * 60L * 1000000L);
    //esp_deep_sleep_start();
    //delay(10000);

    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::connectToWiFi", (String("Could not connect to WiFi!!")).c_str());

    return false;
  }

  Logger::notice("SimpleHomeAlarmEsp32::ConnectionManager::connectToWiFi", (String("Connected to WiFi")).c_str());

  return true;

}

boolean SimpleHomeAlarmEsp32::ConnectionManager::connectToWiFi(const char *wifiSsd, const char *wifiPassword){

  // ENABLING WIFI will disable ACD2 PINS (27, 13)!!!!
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsd, wifiPassword);

  // Only try 15 times to connect to the WiFi
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 15)
  {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
    retries++;
  }

  // If we still couldn't connect to the WiFi, go to deep sleep for a minute and try again.
  if (WiFi.status() != WL_CONNECTED){
    //esp_sleep_enable_timer_wakeup(1 * 60L * 1000000L);
    //esp_deep_sleep_start();

    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::connectToWiFi", (String("Could not connect to WiFi!!")).c_str());

    return false;
  }

  Logger::notice("SimpleHomeAlarmEsp32::ConnectionManager::connectToWiFi", (String("Connected to WiFi")).c_str());

  return true;
}

// cm.connectToAWS(AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT, THINGNAME, AWS_IOT_SUBSCRIBE_TOPIC);
boolean SimpleHomeAlarmEsp32::ConnectionManager::connectToAWS(const char *rootCertificate
  , const char *deviceCertificate
  , const char *devicePrivateKey
  , const char *iotEndpoint
  , const char *thingName
  , const char *subscribeTopic
  , MQTTClientCallbackSimple callback){

    Logger::notice("SimpleHomeAlarmEsp32::ConnectionManager::connectToAWS", (String("start")).c_str());

  // Configure WiFiClientSecure to use the AWS certificates we generated
  wiFiClient.setCACert(rootCertificate);
  wiFiClient.setCertificate(deviceCertificate);
  wiFiClient.setPrivateKey(devicePrivateKey);
  this->callback = callback;
  this->subscribeTopic = String(subscribeTopic);
  this->thingName = String(thingName);
  this->iotEndpoint = String(iotEndpoint);

  return this->reconnectToMqtt();

  Logger::notice("SimpleHomeAlarmEsp32::ConnectionManager::connectToAWS", (String("end")).c_str());

}

boolean SimpleHomeAlarmEsp32::ConnectionManager::reconnectToMqtt(){

  //mqttClient.disconnect();

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  mqttClient.begin(this->iotEndpoint.c_str(), 8883, wiFiClient);

  // Try to connect to AWS and count how many times we retried.
  int retries = 0;

  Logger::notice("SimpleHomeAlarmEsp32::ConnectionManager::reconnectToMqtt", (String("...Connecting to AWS IOT...")).c_str());

  while (!mqttClient.connect(this->thingName.c_str()) && retries < AWS_MAX_RECONNECT_TRIES){
    Serial.print(".");
    delay(200);
    retries++;
  }

  // Make sure that we did indeed successfully connect to the MQTT broker
  // If not we just end the function and wait for the next loop.

  if (!mqttClient.connected()){

    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::reconnectToMqtt", (String("TIMEOUT")).c_str());
    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::reconnectToMqtt", (String("ERROR during publish, last error: ") + String(mqttClient.lastError())).c_str());
    
    return false;
  }

  if (mqttClient.subscribe(this->subscribeTopic)){

    Logger::notice("SimpleHomeAlarmEsp32::ConnectionManager::reconnectToMqtt", (String("Subscribed to ") + String(this->subscribeTopic)).c_str());
    mqttClient.onMessage(this->callback);

    // If we land here, we have successfully connected to AWS!
    // And we can subscribe to topics and send messages.
    return true;
  }
  else{

    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::reconnectToMqtt", (String("Cannot subscribe to ") + String(this->subscribeTopic)).c_str());
    return false;
  } 

  return true;
}

boolean SimpleHomeAlarmEsp32::ConnectionManager::sendIotMessage(const char topic[], const char payload[], size_t payload_size){

  if (!this->isWiFiConnected()){
    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::sendIotMessage", (String("Disconnected from WiFi!! ")).c_str());
    return false;
  }

  if (!this->isAwsConnected()){
    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::sendIotMessage", (String("Disconnected from AWS!! ")).c_str());
    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::sendIotMessage", (String("ERROR during publish, last error: ") + String(mqttClient.lastError())).c_str());

    return false;
  }

  if(
    ((payload_size == -1) && mqttClient.publish(topic, payload))
    || mqttClient.publish(topic, payload, payload_size)
  ){
    //Serial.println(String("Message SENT!!"));
    return true;
  }
  else{
    Logger::error("SimpleHomeAlarmEsp32::ConnectionManager::sendIotMessage", (String("ERROR during publish, last error: ") + String(mqttClient.lastError())).c_str());
    return false;
  }
}

boolean SimpleHomeAlarmEsp32::ConnectionManager::keepAlive(){

  if (!this->isAwsConnected()) {
    if(!this->reconnectToMqtt()){
      return false;
    }
  }

  //the function client.loop() should be called regularly to allow the client to process incoming messages to send publish data and makes a refresh of the connection.
  mqttClient.loop();
  
  return true;

}