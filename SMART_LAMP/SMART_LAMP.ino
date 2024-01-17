#include <Ethernet.h>
#include <EthernetUdp.h>
#include <EEPROM.h>
#include <Wire.h>

uint8_t time_delay =            100;
unsigned int localPort =        8888;
unsigned int UDPremotePort =    8888;
unsigned long baud_serial =     115200;

IPAddress MyIP(192, 168, 2, 21);
IPAddress ServerIP(192, 168, 2, 25);

//Пины, которыми управляются светодиоды
#define yellow_led 2
#define green_led 3
#define blue_led 4
#define red_led 5

//шаблон буферов для обмена данными
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer[50];                       // a string to send back

EthernetUDP Udp;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0xDD};

String inputString;
String inputIPString;

//таймер для отправки данных
unsigned long timer_print_serial;
unsigned long timer_print_udp;
unsigned long timer_print_dxl;

void setup() {
  Serial.begin(baud_serial);
  timer_print_serial = millis();
  timer_print_udp = millis();

  //инициализируем светодиоды
  pinMode(red_led, OUTPUT);
  pinMode(blue_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);

  //проверяем светодиоды
  checkLeds();

  //Формируем MAC-адрес на основе MyIP
  mac[0] = MyIP[0];
  mac[1] = MyIP[1];
  mac[2] = MyIP[2];
  mac[3] = MyIP[3];
  mac[4] = MyIP[2];
  mac[5] = MyIP[3];


  Serial.println("Ready");
  //Стартуем Ethernet
  Ethernet.begin(mac, MyIP);
  Serial.print("My IP: ");
  Serial.println(MyIP);
  Serial.print("Server IP: ");
  Serial.println(ServerIP);
  //Стартуем UDP
  Udp.begin(localPort);
}

void loop() {
  //если в порт пришли данные - читаем их в строку
  while (Serial.available()) {
    char data = (char)Serial.read();
    inputString += data;
    delay(10);
  }

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("-> Udp: ");
    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println(packetBuffer);
    setValue(packetBuffer);
  }

  //Выводим полученные данные
  if (inputString.length() > 0) {
    Serial.print("-> ");
    Serial.println(inputString);
    setValue(inputString);
    inputString = "";
  }

  // Отправка данных
  if (millis() - timer_print_serial > time_delay) {
    timer_print_serial = millis();
    printSerialValue();
    printUdpValue();
  }

}

void printUdpValue() {
  // вывод через udp значений всех лампочек
  char led_red[2]; String(digitalRead(red_led)).toCharArray(led_red, 2);
  char led_blue[2]; String(digitalRead(blue_led)).toCharArray(led_blue, 2);
  char led_green[2]; String(digitalRead(green_led)).toCharArray(led_green, 2);
  char led_yellow[2]; String(digitalRead(yellow_led)).toCharArray(led_yellow, 2);

  // сообщение значений светодиодов
  Udp.beginPacket(ServerIP, UDPremotePort);
  Udp.write("leds: ");
  Udp.write("r: ");
  Udp.write(led_red);
  Udp.write(" b: ");
  Udp.write(led_blue);
  Udp.write(" g: ");
  Udp.write(led_green);
  Udp.write(" y: ");
  Udp.write(led_yellow);
  Udp.write("\n");
  Udp.endPacket();
}

void setValue(String str) {
  // вывод в сериал значения всех лампочек
  if (str.length() >= 4) {
    for (int i = 0; i < str.length(); i++) {
      switch (i) {
        case 0: digitalWrite(red_led, String(str[i]).toInt()); break;
        case 1: digitalWrite(blue_led, String(str[i]).toInt()); break;
        case 2: digitalWrite(green_led, String(str[i]).toInt()); break;
        case 3: digitalWrite(yellow_led, String(str[i]).toInt()); break;
      }
    }
  }
}

void printSerialValue() {
  // вывод в сериал значений всех лампочек
  uint8_t led_red = digitalRead(red_led);
  uint8_t led_blue = digitalRead(blue_led);
  uint8_t led_green = digitalRead(green_led);
  uint8_t led_yellow = digitalRead(yellow_led);
  Serial.println(
    "leds: r: " + String(led_red) + " " +
    "b: " + String(led_blue) + " " +
    "g: " + String(led_green) + " " +
    "y: " + String(led_yellow));
}

void checkLeds() {
  //проверяем светодиоды
  uint8_t time_sleep = 100;
  digitalWrite(red_led, HIGH);
  delay(time_sleep);
  digitalWrite(blue_led, HIGH);
  delay(time_sleep);
  digitalWrite(green_led, HIGH);
  delay(time_sleep);
  digitalWrite(yellow_led, HIGH);
  delay(time_sleep);
  digitalWrite(red_led, LOW);
  delay(time_sleep);
  digitalWrite(blue_led, LOW);
  delay(time_sleep);
  digitalWrite(green_led, LOW);
  delay(time_sleep);
  digitalWrite(yellow_led, LOW);
  delay(time_sleep);

  for (int i = 0; i < 2; i++) {
    digitalWrite(red_led, HIGH);
    digitalWrite(blue_led, HIGH);
    digitalWrite(green_led, HIGH);
    digitalWrite(yellow_led, HIGH);
    delay(time_sleep);
    digitalWrite(red_led, LOW);
    digitalWrite(blue_led, LOW);
    digitalWrite(green_led, LOW);
    digitalWrite(yellow_led, LOW);
    delay(time_sleep);
  }
}
