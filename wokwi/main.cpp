#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <LiquidCrystal_I2C.h>

// 1. Configuraciones y Pines
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
const char* topic_pub = "cursoiot/grupo3/sensor";
const char* topic_sub = "cursoiot/grupo3/comandos";

const int DHT_PIN = 32;
const int LDR_PIN = 34;
const int POT_SUELO = 35;
const int POT_VIENTO = 33;
const int RELAY1 = 14;
const int RELAY2 = 13;
const int LED_ROJO = 5;
const int LED_VERDE = 27;
const int DIP[] = {4, 16, 17, 18, 19, 23, 25, 26};

DHTesp dhtSensor;
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 20, 4);


unsigned long lastMsg = 0;
unsigned long ledVerdeTimer = 0;
bool alertaCritica = false;
String ultimoComando = "ESPERANDO";

String obtenerCultivo(int binario) {
  switch (binario) {
    case 1: return "Soja";      
    case 2: return "Maiz";      
    case 3: return "Trigo";     
    case 4: return "Girasol";   
    case 5: return "Cebada";    
    case 6: return "Sorgo";     
    case 7: return "Alfalfa";   
    default: return "Ninguno";  
  }
  // --- PRUEBA ---
  digitalWrite(LED_ROJO, LOW);   // Enciende
  digitalWrite(LED_VERDE, LOW);  // Enciende
  delay(1000);                   // Espera 1 segundo
  digitalWrite(LED_ROJO, HIGH);  // Apaga
  digitalWrite(LED_VERDE, HIGH); // Apaga
  //
  
}



void callback(char* topic, byte* payload, unsigned int length) {
  ultimoComando = "";
  for (int i = 0; i < length; i++) ultimoComando += (char)payload[i];
  Serial.println("Recibido MQTT: " + ultimoComando);
  
  lcd.setCursor(0, 2);
  lcd.print("CMD:                "); 
  lcd.setCursor(5, 2);
  lcd.print(ultimoComando);

  if (ultimoComando == "ALERTA CRÍTICA") {
    alertaCritica = true;
  } else if (ultimoComando == "MONITOREAR" || ultimoComando == "FUMIGAR") {
    digitalWrite(RELAY1, HIGH);
    alertaCritica = false;
  } else if (ultimoComando == "POSTERGAR") {
    digitalWrite(RELAY1, LOW);
    alertaCritica = false;
  } else if (ultimoComando == "REGAR") {
    digitalWrite(RELAY2, HIGH);
    alertaCritica = false;
  } else if (ultimoComando == "INSPECCIONAR") {
    alertaCritica = false;
  } else if (ultimoComando == "SISTEMA OK") {
    alertaCritica = false;
    digitalWrite(LED_ROJO, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0)); 
  
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  lcd.init();
  lcd.backlight();
  
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  for(int i=0; i<8; i++) pinMode(DIP[i], INPUT_PULLUP);

  lcd.setCursor(0, 0);
  lcd.print("Iniciando WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(" ¡Conectado!");
  lcd.clear();
  lcd.print("WiFi: CONECTADO");
  delay(1000);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    String clientId = "ESP32Weber-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" ¡Conectado!");
      client.subscribe(topic_sub);
    } else {
      Serial.print(" falló, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // 1. LECTURA DE SENSORES EN TIEMPO REAL
  int hSuelo = map(analogRead(POT_SUELO), 0, 4095, 0, 100);
  int vViento = map(analogRead(POT_VIENTO), 0, 4095, 0, 120);
  int radSolar = analogRead(LDR_PIN);

  // 2. LECTURA DIP SWITCH
  int valBin = (!digitalRead(DIP[0]) << 2) | (!digitalRead(DIP[1]) << 1) | !digitalRead(DIP[2]);
  String cultivoNombre = obtenerCultivo(valBin);
  String p_ns = (!digitalRead(DIP[3])) ? "N" : "S";
  String p_eo = (!digitalRead(DIP[4])) ? "E" : "O";
  int nLote = (!digitalRead(DIP[5]) << 2) | (!digitalRead(DIP[6]) << 1) | !digitalRead(DIP[7]);
  String parcelaStr = p_ns + p_eo + "-L" + String(nLote);

  // 3. LÓGICA DE CONTROL AUTOMÁTICO DE RIEGO (Corte al 60%)
  if (hSuelo >= 60 && digitalRead(RELAY2) == HIGH) {
    digitalWrite(RELAY2, LOW);
    Serial.println("Humedad alcanzada (60%). Riego finalizado.");
    ultimoComando = "REGADO OK";
    // Actualización inmediata del LCD para el comando
    lcd.setCursor(0, 2);
    lcd.print("CMD: REGADO OK      ");
  }

  // 4. ACTUALIZACIÓN LCD (Líneas permanentes)
  lcd.setCursor(0, 0);
  lcd.print("CULT: " + cultivoNombre + "       ");
  lcd.setCursor(0, 1);
  lcd.print("PARC: " + parcelaStr + "       ");

  // 5. GESTIÓN DE LEDS
  if (alertaCritica) {
    digitalWrite(LED_ROJO, (millis() / 200) % 2);
  } else {
    digitalWrite(LED_ROJO, LOW);
  }

  if (millis() < ledVerdeTimer) {
    digitalWrite(LED_VERDE, (millis() / 250) % 2);
  } else {
    digitalWrite(LED_VERDE, LOW);
  }

  // 6. ENVÍO MQTT CADA 10 SEGUNDOS
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    TempAndHumidity data = dhtSensor.getTempAndHumidity();

   String payload = "{\"temp\":" + String(data.temperature, 1) + 
                 ",\"hum\":" + String(data.humidity, 1) + 
                 ",\"suelo\":" + String(hSuelo) + 
                 ",\"viento\":" + String(vViento) + 
                 ",\"rad\":" + String(radSolar) + 
                 ",\"cultivo\":\"" + cultivoNombre + "\"" + 
                 ",\"parcela\":\"" + parcelaStr + "\"" + 
                 ",\"token_seguro\":\"AGRO\"}";

    if (client.publish(topic_pub, payload.c_str())) {
      Serial.println("Enviado: " + payload);
      ledVerdeTimer = millis() + 5000;
    }
  }
}