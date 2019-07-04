#include <Arduino.h>

#include "EEPROM.h"
#include "ESP8266WiFi.h"
#include "WiFiManager.h"
#include "PubSubClient.h"
#include "TM1637Display.h"

#define MQTT_PORT   1883

#define TM1637_PIN_CLK D3
#define TM1637_PIN_DIO D4

#define SAVEDATA_MAGIC  0x12345678

typedef struct {
    char mqtt_server[256];
    char mqtt_topic[256];
    uint32_t magic;
} savedata_t;

static char esp_id[16];
static savedata_t savedata;

static WiFiManager wifiManager;
static WiFiManagerParameter mqttServerParam("mqtt_server", "MQTT server", "", sizeof(savedata.mqtt_server));
static WiFiManagerParameter mqttTopicParam("mqtt_topic", "MQTT topic", "", sizeof(savedata.mqtt_topic));

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static TM1637Display display(TM1637_PIN_CLK, TM1637_PIN_DIO);

static void wifiManagerCallback(void)
{
    strcpy(savedata.mqtt_server, mqttServerParam.getValue());
    strcpy(savedata.mqtt_topic, mqttTopicParam.getValue());
    savedata.magic = SAVEDATA_MAGIC;

    EEPROM.put(0, savedata);
    EEPROM.commit();
}

static void mqtt_callback(const char *topic, uint8_t * payload, unsigned int length)
{
    payload[length] = 0;
    Serial.printf("%s = %s\n", topic, payload);

    // ...
}

static bool mqtt_connect(const char *id, const char *topic)
{
    bool ok = mqttClient.connected();
    if (!ok) {
        ok = mqttClient.connect(id);
        if (ok) {
            ok = mqttClient.subscribe(topic);
        }
    }

    return ok;
}

void setup(void)
{
    Serial.begin(115200);
    Serial.printf("\nESP-MQTT2SEVENSEGMENT!\n");

    // get ESP id
    sprintf(esp_id, "%08X", ESP.getChipId());
    Serial.print("ESP ID: ");
    Serial.println(esp_id);

    // connect to wifi
    Serial.println("Starting WIFI manager ...");
    wifiManager.setConfigPortalTimeout(120);
    wifiManager.addParameter(&mqttServerParam);
    wifiManager.addParameter(&mqttTopicParam);
    wifiManager.setSaveConfigCallback(wifiManagerCallback);
    wifiManager.autoConnect("ESP-MQTT");

    mqttClient.setServer(savedata.mqtt_server, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);

    display.setBrightness(7, true);
    display.clear();
}

void loop(void)
{
    mqtt_connect(esp_id, savedata.mqtt_topic);
    mqttClient.loop();
}

