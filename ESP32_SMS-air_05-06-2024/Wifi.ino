//=======================================================================
//  wifi_connect: Connection au WiFi et récupération de l'heure
//=======================================================================
long wifi_connect()
{
  long wifi_signal = 0;

  delay(10);
  //Connection au réseau WiFi:
  Serial.println();
  Serial.print("Connection à ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  digitalWrite(LED, HIGH);
  Serial.println("\nWiFi connecté");
  Serial.print("addresse IP: ");
  Serial.println(WiFi.localIP());
  wifi_signal = WiFi.RSSI();

  // Now configure time services
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", Timezone, 1); // See below for other time zones
  delay(1000); // Wait for time services
  digitalWrite(LED, LOW);
  return wifi_signal;
  WiFi.disconnect();
}

//=======================================================================
