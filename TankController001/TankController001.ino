
#include <FlexiTimer2.h>

volatile char expansionCode[8];
volatile byte expStatePinNo[8];
volatile byte expPowerPinNo[8];
volatile char liftCode[8];
volatile byte liftStatePinNo[8];
volatile byte liftPowerPinNo[8];
volatile byte timeToStop = 0;

void TimerRun() {
  if (timeToStop > 0) {
    timeToStop--;
    Serial.print("StopCount=");
    Serial.print(timeToStop);

    Serial.print(" exp=");
    for (int i = 0; i < 8; i++) {
      Serial.print(expansionCode[i]);
    }

    Serial.print(" lift=");
    for (int i = 0; i < 8; i++) {
      Serial.print(liftCode[i]);
    }

    Serial.println();
  } else {
    //Serial.println("stop");
  }
}

void setup() {
  Serial.begin(115200);
  Serial3.begin(115200);

  for (byte i = 0; i < 8; i++) {
    expansionCode[i] = '0';
    expStatePinNo[i] = 38 + i;
    expPowerPinNo[i] = 38 + 8  + (7 - i);
    pinMode(expPowerPinNo[i], OUTPUT);
    digitalWrite(expPowerPinNo[i] , HIGH);
    pinMode(expStatePinNo[i], OUTPUT);
    digitalWrite(expStatePinNo[i] , HIGH);

    liftCode[i] = '0';
    liftStatePinNo[i] = 22 + i;
    liftPowerPinNo[i] = 22 + 8  + (7 - i);
    pinMode(liftPowerPinNo[i], OUTPUT);
    digitalWrite(liftPowerPinNo[i] , HIGH);
    pinMode(liftStatePinNo[i], OUTPUT);
    digitalWrite(liftStatePinNo[i] , HIGH);
  }

  FlexiTimer2::stop();
  FlexiTimer2::set(100, TimerRun) ; // ms毎に割込み発生
  FlexiTimer2::start();
}

void loop() {

  if (Serial3.available() >= 12) {
    if (Serial3.read() == 'W') {
      if (Serial3.read() == 'M') {
        byte csum = 0;   //チェックsum用
        switch (Serial3.read()) {
          case 'S':   //制御停止
            Serial.print("停止命令受信");
            Serial.println();
            timeToStop = 0;   // コンマ秒指定
            break;

          case 'E':   //展開制御
            char tmpExpNo[9];
            for (byte i = 0; i < 9; i++) { //データ８桁＋チェックSUM１桁
              tmpExpNo[i] = Serial3.read();
            }
            for (byte i = 0; i < 8; i++) {
              csum = csum + (tmpExpNo[i] - 0x30);  //char(ASCII) → byte(数値)
            }
            if (tmpExpNo[8] - 0x30 == csum % 10) {
              for (byte i = 0; i < 8; i++) {
                expansionCode[i] = tmpExpNo[i];
                liftCode[i] = '0';
              }
              timeToStop = 5;   // コンマ秒指定
            } else {
              Serial.print("受信データエラー exp=");
              for (int i = 0; i < 9; i++) {
                if (i == 8) Serial.print(" sum=");
                Serial.print(tmpExpNo[i]);
              }
              Serial.println();
            }
            break;

          case 'L':   //リフト制御
            char tmpLiftNo[9];
            for (byte i = 0; i < 9; i++) {
              tmpLiftNo[i] = Serial3.read();
            }
            for (byte i = 0; i < 8; i++) {
              csum = csum + (tmpLiftNo[i] - 0x30);  //char(ASCII) → byte(数値)
            }
            if (tmpLiftNo[8] - 0x30 == csum % 10) {
              for (byte i = 0; i < 8; i++) {
                expansionCode[i] = '0';
                liftCode[i] = tmpLiftNo[i];
              }
              timeToStop = 5;   // コンマ秒指定
            } else {
              Serial.print("受信データエラー lift=");
              for (int i = 0; i < 9; i++) {
                if (i == 8) Serial.print(" sum=");
                Serial.print(tmpLiftNo[i]);
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
    for (byte i = 0; i < 8; i++) {
      expansionCode[i] = '0';
      digitalWrite(expPowerPinNo[i] , HIGH);
      digitalWrite(expStatePinNo[i] , HIGH);

      liftCode[i] = '0';
      digitalWrite(liftPowerPinNo[i] , HIGH);
      digitalWrite(liftStatePinNo[i] , HIGH);
    }

  } else {
    //制御実行
    for (byte i = 0; i < 8; i++) {
      if (expansionCode[i] == '0') {
        digitalWrite(expPowerPinNo[i] , HIGH);
        digitalWrite(expStatePinNo[i] , HIGH);

      } else {
        if ((digitalRead(expPowerPinNo[i]) == LOW) && (
              (digitalRead(expStatePinNo[i]) == HIGH && expansionCode[i] == '2') ||
              (digitalRead(expStatePinNo[i]) == LOW && expansionCode[i] == '1'))) {
          Serial.println("exp極性が変わる");
          digitalWrite(expPowerPinNo[i] , HIGH);   //極性が変わるときは一旦OFFにする
          delay(100);
        }
        switch (expansionCode[i]) {
          case '1':
            digitalWrite(expStatePinNo[i] , HIGH);
            digitalWrite(expPowerPinNo[i] , LOW);
            break;
          case '2':
            digitalWrite(expStatePinNo[i] , LOW);
            digitalWrite(expPowerPinNo[i] , LOW);
            break;
          default:
            digitalWrite(expPowerPinNo[i] , HIGH);
            digitalWrite(expStatePinNo[i] , HIGH);
        }
      }

      if (liftCode[i] == '0') {
        digitalWrite(liftPowerPinNo[i] , HIGH);
        digitalWrite(liftStatePinNo[i] , HIGH);

      } else {
        if ((digitalRead(liftPowerPinNo[i]) == LOW) && (
              (digitalRead(liftStatePinNo[i]) == HIGH && liftCode[i] == '2') ||
              (digitalRead(liftStatePinNo[i]) == LOW && liftCode[i] == '1'))) {
          Serial.println("lift極性が変わる");
          digitalWrite(liftPowerPinNo[i] , HIGH);   //極性が変わるときは一旦OFFにする
          delay(100);
        }
        switch (liftCode[i]) {
          case '1':
            digitalWrite(liftStatePinNo[i] , HIGH);
            digitalWrite(liftPowerPinNo[i] , LOW);
            break;
          case '2':
            digitalWrite(liftStatePinNo[i] , LOW);
            digitalWrite(liftPowerPinNo[i] , LOW);
            break;
          default:
            digitalWrite(liftPowerPinNo[i] , HIGH);
            digitalWrite(liftStatePinNo[i] , HIGH);
        }
      }
    }
  }

}
