#include <WiFi.h>
#include <time.h>

#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <driver/rtc_io.h>

//=======================================================
// Gestion ILS pour l'anémo
#define WIND_SPD_PIN 39 // ILS de l'anémometre
// Variable utilisée pour calculer windspeed (avec ISR)
volatile unsigned long last_wind_time = 0;
volatile unsigned long initTime = 0;
volatile int windintcount = 0;
int debounce = 150; // debounce latency in ms
//#define noria 0.750 // Pour anémo Young
//WIND SPEED vs CUP WHEEL RPM
//m/s = (0.01250 x rpm) + offset
#define offsetYoung 0.2
//const float WIND_RPM_TO_MPS = 0.01250;
const float WIND_RPM_TO_MPS = 0.3769908; // conversion metres par secondes (Young)

// Gestion ILS pour le pluvio
#define RAIN_PIN 14 // ILS du pluviomètre
volatile unsigned long raintime, rainlast, raininterval, rain;
volatile int basc = 0;
#define auget 0.1 // Taille de l'auget du pluvio
const float RAIN_BUCKETS_TO_MM = 0.21896551; // multiply bucket tips by this for mm (Davis)

//=======================================================
#include <Adafruit_BME280.h>
// Déclaration du capteur BME280 connecté en I2c
Adafruit_BME280 bme;

//=======================================================
// Ecran OLED SSD1306 pour visualisation
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Déclaration du SSD1306 connecté en I2C sans pin reset
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

//=======================================================
// Définition pins
//===========================================
// Les GPIO n° 2, 4, 12 ,13, 14, 15, 25, 26 et 27 ne peuvent pas 
// être utilisées en analogique lorsque le WiFi est actif !
//  
#define WIND_DIR_PIN 32 // Potentiomètre de la girouette
#define BAT_PIN      34 // Diviseur de tension du voltage batterie
//#define PAN_PIN      35 // Diviseur de tension du panneau solaire
#define LUM_PIN      36 // Diviseur de tension de luminosité
#define LED_BUILTIN   2 // Led témoin d'activité de mesure
#define LED          12 // Led témoin d'activité wifi

//===========================================
// Custom structures
//===========================================
struct sensorData
{
int   lumiere;            // Valeur instantanée
float windSpeed;          // Valeur instantanée pour le moment
int   windDirection;      // Valeur instantanée
float rain;               // Valeur cumulée
float barometricPressure; // Valeur moyennée
float tempeSol;           // Valeur moyennée
float humidSol;           // Valeur moyennée
float temperature;        // Valeur moyennée
float humidity;           // Valeur moyennée
float batteryVoltage;     // Valeur instantanée
};
struct sensorData environment;

//rainfall is stored here for historical data uses RTC
struct rainfallData
{
  unsigned int intervalRainfall;
  unsigned int hourlyRainfall[24];
  unsigned int current60MinRainfall[12];
  unsigned int hourlyCarryover;
  unsigned int priorHour;
  unsigned int minuteCarryover;
  unsigned int priorMinute;
};

//===========================================
// RTC Memory storage
//===========================================
RTC_DATA_ATTR volatile int rainTicks = 0;
//RTC_DATA_ATTR int lastHour = 0; // Peut etre utilisé pour calculs pluie horaire
RTC_DATA_ATTR time_t nextUpdate;
RTC_DATA_ATTR struct rainfallData rainfall;
RTC_DATA_ATTR unsigned int elapsedTime = 0;

// Variables pour le calcul des moyennes
RTC_DATA_ATTR unsigned int bootCount = 0;
RTC_DATA_ATTR boolean demarrage = true;
RTC_DATA_ATTR float temp_tempe = 0;
RTC_DATA_ATTR float temp_humid = 0;
RTC_DATA_ATTR float temp_press = 0;
RTC_DATA_ATTR float temp_bat = 0;
//RTC_DATA_ATTR unsigned int temp_windS = 0;
RTC_DATA_ATTR float temp_rain = 0;

//===========================================
// Global instantiation
//===========================================
bool WiFiEnable = true; // A la mise en route pour récupérer l'heure
long rssi = 0;
 
uint32_t ledTimer = 0;
bool blinkState = false;

#define SEC 1E6         // µS dans une seconde
#define WDT_TIMEOUT  60 // Watchdog timer

// Intervalle de mesures
long PasDeTemps = 30; // 30 secondes

// Intervalle d'envoi de message
uint64_t TIME_TO_SEND = 6; // 60 secondes x 5 = 5 minutes

//===========================================
// ISR Prototypes
//===========================================
void IRAM_ATTR rainTick(void);
void IRAM_ATTR windTick(void);

//=============================================================
//Variables for wifi server setup and api keys for IOT
//=============================================================

//===========================================
//WiFi connection
//===========================================
// Identifiants de connection pour le wifi
// WiFi AP SSID
#define WIFI_SSID "YOUR_SSID"
// WiFi password
#define WIFI_PASSWORD "YOUR_PASS"

//===========================================
// MQTT broker pour NodeRed
//===========================================
// Identifiants de connection pour NodeRed
const char* mqtt_server = "192.168.1.xxx";
const int mqttPort = 1883;
const char* mqttUser = "PASS";
const char* mqttPassword = "n96sbq2s53qxv234q7tf5t";
const char mainTopic[20] = "MainTopic/";

//===========================================
// Influxbd2 connection
//===========================================
#define INFLUXDB_URL "http://192.168.1.xxx:8086" // Raspi4
// Identifiants de connection pour la base de données "meteo"
#define INFLUXDB_BUCKET "meteo"
#define DEVICE "SMS-air" // ESP32 Devkit C v4
#define INFLUXDB_TOKEN "ArH-fsVUAGU-nOC6G93JJLgNHhGDEmSQRYrSYPa9zQDbprbUsHQrxhNVlfnaC4Y7tRPBkTRILLAM74ndKEgUTA=="
#define INFLUXDB_ORG "7b66dd3e945d7f31"

//===========================================
//Anemometer Calibration
//===========================================
// 2 switch pulls to GND per revolution.
#define WIND_TICKS_PER_REVOLUTION 1

//===========================================
//General defines
//===========================================
#define RSSI_INVALID -9999

//===========================================
// Timezone information
//===========================================
const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";  // FR
time_t now;
struct tm timeinfo;

//===================== Validation Influxdb2, MQTT ou rien =====================
//const String App = "NULL";         // Pas de connection au Raspberry pour tests
const String App = "INFLUX";       //  Connection à influxdb2 sur Raspberry
//const String App = "NodeRed";     //  Connection à NodeRed sur Raspberry
