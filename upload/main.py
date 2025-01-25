import serial
import json
from paho.mqtt import client as mqtt_client

# Configura el puerto y velocidad en baudios
PORT = 'COM7'  # Cambia esto según tu sistema (por ejemplo, '/dev/ttyUSB0' en Linux o 'COMx' en Windows)
BAUDRATE = 9600

broker = '5.250.190.40'
port = 1883
topic = "gps/tracker"
user = "IC"
passw = "123456"
client_id = 1
# data.type = "GPS"
# data.description = {"lat":0, "lon":0}
def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
    # For paho-mqtt 2.0.0, you need to add the properties parameter.
    # def on_connect(client, userdata, flags, rc, properties):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)
    # Set Connecting Client ID
    client = mqtt_client.Client(client_id)

    # For paho-mqtt 2.0.0, you need to set callback_api_version.
    # client = mqtt_client.Client(client_id=client_id, callback_api_version=mqtt_client.CallbackAPIVersion.VERSION2)

    client.username_pw_set(user, passw)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

cliente = connect_mqtt()

try:
    # Abre el puerto serie
    with serial.Serial(PORT, BAUDRATE, timeout=1) as ser:
        print(f"Conectado al puerto {PORT} a {BAUDRATE} baudios")

        while True:
            # Lee una línea del puerto serie
            linea = ser.readline().decode('utf-8').strip()
            if linea:
                data = json.loads(linea)
                if data:
                    print(data["from"])
                    print(data["lat"])
                    print(data["lon"])
                    print(data["SNR"])
                    dataToMQTT = {
                        "type": "GPS",
                        "description": {
                            "lat": data["lat"],
                            "lon": data["lon"],
                            "id": data["from"]
                        }
                    }

                    cliente.publish(topic, json.dumps(dataToMQTT))

except serial.SerialException as e:
    print(f"Error al acceder al puerto serie: {e}")
except KeyboardInterrupt:
    print("\nConexión cerrada.")
except json.JSONDecodeError as e:
    print("Invalid JSON syntax:", e)    
