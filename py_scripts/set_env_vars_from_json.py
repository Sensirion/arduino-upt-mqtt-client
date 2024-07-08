# Refer to https://docs.platformio.org/en/latest/scripting/construction_environments.html
# and 
# https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_flags.html#stringification
# for information about how this script was made.

import os
import json
Import("projenv")

# Read configuration info
config_file = "mqtt_config.json"

with open(config_file, "r") as file:
    config_data = json.load(file)

mqtt_protocol = config_data["mqtt"].get("protocol", "mqtt")
mqtt_port = config_data["mqtt"].get("port", 1883)
broker_cert = ""

use_ssl = (mqtt_protocol == "mqtts")

if use_ssl:
    mqtt_port = config_data["mqtt"].get("port", 8883)
    broker_cert = fr'{config_data["mqtt"]["broker_certificate"]}'


# append extra flags to only project build environment
projenv.Append(CPPDEFINES=[
    "PIO_ADVANCED_SCRIPTING",
    ("WIFI_SSID_OVERRIDE", projenv.StringifyMacro(f"{config_data['wifi']['ssid']}")),
    ("WIFI_PW_OVERRIDE", projenv.StringifyMacro(f"{config_data['wifi']['password']}")),
    ("MQTT_LWT_TOPIC_OVERRIDE", projenv.StringifyMacro(f"{config_data['mqtt']['lwt_topic']}")),
    ("MQTT_LWT_MSG_OVERRIDE", projenv.StringifyMacro(f"{config_data['mqtt']['lwt_msg']}")),
    ("MQTT_QOS_OVERRIDE", f"{config_data['mqtt']['qos']}"),
    ("MQTT_RETAIN_FLAG_OVERRIDE", f"{config_data['mqtt']['retain_flag']}"),
    ("MQTT_BROKER_FULL_URI_OVERRIDE", projenv.StringifyMacro(f"{mqtt_protocol}://{config_data['mqtt']['broker_domain']}:{mqtt_port}")),
    ("MQTT_USE_SSL_OVERRIDE", 1 if use_ssl else 0),
    ("MQTT_BROKER_CERTIFICATE_OVERRIDE", projenv.StringifyMacro(broker_cert))
])

# Dump global construction environment (for debug purpose)
env_dump_file = "logs/buildlog.log"
print("POST_SCRIPT: project environment dump at", env_dump_file)
os.makedirs(os.path.dirname(env_dump_file), exist_ok=True)
with open(env_dump_file, 'w') as logfile:
    logfile.write(str(projenv.Dump()))
