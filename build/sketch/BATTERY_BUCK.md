#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/BATTERY_BUCK.md"
================================================================
12 V BATTERY + BUCK + INA219 + HYDROSTATIC SENSOR + ESP32
================================================================

   12 V SLA BATTERY
   =================
         +  (RED)  ------------------------+
                                          |
         -  (BLACK) ----------------------+------------------+
                                          |                  |
                                          |                  |
                                      [COMMON GND]          |
                                          |                 |
                                          v                 v

                  BUCK CONVERTER (12V -> 5V)
                  ==========================
         Battery +12V  --->  IN+   o------.
                                IN-  o----+-----> Battery GND
                                OUT+ o---------> ESP32 5V / VIN
                                OUT- o---------> ESP32 GND (same as battery -)

   NOTE: OUT+ is +5V, OUT- is GND


                 INA219 CURRENT SENSOR (IN-LINE ON +12V)
                 =======================================
               .-------------------.
 Battery +12V  |                   |
   (same +12V)-o VIN+         VIN- o-----------.--------> Sensor RED
               |                   |           |
               |      (shunt)      |           |
               '-------------------'           |
                         |                    |
                        GND o-----------------+-----> Battery GND
                        VCC o----------------------> ESP32 3.3V
                        SDA o----------------------> ESP32 SDA (e.g. GPIO 21, GREEN)
                        SCL o----------------------> ESP32 SCL (e.g. GPIO 22, YELLOW)


            HYDROSTATIC SENSOR (4–20 mA)
            ============================
                      +------------------------+
                      |     ALS-MPM-2F        |
                      |                        |
   From INA219 VIN- --o RED   ( +V / supply )  |
                      |                        |
   To GND ------------o BLACK (4–20mA return)  |
                      +------------------------+

   BLACK sensor wire goes **directly** back to Battery GND (and thus ESP32 GND).


                 ESP32 DEV BOARD
                 ===============
        +------------------------------------+
        |                                    |
        |  5V / VIN  o<----------------------'  from Buck OUT+ (5V)
        |  3.3V      o<------------------------- to INA219 VCC
        |  GND       o<--------------.---------- from Battery GND / Buck OUT-
        |  GPIO21    o<--------------'---------> INA219 SDA (GREEN)
        |  GPIO22    o<-----------------------> INA219 SCL (YELLOW)
        |                                    |
        +------------------------------------+


SHARED GND NODE (VERY IMPORTANT)
================================
Battery - (BLACK)
   -> INA219 GND
   -> Sensor BLACK wire
   -> Buck IN-
   -> Buck OUT-
   -> ESP32 GND
All of these MUST be the same electrical point.

