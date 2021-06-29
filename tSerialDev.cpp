//
// 豊四季Tiny BASIC for Arduino STM32 シリアルデバイス制御
// 2017/07/19 by たま吉さん
//

#include "Arduino.h"
#include "tSerialDev.h"

void tSerialDev::Serial_open(uint32_t baud) {
}

// シリアルポートクローズ
void tSerialDev::Serial_close() {
	return;
}
// シリアル1バイト出力
void tSerialDev::Serial_write(uint8_t c) {
}

// シリアル1バイト入力
int16_t tSerialDev::Serial_read() {
	return 0;
}

// シリアル改行出力 
void tSerialDev::Serial_newLine() {
}

// シリアルデータ着信チェック
uint8_t tSerialDev::Serial_available() {
  return 0;
}

// シリアルポートモード設定 
void tSerialDev::Serial_mode(uint8_t c, uint32_t b) {
}
