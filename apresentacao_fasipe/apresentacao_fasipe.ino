/*
  This sketch is based on the basic MQTT example by
  http://knolleary.github.io/pubsubclient/
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <DHT.h>

#define LAMPADA 4
#define interruptor 2
#define LDR A1
#define DEBUG 1
#define DHTPIN 5

// Configurações dos dispositivos
IPAddress deviceIp(192, 168, 1, 43);
byte deviceMac[] = { 0xAB, 0xCD, 0xFE, 0xFE, 0xFE, 0xFE };
char* deviceId  = "SensorDaSala";
char* stateTopicLuminosidade = "home-assistant/SensorDaSala/luminosidade";
char* stateTopicTemperatura = "home-assistant/SensorDaSala/temperatura";
char* stateTopicUmidade = "home-assistant/SensorDaSala/umidade";
char* stateTopicLampada = "home-assistant/SensorDaSala/lampada";
char* commandTopicLampada = "home-assistant/SensorDaSala/lampada/command";
char buf[4]; // Buffer para armazenar o valor do sensor
int updateInterval = 300; //Intervalo de atualização
char message_buff[100];
volatile unsigned long ul_PreviousMillis = 0;
char* statusLampada;
boolean interruptorFlag;

//Configurações do servidor MQTT
IPAddress mqttServer(192, 168, 1, 1);
int mqttPort = 1883;

EthernetClient ethClient;
PubSubClient client(ethClient);

DHT dht;

void reconnect() {
  while (!client.connected()) {
#if DEBUG
    Serial.print("Attempting MQTT connection...");
#endif
    if (client.connect(deviceId, "homeassistant", "password") && client.subscribe("home-assistant/SensorDaSala")) {
#if DEBUG
      Serial.println("connected");
#endif
    } else {
#if DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif
      delay(5000);
    }
  }
}

void callback(char* topic, byte* p_payload, unsigned int length) {
  String payload;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)p_payload[i]);
    payload.concat((char)p_payload[i]);
  }
  if (String(commandTopicLampada).equals(topic)) {
    if (payload.equals("ON")) {
      digitalWrite(LAMPADA, HIGH);
    }
    else if (payload.equals("OFF")) {
      digitalWrite(LAMPADA, LOW);
    }
  }
  Serial.println();
}

void inverteLampada() {
  boolean statusAtual = digitalRead(LAMPADA);
  digitalWrite(LAMPADA, !statusAtual);
  Serial.println("Invertendo o status da lampada lampada");
  digitalRead(LAMPADA) ? statusLampada = "ON" : statusLampada = "OFF";
  client.publish(stateTopicLampada, statusLampada);
  delay(50);
}

void setup() {
  pinMode(LAMPADA, OUTPUT);
  pinMode(interruptor, INPUT);
  Serial.begin(57600);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  Ethernet.begin(deviceMac, deviceIp);
  delay(1500);
  dht.setup(DHTPIN); // data pin 5
}

void loop() {
  unsigned long ul_CurrentMillis = millis();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if(!digitalRead(interruptor)){
    interruptorFlag = 0x01;
    delay(100);
  }
  
  if (digitalRead(interruptor) && interruptorFlag) {
      inverteLampada();
      interruptorFlag = 0x00;
  }
  float umidade = dht.getHumidity();
  // Le a temperatura em Graus Celsius(Padrão)
  float temperatura = dht.getTemperature();
 
  int sensorValue = analogRead(LDR);
  delay(20);
  digitalRead(LAMPADA) ? statusLampada = "ON" : statusLampada = "OFF";
  if ( ul_CurrentMillis - ul_PreviousMillis > updateInterval)
  {
    ul_PreviousMillis = ul_CurrentMillis;
    client.publish(stateTopicLuminosidade, itoa(sensorValue, buf, 10));
    client.publish(stateTopicTemperatura, itoa(temperatura, buf, 10));
    client.publish(stateTopicUmidade, itoa(umidade, buf, 10));
    client.publish(stateTopicLampada, statusLampada);
  }
}
