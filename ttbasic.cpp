/*
  TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
  2017/03/22 Modified by Tamakichi、for Arduino STM32
  2018/09/16 修正,Arduino STM32 R20170323の非サポートに変更
  2021/06/25 修正,Windows 10に移植
 */
#include <windows.h>

// Standard library C-style
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "ttconfig.h"

// コンソール画面でCTRL+Cを無効にする
// (参考) http://www.ne.jp/asahi/hishidama/home/tech/c/windows/ConsoleCtrlHandler.html
BOOL WINAPI controlHandler(DWORD type) {
	return TRUE;
}

void basic();
void setup(void){

}

void loop(void){
  basic();
}

void main() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))     {
        return;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))     {
        return;
    }

	SetConsoleCtrlHandler(controlHandler, TRUE);

	setup();
	for (;;) {
		loop();
	}
}
