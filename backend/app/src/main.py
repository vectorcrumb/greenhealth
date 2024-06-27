import paho.mqtt.client as paho
import json
import time
from typing import Any
from mqtt_pub import MQTTPublisher, ThreadEnd

client = paho.Client(paho.CallbackAPIVersion.VERSION2, client="id", protocol=paho.MQTTv5)
client.connect("localhost", 1883, keepalive=30)
client.publish("debug", "Master connected", qos=0)

def on_sub(client: paho.Client, userdata: Any, mid: int, reason_code_list, properties):
    print(f"SUB: Client {client} got data: {userdata} with mid: {mid} and reasons: {reason_code_list}")

def on_msg(client: paho.Client, userdata: Any, msg: paho.MQTTMessage):
    print(f"MSG: Client {client} got data: {userdata} with message: {msg}")

client.on_subscribe = on_sub
client.on_message = on_msg
client.subscribe("connected", qos=1)

publisher_thread = MQTTPublisher(client)
publisher_thread.start()

client.loop_forever()

publisher_thread.teardown()
publisher_thread.join()