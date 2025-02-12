# Sensirion UPT MQTT Client

Arduino library for publishing UPT `Measurements` to an MQTT broker.   
It offers conveninence method to publish UPT Measurements while simplifying the interface to the ESP-IDF mqtt_client library.
Optionally it can handle Wi-Fi connectivity.

> It is not yet available through library managers.


## Getting started

### Recommended Hardware

This project was developed and tested on Espressif [ESP32 DevKitC](https://www.espressif.com/en/products/devkits/esp32-devkitc) and Lilygo [T-Display S3](https://www.lilygo.cc/products/t-display-s3) hardware.

### IDE

#### PlatformIO (recommended)
We recommend using the [PlatformIO VSCode extension](https://platformio.org/platformio-ide) to compile and flash the code in this example. 
You will find more instructions [here](documentation/platformio_usage.md)

#### Arduino IDE
It is also possible to use the application using *Arduino IDE*.
You will find more instructions [here](documentation/arduino_ide_usage.md)

### Usage examples
Two example scripts are available in the `examples` folder:
- *delegatedWifiUsage*: In this example the main application delegates the WiFi management to the MQTT client. Such approach should be used if your application does no use Wi-Fi overwise and you do not want any fancy Wi-Fi configuration.

- *selfManagedWifiUsage*: In this example the main application will handle the WiFi management, and the MQTT client will not care about it. Such approach should be used in most cases since your application will likely use WiFi for other things.


### API reference
You will find a more detailed API guide [here](documentation/api_guide.md)

![overview_schema](documentation/SchemaUPT_MQTT.png)

## Dependencies

This library uses the following dependencies.

* [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html#about-arduino-esp32)
(In particular, the [Wi-Fi library](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html),
[message logging](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html) and
[MQTT client](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html#api-reference)) \
* [Sensirion UPT Core](https://github.com/Sensirion/arduino-upt-core)
