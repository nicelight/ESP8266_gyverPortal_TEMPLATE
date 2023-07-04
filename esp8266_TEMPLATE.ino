//***********// #define USEMEMORY
//***********// #define ESP_WIFIAP // закомментировать если нужно  подключаться к существующей wifi

// Определяем точкy доступа
#define WIFI_ACCES_POINT esp32              // имя и пароль создаваемой wifi сети
#define WIFI_PASSWIRD 1234567812345678
#define WIFI_IS_HIDDEN 1                        // 1 -скрытая точка, 0 - видимая
#define CHANNEL 1                          // канал 1..11
#define MAX_CONNECTION 4                   // макимальное количество подключений 1..8

// Подключем библиотеки
#include <ESP8266WiFi.h>
#include <GyverNTP.h>
GyverNTP ntp(5); // +5 GMT
#include <WiFiClient.h>
//#include <EEPROM.h>
#include <EEManager.h>
#include <GyverPortal.h>
GyverPortal ui;

#ifdef ESP_WIFIAP
bool wifiHidden = WIFI_IS_HIDDEN;
byte maxConnection = MAX_CONNECTION;
bool wifiChanel = CHANNEL;
const char* ssidAP = "esp";
// const char* ssidAP = "esp";
const char* passwordAP = "1234567812345678"; // пароль от 1 до 8 два раза
#else
// если не точка, то wifi client
const char* ssid = "kuso4ek_raya";
const char* password = "1234567812345678";
#endif


#define LED_PIN 2 // esp32 dev module LED_BUILTIN
uint8_t state = 0; // автомат состояний
bool sensor = 0;
uint32_t ms = 0, prevMs = 0, stateMs = 0, sec = 1;
GPtime upd_UpTime;
unsigned int localPort = 2390;  // local port to listen for UDP packets


uint8_t uptimeHour = 0, uptimeMin = 0, uptimeSec = 0;
uint16_t totalPhotos = 0;
// структура настроек
struct Settings {
  uint32_t shooterTime;
  uint32_t afterSensorTime;
  uint32_t sld;
  uint32_t afterFlashDel;
  char str[20];
};

Settings set; // инициализация структуры типа mem

EEManager memory(set); // инициализация памяти

// Мигаем, если данные пришли
void ledBlink() {
  digitalWrite(LED_PIN, 1);
  delay(40);
  digitalWrite(LED_PIN, 0);
  delay(40);
}//ledBlink()

// страницу web портала строим
void webPageBuild() {
  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);

  GP.UPDATE("label1,label2,hh,mm,ss");// какие поля нужно обновлять

  // обновление случайным числом
  GP.TITLE("Меню");
  GP.HR();
  GP.BUTTON("btn", "Shot");
  GP.LABEL("Всего:");
  GP.LABEL("NAN", "label1");
  GP.LABEL("фоток");
  GP.BREAK();
  GP.LABEL("Сфоткал");
  GP.LABEL("NAN", "label2");
  GP.LABEL("сек назад");
  GP.BREAK();
  GP.LABEL("Аптайм:");
  GP.LABEL("hh", "hh");
  GP.LABEL("ч");
  GP.LABEL("mm", "mm");
  GP.LABEL("м");
  GP.LABEL("ss", "ss");
  GP.BREAK();
  GP.HR();
  //GP.SLIDER("sld", set.sld);
  //GP.BREAK();
  //GP.TEXT("txt", "", set.str);

  GP.LABEL("Задержка датчика");
  GP.NUMBER("uiPhotoGap", "number", set.afterSensorTime); GP.BREAK();
  GP.LABEL(" мс");
  GP.BREAK();
  GP.HR();
  GP.LABEL("Задержка вспышки ");
  GP.NUMBER("uiFlashDel", "number", set.afterFlashDel); GP.BREAK();
  GP.LABEL(" мс");
  GP.BREAK();
  GP.HR();
  GP.LABEL("Удержание затвора");
  GP.NUMBER("uiShooterTime", "number", set.shooterTime); GP.BREAK();
  GP.LABEL("в милисекундах");
  GP.BREAK();
  GP.HR();

  /* examples

    // создаём блок вручную
    GP.BLOCK_TAB_BEGIN("MOTOR CONFIG");
    M_BOX(GP.LABEL("Velocity"); GP.SLIDER("vel"););
    M_BOX(GP.LABEL("Accel."); GP.SLIDER("acc"););
    M_BOX(GP.BUTTON("bkw", "◄"); GP.BUTTON("frw", "►"););
    GP.BLOCK_END();

    GP.HR();
    GP.HR();

    GP.BLOCK_BEGIN(GP_THIN, "", "My thin txt red", GP_RED);
    GP.LABEL("Block thin text red");
    GP.BOX_BEGIN(GP_JUSTIFY);
    GP.LABEL("Slider");
    GP.SLIDER("sld");

    GP.BOX_BEGIN(GP_JUSTIFY);
    GP.LABEL("Buttons");
    GP.BOX_BEGIN(GP_RIGHT);
    GP.BUTTON_MINI("b1", "Kek", "", GP_RED);
    GP.BUTTON_MINI("b1", "Puk");
    GP.BOX_END();
    GP.BOX_END();

    GP.HR();
    GP.HR();

    M_BLOCK(
      M_BOX(GP.LABEL("Some check 1"); GP.CHECK(""); );
    M_BOX(GP.LABEL("Some Switch 1"); GP.SWITCH(""); );
    M_BOX(GP.LABEL("SSID");     GP.TEXT(""); );
    M_BOX(GP.LABEL("passwordAP"); GP.TEXT(""); );
    M_BOX(GP.LABEL("Host");     GP.TEXT(""); );
    );

  */
  GP.BUILD_END();
} // webPageBuild()


// обрабатываем действия на гайвер портале
void webPageAction() {

  if (ui.update()) {
    ui.updateInt("label1", totalPhotos);
    ui.updateInt("label2", sec);
    ui.updateInt("hh", uptimeHour);
    ui.updateInt("mm", uptimeMin);
    ui.updateInt("ss", uptimeSec);
  }//update()
  if (ui.click()) {
    Serial.println("UI CLICK detected");
    // по нажатию на кнопку сфотографируем
    if (ui.click("btn")) ledBlink();
    // перезапишем в shooterTime что ввели в интерфейсе
    if (ui.clickInt("uiPhotoGap", set.afterSensorTime)) {
      Serial.print("set.afterSensorTime: ");
      Serial.println(set.shooterTime);
#ifdef USEMEMORY
      memory.update(); //обновление настроек в EEPROM памяти
#endif
    }
    if (ui.clickInt("uiFlashDel", set.afterFlashDel)) {
      Serial.print("set.afterFlashDel: ");
      Serial.println(set.afterFlashDel);
#ifdef USEMEMORY
      memory.update(); //обновление настроек в EEPROM памяти
#endif
    }
    if (ui.clickInt("uiShooterTime", set.shooterTime)) {
      Serial.print("set.shooterTime: ");
      Serial.println(set.shooterTime);
#ifdef USEMEMORY
      memory.update(); //обновление настроек в EEPROM памяти
#endif
    }
  }//click()
}//webPageAction()


// инициализируем гайвер портал
void webUI_Init() {
  ui.attachBuild(webPageBuild);
  ui.attach(webPageAction);
  ui.start();

}//webUI_Init()


void wifiKeep() {

#ifdef ESP_WIFIAP
  // Инициируем точку доступа WiFi

  WiFi.softAP(ssidAP, passwordAP, wifiChanel, wifiHidden, maxConnection);
  // IPAddress myIP = WiFi.softAPIP();
  Serial.print("wifi_AP IP: ");
  Serial.println(WiFi.softAPIP());
#else
  if  (WiFi.status() == WL_CONNECTED) return;
  else {
    // Подключаемся к Wi-Fi
    Serial.print("try conn to ");
    Serial.print(ssid);
    Serial.print(":");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    byte i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      i++;
      if (i > 40) ESP.restart();
    }
    Serial.print("Connected. \nIP: ");
    // Выводим IP ESP
    Serial.println(WiFi.localIP());
  }
#endif
}//wifiInit()

NTP_Init() {
  ntp.begin();
}


void pinsBegin() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);
}  //pinsBegin()

void uptime_tick() {
  // инкрементируем Аптайм
  if ((ms - prevMs) > 1000) {
    prevMs = ms;
    sec++;
    uptimeSec++;
    //Serial.print("state:");
    //Serial.println(state);
    if (uptimeSec > 59) {
      uptimeSec = 0;
      uptimeMin++;
      if (uptimeMin > 59) {
        uptimeMin = 0;
        uptimeHour++;
        Serial.printf("%lu hours %lu mins\n", uptimeHour, uptimeMin);
      }
    }  //if sec
  }//ms 1000
}//uptime_tick()


void setup() {
  // put your setup code here, to run once:
  // Инициируем последовательный порт
  Serial.begin(115200);
  Serial.println("\n\n\n\n");
  EEPROM.begin(100);  // выделить память (больше или равно размеру даты)
  memory.begin(0, 'a');
  pinsBegin();
  wifiKeep();
  webUI_Init();
  NTP_Init();
}//setup

void loop() {
  // put your main code here, to run repeatedly:
  wifiKeep(); yield();
  ui.tick();  yield();
  ntp.tick(); yield();
  memory.tick();  yield();
  uptime_tick();  yield();
  ms = millis();
}//loop

//uint8_t realSec = ntp.second();
//uint8_t realmin = ntp.minute();
//uint8_t realhour = ntp.hour();
//uint8_t realday = ntp.day();
//uint8_t realmonth = ntp.month();
//uint16_t realyear = ntp.year();
//uint8_t realdayweek = ntp.dayWeek(); 
//String timeString();            // получить строку времени формата ЧЧ:ММ:СС
//String dateString();            // получить строку даты формата ДД.ММ.ГГГГ
//uint8_t status();               // получить статус системы

// 0 - всё ок
// 1 - не запущен UDP
// 2 - не подключен WiFi
// 3 - ошибка подключения к серверу
// 4 - ошибка отправки пакета
// 5 - таймаут ответа сервера
// 6 - получен некорректный ответ сервера
