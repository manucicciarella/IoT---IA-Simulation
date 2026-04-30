#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// =========================
// Pines
// =========================
#define DHTPIN 4
#define DHTTYPE DHT22
#define LDR_PIN 34
#define LED_PIN 2
#define SOIL_PIN 35

// =========================
// WiFi y MQTT
// =========================
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

const char* topic_sensor = "cursoiot/Agus/sensor";
const char* topic_comando = "cursoiot/Agus/comando";

// =========================
// Objetos globales
// =========================
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// =========================
// Estado
// =========================
String estadoSistema = "INICIANDO";
String ultimaParcela = "Parcela 1";

unsigned long lastPublish = 0;
const unsigned long publishInterval = 5000;

float ultimaTemp = 25.0;
float ultimaHum = 50.0;

// =========================
// Utilidades
// =========================
void mostrarLCD(String linea1, String linea2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(linea2.substring(0, 16));
}

int leerHumedadSuelo() {
  int raw = analogRead(SOIL_PIN); // 0..4095
  int porcentaje = map(raw, 0, 4095, 0, 100);
  return constrain(porcentaje, 0, 100);
}


int leerLuzPorcentaje() {
  int raw = analogRead(LDR_PIN);
  int valor = map(raw, 0, 4095, 0, 100);
  return constrain(valor, 0, 100);
}

// =========================
// Callback MQTT
// =========================
void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";

  for (unsigned int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }

  Serial.println("=== MENSAJE RECIBIDO ===");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(mensaje);

  if (mensaje == "ALERTA") {
    estadoSistema = "ALERTA";
    digitalWrite(LED_PIN, HIGH);
  } else if (mensaje == "SISTEMA OK") {
    estadoSistema = "NORMAL";
    digitalWrite(LED_PIN, LOW);
  } else {
    estadoSistema = mensaje;
  }

  mostrarLCD(ultimaParcela, estadoSistema);
  Serial.println("========================");
}

// =========================
// WiFi
// =========================
void conectarWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =========================
// MQTT
// =========================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT... ");

    String clientId = "ESP32_Agus_" + String(random(1000, 9999));

    if (client.connect(clientId.c_str())) {
      Serial.println("conectado");
      client.subscribe(topic_comando);
      Serial.print("Suscripto a: ");
      Serial.println(topic_comando);
    } else {
      Serial.print("fallo rc=");
      Serial.print(client.state());
      Serial.println(" reintento en 2 seg");
      delay(2000);
    }
  }
}

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  mostrarLCD("Iniciando...", "ESP32 + MQTT");

  dht.begin();
  delay(2000);

  randomSeed(micros());

  conectarWiFi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("Sistema listo");
}

// =========================
// Loop
// =========================
void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= publishInterval) {
    lastPublish = now;

    int numParcela = random(1, 5);
    ultimaParcela = "Parcela " + String(numParcela);

    // Lectura DHT
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int humedadSuelo = leerHumedadSuelo();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("Error leyendo DHT22, uso respaldo");
      temp = ultimaTemp;
      hum = ultimaHum;
    } else {
      ultimaTemp = temp;
      ultimaHum = hum;
    }

    // Lectura luz
    int lux = leerLuzPorcentaje();

    // JSON
    String payload = "{";
    payload += "\"parcela\":\"" + ultimaParcela + "\",";
    payload += "\"temp\":" + String(temp, 1) + ",";
    payload += "\"hum\":" + String(hum, 1) + ",";
    payload += "\"soil\":" + String(humedadSuelo) + ",";
    payload += "\"lux\":" + String(lux);
    
    payload += "}";

    bool ok = client.publish(topic_sensor, payload.c_str());

    Serial.println("=== TELEMETRIA ===");
    Serial.println(payload);
    Serial.print("Publish: ");
    Serial.println(ok ? "OK" : "FAIL");
    Serial.print("Estado: ");
    Serial.println(estadoSistema);
    Serial.println("==================");

    String l1 = "T:" + String(temp, 1) + " H:" + String(hum, 0);
    String l2 = "S:" + String(humedadSuelo) + "% R:" + String(lux);
    mostrarLCD(l1, l2);

    delay(2000);

    mostrarLCD(ultimaParcela, estadoSistema);
  }
}