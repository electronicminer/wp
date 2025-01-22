#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ��������
void displayMessage(const String& msg);
bool connectToWiFi();
String sendToAPI(const String& prompt);
String parseResponse(const String& json);
void serialEvent();
String cleanInput(const String& input);
String buildPayload(const String& prompt);
void key_scan();
// Ӳ������
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 42, 41, U8X8_PIN_NONE);
#define KEY 2
int button_value = 0;

// ��������
const char* ssid = "your ssid";// �滻Ϊ��� WiFi SSID ������
const char* password = "your password";// �滻Ϊ��� WiFi SSID ������
const char* api_key = "your api key"; // �滻Ϊ��� DeepSeek API ��Կ
const char* host = "api.deepseek.com";
const int httpsPort = 443;

String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  u8g2.setFont(u8g2_font_helvB08_tf);

  pinMode(KEY, INPUT);

  displayMessage("Initializing...");

  if (connectToWiFi()) {
    displayMessage("WiFi Connected");
  } else {
    displayMessage("WiFi Failed");
    while (1) delay(1000);
  }

  displayMessage("Ready for Input");
  Serial.println("Please enter your message:");
}

void loop() {
  if (stringComplete) {
    String response = sendToAPI(inputString);
    displayMessage(": " + response);
    inputString = "";
    stringComplete = false;
    Serial.println("Please enter your message:");
  }
  key_scan();
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == 'n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}


void displayMessage(const String& msg) {
  u8g2.clearBuffer();
  
  // ��������
  u8g2.setFont(u8g2_font_helvB08_tf);
  Serial.println(msg);
  // ÿ������ʾ���ַ���
  int charsPerLine = 20; // ���������ʾ�豸�������С����
  
  // ÿҳ����ʾ������
  int linesPerPage = 4; // ���������ʾ�豸�������С����
  
  int len = msg.length();
  int currentLine = 0;
  int currentPage = 0;
  
  for (int i = 0; i < len; i += charsPerLine) {
    // ��ȡ��ǰ�е����ַ���
    String line = msg.substring(i, min(i + charsPerLine, len));
    
    // ���㵱ǰ�е� y ����
    int y = (currentLine % linesPerPage) * 15 + 10; // 10 ���и�
    
    // ��ʾ��ǰ��
    u8g2.drawStr(0, y, line.c_str());
    
    currentLine++;
    
    // �����ǰҳ���ˣ���ҳ
    if (currentLine % linesPerPage == 0) {
      u8g2.sendBuffer();
      delay(10000); // �ȴ�һ��ʱ�䣬���û��Ķ���ǰҳ
      u8g2.clearBuffer();
      currentPage++;
    }
  }
  
  // ��ʾ���һҳ
  if (currentLine % linesPerPage != 0) {
    u8g2.sendBuffer();
  }
}

bool connectToWiFi() {
  displayMessage("Connecting WiFi");
  WiFi.begin(ssid, password);
  
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi Connected!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Failed!");
  return false;
}

String cleanInput(const String& input) {
  String cleaned;
  for (unsigned int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (c == '\"') {
      cleaned += "\\\"";
    } else if (c >= 32) {
      cleaned += c;
    }
  }
  return cleaned;
}

String buildPayload(const String& prompt) {
  JsonDocument doc;
  doc["model"] = "deepseek-chat";
  doc["stream"] = false;

  JsonArray messages = doc["messages"].to<JsonArray>();
  JsonObject systemMsg = messages.add<JsonObject>();
  systemMsg["role"] = "system";
  systemMsg["content"] = "You are a helpful assistant";

  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"] = "user";
  userMsg["content"] = prompt;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

String sendToAPI(const String& prompt) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);

  String cleanedPrompt = cleanInput(prompt);
  Serial.println("Cleaned Prompt: " + cleanedPrompt);

  String payload = buildPayload(cleanedPrompt);
  Serial.println("Request Payload:");
  Serial.println(payload);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "waiting for response...");
  if (!client.connect(host, httpsPort)) {
    return "Connection failed";
  }

  String request = String("POST /v1/chat/completions HTTP/1.1\r\n") +
                   "Host: " + host + "\r\n" +
                   "Authorization: Bearer " + api_key + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + payload.length() + "\r\n\r\n" +
                   payload;

  client.print(request);

  String response;
  while (client.connected() || client.available()) {
    if (client.available()) {
      response += client.readStringUntil('\n');
    }
  }

  int jsonStart = response.indexOf('{');
  if (jsonStart == -1) {
    return "No JSON found";
  }

  return parseResponse(response.substring(jsonStart));
}

// ����һ������ parseResponse�����ڽ��� JSON ��Ӧ�ַ���
String parseResponse(const String& json) {
  // ����һ�� JsonDocument �������ڴ洢������� JSON ����
  JsonDocument doc;
  // ʹ�� deserializeJson ����������� JSON �ַ��������� doc ������
  // ����һ�� DeserializationError ���󣬱�ʾ�����������Ƿ�������
  DeserializationError error = deserializeJson(doc, json);
  
  // �������������Ƿ�������
  if (error) {
    // ����������󣬷���һ������������Ϣ���ַ���
    return "Parse Error: " + String(error.c_str());
  }

  // ��������� JSON �������Ƿ���� "choices" ��
  if (!doc.containsKey("choices")) {
    // ��������� "choices" �������� "Invalid Response" �ַ���
    return "Invalid Response";
  }

  // �ӽ������ JSON �����л�ȡ "choices" ���µĵ�һ��Ԫ���е� "message" ���µ� "content" ����ֵ
  // ��ֵ��һ�� const char* ���͵�ָ��
  const char* content = doc["choices"][0]["message"]["content"];
  // ����ȡ���� content �Ƿ�Ϊ��
  return content ? String(content) : "Empty Content";
}

void key_scan() {
  if (digitalRead(KEY) == HIGH) {
    {
      delay(200);
      if (digitalRead(KEY) == HIGH) {
          button_value = 1;
      }
    }
    if (digitalRead(KEY) == LOW)
    {
      button_value = 0;
    }
  }
}