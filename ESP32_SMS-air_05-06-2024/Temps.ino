//===========================================================
//  printTimeNextWake: diagnostic routine to print next wake time
//===========================================================
void printTimeNextWake(void)
{
  getLocalTime(&timeinfo);
  display.printf("Prochain réveil dans: %i secondes\n", nextUpdate - mktime(&timeinfo) );
}

//===========================================================
//  updateWake: calculate next time to wake
//===========================================================
void updateWake (void)
{
  time(&now);
  nextUpdate = PasDeTemps - now % PasDeTemps;
}

//===========================================================
// wakeup_reason: action based on WAKE reason
// 1. Power up
// 2. WAKE on EXT0 - increment rain tip gauge count and sleep
// 3. WAKE on TIMER - send sensor data to IOT target
//===========================================================
//check for WAKE reason and respond accordingly
void wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  //Serial.printf("Wakeup reason: %d\n", wakeup_reason);
  switch (wakeup_reason)
  {
    //Rain Tip Gauge
    case ESP_SLEEP_WAKEUP_EXT0 :
      Serial.printf("Réveil provoqué par un signal externe utilisant RTC_IO\n");
      WiFiEnable = false;
      rainTicks++;
      break;

    //Timer
    case ESP_SLEEP_WAKEUP_TIMER :
      Serial.printf("Réveil provoqué par le Timer\n");
      if (bootCount == TIME_TO_SEND) WiFiEnable = true;
      else WiFiEnable = false;
      // Rainfall interrupt pin set up
      //attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainInt, FALLING);
      // Wind speed interrupt pin set up
//      attachInterrupt(digitalPinToInterrupt(WIND_SPD_PIN), windInt, FALLING);
//      // temps pour acquerir la vitesse du vent
//      initTime = millis();
      break;

    //Initial boot or other default reason
    default :
      Serial.printf("Wakeup non causé par deep sleep: %d\n", wakeup_reason);
      WiFiEnable = true;
      break;
  }
}

//===========================================================
// sleepyTime: prepare for sleep and set
// timer and EXT0 WAKE events
//===========================================================
void sleepyTime(long PasDeTemps)
{
  int elapsedTime;
  Serial.println("Dodo...");

  //  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
  elapsedTime = (int)millis() / 1000;

  //subtract elapsed time to try to maintain interval
  nextUpdate -= elapsedTime;
  if (nextUpdate < 3) {
    nextUpdate = 3;
  }
  Serial.printf("Réveil dans %i secondes\n----------\n", nextUpdate);
  Serial.flush();
  Serial.end();
  esp_sleep_enable_timer_wakeup(nextUpdate * SEC);
  esp_deep_sleep_start();
}

//=======================================================================
