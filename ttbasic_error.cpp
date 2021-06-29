//
// 豊四季Tiny BASIC for Windows 10 エラーメッセージ定義
// 2021/06/28,Arduino STM32版からの移植
//

#include "Arduino.h"
const char* errmsg[]  = {
  "OK",
  "Devision by zero",
  "Overflow",
  "Subscript out of range",
  "Icode buffer full",
  "List full",
  "GOSUB too many nested",
  "RETURN stack underflow",
  "FOR too many nested",
  "NEXT without FOR",
  "NEXT without counter",
  "NEXT mismatch FOR",
  "FOR without variable",
  "FOR without TO",
  "LET without variable",
  "IF without condition",
  "Undefined line number or label",
  "\'(\' or \')\' expected",
  "\'=\' expected",
  "Cannot use system command", // v0.83 メッセージ文変更
  "Illegal value",      // 追加
  "Out of range value", // 追加
  "Syntax error",
  "Internal error",
  "Break",
  "Line too long",
};
