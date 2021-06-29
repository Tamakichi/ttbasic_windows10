//
// �L�l�GTiny BASIC for Windows 10 �G���[���b�Z�[�W��`
// 2021/06/28,Arduino STM32�ł���̈ڐA
//

#ifndef __ttbasic_error_h__
#define __ttbasic_error_h__

// �G���[�R�[�h
enum ErrorInfo {
  ERR_OK,
  ERR_DIVBY0,
  ERR_VOF,
  ERR_SOR,
  ERR_IBUFOF, ERR_LBUFOF,
  ERR_GSTKOF, ERR_GSTKUF,
  ERR_LSTKOF, ERR_LSTKUF,
  ERR_NEXTWOV, ERR_NEXTUM, ERR_FORWOV, ERR_FORWOTO,
  ERR_LETWOV, ERR_IFWOC,
  ERR_ULN,
  ERR_PAREN, ERR_VWOEQ,
  ERR_COM,
  ERR_VALUE, // �ǉ�
  ERR_RANGE, // �ǉ�
  ERR_SYNTAX,
  ERR_SYS,
  ERR_CTR_C,
  ERR_LONG,  // �ǉ�
};

extern const char* errmsg[];
  
#endif
