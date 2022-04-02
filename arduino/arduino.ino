
// main file for arduino i2c interface

#include "ArduinoJson-v6.19.2.h"
#include "Devices.h"
// #include <TCA9548A.h> // mux

StaticJsonDocument<128> doc; // json document for read/write, declared on the stack

// these two are ESSENTIAL for the use of the multiplexer `Device` backend
TCA9548A Device::mux(0x71);      // address of mux
uint8_t Device::channels = 0xFF; // default mux state

// array of devices
// If passed a multiplexer channel, then the multiplexer will only allow the device to communicate on that channel when absolutely necessary
Device *devices[] = {new Device(), new MAX31855(), new SHTC3()};
// , new LC709203F(), new LTC2941_BFG(), new INA219(), new SHTC3(), new MAX1704x_BFG(), new INA260(), new LC709203F(),
// Device *devices[] = {new Device()};

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    while (!Serial)
        delay(1);

    if (TWCR == 0) // do this check so that Wire only gets initialized once
    {
        Wire.begin();
        LOG(F("Wire initialized"));
    }

    // initialize mux
    Device::mux.begin();
    Device::mux.writeRegister(Device::channels); // set base state
    LOG(F("Mux initialized"));
    for (Device *d : devices)
    {
        d->begin();
    }

    LOG(F("Done setup"));
}

void loop()
{
    // check for new data
    if (Serial.available() > 0)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        // incoming data comes in as{DeviceStr:RequestsStr}
        // example: {"LC709203F":"VP"} will get Voltage and Percent from LC709203F device
        deserializeJson(doc, Serial);
        JsonObject root = doc.as<JsonObject>();
        for (JsonPair kv : root)
        {
            // iterate through devices
            String k = kv.key().c_str();
            Device *d = getDevice(k);
            String v = kv.value().as<const char *>();
            // build return packet
            DynamicJsonDocument ret(128);
            ret["D"] = d->D();
            for (char c : v)
            {
                ret[String(c)] = getValue(d, c);
            }
            serializeJson(ret, Serial); // send return package
            Serial.print('\n');         // delimiter between packets
        }
        digitalWrite(LED_BUILTIN, LOW);
    }
    delay(1); // slow the processor down a little
}

Device *getDevice(String _D)
{
    for (Device *d : devices)
    {
        if (d->D() == _D)
        {
            return d;
        }
    }
    ERROR(F("Requested device not found"));
    return NULL; // will return an empty device if not found
}

String getValue(Device *d, char V)
{
    switch (V)
    {
    case 'V':
        return String(d->V(), 3);
    case 'I':
        return String(d->I(), 3);
    case 'C':
        return String(d->C(), 3);
    case 'P':
        return String(d->P(), 3);
    case 'T':
        return String(d->T(), 3);
    case 'H':
        return String(d->H(), 3);
    case 'W':
        return String(d->W(), 3);
    default:
        return "";
    }
}
