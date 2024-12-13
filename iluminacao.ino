#include <WiFi.h>
#include <PubSubClient.h>

#define LUMINOSITY_PIN 34 // Pino analógico conectado ao potenciômetro
#define DIMMER_PIN 25     // Pino PWM conectado ao LED dimmerizável
#define LIGHT_THRESHOLD 5000 // Limiar de luminosidade (em lux)
#define MAX_LIGHT_HOURS 12 // Máximo de horas de luz por dia

// Configuração WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Configuração do Broker MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Tópicos MQTT
const char* topic_luminosity = "planta1/luminosidade";
const char* topic_light_status = "planta1/light_status";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMillis = 0; // Marca o último registro do tempo
unsigned long lastSensorReadMillis = 0; // Tempo da última leitura do sensor
const unsigned long sensorReadInterval = 5000; // Intervalo de leitura do sensor (5 segundos)
unsigned long lightOnTime = 0; // Tempo total de luz artificial ligada (em milissegundos)
bool artificialLightActive = false; // Indica se a luz artificial está ligada

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  String clientID = "ESP32-" + String(WiFi.macAddress());
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect(clientID.c_str())) {
      Serial.println("Conectado ao MQTT!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(DIMMER_PIN, OUTPUT);
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  // Lê os sensores a cada 5 segundos
  if (currentMillis - lastSensorReadMillis >= sensorReadInterval) {
    lastSensorReadMillis = currentMillis;

    // Leitura e mapeamento do potenciômetro para luminosidade (0 a 50.000 lux)
    int luminosityRaw = analogRead(LUMINOSITY_PIN);
    float luminosityLux = map(luminosityRaw, 0, 4095, 0, 50000);

    Serial.print("Luminosidade (lux): ");
    Serial.println(luminosityLux);

    // Publica a luminosidade no MQTT
    String luminosityMessage = String(luminosityLux);
    client.publish(topic_luminosity, luminosityMessage.c_str());

    // Controle de luz artificial
    if (luminosityLux < LIGHT_THRESHOLD && lightOnTime < MAX_LIGHT_HOURS * 3600000UL) {
      artificialLightActive = true;
      float deficit = LIGHT_THRESHOLD - luminosityLux;

      // Ajuste da intensidade do LED (0-255) proporcional ao déficit de luz
      int dimmerValue = map(deficit, 0, LIGHT_THRESHOLD, 0, 255);
      analogWrite(DIMMER_PIN, constrain(dimmerValue, 0, 255));

      Serial.print("Luz artificial ativada. Intensidade: ");
      Serial.println(dimmerValue);

      // Publica o status da luz artificial no MQTT
      client.publish(topic_light_status, "ON");

      // Registrar tempo de luz artificial ativa
      lightOnTime += sensorReadInterval;
    } else {
      artificialLightActive = false;
      analogWrite(DIMMER_PIN, 0); // Desligar luz artificial
      Serial.println("Luz artificial desligada.");

      // Publica o status da luz artificial no MQTT
      client.publish(topic_light_status, "OFF");
    }

    // Reset do período de 12 horas (simulando um ciclo de dia/noite)
    if (lightOnTime >= MAX_LIGHT_HOURS * 3600000UL) {
      Serial.println("Período máximo de luz atingido. Aguardando próximo ciclo.");
    }
  }
}
