
/*!
 *  @file Adafruit_LSM6DSOX.cpp
 *
 *  @mainpage Adafruit LSM6DSOX 6-DoF Accelerometer and Gyroscope library
 *
 *  @section intro_sec Introduction
 *
 * 	I2C Driver for the Adafruit LSM6DSOX 6-DoF Accelerometer and Gyroscope library
 * 
 * 	This is a library for the Adafruit LSM6DSOX breakout:
 * 	https://www.adafruit.com/product/PID_HERE
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *  @section dependencies Dependencies
 *  This library depends on the Adafruit BusIO library
 *  
 *  This library depends on the Adafruit Unified Sensor library

 *  @section author Author
 *
 *  Bryan Siepert for Adafruit Industries
 *
 * 	@section license License
 * 
 * 	BSD (see license.txt)
 *
 * 	@section  HISTORY
 *
 *     v1.0 - First release
 */

#include "Arduino.h"
#include <Wire.h>

#include "Adafruit_LSM6DSOX.h"

/*!
 *    @brief  Instantiates a new LSM6DSOX class
 */
Adafruit_LSM6DSOX::Adafruit_LSM6DSOX(void) {}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  i2c_address
 *            The I2C address to be used.
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @param  sensor_id
 *            The user-defined ID to differentiate different sensors
 *    @return True if initialization was successful, otherwise false.
 */
boolean Adafruit_LSM6DSOX::begin(uint8_t i2c_address, TwoWire *wire, int32_t sensor_id) {
  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  Adafruit_BusIO_Register chip_id =
    Adafruit_BusIO_Register(i2c_dev, LSM6DSOX_WHOAMI);

  // make sure we're talking to the right chip
  if (chip_id.read() != LSM6DSOX_CHIP_ID) {
    return false;
  }
  _sensorid_accel = sensor_id;

  return reset();
    // self._bdu = True
  // self._gyro_data_rate = 3
  // self._if_inc = True  
    //   _if_inc = RWBit(_LSM6DSOX_CTRL3_C, 2)
    // _sim = RWBit(_LSM6DSOX_CTRL3_C, 3)
    // _pp_od = RWBit(_LSM6DSOX_CTRL3_C, 4)
    // _h_lactive = RWBit(_LSM6DSOX_CTRL3_C, 5)
    // _bdu = RWBit(_LSM6DSOX_CTRL3_C, 6)
  // enable accelerometer by setting the data rate to non-zero (disabled)
  setAccelDataRate(LSM6DSOX_RATE_104_HZ);
}

boolean Adafruit_LSM6DSOX::reset(void) {

    Adafruit_BusIO_Register ctrl3 =
      Adafruit_BusIO_Register(i2c_dev, LSM6DSOX_CTRL3_C);

    Adafruit_BusIO_RegisterBits boot =
      Adafruit_BusIO_RegisterBits(&ctrl3, 1, 7);

    Adafruit_BusIO_RegisterBits sw_reset =
      Adafruit_BusIO_RegisterBits(&ctrl3, 1, 0);

    boot.write(true);
    while (boot.read()){
      delay(1);
    }

    sw_reset.write(true);
    while (sw_reset.read()){
      delay(1);
    }


    // _sw_reset = RWBit(_LSM6DSOX_CTRL3_C, 0)

    // _boot = RWBit(_LSM6DSOX_CTRL3_C, 7)
    //     self._sw_reset = True
    //     while self._sw_reset:
    //         sleep(0.001)
    //     self._boot = True
    //     while self._boot:
    //         sleep(0.001)
  return true;
}


/**************************************************************************/
/*!
    @brief  Gets the most recent sensor event, Adafruit Unified Sensor format
    @param  accel
            Pointer to an Adafruit Unified sensor_event_t object to be filled
            with acceleration event data.

    @return True on successful read
*/
/**************************************************************************/
 bool Adafruit_LSM6DSOX::getEvent(sensors_event_t *accel) {
  uint32_t t = millis();
  _read();

  memset(accel, 0, sizeof(sensors_event_t));
  accel->version = 1;
  accel->sensor_id = _sensorid_accel;
  accel->type = SENSOR_TYPE_ACCELEROMETER;
  accel->timestamp = t;
  accel->acceleration.x = accX * 0.061 * SENSORS_GRAVITY_STANDARD/1000;
  accel->acceleration.y = accY * 0.061 * SENSORS_GRAVITY_STANDARD/1000;
  accel->acceleration.z = accZ * 0.061 * SENSORS_GRAVITY_STANDARD/1000;

  return true;
}

/**************************************************************************/
/*!
    @brief Gets the accelerometer data rate.
    @returns The the accelerometer data rate.
*/
LSM6DSOX_data_rate_t Adafruit_LSM6DSOX::getAccelDataRate(void){

    Adafruit_BusIO_Register ctrl1 =
      Adafruit_BusIO_Register(i2c_dev, LSM6DSOX_CTRL1_XL);

    Adafruit_BusIO_RegisterBits accel_data_rate =
      Adafruit_BusIO_RegisterBits(&ctrl1, 4, 4);
    return accel_data_rate.read();
}

/**************************************************************************/
/*!
    @brief Sets the accelerometer data rate.
    @param  data_rate
            The the accelerometer data rate. Must be a
            `LSM6DSOX_data_rate_t`.
*/
void Adafruit_LSM6DSOX::setAccelDataRate(LSM6DSOX_data_rate_t data_rate){

    Adafruit_BusIO_Register ctrl1 =
      Adafruit_BusIO_Register(i2c_dev, LSM6DSOX_CTRL1_XL);

    Adafruit_BusIO_RegisterBits accel_data_rate =
      Adafruit_BusIO_RegisterBits(&ctrl1, 4, 4);

    accel_data_rate.write(data_rate);
}

/******************* Adafruit_Sensor functions *****************/
/*!
 *     @brief  Updates the measurement data for all sensors simultaneously
 */
/**************************************************************************/
void Adafruit_LSM6DSOX::_read(void) {
  // get raw readings
  Adafruit_BusIO_Register data_reg =
      Adafruit_BusIO_Register(i2c_dev, LSM6DSOX_OUTX_L_A, 6);

  uint8_t buffer[6];
  data_reg.read(buffer, 6);

  rawAccX = buffer[1] << 8 | buffer[0];
  rawAccY = buffer[3] << 8 | buffer[2];
  rawAccZ = buffer[5] << 8 | buffer[4];

  accX = rawAccX;
  accY = rawAccY;
  accZ = rawAccZ;

}