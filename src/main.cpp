#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <config.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup_wifi()
{
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PSK);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void subscription_callback(char *topic, byte *payload, unsigned int length)
{
  String topic_str(topic);
  String payload_str;
  payload_str.concat((char *)payload, length);

  if (topic_str == "/watering-assistant/plant1/pump/cmd")
  {
    Serial.print(topic_str);
    Serial.print(": ");
    Serial.println(payload_str);
    digitalWrite(LED_BUILTIN, payload_str == "1" ? LOW : HIGH);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  setup_wifi();
  mqttClient.setServer(MQTT_BROKER, 1883);
  mqttClient.setCallback(subscription_callback);
}

void reconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Connecting to MQTT broker...");
    if (!mqttClient.connect("IoT-Watering-Assistant"))
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
    else
    {
      Serial.println("successful!");
      mqttClient.subscribe("/watering-assistant/plant1/pump/cmd");
    }
  }
}

float readVH400(uint8_t analogPin)
{
  float sensorVoltage = (3.2 / 1023.0) * analogRead(analogPin);

  float vwc = 0;

  if (sensorVoltage <= 1.1)
    vwc = 10 * sensorVoltage - 1;
  else if (sensorVoltage > 1.1 && sensorVoltage <= 1.3)
    vwc = 25 * sensorVoltage - 17.5;
  else if (sensorVoltage > 1.3 && sensorVoltage <= 1.82)
    vwc = 48.08 * sensorVoltage - 47.5;
  else if (sensorVoltage > 1.82 && sensorVoltage <= 2.2)
    vwc = 26.32 * sensorVoltage - 7.89;
  else if (sensorVoltage > 2.2 && sensorVoltage <= 3.0)
    vwc = 62.5 * sensorVoltage - 87.5;

  // limit value to 0 - 100%
  return max(min(vwc, 100.0f), 0.0f);
}

void loop()
{
  if (!mqttClient.connected())
  {
    reconnect();
  }

  mqttClient.loop();

  static uint32_t ms = 0;
  if ((millis() - ms) > 5000)
  {
    ms = millis();

    char sensorStr[8];
    sprintf(sensorStr, "%d", (uint8_t)round(readVH400(A0)));

    mqttClient.publish("/watering-assistant/plant1/sensor/value", sensorStr);
  }
}