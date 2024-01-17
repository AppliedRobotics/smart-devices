#include <Ethernet.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "DxlMaster2.h"

#define baud_serial  115200
#define time_delay   100
bool eeprom_ip =     false;
bool reverse_servo = false;

IPAddress MyIP(192, 168, 42, 21);

EthernetServer server(80);

//Настройки для Dinamixel
#define baud_dxl     1000000
#define dxl_id       1
const float DXL_PROTOCOL_VERSION = 2.0; // протокол передачи данных от DXL IOT к сервоприводам

// Задание ПИД уставок:
#define P_Gain_Pos_addr 84
#define I_Gain_Pos_addr 82
#define D_Gain_Pos_addr 80

uint16_t P_Pos_param = 50;    // 640
uint16_t I_Pos_param = 0;     // 0
uint16_t D_Pos_param = 400;   // 4000

uint16_t speed_servo = 1023;  // 0~1023

DynamixelMotor motor1((uint8_t)dxl_id);

//Пины на которые заведены кнопки
#define button_yellow 22
#define button_red 23
#define button_green 24
#define button_kill_switch 25

//Пины на которые заведены оси джойстика
#define joy_y A0
#define joy_x A1

//Пины, которыми управляются светодиоды
#define yellow_led 2
#define green_led 3
#define blue_led 4
#define red_led 5

//задаем дисплей
LiquidCrystal_I2C lcd(0x27, 20, 4);

//переменные для хранения данных строк дисплея
String LCDLine = "";
String LCDLine1 = "";
String LCDLine2 = "";
String LCDLine3 = "";
String LCDLine4 = "";
uint8_t lineNumber = 0;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x02, 0xED};

//переменные для отслеживания состояния кнопок
uint8_t button_red_state = 0;
uint8_t button_yellow_state = 0;
uint8_t button_green_state = 0;
uint8_t button_red_last_state = 0;
uint8_t button_yellow_last_state = 0;
uint8_t button_green_last_state = 0;

//счетчик нажатий на кнопки
unsigned long red_count = 0;
unsigned long yellow_count = 0;
unsigned long green_count = 0;

//переменная для киллсвитча
unsigned long kill_switch_count = 0;

String inputString;
String inputIPString;

//таймер для отправки данных
unsigned long timer_print_serial;

void setup() {
  Serial.begin(baud_serial);
  DxlMaster.begin(baud_dxl);
  
  motor1.protocolVersion(DXL_PROTOCOL_VERSION);
  
  motor1.enableTorque(0);  // отключаем крутящий момент, когда изменяем EEPROM область в сервоприводе
  motor1.write(DYN2_ADDR_OPERATION_MODE, (uint8_t)3); // установка режима работы сервопривода в качестве шарнира = 3
  motor1.write(DYN2_ADDR_PROFILE_VELOCITY, speed_servo); // установка скорости передвижения сервопривода
  
  // установка параметров ПИД регулятора по номеру регистра:
  motor1.write(P_Gain_Pos_addr, (uint16_t)P_Pos_param);
  motor1.write(I_Gain_Pos_addr, (uint16_t)I_Pos_param);
  motor1.write(D_Gain_Pos_addr, (uint16_t)D_Pos_param);

  timer_print_serial = millis();
  
  //инициализируем дисплей
  lcd.init();
  lcd.backlight();
  
  //чистим его
  lcd.clear();
  
  //инициализируем кнопки
  pinMode(button_red, INPUT_PULLUP);
  pinMode(button_yellow, INPUT_PULLUP);
  pinMode(button_green, INPUT_PULLUP);
  pinMode(button_kill_switch, INPUT_PULLUP);

  //инициализируем светодиоды
  pinMode(red_led, OUTPUT);
  pinMode(blue_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);

  //проверяем светодиоды
  checkLeds();

  //Считываем данные из EEPROM и представляем их как IP адреса
  //IP адрес модуля
  if (eeprom_ip) {
    for (int addr = 0; addr < 4; addr++) {
      MyIP[addr] = EEPROM.read(addr);
    }
  }

  //Формируем MAC-адрес на основе MyIP
  mac[0] = MyIP[0];
  mac[1] = MyIP[1];
  mac[2] = MyIP[2];
  mac[3] = MyIP[3];
  mac[4] = MyIP[2];
  mac[5] = MyIP[3];

  //Стартуем Ethernet
  Ethernet.begin(mac, MyIP);
  server.begin();
  setValueLCD(0, 0);
  Serial.println("Ready");
}

void loop()
{
  if (millis() - timer_print_serial > 1000) {
    timer_print_serial = millis();
    int x = analogRead(joy_x);
    int y = analogRead(joy_y);
    setValueLCD(x, y);
    printSerialValue();
  }
  motor1.enableTorque(1);
  int pos = analogRead(joy_x);
  if (!reverse_servo) {
    motor1.goalPosition(map(pos, 0, 1023, 16384, 0));
  }
  else {
    motor1.goalPosition(map(pos, 0, 1023, 0, 16384));
  }
  EthernetClient client = server.available();
  if (client)
  {
    // Проверяем подключен ли клиент к серверу
    while (client.connected())
    {
      String response = "";
      if (client.available()) {
        String request = client.readStringUntil('\r');
        if (request.indexOf("/text_led") == -1) {
          String inputString = request.substring(request.indexOf("=") + 1, request.indexOf("HTTP") - 1);
          setValue(inputString);
        }

        response += "HTTP/1.1 200 OK";
        response += "Content-Type: text/html";

        response += "<!DOCTYPE html>\n";
        response += "<html lang=\"ru\">\n";
        response += "\n";
        response += "<head>\n";
        response += "    <meta charset=\"UTF-8\">\n";
        response += "    <title>Web server terminal</title>\n";
        response += "    <!-- <meta http-equiv=\"refresh\" content=\"1\" > -->\n";
        response += "</head>\n";
        response += "\n";
        response += "<body>\n";
        response += "    <style>\n";
        response += "        p {font-size: 18px;}\n";
        response += "        input {font-size: 15px;}\n";
        response += "        body {background-color: rgb(245, 245, 245);}\n";
        response += "    </style>\n";
        response += "    <h2 class=\"header\" style=\"text-align: center;\">Терминал</h2>\n";
        response += "    <div class=\"values_btn\">\n";
        response += "        <p>Значение желтой кнопки: <span id=\"btn_y\">0</span></p>\n";
        response += "        <p>Значение красной кнопки: <span id=\"btn_r\">0</span></p>\n";
        response += "        <p>Значение зеленой кнопки: <span id=\"btn_g\">0</span></p>\n";
        response += "        <p>Значение большой кнопки: <span id=\"btn_k\">0</span></p>\n";
        response += "    </div>\n";
        response += "    <script>\n";
        response += "        var btn_y = " + String(!digitalRead(button_yellow)) + ";\n";
        response += "        var btn_r = " + String(!digitalRead(button_red)) + ";\n";
        response += "        var btn_g = " + String(!digitalRead(button_green)) + ";\n";
        response += "        var btn_k = " + String(digitalRead(button_kill_switch)) + ";\n";
        response += "        document.getElementById(\"btn_y\").innerHTML = btn_y\n";
        response += "        document.getElementById(\"btn_r\").innerHTML = btn_r\n";
        response += "        document.getElementById(\"btn_g\").innerHTML = btn_g\n";
        response += "        document.getElementById(\"btn_k\").innerHTML = btn_k\n";
        response += "\n";
        response += "    </script>\n";
        response += "    <div class=\"tb_led\">\n";
        response += "        <form action=\"\">\n";
        response += "            <p>Установить значение светодиодов, 0 - выкл, 1 - вкл (пример: 0000) </p>\n";
        response += "            <input type=\"text\" id=\"text_led\" name=\"text_led\" value=\"0000\" minlength=\"4\" maxlength=\"4\" size=\"10\" />\n";
        response += "            <input type=\"submit\" value=\"отправить\">\n";
        response += "        </form>\n";
        response += "    </div>\n";
        response += "</body>\n";
        response += "\n";
        response += "</html>\n";
        client.println(response);
        client.stop();
      } // end if (client.available())
    }
  }
}

int freeRam () {
  extern int __heap_start, *__brkval;  int v;  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
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
  // вывод в сериал значений всех кнопок
  uint8_t btn_yellow = !digitalRead(button_yellow);
  uint8_t btn_red = !digitalRead(button_red);
  uint8_t btn_green = !digitalRead(button_green);
  uint8_t btn_kill = digitalRead(button_kill_switch);
  Serial.println(
    "btns: y: " + String(btn_yellow) + " " +
    "r: " + String(btn_red) + " " +
    "g: " + String(btn_green) + " " +
    "k: " + String(btn_kill));

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

  // вывод в сериал значений c джойстика
  uint16_t joyx = analogRead(joy_x);
  uint16_t joyy = analogRead(joy_y);
  Serial.println(
    "joy: x: " + String(joyx) + " " +
    "y: " + String(joyy));
  Serial.println();
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

void setValueLCD(int x, int y) {
  lcd.setCursor(7, 0);
  lcd.print("Ready");
  lcd.setCursor(0, 3);
  lcd.print("MyIP:");
  lcd.setCursor(5, 3);
  lcd.print(MyIP);

  lcd.setCursor(0, 1);
  lcd.print("Joy: x:");
  lcd.setCursor(7, 1);
  lcd.print("    ");
  lcd.setCursor(7, 1);
  lcd.print(x);
  lcd.setCursor(12, 1);
  lcd.print("y:");
  lcd.setCursor(14, 1);
  lcd.print("    ");
  lcd.setCursor(14, 1);
  lcd.print(y);


}
