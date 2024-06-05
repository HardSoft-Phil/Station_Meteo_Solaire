//===========================================================
// titreOled
//===========================================================
void titreOled()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(25, 1);
  display.print("SMS-air");
  display.setTextSize(1);
  // Trace une ligne
  display.drawLine(0, 20, 128, 20, WHITE);
  display.setCursor(1, 30);
  display.printf("Station Meteo Solaire");
  display.setCursor(1, 45);
  display.printf(VERSION);
  display.display(); // Update display
  delay(2000);
}

//===========================================================
void oledAffich()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(2, 0);
  display.printf("%02i/%02i/%i - %02iH %02imn\n"
                 , timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900
                 , timeinfo.tm_hour + 2, timeinfo.tm_min);
  // Trace une ligne pour séparer date/heure et données
  display.drawLine(0, 10, 128, 10, WHITE);
  display.setCursor(1, 15);
  display.printf("T= %4.1f", environment.temperature);
  display.print((char)247); // °
  display.print("C");
  display.setCursor(75, 15);
  display.printf("H= %4.1f", environment.humidity);
  display.print((char)37); // %
  display.setCursor(1, 27);
  display.printf("%4.1f hPa", environment.barometricPressure);
  display.setCursor(75, 27);
  display.printf("Pluie");
  display.setCursor(1, 39);
  display.printf("WS= %2.2fm/s", environment.windSpeed);
  display.setCursor(75, 39);
  display.printf("WD= %d", environment.windDirection);
  display.print((char)247); // °
  display.setCursor(1, 52);
  if (environment.batteryVoltage >= 3.30) display.printf("Bat= %1.2fV", environment.batteryVoltage);
  else  display.print("faible");
  display.setCursor(75, 52);
  display.printf("Lux= %2.0d", environment.lumiere);
  display.print((char)37); // %
  display.display();                                                                                                   // Update display
}

//===========================================================
