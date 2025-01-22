基于ESP32，用于通过WiFi连接到DeepSeek API，并通过串口与API进行交互。
主要功能：
1. 硬件初始化：
   - 使用`U8G2`库初始化一个SSD1306 OLED显示屏。
   - 设置一个按键（连接到GPIO 2）用于输入。

2. WiFi连接：
   - 通过`WiFi`库连接到指定的WiFi网络。
   - 如果连接成功，显示“WiFi Connected”；否则，显示“WiFi Failed”并进入无限循环。

3. 串口通信：
   - 通过`Serial`库与计算机进行串口通信。
   - 用户可以输入消息，按下回车键（`n`）后，消息会被发送到DeepSeek API。

4. API请求：
   - 使用`WiFiClientSecure`库通过HTTPS连接到DeepSeek API。
   - 构建一个JSON格式的请求负载，包含用户输入的消息。
   - 发送请求并接收API的响应。

5. 响应解析：
   - 使用`ArduinoJson`库解析API返回的JSON响应。
   - 提取并显示API返回的消息内容。

6. 显示功能：
   - 使用`U8G2`库在OLED显示屏上显示消息。
   - 如果消息过长，会自动分页显示。
代码结构

- setup()：初始化硬件和WiFi连接。
- loop()：主循环，检测串口输入和按键状态，并处理API请求。
- serialEvent()：处理串口输入。
- displayMessage()：在OLED显示屏上显示消息。
- connectToWiFi()：连接到WiFi网络。
- cleanInput()*：清理用户输入，防止JSON格式错误。
- buildPayload()：构建发送到API的JSON负载。
- sendToAPI()：发送请求到API并接收响应。
- parseResponse()：解析API返回的JSON响应。

使用说明

1. 硬件连接：
   - 将SSD1306 OLED显示屏连接到ESP32的I2C引脚（SCL: GPIO 42, SDA: GPIO 41）。
   - 将一个按键连接到GPIO 2和VCC(暂时无用)

2. 软件配置：
   - 替换代码中的`ssid`、`password`和`api_key`为你的WiFi和DeepSeek API的凭据。

3. 运行：
   - 上传代码到ESP32。
   - 打开串口监视器，输入消息并按下回车键。
   - API的响应将显示在OLED屏幕上和串口监视器中。

注意事项

- 确保WiFi网络和API密钥正确。
- 如果API响应时间较长，调整`client.setTimeout()`的值。
