//Для получения корректных данных стоит нажимать кнопку сразу после получения пакета, или имеется вероятность 
//пропустить n-ное количество нажатий из-за промежутка времени для отправки udp-пакета


#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>

uint8_t time_delay =            100;
unsigned int localPort =        8888;
unsigned int UDPremotePort =    8888;
unsigned long baud_serial =     115200;

IPAddress MyIP(192, 168, 42, 21);
IPAddress ServerIP(192, 168, 42, 1);

//Пины на которые заведены кнопки
#define button_kill_switch 22

//шаблон буферов для обмена данными
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer[50];                       // a string to send back

EthernetUDP Udp;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0xDD};

//переменная для киллсвитча
unsigned long kill_switch_count = 0;

bool kill_switch_state = 0;
bool kill_switch_memory = 0;

String inputString;
String inputIPString;

//таймер для отправки данных
unsigned long timer_print_serial;
unsigned long timer_print_udp;
unsigned long timer_kill_switch;

void setup() {
  Serial.begin(baud_serial);
  timer_print_serial = millis();
  timer_print_udp = millis();
  
  //инициализируем кнопку
  pinMode(button_kill_switch, INPUT_PULLUP);

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
  //Стартуем UDP
  Udp.begin(localPort);
  Serial.println("UDP connection is at: ");
  Serial.println(Ethernet.localIP());

}

void loop() {
  // Проверка раз в 10 мс, чтобы дребезг контактов не мешал работе(хоть и подключен INPUT_PULLUP)

  if(millis() - timer_kill_switch > 10){
    timer_kill_switch = millis();
    if(isButtonPressed()){
      timer_print_udp = millis();
    }
  }

  // Если 5 секунд отстутствуют нажатия, то пакет отправляется

  if (millis() - timer_print_udp > 5000) {
    timer_print_udp = millis();
    if(!isButtonPressed()){
      printUdpValue();
      printSerialValue();
    }
  }


}

bool isButtonPressed(){
  kill_switch_state = digitalRead(button_kill_switch);

  if(kill_switch_state == 0 && kill_switch_memory == 1){
    kill_switch_count++;
    kill_switch_memory = 0;
    return 1;
  }
  else if(kill_switch_state == 1 && kill_switch_memory == 0){
    kill_switch_memory = 1;
  }
  return 0;
}


void printUdpValue() {
  // вывод через udp значений всех кнопок
  char btn_kill[3]; String(kill_switch_count).toCharArray(btn_kill, 3);
  

  // сообщение значений кнопок
  if(!isButtonPressed()){
    Udp.beginPacket(ServerIP, UDPremotePort);
    Udp.write("kill: ");
    Udp.write(btn_kill);
    Udp.write("\n");
    Udp.endPacket();
  }
}

void printSerialValue() {
  // вывод в сериал значений всех кнопок
  uint8_t btn_kill = kill_switch_count;
  Serial.println("kill: " + String(btn_kill));
}
