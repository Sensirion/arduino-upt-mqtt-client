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
const char* brokerDomain = "mqtt.someserver.com";
mqttMailingService.setBroker(brokerDomain, false);
```

If you need control over the port, use the following API where you can pass the full broker URI:

```cpp
const char* brokerUri = "mqtt://mqtt.someserver.com:1883";
mqttMailingService.setBrokerURI(brokerUri);
```

#### SSL connection to broker
To connect to mqtt over ssl, update the following configurations:

* Use the `mqtts` protocol and `8883` port (default ssl port). This can be changed through the `setBroker` or `setBrokerURI` API.

  ```cpp
  const char* brokerDomain = "mqtt.someserver.com";
  mqttMailingService.setBroker(brokerDomain, true);
  ```

* Configure the ssl root certificate. The certificate can be configured by using 
  the `MQTT_BROKER_CERTIFICATE_OVERRIDE` environment variable. In this case, do not include the `BEGIN/END CERTIFICATE`
  headers. Enable SSL by using the `enableSsl` API.\
  Alternative: The ssl certificate can also be configured through the `setSslCertificate` API. For this API, you have to include 
  the certificate headers. This API call automatically enableds SSL. \


#### LWT, QoS, ...
More advanced configuration options are available and can be found in the API. The options currently implemented are: 
- Last Will & Testament (LWT) message and topic
- MQTT Event Loop Size
- Quality-of-Service (QOS) 
- Retain Flag

Note that those API calls must be perfomed before starting the client.

#### Measurement formatting
The library lets you define the function used to convert a Measurement object into a message.  
You can do so using `setMeasurementMessageFormatterFn()`

A sample formatting function is available in the file `MeasurementFormatting.cpp`.
You can either choose to use the provided one or define your own.

This configuration step is **required** if you want to send Measurement objects.

#### Measurement to topic
In some cases it can be useful to dynamically define the topic based on the metadata of a Measurement.

The library lets you register a function to define the topic based on the given Measurement.  
You can do so using `setMeasurementToTopicSuffixFn()`. Once configured you are no longer required to provide a topic when using `sendMeasurement`.

A sample function is available in the file `MeasurementFormatting.cpp`.
You can either choose to use the provided one or define your own.


### Send a text message
Once configured and connected, a message can be sent using `sendTextMessage`:

```cpp
mqttMailingService.sendTextMessage("message", "topic/");
```

### Send a Measurement
Once the formatting function configured (see "Measurement formatting"), a `Measurement` can be sent using `sendMeasurement`:

```cpp
MqttMailingService.sendMeasurement(myMeasurement, "topic/");
```

Optionally, a function can be registered to define the topic from the included metadata (see section "Measurement to topic"). Once done the topic can be omitted:

```cpp
MqttMailingService.sendMeasurement(myMeasurement);
```