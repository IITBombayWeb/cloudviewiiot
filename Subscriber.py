import paho.mqtt.client as mqtt
import json
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
from datetime import datetime, timezone

# MQTT broker details
mqtt_broker = "192.168.43.94"
mqtt_port = 1883
mqtt_topic = "temperature_pressure"

# InfluxDB configuration
token = "YGTpsE72XOkG9hIIv6sKY3_7msn3967HqTyiWIUd5PjwUx41MiVs4wdQvw7nY7cMv2kUpHfNKP4ildo7A9qUfw=="
org = "IITB"
url = "http://192.168.43.94:8086"
bucket = "IoT endgame"

# Initialize InfluxDB client
influx_client = InfluxDBClient(url=url, token=token, org=org)
write_api = influx_client.write_api(write_options=SYNCHRONOUS)

# Callback when the client receives a CONNACK response from the server
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    # Subscribing to the topic
    client.subscribe(mqtt_topic)

# Callback when a PUBLISH message is received from the server
def on_message(client, userdata, msg):
    print(f"Message received from topic {msg.topic}: {msg.payload.decode()}")
    try:
        # Parse the JSON payload
        data = json.loads(msg.payload.decode())
        temperature = data.get("Temperature")
        pressure = data.get("Pressure")
        client_id = data.get("id")  # Extract client_id from the payload if included
        timestamp = data.get("Time")  # Extract timestamp from the payload if included

        # Handle missing or invalid values by setting to -10
        try:
            temperature = float(temperature)
        except (TypeError, ValueError):
            temperature = -10

        try:
            pressure = float(pressure)
        except (TypeError, ValueError):
            pressure = -10

        print(f"Temperature: {temperature}")
        print(f"Pressure: {pressure}")
        if client_id:
            print(f"Received from client: {client_id}")
        if timestamp:
            print(f"Timestamp: {timestamp}")
            print(type(timestamp))

        # Write to InfluxDB with different measurements based on client_id
        if client_id == 1:
            measurement = "esp_1"
        elif client_id == 2:
            measurement = "esp_2"
        else:
            measurement = "unknown"

        point = Point(measurement) \
            .tag("client_id", client_id) \
            .field("temperature", temperature) \
            .field("pressure", pressure) \
            .field("Time", timestamp)

        print(f"Writing point: {point}")
        write_api.write(bucket=bucket, org=org, record=point)
        print("Data written to InfluxDB successfully.")

    except json.JSONDecodeError as e:
        print(f"Failed to decode JSON: {e}")
    except Exception as e:
        print(f"An error occurred: {e}")

# Create an MQTT client instance
client = mqtt.Client()

# Assign event callbacks
client.on_connect = on_connect
client.on_message = on_message

# Connect to the MQTT broker
client.connect(mqtt_broker, mqtt_port, 60)

# Blocking call that processes network traffic, dispatches callbacks, and
# handles reconnecting. Other loop*() functions are available for threaded
# or manual use.
client.loop_forever()
