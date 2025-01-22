#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// 函数声明
void displayMessage(const String& msg);
bool connectToWiFi();
String sendToAPI(const String& prompt);
String parseResponse(const String& json);
void serialEvent();
String cleanInput(const String& input);
String buildPayload(const String& prompt);
void key_scan();
// 硬件配置
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 42, 41, U8X8_PIN_NONE);
#define KEY 2
int button_value = 0;

// 网络配置
const char* ssid = "your ssid";// 替换为你的 WiFi SSID 和密码
const char* password = "your password";// 替换为你的 WiFi SSID 和密码
const char* api_key = "your api key"; // 替换为你的 DeepSeek API 密钥
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
  
  // 设置字体
  u8g2.setFont(u8g2_font_helvB08_tf);
  Serial.println(msg);
  // 每行能显示的字符数
  int charsPerLine = 20; // 根据你的显示设备和字体大小调整
  
  // 每页能显示的行数
  int linesPerPage = 4; // 根据你的显示设备和字体大小调整
  
  int len = msg.length();
  int currentLine = 0;
  int currentPage = 0;
  
  for (int i = 0; i < len; i += charsPerLine) {
    // 获取当前行的子字符串
    String line = msg.substring(i, min(i + charsPerLine, len));
    
    // 计算当前行的 y 坐标
    int y = (currentLine % linesPerPage) * 15 + 10; // 10 是行高
    
    // 显示当前行
    u8g2.drawStr(0, y, line.c_str());
    
    currentLine++;
    
    // 如果当前页满了，换页
    if (currentLine % linesPerPage == 0) {
      u8g2.sendBuffer();
      delay(10000); // 等待一段时间，让用户阅读当前页
      u8g2.clearBuffer();
      currentPage++;
    }
  }
  
  // 显示最后一页
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

// 定义一个函数 parseResponse，用于解析 JSON 响应字符串
String parseResponse(const String& json) {
  // 创建一个 JsonDocument 对象，用于存储解析后的 JSON 数据
  JsonDocument doc;
  // 使用 deserializeJson 函数将输入的 JSON 字符串解析到 doc 对象中
  // 返回一个 DeserializationError 对象，表示解析过程中是否发生错误
  DeserializationError error = deserializeJson(doc, json);
  
  // 检查解析过程中是否发生错误
  if (error) {
    // 如果发生错误，返回一个包含错误信息的字符串
    return "Parse Error: " + String(error.c_str());
  }

  // 检查解析后的 JSON 数据中是否包含 "choices" 键
  if (!doc.containsKey("choices")) {
    // 如果不包含 "choices" 键，返回 "Invalid Response" 字符串
    return "Invalid Response";
  }

  // 从解析后的 JSON 数据中获取 "choices" 键下的第一个元素中的 "message" 键下的 "content" 键的值
  // 该值是一个 const char* 类型的指针
  const char* content = doc["choices"][0]["message"]["content"];
  // 检查获取到的 content 是否为空
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