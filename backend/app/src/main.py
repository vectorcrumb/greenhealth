import paho.mqtt.client as paho
import json
import traceback
import time
from typing import Any, Dict
import logging

from utils import message_validator
from comms.mqtt_pub import MQTTPublisher, ThreadEnd
from definitions import TOPIC_DISCOVERY
from devices.device_manager import Manager
from devices.device import DeviceType
from devices.device_manager import create_device

logging.basicConfig(level=logging.DEBUG)


client = paho.Client(
    paho.CallbackAPIVersion.VERSION2, client_id="master", protocol=paho.MQTTv311
)
client.username_pw_set("device", "goodlife")
client.connect("localhost", 1883, keepalive=30)
device_manager = Manager(client)
publisher_thread = MQTTPublisher(client)


def on_sub(client: paho.Client, userdata: Any, mid: int, reason_code_list, properties):
    print(
        f"SUB: Client {client._client_id} got data: {userdata} with mid: {mid} and props: {properties}"
    )


def on_msg(client: paho.Client, userdata: Any, message: paho.MQTTMessage):
    logging.debug(
        f"MSG: Client {client._client_id} got data: {userdata} with message: {message.payload}"
    )
    match message.topic:
        case TOPIC_DISCOVERY:
            try:
                msg: Dict[str, str] = json.loads(message.payload)
                logging.debug(f"Got discovery msg: {msg}")
                if not message_validator(msg, TOPIC_DISCOVERY):
                    raise ValueError(
                        f"JSON msg on '{TOPIC_DISCOVERY}' contains incorrect keys: {msg.keys()}"
                    )
                device_type = DeviceType[msg["type"].upper()]
                device = create_device(
                    device_type=device_type, publisher=publisher_thread, id=msg["id"]
                )
                device_manager.append(msg["id"], device)
            except json.JSONDecodeError:
                logging.error(
                    f"JSON parsing of msg on '{TOPIC_DISCOVERY}' failed. Message contents: {message}"
                )
                traceback.print_exc()
            except KeyError:
                logging.error(f"Discovery msg type is invalid: {msg['type']}")
                traceback.print_exc()


client.on_subscribe = on_sub
client.on_message = on_msg

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
