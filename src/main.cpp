#include <Arduino.h>
#include <ArduinoOTA.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <config.h>  // change configExample.h for environment variables
#define led 13
#define btn 0
#define relay 12

const char* host = HOST_NAME;
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const IPAddress ip(SERVER_IP);
const uint16_t port = SERVER_PORT;
const char* statusTopic = STATUS_TOPIC;
const char* relayTopic = RELAY_TOPIC;

const char* OTApassword = OTA_PASSWORD;

AsyncMqttClient client;
Ticker mqttReconnectTimer;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;
Ticker ledTicker;

void ledTick() {
    digitalWrite(led, !digitalRead(led));
}

unsigned long btnTimeout = 0;
bool unpressed = true;
void btnCheck() {
    if (digitalRead(btn) == 0 && unpressed && millis() - btnTimeout > 100UL) {
        if (digitalRead(relay) == HIGH) {
            digitalWrite(relay, LOW);
            if (client.connected()) client.publish(relayTopic, 1, true, "0");
        } else {
            digitalWrite(relay, HIGH);
            if (client.connected()) client.publish(relayTopic, 1, true, "1");
        }
        unpressed = false;
    }
    if (digitalRead(btn) == 1 && !unpressed) {
        btnTimeout = millis();
        unpressed = true;
    }
}

// Async Mqtt Client part
void connectToWifi() {
    WiFi.begin(ssid, password);
}

void connectToMqtt() {
    client.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
    ledTicker.detach();
    digitalWrite(led, HIGH);
    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
    mqttReconnectTimer.detach();
    wifiReconnectTimer.once(2, connectToWifi);
}

bool boot = false;
void onMqttConnect(bool sessionPresent) {
    digitalWrite(led, LOW);
    if (!boot) {
        client.publish(statusTopic, 1, true, "connected");
        boot = true;
    } else
        client.publish(statusTopic, 1, true, "reconnected");
    client.subscribe(relayTopic, 1);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    digitalWrite(led, HIGH);
    if (WiFi.isConnected()) {
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    if (!strncmp(payload, "1", len)) {
        digitalWrite(relay, HIGH);
    }
    if (!strncmp(payload, "0", len)) {
        digitalWrite(relay, LOW);
    }
}

void setup() {
    pinMode(led, OUTPUT);
    pinMode(relay, OUTPUT);
    pinMode(btn, INPUT);

    WiFi.hostname(host);
    WiFi.mode(WIFI_STA);

    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    client.onConnect(onMqttConnect);
    client.onDisconnect(onMqttDisconnect);
    client.onMessage(onMqttMessage);
    client.setWill(statusTopic, 1, true, "disconnected");
    client.setClientId(host);
    client.setServer(ip, port);

    ledTicker.attach(0.1, ledTick);

    connectToWifi();

    ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });
    ArduinoOTA.setHostname(host);
    ArduinoOTA.setPassword(OTApassword);
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();
    btnCheck();
}
