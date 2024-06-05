//=======================================================
//  readSensors: Lit tous les capteurs et le voltage batterie
//=======================================================
//Entry point for all sensor data reading
void readSensors(struct sensorData *environment)
{
  read_BME(environment);
  read_Lum(environment);
  read_WindSpeed(environment);
  read_WindDirection(environment);
  read_Battery(environment);
}

//***********************************************************
//  read_Lum: Evalue le pourcentage de luminosité ambiante
//=======================================================
void read_Lum(struct sensorData *environment)
{
  uint32_t luminosite = analogRead(LUM_PIN);
  // Résultat en pourcentage
  environment->lumiere = map(luminosite, 0, 4096, 0, 100);

  // Affichage
  Serial.printf("Luminosité: %2.0d", environment->lumiere); Serial.println((char)37);
}
/*
  //***********************************************************
  //  read_Panel: Evalue le voltage du panneau solaire
  //=======================================================
  void read_Panel(struct sensorData *environment)
  { // ATTENTION, on ne mesure que la moitié du voltage réel
  uint32_t Vpanneau = 0;
  for (int i = 0; i < 16; i++) {
    Vpanneau += analogRead(PAN_PIN);
  }
  // Panneau de 0 à 6.5 Volts => 6,5 ÷ 4095 = 0.001586914
  environment->lumiere = (Vpanneau / 16) * 0.001586914;

  // Affichage
  Serial.printf("Luminosité: %3.2f\n", environment->lumiere);
  }
*/
//***********************************************************
//  read_Battery: Evalue le voltage de la batterie
//=======================================================
void read_Battery(struct sensorData *environment)
{ // ATTENTION, on ne mesure que la moitié du voltage réel
  uint32_t Vbatt = 0;
  int steps = 5;
  for (int i = 1; i <= steps; i++)  Vbatt += analogRead(BAT_PIN);
  // 3.3 ÷ 4095 = 0.000805664 ; coeff 2.13 empirique => 0.000805664) * 2.13 = 0.001716064
  environment->batteryVoltage = (Vbatt / steps) * 0.001716064;

  // Affichage
  Serial.printf("Volt bat  : %3.2fV\n", environment->batteryVoltage);
}

//***********************************************************
//  read_BME: Lecture du capteur BME280
//=======================================================
void read_BME(struct sensorData *environment)
{
  environment->temperature = bme.readTemperature();
  environment->humidity = bme.readHumidity();
  environment->barometricPressure = bme.readPressure() / 100.0F;
  temp_tempe += environment->temperature;
  temp_humid += environment->humidity;
  temp_press += environment->barometricPressure;

  // Affichage
  Serial.printf("Température: %6.2f°\nHumidité: %6.2f"
                , environment->temperature, environment->humidity);
  Serial.println((char)37); // %
  Serial.printf("Pression atmosphérique: %6.2fhPa\n", environment->barometricPressure);
  BME_280_Sleep();
}

//=======================================================
void BME_280_Sleep()
{
  Wire.beginTransmission(0x77);
  Wire.write((uint8_t)BME280_REGISTER_CONTROL);
  Wire.write((uint8_t)0b00);
  Wire.endTransmission();
}

//***********************************************************
// Mesure de la direction du vent
//=============================================
//  read_WindDirection: Read ADC to find wind direction
//=============================================
void read_WindDirection(struct sensorData *environment)
{
  int direction = analogRead(WIND_DIR_PIN);
  environment->windDirection = map(direction, 0, 4095, 0, 359);

  // Affichage
  Serial.printf("WD: %2.0d°\n", environment->windDirection);
}

//=============================================
//  windInt: ISR pour capturer le nombre de bips
// Mesure le nombre de tours de l'anémomètre (1 bip par rotation),
// Cette routine mesure le nombre de tours (système anti-rebonds intégré)
// pour anémo à ILS ( Young pas concerné)
// windintcount est le nombre de bips mesurés.
//=============================================
void IRAM_ATTR windInt(void)
{
  unsigned long wind_time = millis();

  if (wind_time - last_wind_time > debounce)
  {
    windintcount++;
    last_wind_time = wind_time;
  }
}

//=============================================
// read_WindSpeed: Calcul de la vitesse du vent en m/s
//  Active la mesure durant 5 secondes.
//=============================================
void read_WindSpeed(struct sensorData *environment)
{
  attachInterrupt(digitalPinToInterrupt(WIND_SPD_PIN), windInt, FALLING);
  // temps pour acquerir la vitesse du vent
  initTime = millis();
  while (millis() < (initTime + 5000));
  if (windintcount <= 1) environment->windSpeed = 0;
  else environment->windSpeed = ((windintcount * WIND_RPM_TO_MPS) / 5) + offsetYoung;
  Serial.printf("WS: %um/s\n", windintcount);
  windintcount = 0;
  detachInterrupt(digitalPinToInterrupt(WIND_SPD_PIN));
  // Affichage
  Serial.printf("WS: %2.2fm/s\n", environment->windSpeed);
}

//***********************************************************
// Mesure de la pluie
//=============================================
//  rainInt: ISR to capture rain relay closure
//=============================================
void IRAM_ATTR rainInt(void)
{
  //
}

//=============================================
// rainClick: Gestion du nombre de basculements
//=============================================
void rainClick()
{
  basc++;
}

//=============================================
//  read_Rain: Calcul du niveau de pluie
//=============================================
void read_Rain(struct sensorData *environment)
{
  environment->rain += basc * auget;
}

//===========================================================
// BlinkLED: Blink BUILTIN x times
//===========================================================
void BlinkLED(int pin, uint32_t frequence, int count)
{
  for (int x = 0; x < count; x++)
  {
    if ((millis() - ledTimer) >= frequence)
    {
      ledTimer = millis();
      blinkState = !blinkState;
      digitalWrite(pin, blinkState);
    }
    digitalWrite(pin, LOW);
  }
}

//====================================================
// processSensorUpdates: Connect to WiFi, read sensors
// and record sensors at IOT destination or MQTT
//====================================================
void processSensorUpdates(void)
{
  //move rainTicks into interval container
  //  rainfall.intervalRainfall = rainTicks;
  //
  //  //move rainTicks into hourly containers
  //  addTipsToHour(rainTicks);
  //  clearRainfallHour(timeinfo.tm_hour + 1);
  rainTicks = 0;

  //Calcul des moyennes
  environment.temperature = temp_tempe / bootCount;
  environment.humidity = temp_humid / bootCount;
  environment.barometricPressure = temp_press / bootCount;

  //send sensor data to Influxdb
  if (App == "INFLUX")
  {
    init_Influx();
    SendDataInflux(&environment);
  }

  //send sensor data to MQTT
  if (App == "NodeRed")
  {
    init_NodeRed();
    SendDataMQTT(&environment);
  }
}

//=======================================================
