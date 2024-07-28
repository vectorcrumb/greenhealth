import paho.mqtt.client as paho
import time
import json
from typing import Any
import logging
from comms.mqtt_pub import MQTTPublishable, MQTTPublisher

logging.basicConfig(level=logging.DEBUG)

MOCK_ID = "9ABF348E"

client = paho.Client(
    paho.CallbackAPIVersion.VERSION2, client_id="mock_turntable", protocol=paho.MQTTv311
)
client.username_pw_set("device", "goodlife")
client.connect("localhost", 1883, keepalive=30)


def on_msg(client: paho.Client, userdata: Any, message: paho.MQTTMessage):
    logging.debug(f"MSG: Client {client} got data: {userdata} with message: {message}")
    match message.topic:
        case ack_case if ack_case == f"{MOCK_ID}/ack":
            logging.info("Received ACK back from backend")


client.on_message = on_msg

publisher_thread = MQTTPublisher(client)
publisher_thread.start()

client.loop_start()
client.subscribe(f"{MOCK_ID}/ack", 1)
client.publish("discovery", json.dumps({"id": MOCK_ID, "type": "ROVER"}))
time.sleep(1.0)


try:
    while True:
        time.sleep(0.01)
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()

publisher_thread.teardown()
publisher_thread.join()
