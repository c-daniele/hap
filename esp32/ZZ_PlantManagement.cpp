#include "ZZ_PlantManagement.h"
#include <Logger.h>

SimpleHomeAlarmEsp32::Sensor::Sensor(SimpleHomeAlarmEsp32::SensorType type, SimpleHomeAlarmEsp32::SensorModel model, uint8_t portNumber, time_t sampleRate, String code, String description){
    this->alarm = false;
    this->type = type;
    this->model = model;
    this->portNumber = portNumber;
    this->sampleRate = sampleRate;
    this->code = code;
    this->description = description;
    this->value = 0;
    this->lastCheck = 0;
}

SimpleHomeAlarmEsp32::Sensor::~Sensor(){}

bool SimpleHomeAlarmEsp32::Sensor::isAlarm() { return alarm; }
void SimpleHomeAlarmEsp32::Sensor::setAlarm(bool alarm) { this->alarm = alarm; }

bool SimpleHomeAlarmEsp32::Sensor::isEnabled() { return enabled; }
void SimpleHomeAlarmEsp32::Sensor::setEnabled(bool enabled) { this->enabled = enabled; }

double SimpleHomeAlarmEsp32::Sensor::getValue() { return this->value; }
void SimpleHomeAlarmEsp32::Sensor::setValue(double value) { this->value = value; }

time_t SimpleHomeAlarmEsp32::Sensor::getSamplerate() { return this->sampleRate; }
void SimpleHomeAlarmEsp32::Sensor::setSampleRate(time_t sampleRate) { this->sampleRate = sampleRate; }
void SimpleHomeAlarmEsp32::Sensor::setLastCheck(time_t lastCheck) { this->lastCheck = lastCheck; }

time_t SimpleHomeAlarmEsp32::Sensor::getLastCheck() { return this->lastCheck; }
String SimpleHomeAlarmEsp32::Sensor::getCode() { return this->code; }
String SimpleHomeAlarmEsp32::Sensor::getdescription() { return this->description; }
uint8_t SimpleHomeAlarmEsp32::Sensor::getPortNumber() { return this->portNumber; }
SimpleHomeAlarmEsp32::SensorType SimpleHomeAlarmEsp32::Sensor::getType() { return this->type; }
SimpleHomeAlarmEsp32::SensorModel SimpleHomeAlarmEsp32::Sensor::getModel() { return this->model; }

void SimpleHomeAlarmEsp32::Sensor::getStatusSerialized(JsonArray &parentNode){
    JsonObject sensorNode = parentNode.createNestedObject();
    sensorNode["deviceCode"] = this->code;
    if(this->value == __DBL_MAX__){
        sensorNode["value"] = String("infinite");
    }
    else{
        sensorNode["value"] = String(this->value, 4);
    }
    sensorNode["alarm"] = this->alarm;
    sensorNode["lastCheck"] = this->lastCheck;
    sensorNode["enabled"] = this->enabled;
//    sensorNode["description"] = this->description;
}

SimpleHomeAlarmEsp32::BoardController::BoardController(){
    //    this->sensorArray = new Sensor*[numSensors];
    //    numSensors = 0;
}

SimpleHomeAlarmEsp32::BoardController::~BoardController(){
    //    delete[] sensorArray;
}

void SimpleHomeAlarmEsp32::BoardController::init(){
    timeOffset = now();
}

void SimpleHomeAlarmEsp32::BoardController::addSensor(SimpleHomeAlarmEsp32::Sensor *s){
    
    Logger::notice("SimpleHomeAlarmEsp32::BoardController::addSensor()", String("Adding Sensor " + s->getCode()).c_str());

    this->sensors.push_back(s);
    pinMode(s->getPortNumber(), INPUT);
}

std::vector<SimpleHomeAlarmEsp32::Sensor *> SimpleHomeAlarmEsp32::BoardController::getAllSensors(){

    std::vector<SimpleHomeAlarmEsp32::Sensor *> outVal;

    for (auto s : this->sensors){
//        if (s->isEnabled()){
            outVal.push_back(s);
//        }
    }

    return outVal;
}

std::vector<SimpleHomeAlarmEsp32::Sensor *> SimpleHomeAlarmEsp32::BoardController::getAlarmedSensors(){
    std::vector<SimpleHomeAlarmEsp32::Sensor *> outVal;

    for (auto s : this->sensors){
        if (s->isAlarm() && s->isEnabled()){
            outVal.push_back(s);
        }
    }

    return outVal;
}

void SimpleHomeAlarmEsp32::BoardController::refresh(){
    time_t nowTime = now();

    //Logger::notice("SimpleHomeAlarmEsp32::BoardController::refresh()", String("Now Time: " + nowTime).c_str());

    for (auto s : this->sensors){
//#ifdef DEBUG
//    Serial.println(String("Checking sensor: ") + String(s->getCode()));
//#endif
        if(s->isEnabled()){

            //Logger::notice("SimpleHomeAlarmEsp32::BoardController::refresh()", String("last check: " + s->getLastCheck()).c_str());

            if ((s->getLastCheck() + s->getSamplerate()) < nowTime){
                this->measure(s);
                s->setLastCheck(nowTime);
            }
        }
        else{
            
            //Logger::notice("SimpleHomeAlarmEsp32::BoardController::refresh()", String("Sensor is not enabled, nothing to do").c_str());

            // if not enabled => turn off alarm
            s->setAlarm(false);
        }
    }
}

void SimpleHomeAlarmEsp32::BoardController::measure(SimpleHomeAlarmEsp32::Sensor *sensor){

    Logger::notice("SimpleHomeAlarmEsp32::BoardController::measure()", (String("Measuring sensor ") + sensor->getCode() + String(" ; PORT: ") + sensor->getPortNumber()).c_str());

    if (sensor->getType() == SimpleHomeAlarmEsp32::SensorType::PIR){

        // detect input signal
        int adcVal = analogRead(sensor->getPortNumber());

        // convert input signal to uint8
        int dacVal = map(adcVal, 0, 4095, 0, 255);

        // adjust scale
        double inputVoltage = adcVal / 4095.0 * REFERENCE_VOLTAGE;

        double pir1ResistorValue = 0;

        // if input voltage is close to reference voltage => infinite resistor => normal situation => NO ALARM
        if ((inputVoltage <= (REFERENCE_VOLTAGE * (1 + (PERCENTAGE_RANGE_TOLERANCE / 100))) && (inputVoltage >= (REFERENCE_VOLTAGE * (1 - (PERCENTAGE_RANGE_TOLERANCE / 100)))))){
            pir1ResistorValue = __DBL_MAX__;
        }
        else{
            double referenceResistor = 10000;
            switch (sensor->getPortNumber()){
                case 33:
                    referenceResistor = REFERENCE_RESISTOR_PIN33;
                    break;
                case 35:
                    referenceResistor = REFERENCE_RESISTOR_PIN35;
                    break;
                case 36:
                    referenceResistor = REFERENCE_RESISTOR_PIN36;
                    break;
            }
            pir1ResistorValue = referenceResistor / ((REFERENCE_VOLTAGE / inputVoltage) - 1);
        }
        sensor->setValue(pir1ResistorValue);
        

        // dacWrite(DAC1, dacVal);
// FIXME
//    char buffer[100];
//    sprintf(buffer, "[PIR_%d] - ADC Val: %d, \t DAC Val: %d, \t Voltage: %.6fV, \t Resistor: %.6f Ohm", sensor->getPortNumber(), adcVal, dacVal, inputVoltage, pir1ResistorValue);
//    Logger::notice("SimpleHomeAlarmEsp32::BoardController::measure()", buffer);
#ifdef DEBUG
        Serial.printf("[PIR_%d] - ADC Val: %d, \t DAC Val: %d, \t Voltage: %.6fV, \t Resistor: %.6f Ohm\n", sensor->getPortNumber(), adcVal, dacVal, inputVoltage, pir1ResistorValue);
#endif

        double threshold = (sensor->getModel() == SimpleHomeAlarmEsp32::SensorModel::Bentel) ? BENTEL_ALARM_THRESHOLD : PYRONIX_ALARM_THRESHOLD;

        if (pir1ResistorValue > threshold){
            sensor->setAlarm(true);
//            sprintf(buffer, "[PIR ALARM] - Resistor: %.2f Ohm", pir1ResistorValue);
//            Logger::notice("SimpleHomeAlarmEsp32::BoardController::measure()", buffer);
#ifdef DEBUG
            Serial.printf("[PIR ALARM] - Resistor: %.2f Ohm\n", pir1ResistorValue);
#endif
        }
        else{
            sensor->setAlarm(false);
            
            //sprintf(buffer, "[PIR OK] - Resistor: %.2f Ohm\n", pir1ResistorValue);
            //Logger::notice("SimpleHomeAlarmEsp32::BoardController::measure()", buffer);
#ifdef DEBUG
            Serial.printf("[PIR OK] - Resistor: %.2f Ohm\n", pir1ResistorValue);
#endif
        }
    }
    else if (sensor->getType() == SimpleHomeAlarmEsp32::SensorType::PROXIMITY){
        if (digitalRead(sensor->getPortNumber()) == HIGH){
            sensor->setAlarm(true);
            sensor->setValue(1);

            Logger::notice("SimpleHomeAlarmEsp32::BoardController::measure()", (String("ALARM SET ON sensor ") + sensor->getCode()).c_str());
        }
        else{
            sensor->setAlarm(false);
            sensor->setValue(0);
        }
    }

    // dummy handling, just for logging
    /*else if (sensor->getType() == SimpleHomeAlarmEsp32::SensorType::PROXIMITY){

        // detect input signal
        int adcVal = analogRead(sensor->getPortNumber());

        // convert input signal to uint8
        int dacVal = map(adcVal, 0, 4095, 0, 255);

        // adjust scale
        double inputVoltage = adcVal / 4095.0 * REFERENCE_VOLTAGE;
        sensor->setValue(inputVoltage);
        sensor->setAlarm(false);

        Serial.printf("[%s] - ADC Val: %d, \t DAC Val: %d, \t Voltage: %.2fV\n", sensor->getCode(), adcVal, dacVal, inputVoltage);

    }*/
}