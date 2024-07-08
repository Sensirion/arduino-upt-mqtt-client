# Usage with Arduino IDE

## Installation
This library can not yet be installed easily using the Arduino Library manager. Once this is the case, appropriate instructions will be written here.

<!-- Start the [Arduino IDE](http://www.arduino.cc/en/main/software) and open the Library Manager via

    Sketch => Include Library => Manage Libraries...

Search for the `Sensririon UPT MQTT Client` library in the `Filter your search...` field and install it by clicking the `install` button.

Alternatively, the library can also be added manually. To do this, download the latest release from github as a .zip file via

    Code => Download Zip

and add it to the [Arduino IDE](http://www.arduino.cc/en/main/software) via

    Sketch => Include Library => Add .ZIP Library...

In both cases, don't forget to _install the dependencies_ listed below. Platformio should take care of this automatically, these dependencies being listed in the `platformio.ini` file. -->


## Configuration
Arduino users may directly change the definitions in ```src/mqtt_cfg.h```.

## Logging
This library was developed utilizing the [ESP logging library](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html). To see info messages, be sure to (1) select your board in the top-left dropdown menu, and (2) select the desired level of verbosity in ```Tools > Core Debug Level```.
