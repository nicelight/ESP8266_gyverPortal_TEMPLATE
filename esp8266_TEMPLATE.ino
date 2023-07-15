#define USEMEMORY  //  использовать  EEPROM для сохранения настроек
// #define ESP_WIFIAP // если нужно не подключаться к wifi а создать свою точку доступа

// Определяем точкy доступа
#define WIFI_ACCES_POINT esp32              // имя и пароль создаваемой wifi сети
#define WIFI_PASSWIRD 1234567890
#define WIFI_IS_HIDDEN 0                        // 1 -скрытая точка, 0 - видимая
#define CHANNEL 1                          // канал 1..11
#define MAX_CONNECTION 4                   // макимальное количество подключений 1..8
#define COLORTHEME "#40c2e6" //цвет кнопочек голубенький

//подключение релюшек включения воды
#define TAP1 10
#define TAP2 9
#define TAP3 8
#define TAP4 11


// Подключем библиотеки
#include <ESP8266WiFi.h>
#include <GyverNTP.h>
GyverNTP ntp(5); // +5 GMT
#include <WiFiClient.h>
//#include <EEPROM.h>
#include <EEManager.h>
#include <GyverPortal.h>
#include <LittleFS.h>
GyverPortal ui(&LittleFS);


#ifdef ESP_WIFIAP
bool wifiHidden = WIFI_IS_HIDDEN;
byte maxConnection = MAX_CONNECTION;
bool wifiChanel = CHANNEL;
const char* ssidAP = "esp";
const char* passwordAP = "1234567890"; // пароль от 1 до 0
#else
// если не точка, то wifi client
const char* ssid = "kuso4ek_raya";
const char* password = "1234567812345678";
//const char* ssid = "esp32";
//const char* password = "1234567890";
#endif


#define LED_PIN 5 // esp32 dev module LED_BUILTIN
uint8_t state = 0; // автомат состояний
bool sensor = 0;
GPtime upd_UpTime;
unsigned int localPort = 2390;  // local port to listen for UDP packets

uint32_t sec = 1;
uint16_t realyear = 0;
uint8_t nowhh = 0;
uint8_t nowmm = 0;
uint8_t nowss = 0;
uint8_t uptimeHour = 0, uptimeMin = 0, uptimeSec = 0;

bool isWatering = 0; // флаг что сейчас полив происходит
uint16_t wateringLeft = 0; // сколько еще поливаем на текущий момент

// структура настроек
struct Settings {
  bool autoWatering;
  int16_t wateringMin; // сколько минут поливать

  uint8_t startMin; // начало полива
  uint8_t startHour;

  uint8_t stopMin;
  uint8_t stopHour;
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
  GP.UPDATE("uptimehh, uptimemm, nowhh, nowmm, nowss, wateringLeft, isWatering, autoWatering");// какие поля нужно обновлять

  GP.TITLE("Меню");
  GP.BREAK(); GP.HR(); GP.HR();

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
  GP.BREAK(); GP.HR();

  // GP.BUTTON("btn", "Полить сейчас", "", COLORTHEME); // "" - id компонента

  GP.LABEL("Автополив деревьев");
  GP.SWITCH("autoWatering", set.autoWatering, COLORTHEME); //
  GP.BREAK();
  GP.LABEL("Начать полив");
  /// начало полива
  GP.SELECT("startHour", "00,01,02,03,04,05,06,07,08,09,10,11,12,\
  13,14,15,16,17,18,19,20,21,22,23", set.startHour);
  GP.SELECT("startMin", "00,01,02,03,04,05,06,07,08,09,10,11,12,\
  13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,\
  28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,\
  43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,0", set.startMin);
  GP.BREAK();

  GP.LABEL("Поливать");
  //  GP.SELECT("wateringMin", "0,1,2,3,4,5,6,7,8,9,10,15,20,\
  //  30,40,50,60,70,80,90,100,120,140,160,180,200,250,300", set.wateringMin);
  GP.SELECT("wateringMin", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,\
  21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,\
  51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70", set.wateringMin);

  GP.LABEL("минут.");
  GP.BREAK(); GP.HR();

  GP.LABEL("Полив");
  GP.SWITCH("isWatering", isWatering, COLORTHEME); //

  GP.LABEL("еще ");
  GP.SELECT("wateringLeft", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,\
  21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,\
  51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70", wateringLeft);
  GP.LABEL("минут.");
  GP.BREAK();
  /*
    GP.LABEL("Окончить полив");
    GP.SELECT("stopHour", "00,01,02,03,04,05,06,07,08,09,10,11,12,\
    13,14,15,16,17,18,19,20,21,22,23", set.stopHour);
    GP.SELECT("stopMin", "00,01,02,03,04,05,06,07,08,09,10,11,12,\
    13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,\
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,\
    43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,0", set.stopMin);
  */

  GP.BREAK(); GP.HR(); GP.BREAK();



  // вызов алерта по кнопке
  GP.ALERT("alert_saved", "Настройки сохранены"); // выскакивает по нажатию на кнопку
  GP.BUTTON_MINI("saveBtn", "Сохранить настройки", "", COLORTHEME); // "" - id компонента
  GP.UPDATE_CLICK("alert_saved", "saveBtn");
  GP.BREAK(); GP.BREAK(); GP.HR(); GP.HR();

  // загрузка прошивки
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
    ui.updateInt("uptimehh", uptimeHour); // аптайм
    ui.updateInt("uptimemm", uptimeMin);

    ui.updateInt("nowhh", nowhh); // текущее время
    ui.updateInt("nowmm", nowmm);
    ui.updateInt("nowss", nowss);

    ui.updateInt("wateringLeft", wateringLeft);
    ui.updateInt("isWatering", isWatering);
    ui.updateBool("autoWatering", set.autoWatering);
    //алерт - настройки сохранены
    if (ui.update("alert_saved")) {
#ifdef USEMEMORY
      memory.update(); //обновление настроек в EEPROM памяти
      ui.answer(1); // показать алерт (текст у него задан в конструкторе)
#endif
    }
  }//update()

  if (ui.click()) {
    Serial.println("UI CLICK detected");
    // переключатель автополива
    if (ui.clickBool("autoWatering", set.autoWatering)) {
    }
    //включение полива прямо сейчас
    if (ui.clickBool("isWatering", isWatering)) {
    }
    // по нажатию на кнопку
    if (ui.click("btn")) {
      ledBlink();
    }
    ui.clickInt("startMin", set.startMin);  // поймать и записать индекс
    ui.clickInt("startHour", set.startHour);  // поймать и записать индекс
    ui.clickInt("wateringMin", set.wateringMin);  // поймать и записать индекс
    ui.clickInt("stopMin", set.stopMin);  // поймать и записать индекс
    ui.clickInt("stopHour", set.stopHour);  // поймать и записать индекс

    ui.clickInt("wateringLeft", wateringLeft);  // поймать и записать индекс
  }//click()

}//webPageAction()


// инициализируем гайвер портал
void webUI_Init() {
  ui.attachBuild(webPageBuild);
  ui.attach(webPageAction);
  ui.start(); //  ui.start("esp");
  ui.enableOTA();   // без пароля
  if (!LittleFS.begin()) Serial.println("FS Error");
  ui.downloadAuto(true);
}//webUI_Init()


void wifiKeep() {
  static uint8_t sw = 0; // автомат
  static uint8_t i = 0; // cчетчик
  static uint32_t prevMs = 0;
  /* debug
    static uint8_t prevsw = 0; // автомат
    if (sw != prevsw ){
      prevsw = sw;
      Serial.print("sw = ");
      Serial.println(sw);
    } */
  switch (sw) {
    case 0:
      sw = 3;
      prevMs = millis();
      break;
    // инициализация подключения
    case 3:
      if (millis() - prevMs >= 200L) {
        prevMs = millis();
#ifdef ESP_WIFIAP
        // Инициируем точку доступа WiFi
        WiFi.softAP(ssidAP, passwordAP, wifiChanel, wifiHidden, maxConnection);
        // IPAddress myIP = WiFi.softAPIP();
        Serial.print("Wifi access point started. IP: ");
        Serial.println(WiFi.softAPIP());
        sw = 10; // на повтор инициализации
#else
        if  (WiFi.status() != WL_CONNECTED) {
          Serial.println();
          Serial.print(millis() / 1000);
          Serial.print(" Try conn wifi to "); Serial.print(ssid); Serial.print(":");
          WiFi.mode(WIFI_STA);
          WiFi.begin(ssid, password);
          sw = 5; // проверяем подключение
        } else sw = 10; // на повтор инициализации
#endif
      }//if ms
      break;
    // пока не подключится, пробуем переподключаться
    case 5:
      if (millis() - prevMs >= 1000L) {
        prevMs = millis();
        if  (WiFi.status() != WL_CONNECTED) {
          Serial.print(".");
          i++;
          // после 10 попыток на отдых
          if (i > 20) {
            i = 0;
            // if (i > 40) ESP.restart();
            sw = 10; // нет подключения, ждем 20 секунд
          }
        }
        else {
          Serial.print("Connected. \nIP: ");
          Serial.println(WiFi.localIP());
          sw = 10;
        }
      }//ms
      break;
    // длительное ожидание и повторная проверка подключения
    case 10:
      if (millis() - prevMs >= 20000L) {
        prevMs = millis();
        sw = 3;
      }
      break;
  }//switch(sw)
}//wifiKeep()


void NTP_Init() {
  //  ntp.setHost("ntp1.stratum2.ru");
  ntp.begin();
  delay(300);
}


void ntpUpdate() {
  static uint8_t sw = 0; // автомат
  static uint32_t prevMs = 0;
  static byte err = 0;
  /* debug
    static uint8_t prevsw = 0; // автомат
    if (sw != prevsw ) {
     prevsw = sw;
     Serial.print("sw = "); Serial.println(sw);
    } */
  if (WiFi.status() == WL_CONNECTED) {
    ntp.tick();
    switch (sw) {
      case 0:
        sw = 5;
        prevMs = millis();
        break;
      case 5:
        if (millis() - prevMs >= 5000L) {
          prevMs = millis();
          err = ntp.status();
          realyear = ntp.year();
          if (!err && (realyear > 2022) && (realyear < 2040)) {
            //if ((realyear > 2022) && (realyear < 2035)) {
            nowss = ntp.second();
            nowmm = ntp.minute();
            nowhh =  ntp.hour();
            sw = 10;
          } else  {
            Serial.printf("year: %lu \t", realyear);
            Serial.printf("ntp error: %lu \n", err);
            //sw = 3;
            sw = 5;
          }
        }//ms
        break;
      //ждем час
      case 10:
        if (millis() - prevMs >= 3600000L) {
          prevMs = millis();
          sw = 5;
        }
        break;

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
    }//switch(sw)
  }//if wifi status == con
}//ntpUpdate()


void pinsBegin() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);
  pinMode(TAP1, OUTPUT);
  digitalWrite(TAP1, 0);
}  //pinsBegin()


void times_tick() {
  static uint32_t prevMs = 0, stateMs = 0;
  // инкрементируем Аптайм
  if ((millis() - prevMs) > 1000L) {
    prevMs = millis();
    sec++;
    uptimeSec++;
    nowss++;
    //Serial.print("state:");
    //Serial.println(state);
    // обонвлять ntp пока не получим нормальный год
    if ((realyear < 2022) || (realyear > 2040)) ntpUpdate();
    if (uptimeSec > 59) {
      uptimeSec = 0;
      uptimeMin++;
      if (uptimeMin > 59) {
        uptimeMin = 0;
        uptimeHour++;
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
}// times_tick()


void autoWatering_tick() {
  static uint8_t prevmm = 0;
  static uint8_t sw = 0; // автомат
  static uint8_t i = 0; // cчетчик
  static uint32_t prevMs = 0;
  static uint16_t prevwateringLeft = 0;
  //debug
  static uint8_t prevsw = 0; // автомат
  if ((sw != prevsw ) || (prevwateringLeft != wateringLeft)) {
    prevwateringLeft = wateringLeft;
    prevsw = sw;
    Serial.print("sw=");
    Serial.print(sw);
    Serial.print(" wateringLeft=");
    Serial.print(wateringLeft);
    Serial.print(" IS=");
    Serial.print(isWatering);
    Serial.print("\ttime ");
    Serial.print(nowhh);
    Serial.print(":");
    Serial.print(nowmm);
    Serial.println();

  }
  switch (sw) {
    case 0:
      sw = 3;
      prevMs = millis();
      break;
    // ждем, когда начать полив
    case 3:
      if (millis() - prevMs >= 1000L) {
        prevMs = millis();
        // если включен автополив и настало время; или если принудительно включили полив
        if ((set.autoWatering && set.wateringMin && (nowmm == set.startMin) && (nowhh == set.startHour))\
            || (isWatering && wateringLeft)) {
          // запомним сколько минут было когда включился  полив
          // как только значение минут изменится, декрементируем время полива
          prevmm = nowmm;
          // если не принудительно включили, установим время остатка полива
          if (!isWatering) wateringLeft = set.wateringMin;
          isWatering = 1;
          digitalWrite(TAP1, isWatering); // даем  ВОДИЧКУ
          sw = 5;
        }//
      }//if ms
      break;
    // отлистываем минуты полива назад, до нуля
    case 5:
      if (millis() - prevMs >= 1000L) {
        prevMs = millis();
        // декремент времени полива каждую минуту
        if (prevmm != nowmm) {
          prevmm = nowmm;
          wateringLeft--;
        }
        // время полива кончилось - останавливаемся
        if (!wateringLeft) {
          prevMs = millis();
          isWatering = 0;
          digitalWrite(TAP1, isWatering); // закрываем ВОДИЧКУ
          sw = 3;
        }
        //принудительно отключили воду  - останавливаемся и отключим автополив
        if (!isWatering) {
          prevMs = millis();
          // костыль - если отключили в первую минуту полива, чтобы не заглючило, отключаем автополив
          if (nowmm == set.startMin) set.autoWatering = 0;
          digitalWrite(TAP1, isWatering); // закрываем ВОДИЧКУ
          sw = 3;
        }
      }//ms
      break;

  }//switch (sw)
}//watering_tick


void checkloop() {
  // put your main code here, to run repeatedly:
  static uint32_t prevMs = 0;
  static uint16_t loopMs = 0;
  loopMs = millis() - prevMs;
  if (loopMs > 5) {
    Serial.print("loop = ");
    Serial.println(loopMs );
  }
  prevMs = millis();
}//checkloop()


void setup() {
  // put your setup code here, to run once:
  // Инициируем последовательный порт
  Serial.begin(115200);
  Serial.println("\n\n\n");
  EEPROM.begin(100);  // выделить память (больше или равно размеру даты)
  memory.begin(0, 'a');
  pinsBegin();
  wifiKeep();
  webUI_Init();
  NTP_Init();
}//setup

void loop() {
  wifiKeep(); yield();
  ui.tick();  yield();
  // ntpUpdate();  yield();
  memory.tick();  yield();
  times_tick();  yield();
  autoWatering_tick(); yield();

}//loop
