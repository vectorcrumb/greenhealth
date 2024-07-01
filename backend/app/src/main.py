import paho.mqtt.client as paho
import json
import traceback
import time
from typing import Any, Dict
import logging

from utils import message_validator
from mqtt_pub import MQTTPublisher, ThreadEnd
from definitions import *
from devices.device_manager import Manager
from devices.device import DeviceType, DeviceFactory

logging.basicConfig(level=logging.DEBUG)

client = paho.Client(paho.CallbackAPIVersion.VERSION2, client_id="master", protocol=paho.MQTTv311)
client.username_pw_set("device", "goodlife")
client.connect("localhost", 1883, keepalive=30)
device_manager = Manager(client)

def on_sub(client: paho.Client, userdata: Any, mid: int, reason_code_list, properties):
    print(f"SUB: Client {client} got data: {userdata} with mid: {mid} and reasons: {reason_code_list}")

def on_msg(client: paho.Client, userdata: Any, message: paho.MQTTMessage):
    logging.debug(f"MSG: Client {client} got data: {userdata} with message: {message}")
    # Requires Python 3.10!
    match message.topic:
        case TOPIC_DISCOVERY:
            try:
                msg: Dict[str, str] = json.loads(message.payload)
                if not message_validator(msg, TOPIC_DISCOVERY):
                    raise ValueError(f"JSON msg on '{TOPIC_DISCOVERY}' contains incorrect keys: {msg.keys()}")
                device_type = DeviceType[msg['type'].upper()]
                device = DeviceFactory.create_device(device_type)
                device_manager.append(msg['id'], device)
            except json.JSONDecodeError:
                logging.error(f"JSON parsing of msg on '{TOPIC_DISCOVERY}' failed. Message contents: {message}")
                traceback.print_exc()
            except KeyError:
                logging.error(f"Discovery msg type is invalid: {msg['type']}")
                traceback.print_exc()

client.on_subscribe = on_sub
client.on_message = on_msg

publisher_thread = MQTTPublisher(client)
publisher_thread.start()

client.loop_start()
client.subscribe(TOPIC_DISCOVERY, qos=1)

try:
    while True:
        time.sleep(0.01)
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()

publisher_thread.teardown()
publisher_thread.join()
