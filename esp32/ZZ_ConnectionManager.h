#ifndef SIMPLEHOMEALARM_ESP32_CONNECTIONMANAGER
#define SIMPLEHOMEALARM_ESP32_CONNECTIONMANAGER

#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include "WiFi.h"

namespace SimpleHomeAlarmEsp32{

    class ConnectionManager{
    private:
        WiFiClientSecure wiFiClient = WiFiClientSecure();
        MQTTClient mqttClient = MQTTClient(2048);
        MQTTClientCallbackSimple callback;
        String subscribeTopic;
        String thingName;
        String iotEndpoint;
        boolean reconnectToMqtt();

    public:
        ConnectionManager(/* args */);
        ~ConnectionManager();
        boolean isWiFiConnected();
        boolean isAwsConnected();
        boolean connectToWiFi(const char* wifiSsd, const char* wifiPassword);
        boolean wifiReconnect();
        boolean connectToAWS(const char * rootCertificate
            , const char * deviceCertificate
            , const char * devicePrivateKey
            , const char * iotEndpoint
            , const char * thingName
            , const char * subscribeTopic
            , MQTTClientCallbackSimple callback);
        boolean sendIotMessage(const char topic[], const char payload[], size_t payload_size = -1);
        boolean keepAlive();
    };
};

#endif