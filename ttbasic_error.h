//
// 豊四季Tiny BASIC for Windows 10 エラーメッセージ定義
// 2021/06/28,Arduino STM32版からの移植
//

#ifndef __ttbasic_error_h__
#define __ttbasic_error_h__

// エラーコード
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
  ERR_VALUE, // 追加
  ERR_RANGE, // 追加
  ERR_SYNTAX,
  ERR_SYS,
  ERR_CTR_C,
  ERR_LONG,  // 追加
};

extern const char* errmsg[];
  
#endif
