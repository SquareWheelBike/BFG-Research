// Header file for BFG implementations

// avr libraries
#include <Arduino.h> // String implemented here
#include <Wire.h>

// bfg libraries
#include <Adafruit_LC709203F.h>
#include <LTC2941.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

// remaining libraries
#include <TCA9548A.h>
#include <Adafruit_SHTC3.h>
#include <SPI.h> // for thermocouple softserial
#include <Adafruit_MAX31855.h>
#include <Adafruit_INA260.h>
#include <Adafruit_INA219.h>

// --------------- UTILITY FUNCTIONS ---------------

void LOG(String msg)
{
    Serial.println("{\"D\":\"LOG\",\"M\":\"" + msg + "\"}");
}

// If there is a major fault, display msg on serial and flash LED
// all messages should be PROGMEM strings, to save stack space
void ERROR(const __FlashStringHelper *msg)
{
    LOG(msg);
    pinMode(LED_BUILTIN, OUTPUT);
    while (1)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }
}

// ------------------ DEVICE BASE CLASS ------------------

class Device
{
public:
    Device() { this->_D = "none"; }
    virtual bool begin() { return true; }
    // void begin();                   // start I2C interface
    virtual String D() { return this->_D; } // Device ID
    virtual float V() { return -1; }        // Voltage (V)
    virtual float I() { return -1; }        // current (mA)
    virtual float C() { return -1; }        // draw so far (mAh)
    virtual float P() { return -1; }        // Battery percentage
    virtual float T() { return -1; }        // Temperature (C)
    virtual float H() { return -1; }        // Humidity (%), relative
    virtual float W() { return -1; }        // Wattage (mW)
    // int S() { return 0; }           // state, 0=discharging, 1=charging
protected:
    String _D; // Device ID
};

// ------------------ BFG DEVICES ------------------

class LC709203F : public Device
{
public:
    LC709203F()
    {
        this->_D = "LC709203F";
    }

    bool begin()
    {
        if (!this->lc.begin())
            ERROR(F("Couldnt find Adafruit LC709203F?\nMake sure a battery is plugged in!"));
        lc.setThermistorB(3950);
        lc.setPackSize(LC709203F_APA_500MAH);
        lc.setAlarmVoltage(3.4);
        LOG("LC709203F initialized");
        return true;
    }

    float V()
    {
        Serial.println(lc.cellVoltage(), 3);
        return lc.cellVoltage();
    }
    float P() { return lc.cellPercent(); }
    float T() { return lc.getCellTemperature(); }

protected:
    Adafruit_LC709203F lc;
};

class LTC2941_BFG : public Device
{
public:
    LTC2941_BFG()
    {
        this->_D = "LTC2941";
    }

    bool begin()
    {
        this->ltc.initialize();
        ltc2941.setBatteryFullMAh(1000);
        LOG("LTC2941 initialized");
        return true;
    }

    float C() { return this->ltc.getmAh(); }
    float P() { return this->ltc.getPercent(); }

protected:
    LTC2941 ltc = ltc2941;
};

// this sucks but it was the only way to get the library to compile
SFE_MAX1704X lipo(MAX1704X_MAX17043);
class MAX1704x_BFG : public Device
{
public:
    MAX1704x_BFG()
    {
        this->_D = "MAX17043";
    }

    bool begin()
    {
        if (TWCR == 0) // do this check so that Wire only gets initialized once
        {
            Wire.begin();
        }
        if (!lipo.begin())
            ERROR(F("MAX17043 not detected. Please check wiring. Freezing."));
        // Quick start restarts the MAX17044 in hopes of getting a more accurate guess for the SOC.
        lipo.quickStart();

        // We can set an interrupt to alert when the battery SoC gets too low. We can alert at anywhere between 1% - 32%:
        lipo.setThreshold(20); // Set alert threshold to 20%.
        LOG("MAX17043 initialized");
        return true;
    }

    float V() { return lipo.getVoltage(); }
    float P() { return lipo.getSOC(); }

    // protected:
    // SFE_MAX1704X lipo(MAX1704X_MAX17043); // Create a MAX17043 object
};

// ------------------ REMAINING I2C DEVICES ------------------

// TODO: add more info to this class
class TCA9548AMUX
{
public:
    TCA9548AMUX()
    {
        this->_D = "TCA9548A";
    }

    bool begin(TwoWire &w)
    {
        // Wire.begin(); // open wire connection
        this->tca.begin(w); // can be started without Wire.begin()
        tca.openAll();      // by default, open all channels. Only mess with this if there are address conflicts
        LOG("TCA9548A initialized");
        return true;
    }

    void openChannel(uint8_t channel)
    {
        this->tca.openChannel(channel);
    }

    void closeChannel(uint8_t channel)
    {
        this->tca.closeChannel(channel);
    }

private:
    TCA9548A tca; // address can be passed into the constructor
    String _D;
};

class SHTC3 : public Device
{
public:
    SHTC3()
    {
        this->_D = "SHTC3";
    }

    bool begin()
    {
        if (!shtc3.begin())
            ERROR(F("Couldn't find SHTC3"));
        // Serial.println("Found SHTC3 sensor");
        LOG("SHTC3 initialized");
        return true;
    }

    float T()
    {
        this->_update();
        return this->temp.temperature;
    }
    float H()
    {
        this->_update();
        return this->humidity.relative_humidity;
    }

private:
    Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
    sensors_event_t humidity, temp;

    void _update()
    {
        this->shtc3.getEvent(&humidity, &temp);
    }
};

class MAX31855 : public Device
{
public:
    MAX31855()
    {
        this->_D = "MAX31855";
    }

    bool begin()
    {
        uint8_t MAXDO = 3, MAXCS = 4, MAXCLK = 5;
        this->thermocouple = new Adafruit_MAX31855(MAXDO, MAXCS, MAXCLK);
        if (!this->thermocouple->begin())
            ERROR(F("Couldn't find MAX31855"));
        LOG("MAX31855 initialized");
        return true;
    }

    float C()
    {
        double c = this->thermocouple->readCelsius();
        if (isnan(c))
            ERROR(F("Something wrong with thermocouple!"));
        return c;
    }

private:
    Adafruit_MAX31855 *thermocouple;
};

class INA260 : public Device
{
public:
    INA260()
    {
        this->_D = "INA260";
    }

    bool begin()
    {
        if (!this->ina.begin())
            ERROR(F("Couldn't find INA260"));
        LOG("INA260 initialized");
        return true;
    }

    float V() { return this->ina.readBusVoltage(); }
    float I() { return this->ina.readCurrent(); }
    float W() { return this->ina.readPower(); }

protected:
    Adafruit_INA260 ina = Adafruit_INA260();
};

class INA219 : public Device
{
public:
    INA219()
    {
        this->_D = "INA219";
    }

    bool begin()
    {
        if (!this->ina.begin())
            ERROR(F("Couldn't find INA260"));
        LOG(F("Began INA219."));
        return true;
    }

    // float V() { return this->ina.getShuntVoltage_mV(); }
    float I() { return this->ina.getCurrent_mA(); }
    // float W() { return this->ina.getPower_mW(); }

protected:
    Adafruit_INA219 ina;
};
