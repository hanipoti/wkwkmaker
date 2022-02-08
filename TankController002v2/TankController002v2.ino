
#include <FlexiTimer2.h>
#include <SPI.h>

//ボードの出力ピンの並び順 5v,10,11,12,7,g
//FlexiTimer2によりPWM不可 9,10
const byte turretLR_DIRPin = 10;
const byte turretLR_PWMPin = 11;
const byte canonUD_DIRPin = 12;
const byte canonUD_PWMPin = 7;

volatile boolean turretLRLeft = true;
volatile int turretLRPower = 0;
volatile boolean turretLRLeftAdjust = true;
volatile int turretLRPowerAdjust = 0;

volatile boolean canonUDUp = true;
volatile int canonUDPower = 0;
volatile boolean canonUDUpAdjust = true;
volatile int canonUDPowerAdjust = 0;


const byte DigitalPotentiometerLeftCSPin = 8;
const byte DigitalPotentiometerRightCSPin = 9;


volatile boolean shootButton = false;


//通信が一定時間無かったら制御を止めるためのフラグ
volatile byte timeToStop = 0;


//デジタルポテンショメーターへの設定書き込み
void spiWrite(byte pin, int value) {

  digitalWrite(pin, LOW);
  SPI.transfer(0);
  SPI.transfer(value);
  digitalWrite(pin, HIGH);
}

//モーターが急激に動かないように、指示値を最大変動値までしか動かないように調整する
void AdjustTheControl(boolean iDIR, int iPower, boolean *oDIR ,int *oPower){

  int adjustSize = 30; //最大変動値

  int powerI = iPower;
  if (iDIR){
    powerI = powerI * -1;
  }
  int powerO = *oPower;
  if (*oDIR){
    powerO = powerO * -1;
  }

  if (abs(powerI - powerO) <= adjustSize){
    powerO = powerI;
  }else{
    if (powerI > powerO){
      powerO = powerO + adjustSize;
    }else{
      powerO = powerO - adjustSize;
    }
  }

  *oDIR = (powerO < 0);
  *oPower = abs(powerO);
}

void TimerRun() {

  AdjustTheControl(turretLRLeft, turretLRPower, &turretLRLeftAdjust, &turretLRPowerAdjust);
  AdjustTheControl(canonUDUp, canonUDPower, &canonUDUpAdjust, &canonUDPowerAdjust);
  
  if (timeToStop > 0) {
    timeToStop--;   //時間経過フラグを減らす
  }

  if (turretLRPowerAdjust > 0 || canonUDPowerAdjust > 0){
    Serial.print("StopCount=");
    Serial.print(timeToStop);

    Serial.print(" turretLR=");
    if (turretLRLeftAdjust) {
      Serial.print("L");
    } else {
      Serial.print("R");
    }
    Serial.print(turretLRPowerAdjust);

    Serial.print(" canonUD=");
    if (canonUDUpAdjust) {
      Serial.print("U");
    } else {
      Serial.print("D");
    }
    Serial.print(canonUDPowerAdjust);

    Serial.print(" ShootButton=");
    if (shootButton) {
      Serial.print("ON");
    } else {
      Serial.print("OFF");
    }

    Serial.println();
  } else {
    //Serial.println("stop");
  }
}

void setup() {
  Serial.begin(115200);  //USB
  Serial3.begin(115200); //ブルートゥース

  pinMode(turretLR_PWMPin, OUTPUT);
  digitalWrite(turretLR_PWMPin , LOW);
  pinMode(turretLR_DIRPin, OUTPUT);
  digitalWrite(turretLR_DIRPin , LOW);

  pinMode(canonUD_PWMPin, OUTPUT);
  digitalWrite(canonUD_PWMPin , LOW);
  pinMode(canonUD_DIRPin, OUTPUT);
  digitalWrite(canonUD_DIRPin , LOW);


  pinMode(DigitalPotentiometerLeftCSPin, OUTPUT);
  digitalWrite(DigitalPotentiometerLeftCSPin , HIGH);
  pinMode(DigitalPotentiometerRightCSPin, OUTPUT);
  digitalWrite(DigitalPotentiometerRightCSPin , HIGH);
  SPI.begin();
  spiWrite(DigitalPotentiometerLeftCSPin, 0);
  spiWrite(DigitalPotentiometerRightCSPin, 0);


  FlexiTimer2::stop();
  FlexiTimer2::set(100, TimerRun) ; // ms毎に割込み発生
  FlexiTimer2::start();
}

void loop() {

  if (Serial3.available() >= 13) {
    if (Serial3.read() == 'W') {
      if (Serial3.read() == 'M') {
        byte csum = 0;   //チェックsum用
        switch (Serial3.read()) {
          case 'S':   //制御停止
            Serial.print("停止命令受信");
            Serial.println();
            timeToStop = 0;   // コンマ秒指定
            break;

          case 'T':    //砲塔制御
            char tmpSerial[10];
            for (byte i = 0; i < 10; i++) { //データ9桁＋チェックSUM1桁
              tmpSerial[i] = Serial3.read();
            }
            for (byte i = 1; i < 9; i++) {
              if (i != 4) {
                csum = csum + (tmpSerial[i] - 0x30);  //char(ASCII) → byte(数値)
              }
            }
            if (tmpSerial[9] - 0x30 == csum % 10) {  //チェックSUM確認
              
              //砲塔旋回
              if (tmpSerial[0] == 'L') {
                turretLRLeft = true;
              } else {
                turretLRLeft = false;
              }
              turretLRPower = (tmpSerial[1] - 0x30) * 100 +
                              (tmpSerial[2] - 0x30) * 10 +
                              (tmpSerial[3] - 0x30);
              
              //主砲上下
              if (tmpSerial[4] == 'U') {
                canonUDUp = true;
              } else {
                canonUDUp = false;
              }
              canonUDPower = (tmpSerial[5] - 0x30) * 100 +
                             (tmpSerial[6] - 0x30) * 10 +
                             (tmpSerial[7] - 0x30);
              
              //発射ボタン
              if (tmpSerial[8] == '1') {
                shootButton = true;
              } else {
                shootButton = false;
              }

              timeToStop = 5;   // コンマ秒指定
              
            } else {
              Serial.print("受信データエラー Turret=");
              for (int i = 0; i < 10; i++) {
                if (i == 9) Serial.print(" sum=");
                Serial.print(tmpSerial[i]);
              }
              Serial.println();
            }

            break;
        }
      }
    }
  }


  if (timeToStop == 0) {
    //制御停止処理
    turretLRPower = 0;
    canonUDPower = 0;

    shootButton = false;
  }

  // モータードライバを制御する
  digitalWrite(turretLR_DIRPin , turretLRLeftAdjust);
  analogWrite(turretLR_PWMPin , constrain(turretLRPowerAdjust * 2, 0 , 255));

  digitalWrite(canonUD_DIRPin , canonUDUpAdjust);
  analogWrite(canonUD_PWMPin , constrain(canonUDPowerAdjust * 2, 0 , 255));
  

}
