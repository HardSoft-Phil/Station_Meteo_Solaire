//###########################################################################
//                     Station Météo Solaire (SMS-air)
//                      Dédiée aux mesures de l'air
//###########################################################################
//          Petite station météo exploitant le mode deep sleep
//===========================================================================
// Inspiré de:
//  => weather station v3.0 et 4.0
//     https://www.instructables.com/Solar-Powered-WiFi-Weather-Station-V30/
//     https://github.com/jhughes1010/weather
//     https://github.com/jhughes1010/weather_v4_lora/tree/master (a explorer)
//     https://github.com/jhughes1010/weather_v4_lora_receiver
//  => ESP32_Miniature_OLED_Weather_Station_SSD1306_v01
//     https://github.com/G6EJD/ESP32-Miniature-Weather-Station/tree/master
//
//===========================================================================
// Basé sur ESP32_Meteo_solaire_proto19-05-2024 mais dédié aux mesures de l'air
//===========================================================================
//  26/05/2023 => Vitesse vent non efficace, pas de données en sortie
//---------------------------------------------------------------------------
//  31/05/2023 => Mesure du vent revue. -> Ca semble bon...
//                Durée d'activité de l'int windInt modifiée pour calcul vitesse vent
//             => Suppression de l'utilisation de LED_BUILTIN
//---------------------------------------------------------------------------
//  05/06/2024 => 
//###########################################################################

//===========================================================
// Include
//===========================================================
#include "Params.h"

#define VERSION "SMS-air du 05-06-2024"

//===========================================================
// setup:
//===========================================================
void setup()
{
  long UpdateIntervalModified = 0;

  //set hardware pins
  pinMode(WIND_SPD_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  setCpuFrequencyMhz (80);
//  rtc_gpio_init(GPIO_NUM_16); // GPIO_NUM_16 = GPIO 14 de l'ESP32
//  rtc_gpio_set_direction(GPIO_NUM_16, RTC_GPIO_MODE_OUTPUT_ONLY);
//  rtc_gpio_set_level(GPIO_NUM_16, 1);

  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); // add current thread to WDT watch

  time(&now);

  Serial.begin(115200);
  delay(100);

  // initialize with the I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setRotation(2); // Rotation à 180 degrés de l'écran
  display.setTextColor(WHITE);

  if(demarrage)
{
  // Message de présentation à l'allumage
  titreOled();
  Serial.printf("\nESP32_Solar-Weather-Station.\n");
  Serial.printf("Version %s\n", VERSION);
  Serial.printf("Station %s\n", DEVICE);
  Serial.printf("Réveil: %d\n\n", bootCount);
  demarrage = false;
}
  BlinkLED(LED_BUILTIN, 200, 2);
 
  bootCount = bootCount + 1;

  updateWake();
  wakeup_reason();

  // Initialise le BME280
  if (!bme.begin(0x76)) Serial.println("Capteur BME280 non détecté !");
  delay(100); // Delai de stabilisation du capteur

  // Lit tous les capteurs (prend un certain temps)
  readSensors(&environment);

  if (WiFiEnable)
  {
    rssi = wifi_connect();
    if (rssi != RSSI_INVALID)
    {
      processSensorUpdates();
      // RAZ mesures temporaires
      bootCount = 0; // RAZ nombre de boots
      temp_tempe = 0;
      temp_humid = 0;
      temp_press = 0;
      //temp_windS = 0;
      //temp_rain = 0;
      temp_bat = 0;
    }
  }

  getLocalTime(&timeinfo);
  oledAffich();
  UpdateIntervalModified = nextUpdate - mktime(&timeinfo);
  if (UpdateIntervalModified < 3) UpdateIntervalModified = 3;
  Serial.printf("%02i/%02i/%i - %02i:%02i:%02i\n"
                , timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900
                , timeinfo.tm_hour + 2 , timeinfo.tm_min, timeinfo.tm_sec);
  // Active le watchdog
  esp_task_wdt_reset();
  BlinkLED(LED_BUILTIN,250, 3); // => Delai de 500 ms x 3 = 1,5 seconde
  // Mise en sommeil
  sleepyTime(UpdateIntervalModified);
}

//===========================================================
// loop: these are not the droids you are looking for
//===========================================================
void loop()
{
}

//===========================================================
