#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient clientDB(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensorReadings("measures");

//=======================================================
void init_Influx ()
{
  // Add tags
  sensorReadings.addTag("device", DEVICE);
//  sensorReadings.addTag("location", "dehors");
  sensorReadings.addTag("location", "dedans");
  sensorReadings.addTag("sensor", "Station");

  // Check server connection
  if (clientDB.validateConnection())
  {
    Serial.print("Connecté à InfluxDB: ");
    Serial.println(clientDB.getServerUrl());
  } else {
    Serial.print("InfluxDB connection ratée: ");
    Serial.println(clientDB.getLastErrorMessage());
  }
}

//=======================================================
void SendDataInflux (struct sensorData *environment)
{
  // Ajoute les mesures "fields" au "point"
  sensorReadings.addField("temperature", environment->temperature);
  sensorReadings.addField("humidity", environment->humidity);
  sensorReadings.addField("pressure", environment->barometricPressure);
  sensorReadings.addField("V_bat", environment->batteryVoltage);
  sensorReadings.addField("Lumi", environment->lumiere);
  sensorReadings.addField("V_Vent", environment->windSpeed);
  sensorReadings.addField("D_Vent", environment->windDirection);
  sensorReadings.addField("Pluie", environment->rain);

  // Write point into buffer
  if (clientDB.writePoint(sensorReadings))
  {
    // Affiche le message envoyé
    Serial.print("Envoi: ");
    Serial.println(clientDB.pointToLineProtocol(sensorReadings));
  }
  else
  {
    Serial.print("Envoi à InfluxDB échoué: ");
    Serial.println(clientDB.getLastErrorMessage());
  }

  // Clear fields for next usage. Tags remain the same.
  sensorReadings.clearFields();
}

//=======================================================
