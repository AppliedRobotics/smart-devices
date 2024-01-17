#include <SPI.h>
#include <Ethernet.h>

// MAC-адрес сетевой платы
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x02, 0xED };
IPAddress ip(192, 168, 42, 21);  // IP-адрес (нужно изменить на актуальный для вашей сети)
EthernetServer server(80);       // создаём сервер на 80 порту

float volts1;
float volts2;
float volts3;
int dist1;
int dist2;
int dist3;

unsigned long long timing = millis();

void setup() {
  Ethernet.begin(mac, ip);  // инициализируем Ethernet
  server.begin();           // начинаем ожидать запросы от клиента
  Serial.begin(115200);

}

void loop() {
  EthernetClient client = server.available();  // (если есть) «получаем» клиента
  if (millis() - timing > 1000) {
    timing = millis();
    volts1 = analogRead(A3) * 0.0048828125;
    volts2 = analogRead(A1) * 0.0048828125;
    volts3 = analogRead(A0) * 0.0048828125;
    dist1 = 65 * pow(volts1, -1.10);
    dist2 = 65 * pow(volts2, -1.10);
    dist3 = 65 * pow(volts3, -1.10);

    Serial.print(dist1);
    Serial.print(" ");
    Serial.print(dist2);
    Serial.print(" ");
    Serial.println(dist3);
  }
  if (client) {  // есть клиент?
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {  // данные от клиента доступны для чтения
        char c = client.read();  // чтение 1 байта (символа) от клиента
        // последняя строка от запроса клиента пустая и заканчивается на \n
        // отвечаем клиенту только после окончания запроса
        if (c == '\n' && currentLineIsBlank) {
          // посылаем стандартный заголовок ответа
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println("Refresh: 1");  // автоматически обновляет страницу каждые 5 секунд
          client.println();
          
          //посылаем саму веб-страницу
          client.println("<!DOCTYPE html>\n");
          client.println("<html lang=\"ru\">\n");
          client.println("\n");
          client.println("<head>\n");
          client.println("<meta charset=\"UTF-8\">\n");
          client.println("<title>IR-sensor data</title>\n");
          client.println("<!-- <meta http-equiv=\"refresh\" content=\"1\" > -->\n");
          client.println("</head>\n");
          client.println("\n");
          client.println("<body>");
          client.println("<h1>Данные датчика линии</h1>\n");
          client.println("<p>Верхний датчик: " + String(dist1) + "см</p>");
          client.println("<p>Средний датчик: " + String(dist2) + "см</p>");
          client.println("<p>Нижний датчик: " + String(dist3) + "см</p>");
          client.println("</body>");
          client.println("</html>");
          break;
        }
        // каждая строка текста, принятая от клиента, оканчивается на \r\n
        if (c == '\n') {
          // последний символ в строке принятого текста
          // начинаем новую строку со следующего прочитанного символа
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // от клиента получен текстовый символ
          currentLineIsBlank = false;
        }
      }             // end if (client.available())
    }               // end while (client.connected())
    delay(1);       // даём время браузеру для приёма наших данных
    client.stop();  // закрываем соединение
  }                 // end if (client)
}
