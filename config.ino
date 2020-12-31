#define CONFIG1

#ifdef CONFIG1
const int ON_OFF_PIN = 4;
const int LEVEL_PIN = 5;
const int SWITCH_PIN = 12;
const int DHT_PIN = 13;
const DHTesp::DHT_MODEL_t DHT_MODEL = DHTesp::DHT22;
const char* AREA = "living";
#endif

#ifdef CONFIG2
const int ON_OFF_PIN = 5;
const int LEVEL_PIN = 4;
const int SWITCH_PIN = 12;
const int DHT_PIN = 13;
const DHTesp::DHT_MODEL_t DHT_MODEL = DHTesp::DHT11;
const char* AREA = "bed";
#endif
