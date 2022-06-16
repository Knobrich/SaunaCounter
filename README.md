# SaunaCounter

![](https://img.shields.io/badge/ECU-ESP32-blue)
![](https://img.shields.io/badge/temperature_sensor-DS18B20-blue)
![](https://img.shields.io/badge/state_display-traffic_light-blue)

Embedded software for an ESP32 driven device that
- calculates the power supply costs of a sauna by
  - counting how long the sauna heater is powered up (operation time)
  - multiplying the operation time with the nominal power of the heater (electrical work) and
  - multiplying the electrical work with the costs per kWh
  
- reads the actual sauna temperature via a DS18B20 sensor and

- displays the operational state via a traffic light:
  - Red:    Sauna is still too cold
  - Yellow: Sauna is almost heated up
  - Green:  Sauna is ready to be used
