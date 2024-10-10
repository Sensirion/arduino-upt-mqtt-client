# API Guide
This guide goes through the main functionalities of the library
For usage examples, please have a look at `examples/` directory.  
For a complete API reference, please have a look at the docstring available in `MqttMailingService.h`.

### Include the library files

```cpp
#include "MqttMailingService.h"
```

Instantiate MqttMailingService globally (before void setup()):

```cpp
MqttMailingService mqttMailingService;
```
Failure to do so globally will likely lead to the instances being destroyed unsafely due to their scope being 
terminated, crashing the board.

### Client start
You can start the client either with `start()` or with `startWithDelegatedWiFi(ssid, pass, shouldBeBlocking)` depending on how you want to handle Wi-Fi connectivity (see dedicated section on that topic).

#### Blocking or not blocking start ?
The method `startWithDelegatedWiFi` provides an optional argument `shouldBeBlocking` (false by default) that would make it a blocking call. 

A blocking start would wait for the WiFi and the MQTT client to connect before returning. This limits the error messages logged and ensures no message is lost during startup.  
However it means that your application will not be runnign during that time... It is up to you to decide if you want a blocking start.

If you choose to have a non-blocking start, the method `isReady()` can help you determine if the connection to the service is ready to forward messages to the MQTT broker.


> **Note**  
Advanced methods are available if you would like to pass your Wi-Fi credentials through macros or *PlatformIO* initialization scripts.

### Wi-Fi connectivity
When using the library you have 2 possible approach to handle the Wi-Fi connectivity:

#### Self managed Wi-Fi connection
In this situation, your application will manage the WiFi connection itself (see `selfManagedWifiUsage.ino` example).  
Please refer to other resources to achieve the desired behaviour (e.g. automatic reconnection).

<details closed>
  <summary>Barebone Wi-Fi connection snippet</summary>
  The following snippet is a barbone example that will establish connection:

```cpp
WiFi.begin(ssid, password);
while (WiFi.isConnected()) {
    Serial.println("Waiting on the Wi-Fi connection");
    sleep(1);
}
Serial.println("Connected to Wi-Fi AP !");
```

Please note that here the Wi-Fi will not automatically re-connect if the connection is lost.
</details>


#### Delegated Wi-Fi connection
In this situation, your application will deleagte the WiFi connection to the MQTT library (see `delegatedWifiUsage.ino` example).  
The library will automatically attempt reconnection on the configured AP when the connection breaks. 

### Configuration

#### Broker URI
The address of the MQTT broker can be configured as follows:

```cpp
const char* broker_uri = "mqtt://mqtt.someserver.com:1883";
mqttMailingService.setBrokerURI(broker_uri);
```

#### SSL connection to broker
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


#### LWT, QoS, ...
More advanced configuration options are available and can be found in the header file. The options currently implemented are: 
- Last Will & Testament (LWT) message and topic
- Quality-of-Service (QOS) 
- Retain Flag

Note that the LWT must be configured before calling starting the client.

### Create and post a new message

The MQTT client sends messages to the broker through a freeRTOS queue, which itself is fed by the user.  
The queue can be obtained through its getter method ```getMailbox()```, and accepts ```DataEvents``` type objects, 
as defined in ```events_source.h```.  

The code to send a simple string to the broker could look as such:

```cpp
mailbox = mqttMailingService.getMailbox();

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



