#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// Definição dos pinos
const int sensorUmidade = 34;
const int sensorPH = 35;
const int sensorN = 32;
const int sensorP = 33;
const int sensorK = 36;

const int servoN = 13;
const int servoP = 12;
const int servoK = 14;
const int servoAcido = 27;
const int servoAlcalino = 26;
const int servoAgua = 15;

// Servos
Servo motorN;
Servo motorP;
Servo motorK;
Servo motorAcido;
Servo motorAlcalino;
Servo motorAgua;

// Limites aceitáveis (valores reais)
const int umidadeMin = 40;    // Umidade mínima (%)
const float phIdeal = 6.5;    // pH ideal
const int nIdeal = 50;        // Nitrogênio ideal
const int pIdeal = 50;        // Fósforo ideal
const int kIdeal = 50;        // Potássio ideal

// Variáveis para leitura
int umidade;
float ph;
int n;
int p;
int k;

// Configuração WiFi e MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

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

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 1 segundos");
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  motorN.attach(servoN);
  motorP.attach(servoP);
  motorK.attach(servoK);
  motorAcido.attach(servoAcido);
  motorAlcalino.attach(servoAlcalino);
  motorAgua.attach(servoAgua);

  motorN.write(0);
  motorP.write(0);
  motorK.write(0);
  motorAcido.write(0);
  motorAlcalino.write(0);
  motorAgua.write(0);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Ler e mapear os valores dos sensores
  umidade = map(analogRead(sensorUmidade), 0, 4095, 0, 100); // Mapear para 0-100%
  ph = map(analogRead(sensorPH), 0, 4095, 4, 9);             // Mapear para 4-9
  n = map(analogRead(sensorN), 0, 4095, 0, 100);             // Mapear para 0-100
  p = map(analogRead(sensorP), 0, 4095, 0, 100);             // Mapear para 0-100
  k = map(analogRead(sensorK), 0, 4095, 0, 100);             // Mapear para 0-100

  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println("%");

  Serial.print("pH: ");
  Serial.println(ph);

  Serial.print("Nitrogênio (N): ");
  Serial.println(n);

  Serial.print("Fósforo (P): ");
  Serial.println(p);

  Serial.print("Potássio (K): ");
  Serial.println(k);

  String umidadeStr = String(umidade);
  String phStr = String(ph);
  String nStr = String(n);
  String pStr = String(p);
  String kStr = String(k);

  client.publish("planta1/umidade", umidadeStr.c_str());
  client.publish("planta1/ph", phStr.c_str());
  client.publish("planta1/n", nStr.c_str());
  client.publish("planta1/p", pStr.c_str());
  client.publish("planta1/k", kStr.c_str());

  if (umidade < umidadeMin) {
    Serial.println("Rega necessária. Conferindo parâmetros...");

    if (ph < phIdeal - 0.5) {
      Serial.println("pH está baixo. Adicionando alcalinizante.");
      motorAlcalino.write(90);
      delay(2000);
      motorAlcalino.write(0);
    } else if (ph > phIdeal + 0.5) {
      Serial.println("pH está alto. Adicionando acidificante.");
      motorAcido.write(90);
      delay(2000);
      motorAcido.write(0);
    } else {
      Serial.println("pH está dentro do intervalo ideal.");
    }

    if (n < nIdeal) {
      Serial.println("Adicionando fertilizante N.");
      motorN.write(90);
      delay(2000);
      motorN.write(0);
    }
    if (p < pIdeal) {
      Serial.println("Adicionando fertilizante P.");
      motorP.write(90);
      delay(2000);
      motorP.write(0);
    }
    if (k < kIdeal) {
      Serial.println("Adicionando fertilizante K.");
      motorK.write(90);
      delay(2000);
      motorK.write(0);
    }

    Serial.println("Liberando água...");
    motorAgua.write(90);
    delay(2000);
    motorAgua.write(0);
  } else {
    Serial.println("Umidade acima do limite mínimo. Nenhuma ação realizada.");
  }

  delay(5000);
}
