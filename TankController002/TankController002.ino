
#include <FlexiTimer2.h>
#include <SPI.h>


volatile boolean turretLRLeft = true;
volatile int turretLRPower = 0;
const byte turretLRCSPin = 8;
const byte turretLRPin = 10;
const byte turretLROnPin = 11;

volatile boolean canonUDUp = true;
volatile int canonUDPower = 0;
const byte canonUDCSPin = 9;
const byte canonUDPin = 12;
const byte canonUDOnPin = 7;

volatile boolean shootButton = false;

volatile byte timeToStop = 0;

void spiWrite(byte pin, int value) {

  digitalWrite(pin, LOW);
  SPI.transfer(0);
  SPI.transfer(value);
  digitalWrite(pin, HIGH);
}


void TimerRun() {
  if (timeToStop > 0) {
    timeToStop--;
    Serial.print("StopCount=");
    Serial.print(timeToStop);

    Serial.print(" turretLR=");
    if (turretLRLeft) {
      Serial.print("L");
    } else {
      Serial.print("R");
    }
    Serial.print(turretLRPower);

    Serial.print(" canonUD=");
    if (canonUDUp) {
      Serial.print("U");
    } else {
      Serial.print("D");
    }
    Serial.print(canonUDPower);

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
  Serial.begin(115200);
  Serial3.begin(115200);

  pinMode(turretLRCSPin, OUTPUT);
  digitalWrite(turretLRCSPin , HIGH);
  pinMode(turretLRPin, OUTPUT);
  digitalWrite(turretLRPin , HIGH);
  pinMode(turretLROnPin, OUTPUT);
  digitalWrite(turretLROnPin , HIGH);

  pinMode(canonUDCSPin, OUTPUT);
  digitalWrite(canonUDCSPin , HIGH);
  pinMode(canonUDPin, OUTPUT);
  digitalWrite(canonUDPin , HIGH);
  pinMode(canonUDOnPin, OUTPUT);
  digitalWrite(canonUDOnPin , HIGH);


  SPI.begin();
  spiWrite(turretLRCSPin, 0);
  spiWrite(canonUDCSPin, 0);


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
            if (tmpSerial[9] - 0x30 == csum % 10) {
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

  static int beforeTurretLRPower;
  static int beforeCanonUDPower;

  if (timeToStop == 0) {
    //制御停止処理
    turretLRLeft = true;
    turretLRPower = 0;
    beforeTurretLRPower = 0;
    canonUDUp = true;
    canonUDPower = 0;
    beforeCanonUDPower = 0;

    digitalWrite(turretLRPin , HIGH);
    digitalWrite(turretLROnPin , HIGH);
    digitalWrite(canonUDPin , HIGH);
    digitalWrite(canonUDOnPin , HIGH);

    spiWrite(turretLRCSPin, 0);
    spiWrite(canonUDCSPin, 0);

    shootButton = false;

  } else {
    //砲塔旋回
    if (turretLRLeft) {
      if (digitalRead(turretLRPin) == LOW &&
          turretLRPower != 0 && beforeTurretLRPower != 0) { //極性が急激に変わるときは一旦OFFにする
        digitalWrite(turretLROnPin , HIGH);
        spiWrite(turretLRCSPin, 0);
        delay(500);
      }
      digitalWrite(turretLRPin , HIGH);
    } else {
      if (digitalRead(turretLRPin) == HIGH &&
          turretLRPower != 0 && beforeTurretLRPower != 0) { //極性が急激に変わるときは一旦OFFにする
        digitalWrite(turretLROnPin , HIGH);
        spiWrite(turretLRCSPin, 0);
        delay(500);
      }
      digitalWrite(turretLRPin , LOW);
    }
    if (turretLRPower == 0) {
      digitalWrite(turretLROnPin , HIGH);
    } else {
      digitalWrite(turretLROnPin , LOW);
    }
    spiWrite(turretLRCSPin, turretLRPower);
    beforeTurretLRPower = turretLRPower;

    //砲塔上下
    if (canonUDUp) {
      if (digitalRead(canonUDPin) == LOW &&
          canonUDPower != 0 && beforeCanonUDPower != 0) { //極性が急激に変わるときは一旦OFFにする
        digitalWrite(canonUDOnPin , HIGH);
        spiWrite(canonUDCSPin, 0);
        delay(500);
      }
      digitalWrite(canonUDPin , HIGH);
    } else {
      if (digitalRead(canonUDPin) == HIGH &&
          canonUDPower != 0 && beforeCanonUDPower != 0) { //極性が急激に変わるときは一旦OFFにする
        digitalWrite(canonUDOnPin , HIGH);
        spiWrite(canonUDCSPin, 0);
        delay(500);
      }
      digitalWrite(canonUDPin , LOW);
    }
    if (canonUDPower == 0) {
      digitalWrite(canonUDOnPin , HIGH);
    } else {
      digitalWrite(canonUDOnPin , LOW);
    }
    spiWrite(canonUDCSPin, canonUDPower);
    beforeCanonUDPower = canonUDPower;
  }

}
