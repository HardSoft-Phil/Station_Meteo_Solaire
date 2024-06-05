#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

//=======================================================
// Voir ESP32_MQTT-Raspi_06-02-2024.ino
//=======================================================
//mqtt data send
long lastMsg = 0;
char msg[50];
int valeur = 0;

//=======================================================
void init_NodeRed ()
{
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);
}

//=======================================================================
// callback
//=======================================================================
void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
}

//=======================================================================
//  SendDataMQTT: send MQTT data to broker with 'retain' flag set to TRUE
//=======================================================================
void SendDataMQTT (struct sensorData *environment)
{
  char buffer[8];

  while (!client.connected()) {
    if (client.connect("ESP8266Client")) client.subscribe("esp32/output");
  }
  client.loop();

  // Convertit la valeur en chaine de caractères
  dtostrf(environment->temperature, 1, 2, buffer);
  client.publish("esp32/temperature", buffer);
  buffer[0] = '\0';
  dtostrf(environment->humidity, 1, 2, buffer);
  client.publish("esp32/humidity", buffer);
  buffer[0] = '\0';
  dtostrf(environment->barometricPressure, 1, 2, buffer);
  client.publish("esp32/pressure", buffer);
}

//=======================================================================
