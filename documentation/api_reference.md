# API Reference
For example code showing API details, please take a peek at ```examples/basicUsage/basicUsage.ino``` in this directory. 
Typical use can be decomposed into three steps:

#### Step 0: Include the library files

```cpp
    #include "MQTTManager.h"
```

Instantiate MQTTManager globally (before void setup()):

```cpp
    MqttMailingService mqttMailman;
```
Failure to do so globally will likely lead to the instances being destroyed unsafely due to their scope being 
terminated, crashing the board.

#### Step 1: establish Wi-Fi connection

The following function call will establish Wi-Fi connectivity.

```cpp
    WiFi.begin(ssid, password);
    while (WiFi.isConnected()) {
        Serial.println("Waiting on the Wi-Fi connection");
        sleep(1);
    }
    Serial.println("Connected to Wi-Fi AP !");
```

Please note that the Wi-Fi will not automatically re-connect if the connection is lost.
It is therefore advised to regularly check if the Wi-Fi is still connected and
try to re-connect if the connection was lost.

#### Step 2: startup the MQTT client

Once the initial Wi-Fi connection is established, you can configure the broker and start the MQTT mailmain.

```cpp
  mqttMailman.setBrokerURI(broker_uri);
  mqttMailman.start();
```

#### Step 3: Feed data to be sent to the MQTT broker

The MQTT client sends messages to the broker through a freeRTOS queue, which itself is fed by the user. 
The queue can be obtained through its getter method ```getMailbox()```, and accepts ```DataEvents``` type objects, 
as defined in ```events_source.h```.
The code to send a simple string to the broker could look as such:

```cpp
    mailbox = mqttMailman.getMailbox();

    MQTTMessage mail = {
        .topic = "myTopic/",
        .payload = "Hello World",
    };

    xQueueSend(mailbox, &mail, 0);
```

Note that for string type payloads, the length of the message is automatically determined by the MQTT client if the 
corresponding field is left to its default value, 0. For other kinds of data, one must cast it to the ```char *``` type 
and specify the number of bytes in the field ```.mqttMsg.len```.
Consider that the queue may fill up faster than the client can dispatch the messages.

The MQTT client checks the queue for new entries every 100 milliseconds, and attempts to send the messages.

### Options

#### Client Options
A Last Will & Testament (LWT) message and topic, as well as the Quality-of-Service (QOS) and Retain Flag to be used for 
sending messages, may be specified for the MQTT client. Note that the LWT must be configured before 
calling ```start()``` or ```startWithDelegatedWiFi()```.

#### SSL

To connect to mqtt over ssl, update the following configurations:

* Use the `mqtts` protocol and `8883` port (default ssl port). This can be changed in mqtt_config.json for PlatformIO, 
  in the mqtt_cfg.h file or through the `setBrokerURI` API.
* Configure the ssl root certificate. The certificate can be configured in the mqtt_config.json or by using 
  the `MQTT_BROKER_CERTIFICATE_OVERRIDE`environment variable. In those cases do not include the `BEGIN/END CERTIFICATE`
  headers.\
  If configured in the `mqtt_cfg.h` make sure to also enable ssl with `MQTT_USE_SSL_OVERRIDE`.
  The ssl certificate can also be configured through the `setSslCertificate` API. For this API, you have to include 
  the certificate headers.\
  When a certificate is configured through the API, ssl is automatically enabled.
