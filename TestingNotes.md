# Testing Notes

## Testing Results

- All devices will work together, with the multiplexer
- The max number of device types allowed at a time is about 6 (PROGMEM limitation)
- The maximum update rate is about 16Hz (serial limitation, can be reduced if needed).

## LC709203

- **WILL NOT WORK without a battery connected**
- MultiMeter measures a steady 3.606V; BMS measures about 3.584V

## MAX17043

- Will initialize without a battery, but will give high readings
- NEEDS Wire() initialized before it will work, but keep in mind initializing Wire multiple times can cause issues

## INA219

- Works with or without a load
- DO NOT connect battery straight to it. duh
- ONLY measures current (I)

## SHTC3 Temperature and Humidity Sensor

- temperature and humidity work fine

## MAX31855

- **USES SOFTWARE SERIAL, *NOT* I2C**
- SoftSerial pins are CLK = 4, CS = 5, DO = 6 on the Micro.
- T() calls thermocouple temp
- C() calls internal temp (shouldn't be used in production, no reason for it)

## LTC2941

- not here yet

## INA260

- For high side power measurement, VIN+- are used as an inline current sensor, and the voltage reading is read between VIN+ and GND. (TESTED)
- For low side power measurement, VIN+- are used as an inline current sensor, and the voltage reading is read between VIN- and VBus. (UNTESTED)

## TCA9548A Multiplexer

- Close all channels right away
- Any time you want to use a channel, open it using the `Device::setmux()` function
- Data is collected by passing a lambda function containing the code to get the data, to `Device::withmux(Func f)`.

