//
// �L�l�GTiny BASIC for Windows 10 �G���[���b�Z�[�W��`
// 2021/06/28,Arduino STM32�ł���̈ڐA
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
  "Cannot use system command", // v0.83 ���b�Z�[�W���ύX
  "Illegal value",      // �ǉ�
  "Out of range value", // �ǉ�
  "Syntax error",
  "Internal error",
  "Break",
  "Line too long",
};
