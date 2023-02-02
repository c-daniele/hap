#ifndef SIMPLEHOMEALARM_ESP32_PLANTMANAGEMENT
#define SIMPLEHOMEALARM_ESP32_PLANTMANAGEMENT

//#define PIN_LED                        2

#define REFERENCE_RESISTOR_PIN33 13120
#define REFERENCE_RESISTOR_PIN35 13120
#define REFERENCE_RESISTOR_PIN36 13120

#define PERCENTAGE_RANGE_TOLERANCE 5
#define REFERENCE_VOLTAGE 3.3
#define BENTEL_ALARM_THRESHOLD 3500

// Standard threshold =~ 2,2kohm
// Alarm threshold =~ 7kohm
// Tamper threshold =~ +Infinite
#define PYRONIX_ALARM_THRESHOLD 5000

#define SENSOR_VECTOR_DEFAULT_SIZE 10
#define JSON_PAYLOAD_SIZE 128

#define DEBUG

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <TimeLib.h>

namespace SimpleHomeAlarmEsp32
{

    enum SensorType
    {
        PIR,
        PROXIMITY,
        OTHER
    };

    enum SensorModel
    {
        Pyronix,
        Bentel,
        Other
    };

    class Sensor
    {
    private:
        bool alarm;
        bool enabled = false;
        double value;
        time_t sampleRate;
        time_t lastCheck;
        SensorType type;
        SensorModel model;
        uint8_t portNumber;
        String code;
        String description;

    public:
        Sensor(SimpleHomeAlarmEsp32::SensorType type, SimpleHomeAlarmEsp32::SensorModel model, uint8_t portNumber, time_t sampleRate, String code, String description = "");
        ~Sensor();

        bool isAlarm();
        bool isEnabled();
        double getValue();
        time_t getSamplerate();
        time_t getLastCheck();
        void getStatusSerialized(JsonArray& parentNode);
        String getCode();
        String getdescription();
        uint8_t getPortNumber();
        SensorType getType();
        SensorModel getModel();

        void setAlarm(bool alarm);
        void setEnabled(bool enabled);
        void setValue(double value);
        void setSampleRate(time_t sampleRate);
        void setLastCheck(time_t lastCheck);
    };

    class BoardController
    {
    private:
        //std::vector<Sensor *> sensors(SENSOR_VECTOR_DEFAULT_SIZE);
        std::vector<Sensor *> sensors;
        time_t timeOffset;
        int numSensors;
        void measure(Sensor *sensor);

    public:
        BoardController();
        ~BoardController();

        void init();
        void addSensor(Sensor* s);
        void refresh();
        std::vector<Sensor *> getAllSensors();
        std::vector<Sensor *> getAlarmedSensors();
    };
};

#endif