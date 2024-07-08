# API Reference
For example code showing API details, please take a peek at ```examples/basicUsage/basicUsage.ino``` in this directory. Typical use can be decomposed into three steps:

#### Step 0: Include the library files

```cpp
    #include "WiFiManager.h"
    #include "MQTTManager.h"
```

Instantiate WiFiManager and MQTTManager globally (before void setup()):

```cpp
    WiFiManager wifiMgr;
    MQTTMessageDispatcher mqttMgr;
```
Failure to do so globally will likely lead to the instances being destroyed in an unsafe manner due to their scope being terminated, crashing the board.

#### Step 1: establish wifi connection

The following function call will establish wifi connectivity
```cpp
    wifiMgr.start("mySSID", "myPassword");
```
Alternatively, using the corresponding macros in ```wifi_cfg.h``` enables using the compacter version of this API without the arguments, as seen in the example. Checkout the docstring corresponding to this function for further options.

#### Step 2: startup the MQTT client

The MQTT client idles unless internet connection via wifi was established. To accomplish this, it examines flags set in a freeRTOS event group by the wifi manager, and thus requires the handle to it in its startup routine. The following syntax may be used:

```cpp
    EventGroupHandle_t wifiEventGroup = wifiMgr.getEventGroupHandle();
    mqttMgr.start(wifiEventGroup, "mqtt://mqtt.myexampleserver.com:1883");
```
In this example, we use the version of the API in which the full URI of the MQTT broker can be specified. Again, similarly to the wifi library, a shorter verion of the API utilizing a macro set in ```wifi_cfg.h``` may be utilized. A convenience function is also provided, enabling WiFi connection and MQTT client start in a single function call.

#### Step 3: Feed data to be sent to the MQTT broker

The MQTT client sends messages to the broker which it recieves to a freeRTOS queue, which itself is fed by the user. The queue can be obtained through its getter method ```getDataEventQueueHandle()```, and accepts ```DataEvents``` type objects, as defined in ```events_source.h```. The code to send a simple string to the broker could look as such:

```cpp
    QueueHandle_t mqttPayloadQueue = mqttMgr.getDataEventQueueHandle();

    MQTTMessage mail = {
        .topic = "myTopic/",
        .payload = "Hello World",
    };

    xQueueSend(mqttPayloadQueue, &mail, 0);
```

Note that for string type payloads, the length of the message is automatically determined by the MQTT client if the corresponding filed is left to it's default value, 0. For other kinds of data, one must cast it to the ```char *``` type and specify the number of bytes in the field ```.mqttMsg.len```. Consider that the queue may fill up faster than the client can dispatch the messages.

The MQTT client checks the queue for new entries every 100 milliseconds, and attempts to send the messages.

### Options

#### Client Options
A Last Will & Testament (LWT) message and topic, as well as the Quality-of-Service (QOS) and Retain Flag to be used for sending messages may be specified for the MQTT client. Note that the LWT must be configured before calling ```start()``` or ```startWithWifiConnection()```.

#### SSL

To connect to mqtt over ssl, update the following configurations:

* Use the `mqtts` protocol and `8883` port (default ssl port). This can be changed in mqtt_config.json for PlatformIO, in the mqtt_cfg.h file or through the `setBrokerURI` API.
* Configure the ssl root certificate. The certificate can be configured in the mqtt_config.json or by using the `MQTT_BROKER_CERTIFICATE_OVERRIDE`environment variable.
  In those cases do not include the `BEGIN/END CERTIFICATE` headers. If configured in the `mqtt_cfg.h` make sure to also enable ssl with `MQTT_USE_SSL_OVERRIDE`.
  The ssl certificate can also be configured through the `setSslCertificate` API. For this API you have to include the certificate headers.
  When a certificate is configured through the API, ssl is automatically enabled.

#### Convenience API
A shortened API is available:
```cpp
    wifiMgr.start("mySSID", "myPassword");

    mqttMailman.setBrokerURI("mqtt://mymqttbroker.com:1883");
    mqttMailman.start(wifiMgr.getEventGroupHandle());
```
is equivalent to
```cpp
    mqttMailman.startWithWifiConnection("mySSID", "myPassword", "mqtt://mymqttbroker.com:1883");
```