//***********// #define USEMEMORY
//***********// #define ESP_WIFIAP // закомментировать если нужно  подключаться к существующей wifi

// Определяем точкy доступа
#define WIFI_ACCES_POINT esp32              // имя и пароль создаваемой wifi сети
#define WIFI_PASSWIRD 1234567812345678
#define WIFI_IS_HIDDEN 1                        // 1 -скрытая точка, 0 - видимая
#define CHANNEL 1                          // канал 1..11
#define MAX_CONNECTION 4                   // макимальное количество подключений 1..8
#define COLORTHEME "#40c2e6" //цвет кнопочек голубенький

// Подключем библиотеки
#include <ESP8266WiFi.h>
#include <GyverNTP.h>
GyverNTP ntp(5); // +5 GMT
#include <WiFiClient.h>
//#include <EEPROM.h>
#include <EEManager.h>
#include <GyverPortal.h>
#include <LittleFS.h>
GyverPortal ui(&LittleFS); // для проверки файлов


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

uint16_t realyear = 0;
uint8_t nowhh = 0;
uint8_t nowmm = 0;
uint8_t nowss = 0;
uint8_t uptimeHour = 0, uptimeMin = 0, uptimeSec = 0;

uint16_t totalPhotos = 0;
// структура настроек
struct Settings {
  uint32_t shooterTime;
  uint32_t afterSensorTime;
  uint32_t sld;
  uint32_t afterFlashDel;
  char str[20];
  bool wateringNow = 0;
};

uint8_t startMin = 10;
uint8_t startHour = 3;
uint8_t wateringMin = 15;

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

  GP.UPDATE("uptimehh, uptimemm, uptimess, nowhh, nowmm, nowss");// какие поля нужно обновлять

  // обновление случайным числом
  GP.TITLE("Меню");
  GP.HR();
  GP.BUTTON("btn", "Shot", "", COLORTHEME); // "" - id компонента
  //-  GP.LABEL("Всего:");
  //-  GP.LABEL("NAN", "label1");
  //-  GP.LABEL("фоток");
  //-  GP.BREAK();
  //-  GP.LABEL("Сфоткал");
  //-  GP.LABEL("NAN", "label2");
  //-  GP.LABEL("сек назад");
  GP.BREAK();
  GP.LABEL("Аптайм:");
  GP.LABEL("uptimehh", "uptimehh");
  GP.LABEL("ч");
  GP.LABEL("uptimemm", "uptimemm");
  GP.LABEL("м");
  GP.BREAK();

  GP.LABEL("Время:");
  GP.LABEL("nowhh", "nowhh");
  GP.LABEL(":");
  GP.LABEL("nowmm", "nowmm");
  GP.LABEL(" :");
  GP.LABEL("nowss", "nowss");
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


  GP.SELECT("startHour", "0,1,2,3,4,5,6,7,8,9,10,11,12,", startHour);
  GP.SELECT("startMin", "0,1,2,3,4,5,6,7,8,9,10,11,12,\
  13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,\
  28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,\
  43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,0", startMin);
  GP.SELECT("wateringMin", "0,1,2,3,4,5,6,7,8,9,10,11,12,\
  13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,\
  28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,\
  43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,0", wateringMin);
  GP.BREAK();

  GP.RELOAD_CLICK("idx");

  GP.BREAK();
  GP.HR();

  GP.SWITCH("treeWatering", 1, COLORTHEME);

  GP.BREAK();
  String s;
  s += F("<a href='");
  s += "/ota_update";
  s += F("'>");
  s += "Firmware update";
  s += F("</a>");
  GP.SEND(s);

  GP.BUILD_END();
} // webPageBuild()


// обрабатываем действия на гайвер портале
void webPageAction() {

  if (ui.update()) {
    //-    ui.updateInt("label1", totalPhotos);
    //-    ui.updateInt("label2", sec);
    ui.updateInt("uptimehh", uptimeHour);
    ui.updateInt("uptimemm", uptimeMin);
    ui.updateInt("nowhh", nowhh);
    ui.updateInt("nowmm", nowmm);
    ui.updateInt("nowss", nowss);
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
    ui.clickInt("startMin", startMin);  // поймать и записать индекс
    ui.clickInt("startMin", startHour);  // поймать и записать индекс
    ui.clickInt("wateringMin", wateringMin);  // поймать и записать индекс

  }//click()
}//webPageAction()


// инициализируем гайвер портал
void webUI_Init() {
  ui.attachBuild(webPageBuild);
  ui.attach(webPageAction);
  ui.start();
  ui.enableOTA();   // без пароля
  if (!LittleFS.begin()) Serial.println("FS Error");
  ui.downloadAuto(true);
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

void NTP_Init() {
  ntp.begin();
  delay(300);
  if (!ntpUpdate()) Serial.println("ntp can't update");

}

bool ntpUpdate() {
  realyear = ntp.year();
  for (int i = 0; i < 10; i++) {
    byte err = ntp.status();
    if (!err && (realyear > 2022) && (realyear < 2040)) {
      //if ((realyear > 2022) && (realyear < 2035)) {
      nowss = ntp.second();
      nowmm = ntp.minute();
      nowhh =  ntp.hour();
      return 1;
    } else  {
      Serial.printf("year: %lu \t", realyear);
      Serial.printf("ntp error: %lu \n", err);
    }
  }//for
  return 0;
  //uint8_t realday = ntp.day();
  //uint8_t realmonth = ntp.month();
  //uint16_t realyear = ntp.year();
  //uint8_t realdayweek = ntp.dayWeek();
  //String timeString();            // получить строку времени формата ЧЧ:ММ:СС
  //String dateString();            // получить строку даты формата ДД.ММ.ГГГГ
  //uint8_t ntp.status();               // получить статус системы


  // 0 - всё ок
  // 1 - не запущен UDP
  // 2 - не подключен WiFi
  // 3 - ошибка подключения к серверу
  // 4 - ошибка отправки пакета
  // 5 - таймаут ответа сервера
  // 6 - получен некорректный ответ сервера}//ntpUpdate()
}//ntpUpdate()


void pinsBegin() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);
}  //pinsBegin()

void times_tick() {
  // инкрементируем Аптайм
  if ((ms - prevMs) > 1000L) {
    prevMs = ms;
    sec++;
    uptimeSec++;
    nowss++;
    //Serial.print("state:");
    //Serial.println(state);
    // обонвлять ntp пока не получим нормальный год
    if((realyear < 2022) || (realyear > 2040)) ntpUpdate();
    if (uptimeSec > 59) {
      uptimeSec = 0;
      uptimeMin++;
      if (uptimeMin > 59) {
        uptimeMin = 0;
        uptimeHour++;
        if (!ntpUpdate()) Serial.println("ntp can't update");
        Serial.printf("%lu uptime hours %lu mins\n", uptimeHour, uptimeMin);
      }
    }  //if sec
    if (nowss > 59) {
      nowss = 0;
      nowmm++;
      if (nowmm > 59) {
        nowmm = 0;
        nowhh++;
        if (nowhh > 23) nowhh = 0;
        Serial.printf("%lu real hours %lu mins\n", nowhh, nowmm);
      }
    }  //if sec

  }//ms 1000
}//times_tick()


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
  times_tick();  yield();
  ms = millis();

}//loop
