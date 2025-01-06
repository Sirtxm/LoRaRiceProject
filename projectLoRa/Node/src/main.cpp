#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <sensorRead.h>
#include <loraTxRx.h>
#include <Adafruit_INA219.h>
#include <EasyButton.h>
#include <EEPROM.h>
// #include <HCSR04.h>
#define ONE            1
#define TWO            2
#define THREE          3
#define FOUR           4
#define FIVE           5
#define SIX            6
#define SEVEN          7
#define BUTTON_MODE    2
#define BUTTON_SETZERO 4
#define LED_SETZERO    15
#define LED_STATUS_R     33
#define LED_STATUS_G     32
//====================================EEPROM Address==============================
#define addr_mode            0x10
#define addr_timeNormalMode  0x20
#define addr_timeDebugMode   0x30
#define addr_zeroDistance    0x40
#define addr_statusNode      0x50
gpio_num_t wakeup_gpio = GPIO_NUM_2;
gpio_num_t ledStatus_R_gpio = GPIO_NUM_33;
gpio_num_t ledStatus_G_gpio = GPIO_NUM_32;
// Adafruit_INA219 ina219;
bool statusNode = false;
bool stateSetZero = false;
bool mode = false;
float timeNormalMode = 1;
float timeDebugMode = 3;
bool ledSetZeroState = true;
bool SuccessReceived = false;
int count = 0;
int state = 0;
bool stateSendData = false;
int countSendData = 0;
int swPumpPressed = 0;
String Mymessage = "";

unsigned long previousMillisSw = 0;
unsigned long previousMillisLed = 0;
unsigned long buttonPressStartTime = 0;
volatile unsigned long lastDebounceTimeMode = 0; // เวลาล่าสุดที่ debounce
volatile unsigned long debounceDelayMode = 200; // เวลา debounce (milliseconds)
volatile unsigned long lastDebounceTimeSetZeRo = 0; // เวลาล่าสุดที่ debounce
volatile unsigned long debounceDelaySetZeRo = 200; // เวลา debounce (milliseconds)
void onPressedForDuration();
void setMode(bool mode);
bool updateConfig(String receiveData);
void updateEEPROM();
void setZero();
ICACHE_RAM_ATTR void handleInterruptMode() {
  // ตรวจสอบ debounce
  if ((millis() - lastDebounceTimeMode) > debounceDelayMode) {
    mode = !mode;
    swPumpPressed = 1;
    // Serial.print("Modeeeeeeeeeeee ");
    // Serial.println(mode);
  }
  lastDebounceTimeMode = millis();
}
ICACHE_RAM_ATTR void handleInterruptSetZeRo() {
  // ตรวจสอบ debounce
  if ((millis() - lastDebounceTimeSetZeRo) > debounceDelaySetZeRo) {
    stateSetZero = !stateSetZero;
  }
  lastDebounceTimeSetZeRo = millis();
}
void check_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :  Serial.println("Wakeup caused by external signal using RTC_IO");
                                  Serial.print("mode IN");Serial.println(mode); 
                                  mode = !mode;
                                  Serial.print("mode OUT");Serial.println(mode);
                                  setMode(mode); 
                                  break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
void setup() {
  Serial.begin(115200);
  //HCSR04.begin(triggerPin, echoPin);
   // Setup VL53L1X
   while (!Serial) delay(10);

  Serial.println(F("Adafruit VL53L1X sensor demo"));

  Wire.begin(32,33);
  if (!vl53.begin(0x29, &Wire)) {
    Serial.print(F("Error on init of VL sensor: "));
    Serial.println(vl53.vl_status);
    while (1) delay(10);
  }
  Serial.println(F("VL53L1X sensor OK!"));

  Serial.print(F("Sensor ID: 0x"));
  Serial.println(vl53.sensorID(), HEX);

  if (!vl53.startRanging()) {
    Serial.print(F("Couldn't start ranging: "));
    Serial.println(vl53.vl_status);
    while (1) delay(10);
  }
  Serial.println(F("Ranging started"));

  // Valid timing budgets: 15, 20, 33, 50, 100, 200, and 500ms!
  vl53.setTimingBudget(500); // เอาไว้ปรับความเร็ว ในการ อ่านของ seensor ยิ่งมากยิ่งช้า ค่าหน่วงมาก ความแม่นยำสูง
  Serial.print(F("Timing budget (ms): "));
  Serial.println(vl53.getTimingBudget());
  vl53.VL53L1X_SetOffset(25); // เอาไว้ใช้ offset ค่า โดยค่าที่ใส่มันจะเป็นหน่วย มิลลิ 20 = 2 cm
  vl53.VL53L1X_SetXtalk(0); // เอาไว้ ตั้งค่า crosstalk
  vl53.VL53L1X_SetDistanceMode(2); // ปรับ ประสิทธิภาพระยะการอ่านของ sensor โดยที่ 1 = Short , 2 = Long
 
  if (vl53.VL53L1X_SetROI(5,5) == 0) {   //(ROIcenter,ROIsize)
    Serial.println(F("ตั้งค่า ROI สำเร็จ"));
  } else {
    Serial.println(F("ตั้งค่า ROI ล้มเหลว"));
  }
  //
  Serial.println("LoRa Node 1"); 
  // mySerial.begin(9600);
  EEPROM.begin(512); // You can change the size based on your requirements
  // Set data to EEPROM
  EEPROM.put(addr_mode, mode);
  EEPROM.put(addr_timeNormalMode, timeNormalMode);
  EEPROM.put(addr_timeDebugMode, timeDebugMode);
  EEPROM.put(addr_zeroDistance, zeroDistance);
  EEPROM.put(addr_statusNode, statusNode);
  EEPROM.commit();
  //EEPROM.get(addr_mode, mode);
  //EEPROM.get(addr_timeNormalMode, timeNormalMode);
  //EEPROM.get(addr_timeDebugMode, timeDebugMode);
  //EEPROM.get(addr_zeroDistance, zeroDistance);
  //EEPROM.get(addr_statusNode, statusNode);
  Serial.print("mode: ");  Serial.println(mode);
  Serial.print("timeNormalMode: ");  Serial.println(timeNormalMode);
  Serial.print("timeDebugMode: ");  Serial.println(timeDebugMode);
  Serial.print("zeroDistance: ");  Serial.println(zeroDistance);
  Serial.print("statusNode: ");  Serial.println(statusNode);
  pinMode(BUTTON_MODE,INPUT_PULLUP);
  pinMode(BUTTON_SETZERO,INPUT_PULLUP);
  pinMode(LED_SETZERO,OUTPUT);
  pinMode(LED_STATUS_R,OUTPUT);
  pinMode(LED_STATUS_G,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), handleInterruptMode, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_SETZERO), handleInterruptSetZeRo, RISING);
  check_wakeup_reason();
  esp_sleep_enable_ext0_wakeup(wakeup_gpio,0); //1 = High, 0 = Low
  setMode(mode);
  //esp_sleep_enable_timer_wakeup(10* 1000 * 1000);
  // esp_sleep_enable_timer_wakeup(5*60 * 1000 * 1000);
  state = TWO;
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(11);
  
  while (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    delay(500);
  }
  Serial.println("LoRa init succeeded.");

  while (!Serial) 
  {
    delay(1);
  }
 //////////////////////////////////////////////////////////////
  Wire.begin(32,33); // SDA = 21, SCL = 22 (เปลี่ยนตามขาของ ESP32)
  bool statusBme;
  statusBme = bme.begin(0x76);  
  if (!statusBme) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  bool statusIna;
  statusIna = bme.begin(0x76);  
  if (!statusIna) {
    Serial.println("Failed to find INA219 chip");
    while (1);
  }
  // Initialize the INA219.
  ina219.begin();
  if (! ina219.begin())
  {
    Serial.println("Failed to find INA219 chip");
    while (1) 
    {
      delay(10);
    }
  }
  /////////////////////////////////////////////////////////////
  Serial.println("Startttttt");
  delay(1000);
}

void loop() {
  // unsigned long currentMillisLed = millis();
  // if (currentMillisLed - previousMillisLed >= (timeNormalMode * 60 * 1000000)*2) {
  //   if(state == FIVE){

  //   }
  // }
  if(state == ONE){                   //อ่านค่าเซ็นเซอร์
    if(stateSetZero){
      setZero();
    }
    unsigned long currentMillisSw = millis();
    if (currentMillisSw - previousMillisSw >= 500) {
        previousMillisSw = currentMillisSw;
        updateSensorLaser();
        digitalWrite(LED_SETZERO,ledSetZeroState);
        ledSetZeroState = !ledSetZeroState;
        count++;
        Serial.println(count);
        Serial.print("adjustedDistance ");
        Serial.println(adjustedDistance);
    }
    if(count == 8*2){
      state = TWO;
    }
    // state = TWO;
    
  }else if(state == TWO){                   //อ่านค่าเซ็นเซอร์
    updateSensorTemp_Hum();
    updateSensorVoltage();
    updateSensorLaser();
    Serial.println(state); 
    state = THREE;
  }else if(state == THREE){             //ส่งข้อมูลไปเกตเวย์พร้อมขอค่าconfigต่างๆ
    // Serial.print("state:");
    // Serial.println(state);
    Serial.print(adjustedDistance);   Serial.print(temp);   Serial.print(hum);   Serial.print(busvoltage);   Serial.print(statusNode);   Serial.print(mode);   Serial.print(timeNormalMode);   Serial.println(timeDebugMode); 
    Mymessage = String(adjustedDistance)+","+String(temp)+","+String(hum)+","+String(busvoltage)+","+String(statusNode)+","+String(mode)+","+String(timeNormalMode)+","+String(timeDebugMode);
    Serial.println(Mymessage);
    Serial.println(adjustedDistance);
    sendMessage(Mymessage);
    delay(10);
    state = FOUR;
  }else if(state == FOUR){            //รอรับการยืนยันความถูกต้องของข้อมูลจากเกตเวย์
    // Serial.print("state:");
    // Serial.println(state);
    onReceive();
    if(sendSuccess == true && resendData == false){
      state = FIVE;
    }else if(sendSuccess == false && resendData == true){
      if(countSendData >= 5){
        statusNode = 0;
        digitalWrite(LED_STATUS_R,!statusNode);
        digitalWrite(LED_STATUS_G,statusNode);
        gpio_hold_en(ledStatus_R_gpio);
        gpio_hold_dis(ledStatus_G_gpio);
        Serial.print("OFF LED_STATUS");
        delay(10);
        state = SIX;
      }else{
        countSendData ++;
        Serial.print("countSendData");
        Serial.println(countSendData);
        state = THREE;
      }
    }
  }else if(state == FIVE){           //Updateข้อมูลconfigต่างๆจากเกตเวย์
    // Serial.print("state:");
    // Serial.println(state);
    if(updateConfig(receiveData)){
      Serial.print("statusNode: ");  Serial.println(statusNode);
      digitalWrite(LED_STATUS_R,!statusNode);
      digitalWrite(LED_STATUS_G,statusNode);
      gpio_hold_dis(ledStatus_R_gpio);
      gpio_hold_en(ledStatus_G_gpio);
      state = SIX;
    }else{
      state = THREE;
    }
  }else if(state == SIX){           //เก็บLog
    Serial.print("state:");
    Serial.println(state);
    Serial.print("state:"); Serial.println(state);
    Serial.print("mode");   Serial.println(mode);
    Serial.print("timeNormalMode"); Serial.println(timeNormalMode);
    Serial.print("timeDebugMode"); Serial.println(timeDebugMode);
    Serial.print("statusNode"); Serial.println(statusNode);
    updateEEPROM();
    state = SEVEN;
  }else if(state == SEVEN){           //เก็บLogและเข้าโหมดsleep
    setMode(mode);
    Serial.print("state:"); Serial.println(state);
    Serial.print("mode");   Serial.println(mode);
    Serial.print("timeNormalMode"); Serial.println(timeNormalMode);
    Serial.print("timeDebugMode"); Serial.println(timeDebugMode);
    Serial.print("timeDebugMode"); Serial.println(timeDebugMode);
    Serial.print("statusNode"); Serial.println(statusNode);
    delay(1000);
    esp_deep_sleep_start(); // เข้าสู่โหมด Deep Sleep
  
  }
  delay(10);
}
void setMode(bool mode){
  if(mode){
    updateEEPROM();
    esp_sleep_enable_timer_wakeup(timeNormalMode * 60 * 1000000);  // ตื่นขึ้นทุก 5 นาที
    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>NormalMode<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
  }else{
    updateEEPROM();
    esp_sleep_enable_timer_wakeup(timeDebugMode * 60 * 1000000);  // ตื่นขึ้นทุก 1 นาที
    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>DebugMode<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
  }
}
bool updateConfig(String receiveData){
  String data  = receiveData;
  char delimiter = ',';
  // แบ่งข้อมูลโดยใช้ String::indexOf() และ String::substring()
  String dataValues[7]; // สร้างอาร์เรย์ String มีขนาด 7 สมาชิก
  int startIndex = 0;
  int endIndex;
  for (int i = 0; i < 7; i++) {
      endIndex = data.indexOf(delimiter, startIndex);
      dataValues[i] = data.substring(startIndex, endIndex);
      startIndex = endIndex + 1;
  }
  // แบ่งข้อมูลสุดท้าย
  dataValues[7] = data.substring(startIndex);
  // data = "161,187,10,Pass,0,5,1";
  if(dataValues[3] == "Pass"){
    mode = dataValues[4].toInt();
    timeNormalMode = dataValues[5].toFloat();
    timeDebugMode = dataValues[6].toFloat();
    statusNode = dataValues[7].toFloat();
    Serial.println(">>>>>>>>>>>>>>>>>>>updateConfig<<<<<<<<<<<<<<<<<<<<<<");
    Serial.print("mode");
    Serial.println(mode);
    Serial.print("timeNormalMode");
    Serial.println(timeNormalMode);
    Serial.print("timeDebugMode");
    Serial.println(timeDebugMode);
    Serial.print("statusNode");
    Serial.println(statusNode);
    Serial.println(">>>>>>>>>>>>>>>>>>>updateConfig<<<<<<<<<<<<<<<<<<<<<<");
    return true;
  }else{
    return false;
  }
}
void updateEEPROM(){
  // EEPROM.put(addr_mode, mode);
  // EEPROM.put(addr_timeNormalMode, timeNormalMode);
  // EEPROM.put(addr_timeDebugMode, timeDebugMode);
  // EEPROM.put(addr_statusNode, statusNode);
  // EEPROM.commit();
  EEPROM.get(addr_mode, mode);
  EEPROM.get(addr_timeNormalMode, timeNormalMode);
  EEPROM.get(addr_timeDebugMode, timeDebugMode);
  EEPROM.get(addr_statusNode, statusNode);
  Serial.println(">>>>>>>>>>>>>>>>>>>updateEEPROM<<<<<<<<<<<<<<<<<<<<<<");
  Serial.print("mode: ");  Serial.println(mode);
  Serial.print("timeNormalMode: ");  Serial.println(timeNormalMode);
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}
void setZero(){
  zeroDistance = distance;
  EEPROM.put(addr_zeroDistance, zeroDistance);
  EEPROM.commit();
  EEPROM.get(addr_zeroDistance, zeroDistance);
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> Set ZERO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
  Serial.print("zeroDistance: ");  Serial.println(zeroDistance);
  stateSetZero = false;
}
