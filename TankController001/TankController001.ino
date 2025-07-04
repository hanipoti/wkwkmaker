//2025/07/04 左右展開をリレー制御からモータードライバに変更
//

#include <FlexiTimer2.h>

volatile char expansionCode[8]; //左右展開 モーターごとの制御信号を格納する 0=停止、1=伸び、2=縮み
volatile byte expStatePinNo[8]; //左右展開 モーター方向ピン
volatile byte expPowerPinNo[8]; //左右展開 モーターオンオフピン
volatile char liftCode[8];       //リフト制御 モーターごとの制御信号を格納する 0=停止、1=伸び、2=縮み
volatile byte liftStatePinNo[8]; //リフト制御 モーター方向ピン
volatile byte liftPowerPinNo[8]; //リフト制御 モーターオンオフピン
volatile byte timeToStop = 0;   //制御停止するまでのカウンター 信号を失った時に止める用

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
    //左右展開 モータードライバ制御 LOW→OFF
    expansionCode[i] = '0';
    expStatePinNo[i] = 38 + (i * 2);
    expPowerPinNo[i] = 39 + (i * 2);
    pinMode(expPowerPinNo[i], OUTPUT);
    digitalWrite(expPowerPinNo[i] , LOW);
    pinMode(expStatePinNo[i], OUTPUT);
    digitalWrite(expStatePinNo[i] , LOW);

    //リフト制御 モータードライバ制御 LOW→OFF
    liftCode[i] = '0';
    liftStatePinNo[i] = 22 + (i * 2);
    liftPowerPinNo[i] = 23 + (i * 2);
    pinMode(liftPowerPinNo[i], OUTPUT);
    digitalWrite(liftPowerPinNo[i] , LOW);
    pinMode(liftStatePinNo[i], OUTPUT);
    digitalWrite(liftStatePinNo[i] , LOW);
  }

  FlexiTimer2::stop();
  FlexiTimer2::set(100, TimerRun) ; // 0.1秒毎に割込み発生
  FlexiTimer2::start();

  Serial.println("Start");
}

void loop() {

  //シリアル通信受信　解析　展開
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

          case 'E':   //左右展開
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

  //モータードライバ制御
  if (timeToStop == 0) {
    //制御停止処理
    for (byte i = 0; i < 8; i++) {
      //左右展開
      expansionCode[i] = '0';
      digitalWrite(expPowerPinNo[i] , LOW);
      digitalWrite(expStatePinNo[i] , LOW);

      //リフト制御
      liftCode[i] = '0';
      digitalWrite(liftPowerPinNo[i] , LOW);
      digitalWrite(liftStatePinNo[i] , LOW);
    }

  } else {
    //制御実行
    for (byte i = 0; i < 8; i++) {
      //左右展開
      if (expansionCode[i] == '0') {
        digitalWrite(expPowerPinNo[i] , LOW);
        digitalWrite(expStatePinNo[i] , LOW);

      } else {
        if ((digitalRead(expPowerPinNo[i]) == HIGH) && (
              (digitalRead(expStatePinNo[i]) == LOW && expansionCode[i] == '2') ||
              (digitalRead(expStatePinNo[i]) == HIGH && expansionCode[i] == '1'))) {
          Serial.println("exp極性が変わる");
          digitalWrite(expPowerPinNo[i] , LOW);   //極性が変わるときは一旦OFFにする
          delay(100);
        }
        switch (expansionCode[i]) {
          case '1':
            digitalWrite(expStatePinNo[i] , LOW);
            digitalWrite(expPowerPinNo[i] , HIGH);
            break;
          case '2':
            digitalWrite(expStatePinNo[i] , HIGH);
            digitalWrite(expPowerPinNo[i] , HIGH);
            break;
          default:
            digitalWrite(expPowerPinNo[i] , LOW);
            digitalWrite(expStatePinNo[i] , LOW);
        }
      }

      //リフト制御
      if (liftCode[i] == '0') {
        digitalWrite(liftPowerPinNo[i] , LOW);
        digitalWrite(liftStatePinNo[i] , LOW);

      } else {
        if ((digitalRead(liftPowerPinNo[i]) == HIGH) && (
              (digitalRead(liftStatePinNo[i]) == LOW && liftCode[i] == '2') ||
              (digitalRead(liftStatePinNo[i]) == HIGH && liftCode[i] == '1'))) {
          Serial.println("lift極性が変わる");
          digitalWrite(liftPowerPinNo[i] , LOW);   //極性が変わるときは一旦OFFにする
          delay(100);
        }
        switch (liftCode[i]) {
          case '1':
            digitalWrite(liftStatePinNo[i] , LOW);
            digitalWrite(liftPowerPinNo[i] , HIGH);
            break;
          case '2':
            digitalWrite(liftStatePinNo[i] , HIGH);
            digitalWrite(liftPowerPinNo[i] , HIGH);
            break;
          default:
            digitalWrite(liftPowerPinNo[i] , LOW);
            digitalWrite(liftStatePinNo[i] , LOW);
        }
      }
    }
  }

}
