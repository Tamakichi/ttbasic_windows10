/*
 TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
 GNU General Public License
   2017/03/22, Modified by Tamakichi、for Windows 10
 */

// 2021/06/18 豊四季Tiny BASIC for Arduino STM32 V0.87からのWindows版に移植

#include "Arduino.h"
#include <windows.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "ttconfig.h"     // コンパイル定義
#include "tscreenBase.h"  // コンソール基本
#include "tTermscreen.h"  // シリアルコンソール

// エラーメッセージ定義
uint8_t err;// Error message index
#include "ttbasic_error.h"

#define STR_EDITION "Windows 10"
#define STR_VARSION "Edition V0.01"

// TOYOSHIKI TinyBASIC プログラム利用域に関する定義
#define SIZE_LINE 128    // コマンドライン入力バッファサイズ + NULL
#define SIZE_IBUF 128    // 中間コード変換バッファサイズ
#define SIZE_LIST 4096   // プログラム領域サイズ(4kバイト)
#define SIZE_VAR  210    // 利用可能変数サイズ(A-Z,A0:A6-Z0:Z6の26+26*7=208)
#define SIZE_ARRY 100    // 配列変数サイズ(@(0)～@(99)
#define SIZE_GSTK 20     // GOSUB stack size(2/nest) :10ネストまでOK
#define SIZE_LSTK 50     // FOR stack size(5/nest) :  10ネストまでOK
#define SIZE_MEM  1024   // 自由利用データ領域

// SRAMの物理サイズ(バイト)
#define SRAM_SIZE      20480 // STM32F103C8T6

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// 入出力キャラクターデバイス
#define CDEV_SCREEN   0  // メインスクリーン
#define CDEV_MEMORY   3  // メモリー

// **** スクリーン管理 *************
#define CON_MODE_DEVICE    0        // コンソールモード デバイス 
#define CON_MODE_SERIAL    1        // コンソールモード シリアル
#define SCSIZE_MODE_SERIAL 0        // スクリーンサイズモード指定なし（シリアルコンソールモード）
uint8_t* workarea = NULL;           // 画面用動的獲得メモリ

void initScreenEnv();
tscreenBase* sc;   // 利用デバイススクリーン用ポインタ
tTermscreen sc1;   // ターミナルスクリーン

// プロトタイプ宣言
char* getParamFname();
int16_t getNextLineNo(int16_t lineno);
void mem_putch(uint8_t c);
void iprint(uint8_t devno,uint8_t nonewln) ;
unsigned char* iexe();
short iexp(void);
void error(uint8_t flgCmd);
int isJMS( uint8_t *str, uint16_t nPos );


// **** 仮想メモリ定義 ****************
#define V_VRAM_TOP  0x0000
#define V_VAR_TOP   0x1900 // V0.84で変更
#define V_ARRAY_TOP 0x1AA0 // V0.84で変更
#define V_PRG_TOP   0x1BA0 // V0.84で変更
#define V_MEM_TOP   0x2BA0 // V0.84で変更
#define V_FNT_TOP   0x2FA0 // V0.84で変更
#define V_GRAM_TOP  0x37A0 // V0.84で変更
#define V_PRG2_TOP  0x4F40 // V0.87で追加

// 定数
#define CONST_HIGH   1
#define CONST_LOW    0
#define CONST_ON     1
#define CONST_OFF    0
#define CONST_LSB    LSBFIRST
#define CONST_MSB    MSBFIRST
#define SRAM_TOP     0x20000000

// Terminal control(文字の表示・入力は下記の3関数のみ利用)
#define c_getch( ) sc->get_ch()
#define c_kbhit( ) sc->isKeyIn()

// 指定デバイスへの文字の出力
//  c     : 出力文字
//  devno : デバイス番号 0:メインスクリーン 1:シリアル 2:グラフィック 3:、メモリー 4:ファイル
inline void c_putch(uint8_t c, uint8_t devno = CDEV_SCREEN) {
  if (devno == CDEV_SCREEN )
    sc->putch(c); // メインスクリーンへの文字出力
  else if (devno == CDEV_MEMORY)
   mem_putch(c); // メモリーへの文字列出力
} 

//  改行
//  devno : デバイス番号 0:メインスクリーン 1:シリアル 2:グラフィック 3:、メモリー 4:ファイル
inline void newline(uint8_t devno=CDEV_SCREEN) {
 if (devno== CDEV_SCREEN )
   sc->newLine();        // メインスクリーンへの文字出力
  else if (devno == CDEV_MEMORY )
    mem_putch('\n'); // メモリーへの文字列出力
}

// tick用支援関数
void iclt() {
  //systick_uptime_millis = 0;
}

// 乱数
short getrnd(short value) {
	return rand() % value;
}

// キーワードテーブル
const char *kwtbl[] = {
 "GOTO", "GOSUB", "RETURN", "FOR", "TO", "STEP", "NEXT", "IF", "END", "ELSE",       // 制御命令(10)
 ",", ";", ":", "\'","-", "+", "*", "/", "%", "(", ")", "$", "`","<<", ">>", "|", "&",  // 演算子・記号(31)
 ">=", "#", ">", "=", "<=", "!=", "<>","<", "AND", "OR", "!", "~", "^", "@",     
 "CLT", "WAIT",  // 時間待ち・時間計測コマンド(2) 
 "POKE",         // 記憶領域操作コマンド(1)
 "PRINT", "?", "INPUT", "CLS", "COLOR", "ATTR" ,"LOCATE", "REDRAW", "CSCROLL", // キャラクタ表示コマンド(9) 
 "CHR$", "BIN$", "HEX$", "DMP$", "STR$",                       // 文字列関数(5)
 "GETS", // 文字列入力
 "ABS", "MAP", "ASC", "FREE", "RND",  "INKEY", "LEN","BYTE",   // 数値関数(24)
 "TICK", "PEEK", "VPEEK",

 "HIGH", "LOW", "ON", "OFF",  // 定数
 "CW", "CH","LSB", "MSB",
 "MEM", "VRAM", "VAR", "ARRAY","PRG","FNT","GRAM","PRG2",
 "UP", "DOWN", "RIGHT", "LEFT",
 "LOAD", "SAVE", "BLOAD", "BSAVE", "LIST", "NEW", "REM", "LET", "CLV",  // プログラム関連 コマンド(16)
 "LRUN", "FILES", "SYSINFO",
 "WIDTH", // 表示切替
 "RENUM", "RUN", "DELETE", "OK",           // システムコマンド(4)
};

// Keyword count
#define SIZE_KWTBL (sizeof(kwtbl) / sizeof(const char*))

// i-code(Intermediate code) assignment
enum ICode:uint8_t { 
 I_GOTO, I_GOSUB, I_RETURN, I_FOR, I_TO, I_STEP, I_NEXT, I_IF, I_END, I_ELSE,   // 制御命令(10)
 I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_DOLLAR, I_APOST,  // 演算子・記号(31)
 I_LSHIFT, I_RSHIFT, I_OR, I_AND, I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2, I_LT, I_LAND, I_LOR, I_LNOT,
 I_BITREV, I_XOR,  I_ARRAY, 
 I_CLT, I_WAIT,  // 時間待ち・時間計測コマンド(2)
 I_POKE,         // 記憶領域操作コマンド(1)
 I_PRINT, I_QUEST, I_INPUT, I_CLS, I_COLOR, I_ATTR, I_LOCATE,  I_REFLESH, I_CSCROLL,  // キャラクタ表示コマンド(9)  
 I_CHR, I_BIN, I_HEX, I_DMP, I_STRREF,   // 文字列関数(5)
 I_GETS, // 文字列入力
 I_ABS, I_MAP, I_ASC, I_FREE, I_RND, I_INKEY, I_LEN, I_BYTE,   // 数値関数(24)
 I_TICK, I_PEEK, I_VPEEK,

 I_HIGH, I_LOW, I_ON, I_OFF,// 定数
 I_CW, I_CH,
 I_LSB, I_MSB, 
 I_MEM, I_VRAM, I_MVAR, I_MARRAY,I_MPRG,I_MFNT,I_GRAM,I_MPRG2,
 I_UP, I_DOWN, I_RIGHT, I_LEFT,
 I_LOAD, I_SAVE, I_BLOAD, I_BSAVE, I_LIST, I_NEW, I_REM, I_LET, I_CLV,  // プログラム関連 コマンド(16)
 I_LRUN, I_FILES, I_INFO,
 I_WIDTH, // 表示切替
 I_RENUM, I_RUN, I_DELETE, I_OK,  // システムコマンド(4)

// 内部利用コード
  I_NUM, I_STR, I_HEXNUM, I_BINNUM, I_VAR,
  I_EOL, 
};

// List formatting condition
// 後ろに空白を入れない中間コード
const uint8_t i_nsa[] = {
  I_RETURN, I_END, 
  I_CLT,
  I_HIGH, I_LOW,  I_ON, I_OFF,I_CW, I_CH,
  I_UP, I_DOWN, I_RIGHT, I_LEFT,
  I_INKEY,I_VPEEK, I_CHR, I_ASC, I_HEX, I_BIN,I_LEN, I_BYTE, I_STRREF,I_GETS,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,I_QUEST,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_DOLLAR, I_APOST,I_LSHIFT, I_RSHIFT, I_OR, I_AND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2, I_LT, I_LNOT, I_BITREV, I_XOR,
  I_ARRAY, I_RND, I_ABS, I_FREE, I_TICK, I_PEEK,
  I_MAP, I_DMP,
  I_LSB, I_MSB, I_MEM, I_VRAM, I_MVAR, I_MARRAY, I_MPRG, I_MFNT,I_GRAM, I_MPRG2,
};

// 前が定数か変数のとき前の空白をなくす中間コード
const uint8_t  i_nsb[] = {
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_LSHIFT, I_RSHIFT, I_OR, I_AND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2,I_LT, I_LNOT, I_BITREV, I_XOR,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_EOL
};

// 必ず前に空白を入れる中間コード
const uint8_t i_sf[]  = {
  I_ATTR, I_CLS, I_COLOR, I_END, I_FILES, I_TO, I_STEP,I_QUEST,I_LAND, I_LOR,
  I_GOSUB,I_GOTO,I_INKEY,I_INPUT,I_LET,I_LIST,I_ELSE,
  I_LOAD,I_LOCATE,I_NEW,I_POKE,I_PRINT,I_REFLESH,I_REM,I_RENUM,I_CLT,
  I_RETURN,I_RUN,I_SAVE,I_WAIT, 
  I_CSCROLL,
};

// 例外検索関数
inline char sstyle(uint8_t code,
  const uint8_t *table, uint8_t count) {
  while(count--) //中間コードの数だけ繰り返す
    if (code == table[count]) //もし該当の中間コードがあったら
      return 1; //1を持ち帰る
  return 0; //（なければ）0を持ち帰る
}

// キーワード並び例外チェックマクロ
#define nospacea(c) sstyle(c, i_nsa, sizeof(i_nsa))  // 後ろに空白を入れない中間コードか？
#define nospaceb(c) sstyle(c, i_nsb, sizeof(i_nsb))  // 前が定数か変数のとき前の空白をなくす中間コードか？
#define spacef(c) sstyle(c, i_sf, sizeof(i_sf))      // 必ず前に空白を入れる中間コードか？
  
// RAM mapping
char lbuf[SIZE_LINE];          // コマンド入力バッファ
char tbuf[SIZE_LINE];          // テキスト表示用バッファ
int16_t tbuf_pos = 0;
unsigned char ibuf[SIZE_IBUF];    // i-code conversion buffer
short var[SIZE_VAR];              // 変数領域
short arr[SIZE_ARRY];             // 配列領域
unsigned char listbuf[SIZE_LIST]; // プログラムリスト領域
uint8_t mem[SIZE_MEM];            // 自由利用データ領域

unsigned char* clp;               // Pointer current line
unsigned char* cip;               // Pointer current Intermediate code
unsigned char* gstk[SIZE_GSTK];   // GOSUB stack
unsigned char gstki;              // GOSUB stack index
unsigned char* lstk[SIZE_LSTK];   // FOR stack
unsigned char lstki;              // FOR stack index

uint8_t prevPressKey = 0;         // 直前入力キーの値(INKEY()、[ESC]中断キー競合防止用)
uint8_t lfgSerial1Opened = false; // Serial1のオープン設定フラグ

//強制的な中断の判定
uint8_t isBreak() {
  uint8_t c = c_kbhit();
  if (c) {
      if (c == KEY_CTRL_C || c==KEY_ESCAPE ) { // 読み込んでもし[ESC],［CTRL_C］キーだったら
        err = ERR_CTR_C;                       // エラー番号をセット
        prevPressKey = 0;
      } else {
        prevPressKey = c;
      }
   }
   return err;
}

// メモリへの文字出力
inline void mem_putch(uint8_t c) {
  if (tbuf_pos < SIZE_LINE) {
   tbuf[tbuf_pos] = c;
   tbuf_pos++;
  }
}

// メモリ書き込みポインタのクリア
inline void cleartbuf() {
  tbuf_pos=0;
  memset(tbuf,0,SIZE_LINE);
}

// 仮想アドレスを実アドレスに変換
//  引数   :  vadr 仮想アドレス
//  戻り値 :  NULL以外 実アドレス、NULL 範囲外
//
uint8_t* v2realAddr(uint16_t vadr) {
  uint8_t* radr = NULL; 
  if (vadr < sc->getScreenByteSize()) {   // VRAM領域
    radr = vadr+sc->getScreen();
  } else if ((vadr >= V_VAR_TOP) && (vadr < V_ARRAY_TOP)) { // 変数領域
    radr = vadr-V_VAR_TOP+(uint8_t*)var;
  } else if ((vadr >= V_ARRAY_TOP) && (vadr < V_PRG_TOP)) { // 配列領域
    radr = vadr - V_ARRAY_TOP+(uint8_t*)arr;
  } else if ((vadr >= V_PRG_TOP) && (vadr < V_MEM_TOP)) {   // プログラム領域
    radr = vadr - V_PRG_TOP + (uint8_t*)listbuf;
  } else if ((vadr >= V_MEM_TOP) && (vadr < V_FNT_TOP)) {   // ユーザーワーク領域
    radr = vadr - V_MEM_TOP + mem;    
  } else if ((vadr >= V_FNT_TOP) && (vadr < V_GRAM_TOP)) {  // フォント領域
    //radr = vadr - V_FNT_TOP + getFontAdr()+3;
  } else if ((vadr >= V_GRAM_TOP) && (vadr < V_GRAM_TOP+6048)) { // グラフィク表示用メモリ領域
  } else if ((vadr >= V_PRG2_TOP) && (vadr < V_PRG2_TOP+4096)) { // フラッシュメモリプログラム領域

  } else {
      radr = NULL;
  }
  return radr;
}

// Standard C libraly (about) same functions
inline char c_toupper(char c) {
  return(c <= 'z' && c >= 'a' ? c - 32 : c);
}
inline char c_isprint(char c) {
  return (c);
}
inline char c_isspace(char c) {
  return(c == ' ' || (c <= 13 && c >= 9));
}

// 全角判定
inline uint8_t isZenkaku(uint8_t c){
   return (((c>=0x81)&&(c<=0x9f))||((c>=0xe0)&&(c<=0xfc))) ? 1:0;
}

// 文字列の右側の空白文字を削除する
char* tlimR(char* str) {
  uint16_t len = strlen(str);
  for (uint16_t i = len - 1; i>0 ; i--) {
    if (str[i] == ' ') {
      str[i] = 0;
    } else {
      break;
    }
  }
  return str;
}

// コマンド引数取得(int16_t,引数チェックあり)
inline uint8_t getParam(int16_t& prm, int16_t  v_min,  int16_t  v_max, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err &&  (prm < v_min || prm > v_max)) 
    err = ERR_VALUE;
  else if (flgCmma && *cip++ != I_COMMA) {
    err = ERR_SYNTAX;
 }
  return err;
}

// コマンド引数取得(int16_t,引数チェックなし)
inline uint8_t getParam(uint16_t& prm, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err && flgCmma && *cip++ != I_COMMA) {
   err = ERR_SYNTAX;
  }
  return err;
}

// コマンド引数取得(uint16_t,引数チェックなし)
inline uint8_t getParam(int16_t& prm, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err && flgCmma && *cip++ != I_COMMA) {
   err = ERR_SYNTAX;
  }
  return err;
}

// コマンド引数取得(int32_t,引数チェックなし)
inline uint8_t getParam(int32_t& prm, uint8_t flgCmma) {
  prm = iexp(); 
  if (!err && flgCmma && *cip++ != I_COMMA) {
   err = ERR_SYNTAX;
  }
  return err;
}

// '('チェック関数
inline uint8_t checkOpen() {
  if (*cip != I_OPEN)  err = ERR_PAREN;
  else cip++;
  return err;
}

// ')'チェック関数
inline uint8_t checkClose() {
  if (*cip != I_CLOSE)  err = ERR_PAREN;
  else cip++;
  return err;
}

// 1桁16進数文字を整数に変換する
uint16_t hex2value(char c) {
  if (c <= '9' && c >= '0')
    return c - '0';
  else if (c <= 'f' && c >= 'a')
    return c - 'a' + 10;
  else if (c <= 'F' && c >= 'A')
    return c - 'A' + 10;
  return 0;
}

// 文字列出力
inline void c_puts(const char *s, uint8_t devno=0) {
  uint8_t prev_curs = sc->IsCurs();
  if (prev_curs) sc->show_curs(0);
  while (*s) c_putch(*s++, devno); //終端でなければ出力して繰り返す
  if (prev_curs) sc->show_curs(1);
}

// Print numeric specified columns
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
//  devno : 出力先デバイスコード
// 機能
// 'SNNNNN' S:符号 N:数値 or 空白 
//  dで桁指定時は空白補完する
//
void putnum(int16_t value, int16_t d, uint8_t devno=0) {
  uint8_t dig;  // 桁位置
  uint8_t sign; // 負号の有無（値を絶対値に変換した印）
  uint16_t new_value;
  char c = ' ';
  if (d < 0) {
    d = -d;
    c = '0';
  }

  if (value < 0) {     // もし値が0未満なら
    sign = 1;          // 負号あり
    //value = -value;    // 値を絶対値に変換
    new_value = -value;
  } else {
    sign = 0;          // 負号なし
    new_value = value;
  }

  lbuf[6] = 0;         // 終端を置く
  dig = 6;             // 桁位置の初期値を末尾に設定
  do { //次の処理をやってみる
    lbuf[--dig] = (new_value % 10) + '0'; // 1の位を文字に変換して保存
    new_value /= 10;                      // 1桁落とす
  } while (new_value > 0);                // 値が0でなければ繰り返す

  if (sign) //もし負号ありなら
    lbuf[--dig] = '-'; // 負号を保存

  while (6 - dig < d) { // 指定の桁数を下回っていれば繰り返す
    c_putch(c,devno);   // 桁の不足を空白で埋める
    d--;                // 指定の桁数を1減らす
  }
  c_puts(&lbuf[dig],devno);   // 桁位置からバッファの文字列を表示
}

// 16進数の出力
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
//  devno : 出力先デバイスコード
// 機能
// 'XXXX' X:数値
//  dで桁指定時は0補完する
//  符号は考慮しない
// 
void putHexnum(short value, uint8_t d, uint8_t devno=0) {
  uint16_t  hex = (uint16_t)value; // 符号なし16進数として参照利用する
  uint16_t  h;
  uint16_t dig;

  // 表示に必要な桁数を求める
  if (hex >= 0x1000) 
    dig = 4;
  else if (hex >= 0x100) 
    dig = 3;
  else if (hex >= 0x10) 
    dig = 2;
  else 
    dig = 1;

  if (d != 0 && d > dig) 
    dig = d;

  for (uint8_t i = 0; i < 4; i++) {
    h = ( hex >> (12 - i * 4) ) & 0x0f;
    lbuf[i] = (h >= 0 && h <= 9) ? h + '0': h + 'A' - 10;
  }
  lbuf[4] = 0;
  c_puts(&lbuf[4-dig],devno);
}

// 2進数の出力
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
//  devno : 出力先デバイスコード
// 機能
// 'BBBBBBBBBBBBBBBB' B:数値
//  dで桁指定時は0補完する
//  符号は考慮しない
// 

void putBinnum(int16_t value, uint8_t d, uint8_t devno=0) {
  uint16_t  bin = (uint16_t)value; // 符号なし16進数として参照利用する
  uint16_t  dig = 0;               // 先頭が1から始まる桁数

  // 最初に1が現れる桁を求める
  for (uint8_t i=0; i < 16; i++) {
    if ( (0x8000>>i) & bin ) {
      dig = 15 - i;
      break;
    }
  }
  dig++;
  
  // 実際の桁数が指定表示桁数を超える場合は、実際の桁数を採用する
  if (d > dig) 
    dig = d;

  // ビット文字列の出力処理
  for (int8_t i=dig-1; i>=0; i--)
   c_putch((bin & (1<<i)) ? '1':'0', devno);

}

// 数値の入力
int16_t getnum() {
  int16_t value, tmp; //値と計算過程の値
  char c; //文字
  uint8_t len; //文字数
  uint8_t sign; //負号

  len = 0; //文字数をクリア
  while(1) {
    c = c_getch();
    if (c == KEY_ENTER && len) {
        break;
    } else if (c == KEY_CTRL_C || c==27) {
      err = ERR_CTR_C;
        break;
    } else 
    //［BackSpace］キーが押された場合の処理（行頭ではないこと）
    if (((c == 8) || (c == 127)) && (len > 0)) {
      len--; //文字数を1減らす
      //c_putch(8); c_putch(' '); c_putch(8); //文字を消す
      sc->movePosPrevChar();
      sc->delete_char();
    } else
    //行頭の符号および数字が入力された場合の処理（符号込みで6桁を超えないこと）
    if ((len == 0 && (c == '+' || c == '-')) ||
		(len < 6 && isdigit(c))) {
      lbuf[len++] = c; //バッファへ入れて文字数を1増やす
      c_putch(c); //表示
    } else {
      sc->beep();
    }
  }
  newline(); //改行
  lbuf[len] = 0; //終端を置く

  switch (lbuf[0]) { //先頭の文字で分岐
  case '-': //「-」の場合
    sign = 1; //負の値
    len = 1;  //数字列はlbuf[1]以降
    break;
  case '+': //「+」の場合
    sign = 0; //正の値
    len = 1;  //数字列はlbuf[1]以降
    break;
  default:  //どれにも該当しない場合
    sign = 0; //正の値
    len = 0;  //数字列はlbuf[0]以降
    break;
  }

  value = 0; //値をクリア
  tmp = 0; //計算過程の値をクリア
  while (lbuf[len]) { //終端でなければ繰り返す
    tmp = 10 * value + lbuf[len++] - '0'; //数字を値に変換
    if (value > tmp) { //もし計算過程の値が前回より小さければ
      err = ERR_VOF; //オーバーフローを記録
    }
    value = tmp; //計算過程の値を記録
  }

  if (sign) //もし負の値なら
    return -value; //負の値に変換して持ち帰る

  return value; //値を持ち帰る
}

// キーワード検索
//[戻り値]
//  該当なし   : -1
//  見つかった : キーワードコード
int16_t lookup(char* str, uint16_t len) {
  int16_t fd_id;
  int16_t prv_fd_id = -1;
  uint16_t fd_len,prv_len;

  for (uint16_t j = 1; j <= len; j++) {
    fd_id = -1;
    for (uint16_t i = 0; i < SIZE_KWTBL; i++) {
      if (!strnicmp(kwtbl[i], str, j)) {
        fd_id = i;
        fd_len = j;        
        break;
      }
    }
    if (fd_id >= 0) {
      prv_fd_id = fd_id;
      prv_len = fd_len;
    } else {
      break;
    }
  }
  if (prv_fd_id >= 0) {
    prv_fd_id = -1;
    for (uint16_t i = 0; i < SIZE_KWTBL; i++) {
      if ( (strlen(kwtbl[i]) == prv_len) && !strnicmp(kwtbl[i], str, prv_len) ) {
        prv_fd_id = i;
        break;
      }
    }
  }
  return prv_fd_id;
}

//
// テキストを中間コードに変換
// [戻り値]
//   0 または 変換中間コードバイト数
//
uint8_t toktoi() {
  int16_t i;
  int16_t key;
  uint8_t len = 0;  // 中間コードの並びの長さ
  char* ptok;             // ひとつの単語の内部を指すポインタ
  char* s = lbuf;         // 文字列バッファの内部を指すポインタ
  char c;                 // 文字列の括りに使われている文字（「"」または「'」）
  uint32_t value;         // 定数
  uint32_t tmp;           // 変換過程の定数
  uint16_t hex;           // 16進数定数
  uint16_t hcnt;          // 16進数桁数
  uint16_t bin;           // 2進数定数
  uint16_t bcnt;          // 2進数桁数
  uint8_t var_len;        // 変数名長さ
  char var_name[3];       // 変数名
  
  while (*s) {                  //文字列1行分の終端まで繰り返す
    while (c_isspace(*s)) s++;  //空白を読み飛ばす

    key = lookup(s, strlen(s));
    if (key >= 0) {    
      // 該当キーワードあり
      if (len >= SIZE_IBUF - 1) {      // もし中間コードが長すぎたら
        err = ERR_IBUFOF;              // エラー番号をセット
        return 0;                      // 0を持ち帰る
      }
      ibuf[len++] = key;                 // 中間コードを記録
      s+= strlen(kwtbl[key]);

    } else {
      //err = ERR_SYNTAX; //エラー番号をセット
      //return 0;
    }

    // 16進数の変換を試みる $XXXX
    if (key == I_DOLLAR) {
		if (isxdigit(*s)) {   // もし文字が16進数文字なら
        hex = 0;              // 定数をクリア
        hcnt = 0;             // 桁数
        do { //次の処理をやってみる          
          hex = (hex<<4) + hex2value(*s++); // 数字を値に変換
          hcnt++;
		} while (isxdigit(*s)); //16進数文字がある限り繰り返す

        if (hcnt > 4) {      // 桁溢れチェック
          err = ERR_VOF;     // エラー番号オバーフローをセット
          return 0;          // 0を持ち帰る
        }
  
        if (len >= SIZE_IBUF - 3) { // もし中間コードが長すぎたら
          err = ERR_IBUFOF;         // エラー番号をセット
          return 0;                 // 0を持ち帰る
        }
        //s = ptok; // 文字列の処理ずみの部分を詰める
        len--;    // I_DALLARを置き換えるために格納位置を移動
        ibuf[len++] = I_HEXNUM;  //中間コードを記録
        ibuf[len++] = hex & 255; //定数の下位バイトを記録
        ibuf[len++] = hex >> 8;  //定数の上位バイトを記録
      }      
    }
 
    // 2進数の変換を試みる $XXXX
    if (key == I_APOST) {
      if ( *s == '0'|| *s == '1' ) {    // もし文字が2進数文字なら
        bin = 0;              // 定数をクリア
        bcnt = 0;             // 桁数
        do { //次の処理をやってみる
          bin = (bin<<1) + (*s++)-'0' ; // 数字を値に変換
          bcnt++;
        } while ( *s == '0'|| *s == '1' ); //16進数文字がある限り繰り返す

        if (bcnt > 16) {      // 桁溢れチェック
          err = ERR_VOF;     // エラー番号オバーフローをセット
          return 0;          // 0を持ち帰る
        }
  
        if (len >= SIZE_IBUF - 3) { // もし中間コードが長すぎたら
          err = ERR_IBUFOF;         // エラー番号をセット
          return 0;                 // 0を持ち帰る
        }
        len--;    // I_APOSTを置き換えるために格納位置を移動
        ibuf[len++] = I_BINNUM;  //中間コードを記録
        ibuf[len++] = bin & 255; //定数の下位バイトを記録
        ibuf[len++] = bin >> 8;  //定数の上位バイトを記録
      }      
    }

    //コメントへの変換を試みる
    if(key == I_REM|| key == I_SQUOT) {       // もし中間コードがI_REMなら
      while (c_isspace(*s)) s++;         // 空白を読み飛ばす
      ptok = s;                          // コメントの先頭を指す

      for (i = 0; *ptok++; i++);         // コメントの文字数を得る
      if (len >= SIZE_IBUF - 2 - i) {    // もし中間コードが長すぎたら
        err = ERR_IBUFOF;                // エラー番号をセット
        return 0;                        // 0を持ち帰る
      }

      ibuf[len++] = i;                   // コメントの文字数を記録
      while (i--) {                      // コメントの文字数だけ繰り返す
        ibuf[len++] = *s++;              // コメントを記録
      }
      break;                             // 文字列の処理を打ち切る（終端の処理へ進む）
    }

   if (key >= 0)                            // もしすでにキーワードで変換に成功していたら以降はスキップ
     continue;
   
    //定数への変換を試みる
    ptok = s;                            // 単語の先頭を指す
    if (isdigit(*ptok)) {              // もし文字が数字なら
      value = 0;                         // 定数をクリア
      tmp = 0;                           // 変換過程の定数をクリア
      do { //次の処理をやってみる
        tmp = 10 * value + *ptok++ - '0'; // 数字を値に変換
        if (tmp > 32768) {                // もし32768より大きければ          
          err = ERR_VOF;                  // エラー番号をセット
          return 0;                       // 0を持ち帰る
        }      
        value = tmp; //0を持ち帰る
	  } while (isdigit(*ptok)); //文字が数字である限り繰り返す

      if (len >= SIZE_IBUF - 3) { //もし中間コードが長すぎたら
        err = ERR_IBUFOF; //エラー番号をセット
        return 0; //0を持ち帰る
      }

      if ( (value == 32768) && (len > 0) && (ibuf[len-1] != I_MINUS)) {
        // valueが32768のオーバーフローエラー☑
        err = ERR_VOF;                  // エラー番号をセット
        return 0;                       // 0を持ち帰る       
      }

      s = ptok; //文字列の処理ずみの部分を詰める      
      ibuf[len++] = I_NUM; //中間コードを記録
      ibuf[len++] = value & 255; //定数の下位バイトを記録
      ibuf[len++] = value >> 8; //定数の上位バイトを記録
    }
    else

    //文字列への変換を試みる
    if (*s == '\"' ) { //もし文字が '\"'
      c = *s++; //「"」か「'」を記憶して次の文字へ進む
      ptok = s; //文字列の先頭を指す
      //文字列の文字数を得る
      for (i = 0; (*ptok != c) && c_isprint(*ptok); i++)
        ptok++;
      if (len >= SIZE_IBUF - 1 - i) { //もし中間コードが長すぎたら
        err = ERR_IBUFOF; //エラー番号をセット
        return 0; //0を持ち帰る
      }
      ibuf[len++] = I_STR; //中間コードを記録
      ibuf[len++] = i; //文字列の文字数を記録
      while (i--) { //文字列の文字数だけ繰り返す
        ibuf[len++] = *s++; //文字列を記録
      }
      if (*s == c) s++; //もし文字が「"」か「'」なら次の文字へ進む
    }
    else

    //変数への変換を試みる(2017/07/26 A～Z9:対応)
    //  1文字目
    if (isalpha(*ptok)) { //もし文字がアルファベットなら
      var_len = 0;
      if (len >= SIZE_IBUF - 2) { //もし中間コードが長すぎたら
        err = ERR_IBUFOF; 
        return 0;
      }
      var_name[var_len] = c_toupper(*ptok) - 'A';
      var_len++;

      //  2文字目('0'～'6'までが有効)
      if(isdigit(*(ptok+1)) && *(ptok+1) <='6' ) { //もしも文字が数字なら
         var_name[var_len] = *(ptok+1) - '0' + 1;
         var_len++;  
       } else {
         var_name[1] = 0;         
       }

      //もし変数が3個並んだら
      if (len >= 4 && ibuf[len - 2] == I_VAR && ibuf[len - 4] == I_VAR) {
         err = ERR_SYNTAX; //エラー番号をセット
         return 0; //0を持ち帰る
       }

      // 中間コードに変換
      ibuf[len++] = I_VAR; //中間コードを記録
      ibuf[len++] = var_name[0]+var_name[1]*26;
      s+=var_len; //次の文字へ進む
    }
    else

    //どれにも当てはまらなかった場合
    {
      err = ERR_SYNTAX; //エラー番号をセット
      return 0; //0を持ち帰る
    }
  } //文字列1行分の終端まで繰り返すの末尾

  ibuf[len++] = I_EOL; //文字列1行分の終端を記録
  return len; //中間コードの長さを持ち帰る
}


// Return free memory size
short getsize() {
  unsigned char* lp; //ポインタ

  for (lp = listbuf; *lp; lp += *lp); //ポインタをリストの末尾へ移動
  return listbuf + SIZE_LIST - lp - 1; //残りを計算して持ち帰る
}

// Get line numbere by line pointer
short getlineno(unsigned char *lp) {
  if(*lp == 0) //もし末尾だったら
    return -1;
  return *(lp + 1) | *(lp + 2) << 8; //行番号を持ち帰る
}

// Search line by line number
unsigned char* getlp(short lineno) {
  unsigned char *lp; //ポインタ

  for (lp = listbuf; *lp; lp += *lp) //先頭から末尾まで繰り返す
    if (getlineno(lp) >= lineno) //もし指定の行番号以上なら
      break; //繰り返しを打ち切る

  return lp; //ポインタを持ち帰る
}

// ラベルでラインポインタを取得する
// pLabelは [I_STR][長さ][ラベル名] であること
unsigned char* getlpByLabel(uint8_t* pLabel) {
  unsigned char *lp; //ポインタ
  uint8_t len;
  pLabel++;
  len = *pLabel; // 長さ取得
  pLabel++;      // ラベル格納位置
  
  for (lp = listbuf; *lp; lp += *lp)  { //先頭から末尾まで繰り返す
    if ( *(lp+3) == I_STR ) {
       if (len == *(lp+4)) {
           if (strncmp((char*)pLabel, (char*)(lp+5), len) == 0) {
              return lp;
           }
       }
    }  
  }
  return NULL;
}

// 行番号から行インデックスを取得する
uint16_t getlineIndex(uint16_t lineno) {
  unsigned char *lp; //ポインタ
  uint16_t index = 0;	
  uint16_t rc = 32767;
  for (lp = listbuf; *lp; lp += *lp) { // 先頭から末尾まで繰り返す
  	if (getlineno(lp) >= lineno) {     // もし指定の行番号以上なら
        rc = index;
  		break;                         // 繰り返しを打ち切る
  	}
  	index++;
  }
  return rc; 
}	

// ELSE中間コードをサーチする
// 引数   : 中間コードプログラムポインタ
// 戻り値 : NULL 見つからない
//          NULL以外 LESEの次のポインタ
//
uint8_t* getELSEptr(uint8_t* p) {
 uint8_t* rc = NULL;
 uint8_t* lp;
 
 // ブログラム中のGOTOの飛び先行番号を付け直す
  for (lp = p; *lp != I_EOL ; ) {
    switch(*lp) {
    case I_IF:    // IF命令
      goto DONE;
        break;
    case I_ELSE:  // ELSE命令
      rc = lp+1;
      goto DONE;
        break;
      break;
    case I_STR:     // 文字列
      lp += lp[1]+1;            
      break;
    case I_NUM:     // 定数
    case I_HEXNUM: 
    case I_BINNUM:
      lp+=3;        // 整数2バイト+中間コード1バイト分移動
      break;
    case I_VAR:     // 変数
      lp+=2;        // 変数名
      break;
    default:        // その他
      lp++;
      break;
    }
  }  
DONE:
  return rc;
}

// プログラム行数を取得する
uint16_t countLines(int16_t st=0, int16_t ed=32767) {
  unsigned char *lp; //ポインタ
  uint16_t cnt = 0;  
  int16_t lineno;
  for (lp = listbuf; *lp; lp += *lp)  {
    lineno = getlineno(lp);
    if (lineno < 0)
      break;
    if ( (lineno >= st) && (lineno <= ed)) 
      cnt++;
  }
  return cnt;   
}

// Insert i-code to the list
// [listbuf]に[ibuf]を挿入
//  [ibuf] : [1:データ長][1:I_NUM][2:行番号][中間コード]
//
void inslist() {
  unsigned char *insp;     // 挿入位置ポインタ
  unsigned char *p1, *p2;  // 移動先と移動元ポインタ
  short len;               // 移動の長さ

  // 空きチェク(これだと、空き不足時に行番号だけ入力時の行削除が出来ないかも.. @たま吉)
  if (getsize() < *ibuf) { // もし空きが不足していたら
    err = ERR_LBUFOF;      // エラー番号をセット
    return;                // 処理を打ち切る
  }

  insp = getlp(getlineno(ibuf)); // 挿入位置ポインタを取得

  // 同じ行番号の行が存在したらとりあえず削除
  if (getlineno(insp) == getlineno(ibuf)) { // もし行番号が一致したら
    p1 = insp;                              // p1を挿入位置に設定
    p2 = p1 + *p1;                          // p2を次の行に設定
    while ((len = *p2) != 0) {              // 次の行の長さが0でなければ繰り返す
      while (len--)                         // 次の行の長さだけ繰り返す
        *p1++ = *p2++;                      // 前へ詰める
    }
    *p1 = 0; // リストの末尾に0を置く
  }

  // 行番号だけが入力された場合はここで終わる
  if (*ibuf == 4) // もし長さが4（[長さ][I_NUM][行番号]のみ）なら
    return;

  // 挿入のためのスペースを空ける

  for (p1 = insp; *p1; p1 += *p1); // p1をリストの末尾へ移動
  len = p1 - insp + 1;             // 移動する幅を計算
  p2 = p1 + *ibuf;                 // p2を末尾より1行の長さだけ後ろに設定
  while (len--)                    // 移動する幅だけ繰り返す
    *p2-- = *p1--;                 // 後ろへズラす

  // 行を転送する
  len = *ibuf;     // 中間コードの長さを設定
  p1 = insp;       // 転送先を設定
  p2 = ibuf;       // 転送元を設定
  while (len--)    // 中間コードの長さだけ繰り返す
    *p1++ = *p2++; // 転送
}

//指定中間コード行レコードのテキスト出力
void putlist(unsigned char* ip, uint8_t devno=0) {
  unsigned char i;  // ループカウンタ
  uint8_t var_code; // 変数コード
  
  while (*ip != I_EOL) { //行末でなければ繰り返す
    //キーワードの処理
    if (*ip < SIZE_KWTBL) { //もしキーワードなら    
      c_puts(kwtbl[*ip],devno); //キーワードテーブルの文字列を表示
      if (*(ip+1) != I_COLON) 
        if ( ((!nospacea(*ip) || spacef(*(ip+1))) && (*ip != I_COLON) && (*ip != I_SQUOT))
        || ((*ip == I_CLOSE)&& (*(ip+1) != I_COLON  && *(ip+1) != I_EOL && !nospaceb(*(ip+1)))) ) //もし例外にあたらなければ
          c_putch(' ',devno); //空白を表示

      if (*ip == I_REM||*ip == I_SQUOT) { //もし中間コードがI_REMなら
        ip++; //ポインタを文字数へ進める
        i = *ip++; //文字数を取得してポインタをコメントへ進める
        while (i--) //文字数だけ繰り返す
          c_putch(*ip++,devno); //ポインタを進めながら文字を表示
        return;
      }
      ip++;//ポインタを次の中間コードへ進める
    }
    else

    //定数の処理
    if (*ip == I_NUM) { //もし定数なら
      ip++; //ポインタを値へ進める
      putnum(*ip | *(ip + 1) << 8, 0,devno); //値を取得して表示
      ip += 2; //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
        c_putch(' ',devno); //空白を表示
    }
    else

    //16進定数の処理
    if (*ip == I_HEXNUM) { //もし16進定数なら
      ip++; //ポインタを値へ進める
      c_putch('$',devno); //空白を表示
      putHexnum(*ip | *(ip + 1) << 8, 2,devno); //値を取得して表示
      ip += 2; //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
        c_putch(' ',devno); //空白を表示
    }
    else

    //2進定数の処理
    if (*ip == I_BINNUM) { //もし2進定数なら
      ip++; //ポインタを値へ進める
      c_putch('`',devno); //"`"を表示
      if (*(ip + 1))
          putBinnum(*ip | *(ip + 1) << 8, 16,devno); //値を取得して16桁で表示
      else
          putBinnum(*ip , 8,devno);  //値を取得して8桁で表示
      ip += 2; //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
        c_putch(' ',devno); //空白を表示
    }
    else     
    //変数の処理(2017/07/26 変数名 A～Z 9対応)
    if (*ip == I_VAR) { //もし定数なら
      ip++; //ポインタを変数番号へ進める
      var_code = *ip++;      
      c_putch( (var_code%26) + 'A',devno); //変数名を取得して表示
      if (var_code/26)
         c_putch( (var_code/26)+'0'-1, devno);

      if (!nospaceb(*ip)) //もし例外にあたらなければ
        c_putch(' ',devno); //空白を表示
    }
    else

    //文字列の処理
    if (*ip == I_STR) { //もし文字列なら
      char c; //文字列の括りに使われている文字（「"」または「'」）

      //文字列の括りに使われている文字を調べる
      c = '\"'; //文字列の括りを仮に「"」とする
      ip++; //ポインタを文字数へ進める
      for (i = *ip; i; i--) //文字数だけ繰り返す
        if (*(ip + i) == '\"') { //もし「"」があれば
          c = '\''; //文字列の括りは「'」
          break; //繰り返しを打ち切る
        }

      //文字列を表示する
      c_putch(c,devno); //文字列の括りを表示
      i = *ip++; //文字数を取得してポインタを文字列へ進める
      while (i--) //文字数だけ繰り返す
        c_putch(*ip++,devno); //ポインタを進めながら文字を表示
      c_putch(c,devno); //文字列の括りを表示
      if (*ip == I_VAR || *ip ==I_ELSE) //もし次の中間コードが変数だったら
        c_putch(' ',devno); //空白を表示
    }

    else { //どれにも当てはまらなかった場合
      err = ERR_SYS; //エラー番号をセット
      return;
    }
  }
}

// Get argument in parenthesis
short getparam() {
  short value; //値
  if (checkOpen()) return 0;
  if (getParam(value,false) )  return 0;
  if (checkClose()) return 0;
  return value; //値を持ち帰る
}

// INPUT handler
void iinput() {
  short value;          // 値
  short index;          // 配列の添え字or変数番号
  unsigned char i;      // 文字数
  unsigned char prompt; // プロンプト表示フラグ
  short ofvalue;        // オーバーフロー時の設定値
  uint8_t flgofset =0;  // オーバーフロ時の設定値指定あり

  sc->show_curs(1);
  prompt = 1;       // まだプロンプトを表示していない

  // プロンプトが指定された場合の処理
  if(*cip == I_STR){   // もし中間コードが文字列なら
    cip++;             // 中間コードポインタを次へ進める
    i = *cip++;        // 文字数を取得
    while (i--)        // 文字数だけ繰り返す
      c_putch(*cip++); // 文字を表示
    prompt = 0;        // プロンプトを表示した

    if (*cip != I_COMMA) {
      err = ERR_SYNTAX;
      goto DONE;
    }
    cip++;
  }

  // 値を入力する処理
  switch (*cip++) {         // 中間コードで分岐
  case I_VAR:             // 変数の場合
    index = *cip;         // 変数番号の取得
    cip++;
   
    // オーバーフロー時の設定値
    if (*cip == I_COMMA) {
      cip++;
      ofvalue = iexp();
      if (err) {
        goto DONE;
      }
      flgofset = 1;
    }
    
    if (prompt) {          // もしまだプロンプトを表示していなければ
      if (index >=26) {
       c_putch('A'+index%26);    // 変数名を表示
       c_putch('0'+index/26-1);  // 変数名を表示
      } else {
        c_putch('A'+index);  // 変数名を表示
      }
      c_putch(':');        //「:」を表示
    }
    
    value = getnum();     // 値を入力
    if (err) {            // もしエラーが生じたら
      if (err == ERR_VOF && flgofset) {
        err = ERR_OK;
        value = ofvalue;
      } else {
        return;            // 終了
      }
    }
    var[index] = value;  // 変数へ代入
    break;               // 打ち切る

  case I_ARRAY: // 配列の場合
    index = getparam();       // 配列の添え字を取得
    if (err)                  // もしエラーが生じたら
      goto DONE;

    if (index >= SIZE_ARRY) { // もし添え字が上限を超えたら
      err = ERR_SOR;          // エラー番号をセット
      goto DONE;
    }

    // オーバーフロー時の設定値
    if (*cip == I_COMMA) {
      cip++;
      ofvalue = iexp();
      if (err) {
        goto DONE;
      }
      flgofset = 1;
    }

    if (prompt) { // もしまだプロンプトを表示していなければ
      c_puts("@(");     //「@(」を表示
      putnum(index, 0); // 添え字を表示
      c_puts("):");     //「):」を表示
    }
    value = getnum(); // 値を入力
    if (err) {           // もしエラーが生じたら
      if (err == ERR_VOF && flgofset) {
        err = ERR_OK;
        value = ofvalue;
      } else {
        goto DONE;
      }
    }
    arr[index] = value; //配列へ代入
    break;              // 打ち切る

  default: // 以上のいずれにも該当しなかった場合
    err = ERR_SYNTAX; // エラー番号をセット
    goto DONE;
  } // 中間コードで分岐の末尾
DONE:  
  sc->show_curs(0);
}

// Variable assignment handler
void ivar() {
  short value; //値
  short index; //変数番号

  index = *cip++; //変数番号を取得して次へ進む

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return;
  }
  cip++; //中間コードポインタを次へ進める
  if (*cip == I_STR) {
    cip++;
    value = (int16_t)((uint32_t)cip - (uint32_t)listbuf + V_PRG_TOP);
    cip += *cip+1;
  } else {
  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return; //終了
  }
  var[index] = value; //変数へ代入
}

// Array assignment handler
void iarray() {
  short value; //値
  short index; //配列の添え字

  index = getparam(); //配列の添え字を取得
  if (err) //もしエラーが生じたら
    return; //終了

  if (index >= SIZE_ARRY || index < 0 ) { //もし添え字が上下限を超えたら
    err = ERR_SOR; //エラー番号をセット
    return; //終了
  }

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return; //終了
  }

  // 例: @(n)=1,2,3,4,5 の連続設定処理
  do {
    cip++; 
    if (*cip == I_STR) {
      cip++;
      value = (int16_t)((uint32_t)cip - (uint32_t)listbuf + V_PRG_TOP);
      cip += *cip+1;
    } else {
      value = iexp(); // 式の値を取得
      if (err)        // もしエラーが生じたら
        return;       // 終了
    }
    if (index >= SIZE_ARRY) { // もし添え字が上限を超えたら
      err = ERR_SOR;          // エラー番号をセット
      return; 
    }
    arr[index] = value; //配列へ代入
    index++;
  } while(*cip == I_COMMA);
} 

// LET handler
void ilet() {
  switch (*cip) { //中間コードで分岐
  case I_VAR: // 変数の場合
    cip++;     // 中間コードポインタを次へ進める
    ivar();    // 変数への代入を実行
    break;

  case I_ARRAY: // 配列の場合
    cip++;      // 中間コードポインタを次へ進める
    iarray();   // 配列への代入を実行
    break;
  
  default:      // 以上のいずれにも該当しなかった場合
    err = ERR_LETWOV; // エラー番号をセット
    break;            // 打ち切る
  }
}

// RUN command handler
void irun(uint8_t* start_clp = NULL) {
  uint8_t*   lp;     // 行ポインタの一時的な記憶場所
  gstki = 0;         // GOSUBスタックインデクスを0に初期化
  lstki = 0;         // FORスタックインデクスを0に初期化

  if (start_clp != NULL) {
    clp = start_clp;
  } else { 
    clp = listbuf;   // 行ポインタをプログラム保存領域の先頭に設定
  }

  while (*clp) {     // 行ポインタが末尾を指すまで繰り返す
    cip = clp + 3;   // 中間コードポインタを行番号の後ろに設定
    lp = iexe();     // 中間コードを実行して次の行の位置を得る
    if (err)         // もしエラーを生じたら
      return;    
    clp = lp;         // 行ポインタを次の行の位置へ移動
  }
}

// LISTコマンド
//  devno : デバイス番号 0:メインスクリーン 1:シリアル 2:グラフィック 3:、メモリー 4:ファイル
void ilist(uint8_t devno=0) {
  int16_t lineno = 0;          // 表示開始行番号
  int16_t endlineno = 32767;   // 表示終了行番号
  int16_t prnlineno;           // 出力対象行番号
  
  //表示開始行番号の設定
  if (*cip != I_EOL && *cip != I_COLON) {
    // 引数あり
    if (getParam(lineno,0,32767,false)) return;                // 表示開始行番号
    endlineno = lineno;
    if (*cip == I_COMMA) {
      cip++;                         // カンマをスキップ
      if (getParam(endlineno,lineno,32767,false)) return;      // 表示終了行番号
     }
   }
 
  //行ポインタを表示開始行番号へ進める
  for ( clp = listbuf; *clp && (getlineno(clp) < lineno); clp += *clp); 
  
  //リストを表示する
  while (*clp) {               // 行ポインタが末尾を指すまで繰り返す

    //強制的な中断の判定
    if (isBreak())
      return;

    prnlineno = getlineno(clp);// 行番号取得
    if (prnlineno > endlineno) // 表示終了行番号に達したら抜ける
       break; 
    putnum(prnlineno, 0,devno);// 行番号を表示
    c_putch(' ',devno);        // 空白を入れる
    putlist(clp + 3,devno);    // 行番号より後ろを文字列に変換して表示
    if (err)                   // もしエラーが生じたら
      break;                   // 繰り返しを打ち切る
    newline(devno);            // 改行
    clp += *clp;               // 行ポインタを次の行へ進める
  }
}

// フラッシュメモリ内保存プログラムのエクスポート
// EXPORT [sno[,eno]]
void iexport() {
}

// プログラム消去
// 引数 0:全消去、1:プログラムのみ消去、2:変数領域のみ消去
void inew(uint8_t mode = 0) {
  unsigned char i; //ループカウンタ

  //変数と配列の初期化
  if (mode == 0|| mode == 2) {
    for (i = 0; i < SIZE_VAR; i++) //変数の数だけ繰り返す
      var[i] = 0; //変数を0に初期化
    
    for (i = 0; i < SIZE_ARRY; i++) //配列の数だけ繰り返す
      arr[i] = 0; //配列を0に初期化
  }
  //実行制御用の初期化
  if (mode !=2) {
    gstki = 0; //GOSUBスタックインデクスを0に初期化
    lstki = 0; //FORスタックインデクスを0に初期化
    *listbuf = 0; //プログラム保存領域の先頭に末尾の印を置く
    clp = listbuf; //行ポインタをプログラム保存領域の先頭に設定
  }
}

// RENUME command handler
void irenum() {
  uint16_t startLineNo = 10;  // 開始行番号
  uint16_t increase = 10;     // 増分
  uint8_t* ptr;               // プログラム領域参照ポインタ
  uint16_t len;               // 行長さ
  uint16_t i;                 // 中間コード参照位置
  uint16_t newnum;            // 新しい行番号
  uint16_t num;               // 現在の行番号
  uint16_t index;             // 行インデックス
  uint16_t cnt;               // プログラム行数
  
  // 開始行番号、増分引数チェック
  if (*cip == I_NUM) {               // もしRENUMT命令に引数があったら
    startLineNo = getlineno(cip);    // 引数を読み取って開始行番号とする
    cip+=3;
    if (*cip == I_COMMA) {
        cip++;                        // カンマをスキップ
        if (*cip == I_NUM) {          // 増分指定があったら
           increase = getlineno(cip); // 引数を読み取って増分とする
        } else {
           err = ERR_SYNTAX;          // カンマありで引数なしの場合はエラーとする
           return;
       }
    }
  }

  // 引数の有効性チェック
  cnt = countLines()-1;
  if (startLineNo <= 0 || increase <= 0) {
    err = ERR_VALUE;
    return;   
  }
  if (startLineNo + increase * cnt > 32767) {
    err = ERR_VALUE;
    return;       
  }

  // ブログラム中のGOTOの飛び先行番号を付け直す
  for (  clp = listbuf; *clp ; clp += *clp) {
     ptr = clp;
     len = *ptr;
     ptr++;
     i=0;
     // 行内検索
     while( i < len-1 ) {
        switch(ptr[i]) {
        case I_GOTO:  // GOTO命令
        case I_GOSUB: // GOSUB命令
          i++;
          if (ptr[i] == I_NUM) {
            num = getlineno(&ptr[i]);    // 現在の行番号を取得する
            index = getlineIndex(num);   // 行番号の行インデックスを取得する
            if (index == 32767) {
               // 該当する行が見つからないため、変更は行わない
               i+=3;
               continue;
            } else {
               // とび先行番号を付け替える
               newnum = startLineNo + increase*index;
               ptr[i+2] = newnum>>8;
               ptr[i+1] = newnum&0xff;
               i+=3;
               continue;
            }
          } 
          break;
      case I_STR:  // 文字列
        i++;
        i+=ptr[i]; // 文字列長分移動
        break;
      case I_NUM:  // 定数
      case I_HEXNUM: 
      case I_BINNUM: 
        i+=3;      // 整数2バイト+中間コード1バイト分移動
        break;
      case I_VAR:  // 変数
        i+=2;      // 変数名
        break;
      default:     // その他
        i++;
        break;
      }
    }
  }
  
  // 各行の行番号の付け替え
  index = 0;
  for (  clp = listbuf; *clp ; clp += *clp ) {
     newnum = startLineNo + increase * index;
     *(clp+1)  = newnum&0xff;
     *(clp+2)  = newnum>>8;
     index++;
  }
}

// 指定行の削除
// DELETE 行番号
// DELETE 開始行番号,終了行番号
void idelete() {
  int16_t sNo;
  int16_t eNo;
  uint8_t  *lp;      // 削除位置ポインタ 
  uint8_t *p1, *p2;  // 移動先と移動元ポインタ
  int16_t len;       // 移動の長さ

  if ( getParam(sNo, false) ) return;
  if (*cip == I_COMMA) {
     cip++;
     if ( getParam(eNo, false) ) return;  
  } else {
     eNo = sNo;
  }

  if (eNo < sNo) {
    err = ERR_VALUE;
    return;
  }

  if (eNo == sNo) {
    lp = getlp(eNo); // 削除位置ポインタを取得    
    if (getlineno(lp) == sNo) {
      // 削除
      p1 = lp;                              // p1を挿入位置に設定
      p2 = p1 + *p1;                        // p2を次の行に設定
      while ((len = *p2) != 0) {            // 次の行の長さが0でなければ繰り返す
        while (len--)                       // 次の行の長さだけ繰り返す
          *p1++ = *p2++;                    // 前へ詰める
      }
      *p1 = 0; // リストの末尾に0を置く
    }
  } else {
    for (uint16_t i = sNo; i <= eNo;i++) {
      lp = getlp(i); // 削除位置ポインタを取得
      if (getlineno(lp) == i) {               // もし行番号が一致したら
        p1 = lp;                              // p1を挿入位置に設定
        p2 = p1 + *p1;                        // p2を次の行に設定
        while ((len = *p2) != 0) {            // 次の行の長さが0でなければ繰り返す
          while (len--)                       // 次の行の長さだけ繰り返す
            *p1++ = *p2++;                    // 前へ詰める
        }
        *p1 = 0; // リストの末尾に0を置く
      }
    }
  }
}

// プログラムファイル一覧表示 FILES ["ファイルパス"]
void ifiles() {

}
  

// 画面クリア
// CLS[モード]
void icls() {
  int16_t mode = 0;
  if (mode == 0) {
    sc->cls();
    sc->locate(0,0);
  } 
}

// 時間待ち
void iwait() {
  int16_t tm;
  if ( getParam(tm, 0, 32767, false) ) return;
  Sleep(tm);
}

// カーソル移動 LOCATE x,y
void ilocate() {
  int16_t x,  y;
  if ( getParam(x, true) ) return;
  if ( getParam(y, false) ) return;
  if ( x >= sc->getWidth() )   // xの有効範囲チェック
     x = sc->getWidth() - 1;
  else if (x < 0)  x = 0;  
  if( y >= sc->getHeight() )   // yの有効範囲チェック
     y = sc->getHeight() - 1;
  else if(y < 0)   y = 0;

  // カーソル移動
  sc->locate((uint16_t)x, (uint16_t)y);
}

// コンソール画面の文字色の指定 COLOLR fc,bc
void icolor() {
 int16_t fc,  bc = 0;
 if ( getParam(fc, 0, 8, false) ) return;
 if(*cip == I_COMMA) {
    cip++;
    if ( getParam(bc, 0, 8, false) ) return;  
 }
  // 文字色の設定
  sc->setColor((uint16_t)fc, (uint16_t)bc);  
}

// 文字属性の指定 ATTRコマンド
void iattr() {
  int16_t attr;
  if ( getParam(attr, 0, 4, false) ) return;
  sc->setAttr(attr); 
}

// キー入力文字コードの取得 INKEY()関数
int16_t iinkey() {
  int16_t rc = 0;
  
  if (prevPressKey) {
    // 一時バッファに入力済キーがあればそれを使う
    rc = prevPressKey;
    prevPressKey = 0;
  } else if (c_kbhit( )) {
    // キー入力
    rc = c_getch();
  }
  return rc;
}

// メモリ参照　PEEK(adr[,bnk])
int16_t ipeek() {
  int16_t value =0, vadr;
  uint8_t* radr;

  if (checkOpen()) return 0;
  if ( getParam(vadr, false) )  return 0;
  if (checkClose()) return 0;  
  radr = v2realAddr(vadr);
  if (radr)
    value = *radr;
  else 
    err = ERR_RANGE;
  return value;
}

// スクリーン座標の文字コードの取得 'VPEEK(X,Y)'
int16_t ivpeek() {
  int16_t value; // 値
  int16_t x, y;  // 座標

  if (checkOpen()) return 0;
  if ( getParam(x, true) )  return 0;
  if ( getParam(y, false) ) return 0;
  if (checkClose()) return 0;
  value = (x < 0 || y < 0 || x >=sc->getWidth() || y >=sc->getHeight()) ? 0: sc->vpeek(x, y);
  return value;
}

// TICK関数
int16_t itick() {
  int16_t value; // 値
  if ((*cip == I_OPEN) && (*(cip + 1) == I_CLOSE)) {
      // 引数無し
      value = 0;
      cip+=2;
   } else {
      value = getparam(); // 括弧の値を取得
      if (err)
        return 0;
   }
   return 0; 
}


// 16進文字出力 'HEX$(数値,桁数)' or 'HEX$(数値)'
void ihex(uint8_t devno=CDEV_SCREEN) {
  short value; // 値
  short d = 0; // 桁数(0で桁数指定なし)

  if (checkOpen()) return;
  if (getParam(value,false)) return;  
  if (*cip == I_COMMA) {
     cip++;
     if (getParam(d,0,4,false)) return;  
  }
  if (checkClose()) return;  
  putHexnum(value, d, devno);    
}

// 2進数出力 'BIN$(数値, 桁数)' or 'BIN$(数値)'
void ibin(uint8_t devno=CDEV_SCREEN) {
  int16_t value; // 値
  int16_t d = 0; // 桁数(0で桁数指定なし)

  if (checkOpen()) return;
  if (getParam(value,false)) return;  
  if (*cip == I_COMMA) {
     cip++;
     if (getParam(d,0,16,false)) return;  
  }
  if (checkClose()) return;
  putBinnum(value, d, devno);    
}

// CHR$() 全角対応
void ichr(uint8_t devno=CDEV_SCREEN) {
  uint16_t value; // 値
  if (checkOpen()) return;
  for(;;) {
    if (getParam(value,false)) return;
    if (value <= 0xff) {
       c_putch(value, devno);
    } else {
       c_putch(value>>8,  devno);
       c_putch(value&0xff,devno);
    }
    if (*cip == I_COMMA) {
       cip++;
       continue;
    }
    break;
  }
  if (checkClose()) return;
}

// 小数点数値出力 DMP$(数値) or DMP(数値,小数部桁数) or DMP(数値,小数部桁数,整数部桁指定)
void idmp(uint8_t devno=CDEV_SCREEN) {
  int32_t value;     // 値
  int32_t v1,v2;
  int16_t n = 2;    // 小数部桁数
  int16_t dn = 0;   // 整数部桁指定
  int32_t base=1;
  
  if (checkOpen()) return;
  if (getParam(value, false)) return;
  if (*cip == I_COMMA) { 
    cip++; 
    if (getParam(n, 0,4,false)) return;
    if (*cip == I_COMMA) { 
       cip++; 
      if (getParam(dn,-6,6,false)) return;
    }  
  }
  if (checkClose()) return;
  
  for (uint16_t i=0; i<n;i++) {
    base*=10;
  }
  v1 = value / base;
  v2 = value % base;
  if (v1 == 0 && value <0)
    c_putch('-',devno);  
  putnum(v1, dn, devno);
  if (n) {
    c_putch('.',devno);
    putnum(v2<0 ?-v2:v2, -n, devno);
  }
}

// 文字列参照 STR$(変数) 全角対応
// STR(文字列参照変数|文字列参照配列変数|文字列定数,[pos,n])
// ※変数,配列は　[LEN][文字列]への参照とする
// 引数
//  devno: 出力先デバイス番号
// 戻り値
//  なし
//

void istrref(uint8_t devno=CDEV_SCREEN) {
  int16_t len;  // 文字列長
  int16_t top;  // 文字取り出し位置
  int16_t n;    // 取り出し文字数
  int16_t index;
  uint8_t *ptr;  // 文字列先頭
  
  if (checkOpen()) return;
  if (*cip == I_VAR) {
    // 変数
    cip++;
    ptr = v2realAddr(var[*cip]);
    len = *ptr;
    ptr++;
    cip++;
  } else if (*cip == I_ARRAY) {
    // 配列変数
    cip++; 
    if (getParam(index, 0, SIZE_ARRY-1, false)) return;
    ptr = v2realAddr(arr[index]);
    len = *ptr;
    ptr++;    
  } else if (*cip == I_STR) {
    // 文字列定数
    cip++;
    len = *cip;
    cip++;
    ptr = cip;
    cip+=len;    
  } else {
    err = ERR_SYNTAX;
    return;
  }
  top = 1; // 文字取り出し位置
  n = len; // 取り出し文字数
  if (*cip == I_COMMA) {
    // 引数：文字取り出し位置、取り出し文字数の取得 
    cip++;
    if (getParam(top, 1,len,true)) return;
    if (getParam(n,1,len-top+1,false)) return;
  }
  if (checkClose()) return;

  // 全角を考慮した文字位置の取得
  int16_t i;
  int16_t wtop = 1;
  for (i=0; i < len; i++) {
    if (wtop == top) {
      break;
    }
    if (isZenkaku(ptr[i])) {
      i++;  
    }
    wtop++;
  }
  if (wtop == top) {
    //実際の取り出し位置
    top = i+1;
  } else {
    err = ERR_VALUE;
    return;
  }
  
  // 全角を考慮した取り出し文字列の出力
  int16_t cnt=0;
  for (uint16_t i = top-1 ; i < len; i++) {
    if (cnt == n) {
      break;
    }  
    c_putch(ptr[i], devno);
    if (isZenkaku(ptr[i])) {
      i++; c_putch(ptr[i], devno);
    }
    cnt++;
  }
  return;
}

// POKEコマンド POKE ADR,データ[,データ,..データ]
void ipoke() {
  uint8_t* adr;
  int16_t value;
  int16_t vadr;
  
  // アドレスの指定
  vadr = iexp(); if(err) return ; 
  if (vadr < 0 ) { err = ERR_VALUE; return; }
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }
  if(vadr>=V_FNT_TOP && vadr < V_GRAM_TOP) { err = ERR_RANGE; return; }       // フォント領域
  if(vadr>=V_PRG2_TOP && vadr < V_PRG2_TOP+4096) { err = ERR_RANGE; return; } // プログラム保存領域
  // 例: 1,2,3,4,5 の連続設定処理
  do {
    adr = v2realAddr(vadr);
    if (!adr) {
      err = ERR_RANGE;
      break;
    }
    cip++;          // 中間コードポインタを次へ進める
    if (getParam(value,false)) return; 
    *((uint8_t*)adr) = (uint8_t)value;
    vadr++;
  } while(*cip == I_COMMA);
}

// LEN(文字列) 全角対応文字列長取得
int16_t ilen(uint8_t flgZen=0) {
  int16_t len = 0; // 文字列長
  int16_t index;   // 配列添え字
  uint8_t* str;    // 文字列先頭位置
  int16_t wlen = 0;
  int16_t pos = 0;
  
  if (checkOpen()) 
    return 0;
    
  if ( *cip == I_VAR)  {
    // 変数の場合
     cip++;
     str = v2realAddr(var[*cip]);
     len = *str; // 文字列長の取得
     str++;      // 文字列先頭
     cip++;     
  } else if ( *cip == I_ARRAY) {
    // 配列変数の場合
     cip++; 
     if (getParam(index, 0, SIZE_ARRY-1, false)) return 0;
     str = v2realAddr(arr[index]);
     len = *str; // 文字列長の取得
     str++;      // 文字列先頭
  } else if ( *cip == I_STR) {
    // 文字列定数の場合
     cip++;  len = *cip; // 文字列長の取得
     cip++;  str = cip;  // 文字列先頭の取得
     cip+=len;
  } else {
    err = ERR_SYNTAX;
    return 0;
  }
  checkClose();
  if (flgZen) {
    // 文字列をスキャンし、長さを求める
    while(pos < len) {
      if (isZenkaku(*str)) {
        str++;
        pos++;  
      }
      wlen++;
      str++;
      pos++;
    }  
  } else {
    wlen = len;
  }
  return wlen;
}
  
int16_t imap() {
  int32_t value,l1,h1,l2,h2,rc;
  if (checkOpen()) return 0;
  if ( getParam(value,true)||getParam(l1,true)||getParam(h1,true)||getParam(l2,true)||getParam(h2,false) ) 
    return 0;
  if (checkClose()) return 0;
  if (l1 >= h1 || l2 >= h2 || value < l1 || value > h1) {
    err = ERR_VALUE;
    return 0;
  }
  rc = (value-l1)*(h2-l2)/(h1-l1)+l2;
  return rc;  
}

// 文字コード取得（全角文字対応）
// ASC(文字列)
// ASC(文字列,文字位置)
// ASC(変数,文字位置)
int16_t iasc() {
  uint16_t value =0;
  int16_t len;     // 文字列長
  int16_t pos =1;  // 文字位置
  int16_t index;   // 配列添え字
  uint8_t* str;    // 文字列先頭位置
  
  if (checkOpen()) return 0;
  if ( *cip == I_STR) {  // 文字列定数の場合
     cip++;  len = *cip; // 文字列長の取得
     cip++;  str = cip;  // 文字列先頭の取得
     cip+=len;
  } else if ( *cip == I_VAR) {   // 変数の場合
     cip++;   str = v2realAddr(var[*cip]);
     len = *str;
     str++;
     cip++;     
  } else if ( *cip == I_ARRAY) { // 配列変数の場合
     cip++; 
     if (getParam(index, 0, SIZE_ARRY-1, false)) return 0;
     str = v2realAddr(arr[index]);
     len = *str;
     str++;
  } else {
    err = ERR_SYNTAX;
    return 0;
  }
  if ( *cip == I_COMMA) {
    cip++;
    if (getParam(pos,1,len,false)) return 0;
  }

  int16_t tmpPos = 0;
  int16_t i;
  for (i = 0; i < len; i++) {      
    if (pos == tmpPos+1)
      break;
    if (isZenkaku(str[i])) {
      i++;  
    }
    tmpPos++;
  }
  if (pos != tmpPos+1) {
    value = 0;
  } else {
    value = str[i];
    if(isZenkaku(str[i])) {
      value<<=8;
      value+= str[i+1];
    }
  }
  checkClose();
  return value;
}

// 文字列入力関数
// GETS(仮想アドレス)
// GETS(仮想アドレス,リミット)
// GETS(仮想アドレス,リミット,領域確保モード)
// GETS(仮想アドレス,リミット,領域確保モード,仮想アドレス)
// リミット:長さ 、
// 領域確保モード:0 入力 1:入力せず領域のみ確保or仮想アドレス内容コピー(リミット+長さ)
// 
//
int16_t igets() {
  int16_t vadr;                // 文字列格納仮想アドレス
  int16_t maxlen =32;          // デフォルト最大入力文字数
  int16_t value = -1;          // 文字格納仮想アドレス
  uint8_t* adr;                // 文字列格納実アドレス
  char* text;                  // 入力文字列先頭アドレス
  int16_t  len;                // 入力文字列長
  int16_t  mode = 0;           // 領域確保モード
  int16_t src_vadr = -1;       // コピー元文字列格納仮想アドレス
  uint8_t* src_adr;            // コピー元文字列格納実アドレス

  uint8_t rc;

  // 引数の取得
  if (checkOpen())  return 0;
  if (getParam(vadr, 0, 32767, false )) return value; // 文字列格納仮想アドレス
  if (*cip == I_COMMA) {
    cip++;
    if ( getParam(maxlen,  1,  SIZE_LINE, false) ) return value; // リミット
    if (*cip == I_COMMA) {
      cip++;
      if ( getParam(mode,  0,  1, false) ) return value;       // 領域確保モード
      if (*cip == I_COMMA) {
        cip++;
        if (getParam(src_vadr, 0, 32767, false )) return value; // コピー元文字列格納仮想アドレス      
        // 引数の整合性チェック
        if (v2realAddr(src_vadr) == 0 || v2realAddr(src_vadr+maxlen) == 0) {
          err = ERR_RANGE; return 0;
        }
        src_adr  = v2realAddr(src_vadr);
      }
    } 
  }

  checkClose();
  if (err) {
    return value;
  }

  // 文字列格納仮想アドレスの引数の整合性チェック
  if (v2realAddr(vadr) == 0 || v2realAddr(vadr+maxlen) == 0) {
     err = ERR_RANGE; return 0;
  }
  adr  = v2realAddr(vadr);

  if (mode == 0) {
    // 文字列の入力
    rc = sc->editLine();
    if (!rc) {
      // 入力中断
      adr[0] = 0;      // 長さのセット
      err = ERR_CTR_C; // エラー番号をセット
      newline();
      return value;
    }
    
    text = (char*)sc->getText(); // スクリーンバッファからテキスト取得 
    len = strlen(text);
    if (len) {
      if (len > maxlen)
        len = maxlen;
      strncpy((char*)&adr[1], text,len);
      adr[1+len] = 0;
      tlimR((char*)&adr[1]);   //文末の余分空白文字の削除
      len = strlen((char*)&adr[1]);   
    
      if ( len> 0 && isJMS(&adr[1],len-1) == 1) {
        // 最後の文字が全角1バイト目の場合は削除する
        adr[1+len-1] = 0;
        len--;
      }
    }
    adr[0] = len; // 長さのセット 
    adr[1+len] = 0;    
    newline();
  } else {
    // 領域確保or文字列のコピー
    if (src_vadr == -1) {
      // 領域初期化
      adr[0] = maxlen;        // 長さのセット
      memset(adr+1,0,maxlen); // 領域の初期化
    } else {
      // 文字列のコピー
      maxlen = MIN(strlen((char *)src_adr),maxlen) ;
      memcpy(adr+1,src_adr,maxlen);
      adr[0] = maxlen;        // 長さのセット
    }
  }
  value = vadr;
  return value; 
}

// 文字列比較
// STRCMP(文字列1,文字列2)
// STRCMP(文字列1,文字列2,長さ)
// 戻り値 1：一致、 0 ：不一致
int16_t istrcmp() {
  int16_t len[2];   // 文字列長
  int16_t index;    // 配列添え字
  int16_t optlen = -1; // 比較長さ
  uint8_t* str[2];  // 文字列先頭位置
  int16_t pos = 0;

  if (checkOpen())  return -1;
  for (int16_t i =0; i < 2; i++) {
    if ( *cip == I_VAR)  {
      // 変数の場合
      cip++;
      str[i] = v2realAddr(var[*cip]);
      len[i] = *str[i]; // 文字列長の取得
      str[i]++;      // 文字列先頭
      cip++;     
    } else if ( *cip == I_ARRAY) {
      // 配列変数の場合
      cip++; 
      if (getParam(index, 0, SIZE_ARRY-1, false)) return -1;
      str[i] = v2realAddr(arr[index]);
      len[i] = *str[i]; // 文字列長の取得
      str[i]++;      // 文字列先頭
    } else if ( *cip == I_STR) {
      // 文字列定数の場合
      cip++;  len[i] = *cip; // 文字列長の取得
      cip++;  str[i] = cip;  // 文字列先頭の取得
      cip+=len[i];
    } else {
      err = ERR_SYNTAX;
      return -1;
    }
    if (!i) {
      if (*cip != I_COMMA) {
        err = ERR_SYNTAX;
        return -1;
      } else {
        cip++;
      }
    }
  }

  // 長さ引数取得
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(optlen, 0, 32767, false)) return -1;
  }
  if (checkClose()) return -1;
  
  if (optlen == -1 ) {
    if (len[0] != len[1])
      return 0;
    else 
      return  strncmp((char*)str[0],(char*)str[1],len[0]) ? 0:1;
  } else {
    if (optlen > len[0] || optlen > len[1])
      return 0;
    else
      return  strncmp((char*)str[0],(char*)str[1],optlen) ? 0:1;
  }
}

// PRINT handler
void iprint(uint8_t devno=0,uint8_t nonewln=0) {
  short value;     //値
  short len;       //桁数
  unsigned char i; //文字数
  
  len = 0; //桁数を初期化
  while (*cip != I_COLON && *cip != I_EOL) { //文末まで繰り返す
    switch (*cip++) { //中間コードで分岐
    case I_STR:       //文字列
      i = *cip++;     //文字数を取得
      while (i--)     //文字数だけ繰り返す
        c_putch(*cip++, devno); //文字を表示
      break; 

    case I_SHARP: //「#
      len = iexp(); //桁数を取得
      if (err) {
        return;
      }
      break; 

    case I_CHR: ichr(devno); break;      // CHR$()関数
    case I_HEX:   ihex(devno); break;    // HEX$()関数
    case I_BIN:   ibin(devno); break;    // BIN$()関数
    case I_DMP:   idmp(devno); break;    // DMP$()関数
    case I_STRREF:istrref(devno); break; // STR$()関数
    case I_ELSE:                         // ELSE文がある場合は打ち切る
       newline(devno);
       return;
       break;
       
    default: //以上のいずれにも該当しなかった場合（式とみなす）
      cip--;
      value = iexp();   // 値を取得
      if (err) {
        newline();
        return;
      }
      putnum(value, len,devno); // 値を表示
      break;
    } //中間コードで分岐の末尾
    
    if (err)  {
        newline(devno);
        return;
    }
    if (nonewln && *cip == I_COMMA) { // 文字列引数流用時はここで終了
        return;
    }
    if (*cip == I_ELSE) {
        newline(devno); 
        return;
    } else if (*cip == I_COMMA || *cip == I_SEMI) { // もし',' ';'があったら
      cip++;
      if (*cip == I_COLON || *cip == I_EOL || *cip == I_ELSE) //もし文末なら      
        return;
    } else {    //',' ';'がなければ
      if (*cip != I_COLON && *cip != I_EOL) { //もし文末でなければ
        err = ERR_SYNTAX;
        newline(devno); 
        return;
      }
    }
  }
  if (!nonewln) {
    newline(devno);
  }
}


// ターミナルスクリーンの画面サイズ指定 WIDTH W,H
void iwidth() {
  int16_t w, h;

  // 引数チェック
  if ( getParam(w,  16, SIZE_LINE, true) ) return;   // w
  if ( getParam(h,  10,  45, false) ) return;        // h
  sc->cls();
  sc->locate(0,0);
  sc->end();
  sc->init(w, h, SIZE_LINE, workarea); // スクリーン初期設定
}
  
// エラーメッセージ出力
// 引数: dlfCmd プログラム実行時 false、コマンド実行時 true
void error(uint8_t flgCmd = false) {
  sc->show_curs(0);
  if (err) { 
    // もしプログラムの実行中なら（cipがリストの中にあり、clpが末尾ではない場合）
    if (cip >= listbuf && cip < listbuf + SIZE_LIST && *clp && !flgCmd) {
      // エラーメッセージを表示
      c_puts(errmsg[err]);       
      c_puts(" in ");
      putnum(getlineno(clp), 0); // 行番号を調べて表示
      newline();

      // リストの該当行を表示
      putnum(getlineno(clp), 0);
      c_puts(" ");
      putlist(clp + 3);          
      newline();
    } else {                   // 指示の実行中なら
      c_puts(errmsg[err]);     // エラーメッセージを表示
      newline();               // 改行
    }
  } 
  c_puts(errmsg[0]);           //「OK」を表示
  newline();                   // 改行
  err = 0;                     // エラー番号をクリア
  sc->show_curs(1);
}

// Get value
int16_t ivalue() {
  int16_t value; // 値

  switch (*cip++) { //中間コードで分岐

  //定数の取得
  case I_NUM:    // 定数
  case I_HEXNUM: // 16進定数
  case I_BINNUM: // 2進数定数
    value = *cip | *(cip + 1) << 8; //定数を取得
    cip += 2;
    break; 

  //+付きの値の取得
  case I_PLUS: //「+」
    value = ivalue(); //値を取得
    break; 

  //負の値の取得
  case I_MINUS: //「-」
    value = 0 - ivalue(); //値を取得して負の値に変換
    break; 

  case I_LNOT: //「!」
    value = !ivalue(); //値を取得してNOT演算
    break; 

  case I_BITREV: // 「~」 ビット反転
    //cip++; //中間コードポインタを次へ進める
    value = ~((uint16_t)ivalue()); //値を取得してNOT演算
    break;
  
  //変数の値の取得
  case I_VAR: //変数
    value = var[*cip++]; //変数番号から変数の値を取得して次を指し示す
    break;

  //括弧の値の取得
  case I_OPEN: //「(」
     cip--;
    value = getparam(); //括弧の値を取得
    break;

  //配列の値の取得
  case I_ARRAY: //配列
    value = getparam(); //括弧の値を取得
    if (!err) {
      if (value >= SIZE_ARRY)
         err = ERR_SOR;         // 添え字が範囲を超えた
      else 
         value = arr[value];    // 配列の値を取得
    }
    break;
  case I_RND: //関数RND
    value = getparam(); //括弧の値を取得
    if (!err) {
      if (value < 0 )
        err = ERR_VALUE;
      else
       value = getrnd(value); //乱数を取得
    }
    break;

  case I_ABS: //関数ABS
    value = getparam(); //括弧の値を取得
    if (value == -32768)
      err = ERR_VOF;
    if (err)
      break;
    if (value < 0) 
      value *= -1; //正負を反転
    break;

  case I_FREE: //関数FREE
   if (checkOpen()||checkClose()) break;
    value = getsize(); //プログラム保存領域の空きを取得
    break;

  case I_INKEY: //関数INKEY
   if (checkOpen()||checkClose()) break;   
    value = iinkey(); // キー入力値の取得
    break;

  case I_VPEEK: value = ivpeek();  break; // 関数VPEEK
  case I_MAP:   value = imap();    break; // 関数MAP(V,L1,H1,L2,H2)
  case I_ASC:   value = iasc();    break; // 関数ASC(文字列)
  case I_BYTE:  value = ilen();    break; // 関数BYTE(文字列)   
  case I_LEN:   value = ilen(1);   break; // 関数LEN(文字列)
  case I_TICK:  value = itick();   break; // 関数itick()
  case I_PEEK:  value = ipeek();   break; // PEEK()関数
  case I_GETS:  value = igets();    break;    // 関数GETS()  

  // 定数
  case I_HIGH:  value = CONST_HIGH; break;
  case I_LOW:   value = CONST_LOW;  break;
  case I_ON:    value = CONST_ON;   break;
  case I_OFF:   value = CONST_OFF;  break;
  //case I_LSB:   value = CONST_LSB;  break;
  //case I_MSB:   value = CONST_MSB;  break;

  case I_VRAM:  value = V_VRAM_TOP;  break;
  case I_MVAR:  value = V_VAR_TOP;   break;
  case I_MARRAY:value = V_ARRAY_TOP; break; 
  case I_MPRG:  value = V_PRG_TOP;   break;
  case I_MEM:   value = V_MEM_TOP;   break; 
  case I_MFNT:  value = V_FNT_TOP;   break;
  case I_GRAM:  value = V_GRAM_TOP;  break;
  case I_MPRG2: value = V_PRG2_TOP;  break;

  // 画面サイズ定数の参照
  case I_CW: value = sc->getWidth()   ; break;
  case I_CH: value = sc->getHeight()  ; break;

  // カーソル・スクロール等の方向
  case I_UP:    value = 0   ; break;
  case I_DOWN:  value = 1   ; break;
  case I_RIGHT: value = 2   ; break;
  case I_LEFT:  value = 3   ; break;

  default: //以上のいずれにも該当しなかった場合
    cip--;
    err = ERR_SYNTAX; //エラー番号をセット
    break; //ここで打ち切る
  }
  return value; //取得した値を持ち帰る
}

// multiply or divide calculation
short imul() {
  short value, tmp; //値と演算値

  value = ivalue(); //値を取得
  if (err) 
    return -1;

  while (1) //無限に繰り返す
  switch(*cip){ //中間コードで分岐

  case I_MUL: //掛け算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value *= tmp; //掛け算を実行
    break;

  case I_DIV: //割り算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    if (tmp == 0) { //もし演算値が0なら
      err = ERR_DIVBY0; //エラー番号をセット
      return -1;
    }
    value /= tmp; //割り算を実行
    break; 
    
  case I_DIVR: //剰余の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    if (tmp == 0) { //もし演算値が0なら
      err = ERR_DIVBY0; //エラー番号をセット
      return -1; //終了
    }
    value %= tmp; //割り算を実行
    break; 

  case I_LSHIFT: // シフト演算 "<<" の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)<<tmp;
    break;

  case I_RSHIFT: // シフト演算 ">>" の場合
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)>>tmp;
    break; 

   case I_AND:  // 算術積(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)&((uint16_t)tmp);
    break; //ここで打ち切る

   case I_OR:   //算術和(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)|((uint16_t)tmp);
    break; 

   case I_XOR: //非排他OR(ビット演算)
    cip++; //中間コードポインタを次へ進める
    tmp = ivalue(); //演算値を取得
    value =((uint16_t)value)^((uint16_t)tmp);
   
  default: //以上のいずれにも該当しなかった場合
    return value; //値を持ち帰る
  } //中間コードで分岐の末尾
}

// add or subtract calculation
short iplus() {
  short value, tmp; //値と演算値
  value = imul(); //値を取得
  if (err) 
    return -1;

  while (1) 
  switch(*cip){
  case I_PLUS: //足し算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = imul(); //演算値を取得
    value += tmp; //足し算を実行
    break;

  case I_MINUS: //引き算の場合
    cip++; //中間コードポインタを次へ進める
    tmp = imul(); //演算値を取得
    value -= tmp; //引き算を実行
    break;

  default: //以上のいずれにも該当しなかった場合
    return value; //値を持ち帰る
  } //中間コードで分岐の末尾
}

// The parser
short iexp() {
  short value, tmp; //値と演算値

  value = iplus(); //値を取得
  if (err) //もしエラーが生じたら
    return -1; //終了

  // conditional expression 
  while (1) //無限に繰り返す
  switch(*cip++){ //中間コードで分岐

  case I_EQ: //「=」の場合
    tmp = iplus(); //演算値を取得
    value = (value == tmp); //真偽を判定
    break; 
  case I_NEQ:   //「!=」の場合
  case I_NEQ2:  //「<>」の場合
  case I_SHARP: //「#」の場合
    tmp = iplus(); //演算値を取得
    value = (value != tmp); //真偽を判定
    break;
  case I_LT: //「<」の場合
    tmp = iplus(); //演算値を取得
    value = (value < tmp); //真偽を判定
    break;
  case I_LTE: //「<=」の場合
    tmp = iplus(); //演算値を取得
    value = (value <= tmp); //真偽を判定
    break;
  case I_GT: //「>」の場合
    tmp = iplus(); //演算値を取得
    value = (value > tmp); //真偽を判定
    break;
  case I_GTE: //「>=」の場合
    tmp = iplus(); //演算値を取得
    value = (value >= tmp); //真偽を判定
    break;
 case I_LAND: // AND (論理積)
    tmp = iplus(); //演算値を取得
    value = (value && tmp); //真偽を判定
    break;
 case I_LOR: // OR (論理和)
    tmp = iplus(); //演算値を取得
    value = (value || tmp); //真偽を判定
    break; 
  default: //以上のいずれにも該当しなかった場合
    cip--;
    return value; //値を持ち帰る
  } //中間コードで分岐の末尾
}

// 左上の行番号の取得
int16_t getTopLineNum() {
  uint8_t* ptr = sc->getScreen();
  uint32_t n = 0;
  int rc = -1;  
  while (isdigit(*ptr)) {
    n *= 10;
    n+= *ptr-'0';
    if (n>32767) {
      n = 0;
      break;
    }
    ptr++;
  }
  if (!n)
    rc = -1;
  else
    rc = n;
  return rc;
}

// 左下の行番号の取得
int16_t getBottomLineNum() {
  uint8_t* ptr = sc->getScreen()+sc->getWidth()*(sc->getHeight()-1);
  uint32_t n = 0;
  int rc = -1;  
  while (isdigit(*ptr)) {
    n *= 10;
    n+= *ptr-'0';
    if (n>32767) {
      n = 0;
      break;
    }
    ptr++;
  }
  if (!n)
    rc = -1;
  else
    rc = n;
  return rc;  
}

// 指定した行の前の行番号を取得する
int16_t getPrevLineNo(int16_t lineno) {
  uint8_t* lp, *prv_lp = NULL;
  int16_t rc = -1;
  for ( lp = listbuf; *lp && (getlineno(lp) < lineno); lp += *lp) {
    prv_lp = lp;
  }
  if (prv_lp)
    rc = getlineno(prv_lp);
  return rc;
}

// 指定した行の次の行番号を取得する
int16_t getNextLineNo(int16_t lineno) {
  uint8_t* lp;
  int16_t rc = -1;
  
  lp = getlp(lineno); 
  if (lineno == getlineno(lp)) { 
    // 次の行に移動
    lp+=*lp;
    rc = getlineno(lp);
  }
  return rc;
}

// 指定した行のプログラムテキストを取得する
char* getLineStr(int16_t lineno) {
    uint8_t* lp = getlp(lineno);
    if (lineno != getlineno(lp)) 
      return NULL;
    
    // 行バッファへの指定行テキストの出力
    cleartbuf();
    putnum(lineno, 0,3); // 行番号を表示
    c_putch(' ',3);    // 空白を入れる
    putlist(lp+3,3);   // 行番号より後ろを文字列に変換して表示        
    c_putch(0,3);      // \0を入れる
    return tbuf;
}

// システム情報の表示
void iinfo() {
  char top = 't';
  uint32_t adr = (uint32_t)&top;
  uint8_t* tmp = (uint8_t*)malloc(1);
  uint32_t hadr = (uint32_t)tmp;
  free(tmp);

  // スタック領域先頭アドレスの表示
  c_puts("Stack Top:");
  putHexnum((int16_t)(adr>>16),4);putHexnum((int16_t)(adr&0xffff),4);
  newline();
  
  // ヒープ領域先頭アドレスの表示
  c_puts("Heap Top :");
  putHexnum((int16_t)(hadr>>16),4);putHexnum((int16_t)(hadr&0xffff),4);
  newline();

  // SRAM未使用領域の表示
  c_puts("SRAM Free:");
  putnum((int16_t)(adr-hadr),0);
  newline();

  // シンボル定義の表示
  c_puts("symbol Num:");
  putnum((int16_t)I_EOL,0);
  newline();  
     
 
}

// ラベル
void ilabel() {
   cip+= *cip+1;   
}

// GOTO
void igoto() {
  uint8_t* lp;       // 飛び先行ポインタ
  int16_t lineno;    // 行番号

  if (*cip == I_STR) { 
    // ラベル参照による分岐先取得 
    lp = getlpByLabel(cip);                   
    if (lp == NULL) {
      err = ERR_ULN;                          // エラー番号をセット
      return;
    }  
  } else {
    // 引数の行番号取得
    lineno = iexp();                          
    if (err)  return;         
    lp = getlp(lineno);                       // 分岐先のポインタを取得
    if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
      err = ERR_ULN;                          // エラー番号をセット
      return; 
    }
  }
  clp = lp;        // 行ポインタを分岐先へ更新
  cip = clp + 3;   // 中間コードポインタを先頭の中間コードに更新  
}

// GOSUB
void igosub() {
  uint8_t* lp;       // 飛び先行ポインタ
  int16_t lineno;    // 行番号

  if (*cip == I_STR) {
    // ラベル参照による分岐先取得 
    lp = getlpByLabel(cip);                   
    if (lp == NULL) {
      err = ERR_ULN;  // エラー番号をセット
      return; 
    }  
  } else {
    // 引数の行番号取得
    lineno = iexp();
    if (err)
      return;  

    lp = getlp(lineno);                       // 分岐先のポインタを取得
    if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
      err = ERR_ULN;                          // エラー番号をセット
      return; 
    }
  }
  
  //ポインタを退避
  if (gstki > SIZE_GSTK - 2) {              // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;                       // エラー番号をセット
      return; 
  }
  gstk[gstki++] = clp;                      // 行ポインタを退避
  gstk[gstki++] = cip;                      // 中間コードポインタを退避

  clp = lp;                                 // 行ポインタを分岐先へ更新
  cip = clp + 3;                            // 中間コードポインタを先頭の中間コードに更新
}

// RETURN
void ireturn() {
  if (gstki < 2) {    // もしGOSUBスタックが空なら
    err = ERR_GSTKUF; // エラー番号をセット
    return; 
  }
  cip = gstk[--gstki]; //行ポインタを復帰
  clp = gstk[--gstki]; //中間コードポインタを復帰
  return;  
}

// FOR
void ifor() {
  short index, vto, vstep; // FOR文の変数番号、終了値、増分
  
  // 変数名を取得して開始値を代入（例I=1）
  if (*cip++ != I_VAR) { // もし変数がなかったら
    err = ERR_FORWOV;    // エラー番号をセット
    return;
  }
  index = *cip; // 変数名を取得
  ivar();       // 代入文を実行
  if (err)      // もしエラーが生じたら
    return;

  // 終了値を取得（例TO 5）
  if (*cip == I_TO) { // もしTOだったら
    cip++;             // 中間コードポインタを次へ進める
    vto = iexp();      // 終了値を取得
  } else {             // TOではなかったら
    err = ERR_FORWOTO; //エラー番号をセット
    return;
  }

  // 増分を取得（例STEP 1）
  if (*cip == I_STEP) { // もしSTEPだったら
    cip++;              // 中間コードポインタを次へ進める
    vstep = iexp();     // 増分を取得
  } else                // STEPではなかったら
    vstep = 1;          // 増分を1に設定

  // もし変数がオーバーフローする見込みなら
  if (((vstep < 0) && (-32767 - vstep > vto)) ||
    ((vstep > 0) && (32767 - vstep < vto))){
    err = ERR_VOF; //エラー番号をセット
    return;
  }

  // 繰り返し条件を退避
  if (lstki > SIZE_LSTK - 5) { // もしFORスタックがいっぱいなら
    err = ERR_LSTKOF;          // エラー番号をセット
    return;
  }
  lstk[lstki++] = clp; // 行ポインタを退避
  lstk[lstki++] = cip; // 中間コードポインタを退避

  // FORスタックに終了値、増分、変数名を退避
  // Special thanks hardyboy
  lstk[lstki++] = (unsigned char*)(uintptr_t)vto;
  lstk[lstki++] = (unsigned char*)(uintptr_t)vstep;
  lstk[lstki++] = (unsigned char*)(uintptr_t)index;  
}

// NEXT
void inext() {
  short index, vto, vstep; // FOR文の変数番号、終了値、増分

  if (lstki < 5) {    // もしFORスタックが空なら
    err = ERR_LSTKUF; // エラー番号をセット
    return;
  }

  // 変数名を復帰
  index = (short)(uintptr_t)lstk[lstki - 1]; // 変数名を復帰
  if (*cip++ != I_VAR) {                     // もしNEXTの後ろに変数がなかったら
    err = ERR_NEXTWOV;                       // エラー番号をセット
    return;
  }
  if (*cip++ != index) { // もし復帰した変数名と一致しなかったら
    err = ERR_NEXTUM;    // エラー番号をセット
    return;
  }

  vstep = (short)(uintptr_t)lstk[lstki - 2]; // 増分を復帰
  var[index] += vstep;                       // 変数の値を最新の開始値に更新
  vto = (short)(uintptr_t)lstk[lstki - 3];   // 終了値を復帰

  // もし変数の値が終了値を超えていたら
  if (((vstep < 0) && (var[index] < vto)) ||
    ((vstep > 0) && (var[index] > vto))) {
    lstki -= 5;  // FORスタックを1ネスト分戻す
    return;
  }

  // 開始値が終了値を超えていなかった場合
  cip = lstk[lstki - 4]; //行ポインタを復帰
  clp = lstk[lstki - 5]; //中間コードポインタを復帰
}

// IF
void iif() {
  int16_t condition;    // IF文の条件値
  uint8_t* newip;       // ELSE文以降の処理対象ポインタ

  condition = iexp(); // 真偽を取得
  if (err) {          // もしエラーが生じたら
    err = ERR_IFWOC;  // エラー番号をセット
    return;
  }
  if (condition) {    // もし真なら
    return;
  } else { 
    // 偽の場合の処理
    // ELSEがあるかチェックする
    // もしELSEより先にIFが見つかったらELSE無しとする        
    // ELSE文が無い場合の処理はREMと同じ
    newip = getELSEptr(cip);
    if (newip != NULL) {
      cip = newip;
      return;
    }
    while (*cip != I_EOL) // I_EOLに達するまで繰り返す
    cip++;                // 中間コードポインタを次へ進める        
  }
}

// スキップ
void iskip() {
  while (*cip != I_EOL) // I_EOLに達するまで繰り返す
    cip++;              // 中間コードポインタを次へ進める  
}

// END
void iend() {
  while (*clp)    // 行の終端まで繰り返す
    clp += *clp;  // 行ポインタを次へ進める  
}

// 中間コードの実行
// 戻り値      : 次のプログラム実行位置(行の先頭)
unsigned char* iexe() {
  err = 0;

  while (*cip != I_EOL) { //行末まで繰り返す
  
    //強制的な中断の判定
    if (isBreak())
      break;    

    //中間コードを実行
    switch (*cip++) {
    case I_STR:    ilabel();          break;  // 文字列の場合(ラベル)
    case I_GOTO:   igoto();           break;  // GOTOの場合
    case I_GOSUB:  igosub();          break;  // GOSUBの場合
    case I_RETURN: ireturn();         break;  // RETURNの場合
    case I_FOR:    ifor();            break;  // FORの場合
    case I_NEXT:   inext();           break;  // NEXTの場合
    case I_IF:     iif();             break;  // IFの場合
    case I_ELSE:   iskip();           break;  // 単独のELSEの場合     
    case I_SQUOT:  iskip();           break;  // 'の場合
    case I_REM:    iskip();           break;  // REMの場合
    case I_END:    iend();            break;  // ENDの場合
    case I_CLS:    icls();            break;  // CLS
    case I_WAIT:   iwait();           break;  // WAIT
    case I_LOCATE: ilocate();         break;  // LOCATE
    case I_COLOR:  icolor();          break;  // COLOR
    case I_ATTR:   iattr();           break;  // ATTR
    case I_VAR:    ivar();            break;  // 変数（LETを省略した代入文）
    case I_ARRAY:  iarray();          break;  // 配列（LETを省略した代入文）
    case I_LET:    ilet();            break;  // LET
    case I_QUEST:  iprint();          break;  // PRINT
    case I_PRINT:   iprint();         break;  // PRINT
    case I_INPUT:   iinput();         break;  // INPUT
    case I_POKE:    ipoke();          break;  // POKEコマンド
    case I_REFLESH:   sc->refresh();   break;  // REFLESHコマンド 画面再表示
    case I_CLV:       inew(2);        break;  // CLV 変数領域消去
    case I_INFO:      iinfo();        break;  // システム情報の表示(デバッグ用)
  
    case I_LIST:       sc->show_curs(0); ilist();  sc->show_curs(1);break;  // LIST
    case I_NEW:        inew();        break;   // NEW
    case I_WIDTH:      iwidth();      break;
    case I_RUN:    // RUN
    case I_RENUM:  // RENUM
    case I_DELETE: // DELETE
      err = ERR_COM; //エラー番号をセット
      return NULL; //終了

    case I_COLON: // 中間コードが「:」の場合
    case I_OK:    // OK
      break; 

    default:
     cip--;
    
     // 以上のいずれにも該当しない場合
     err = ERR_SYNTAX; //エラー番号をセット
     break;
    }  //中間コードで分岐の末尾
  
    if (err)
      return NULL;
  }
  return clp + *clp;
}

//Command precessor
uint8_t icom() {
  uint8_t rc = 1;
  cip = ibuf;          // 中間コードポインタを中間コードバッファの先頭に設定

  switch (*cip++) {    // 中間コードポインタが指し示す中間コードによって分岐
  case I_RUN:   sc->show_curs(0); irun();  sc->show_curs(1);   break; // RUN命令
  case I_RENUM: irenum(); break; // I_RENUMの場合
  case I_DELETE:idelete();  break;
  case I_REM:
  case I_SQUOT:    
  case I_OK:    rc = 0;     break; // I_OKの場合
  default:    // どれにも該当しない場合
    cip--;
    sc->show_curs(0);
    iexe();           // 中間コードを実行
    sc->show_curs(1);
    break;
  }
  return rc;
}

/*
  TOYOSHIKI Tiny BASIC
  The BASIC entry point
*/

void basic() {
  unsigned char len; // 中間コードの長さ
  uint8_t rc;        // 関数戻り値受け取り用

  
  // プログラム領域の初期化
  inew();              
  srand((unsigned int)time(NULL));

// ワークエリアの獲得
  workarea = (uint8_t*)malloc(5760); // SCREEN0で128x45まで

  // デバイススクリーンの初期化設定
  sc = &sc1;
  ((tTermscreen*)sc)->init(TERM_W,TERM_H,SIZE_LINE, workarea); // スクリーン初期設定

  char* textline;    // 入力行

  // 起動メッセージ  
  icls();
  sc->show_curs(0);                // 高速描画のためのカーソル非表示指定
  c_puts("TOYOSHIKI TINY BASIC");  // 「TOYOSHIKI TINY BASIC」を表示
  newline();                       // 改行
  c_puts(STR_EDITION);             // 版を区別する文字列「EDITION」を表示
  c_puts(" " STR_VARSION);         // バージョンの表示
  newline();                       // 改行
  err = 0;
  error();                         // 「OK」またはエラーメッセージを表示してエラー番号をクリア
  sc->show_curs(1);                // カーソル表示
      
  // 端末から1行を入力して実行（メインループ）
  sc->show_curs(1);
  while (1) { //無限ループ
    rc = sc->edit();  // エディタ入力
    if (rc) {
      textline = (char*)sc->getText(); // スクリーンバッファからテキスト取得
      if (!strlen(textline) ) {
        // 改行のみ
        newline();
        continue;
      }
      
      if (strlen(textline) >= SIZE_LINE) {
        // 入力文字が有効文字長を超えている
         err = ERR_LONG;
         newline();
         error();
         continue;  
      }
      // 行バッファに格納し、改行する
      strcpy(lbuf, textline);
      tlimR((char*)lbuf); //文末の余分空白文字の削除
      newline();
    } else {
      // 入力なし
      continue;
    }
    
    // 1行の文字列を中間コードの並びに変換
    len = toktoi();      // 文字列を中間コードに変換して長さを取得
    if (err) {           // もしエラーが発生したら
      error(true);       // エラーメッセージを表示してエラー番号をクリア
      continue;          // 繰り返しの先頭へ戻ってやり直し
    }

    //中間コードの並びがプログラムと判断される場合
    if (*ibuf == I_NUM) { // もし中間コードバッファの先頭が行番号なら
      *ibuf = len;        // 中間コードバッファの先頭を長さに書き換える
      inslist();          // 中間コードの1行をリストへ挿入
      if (err)            // もしエラーが発生したら
        error();          // エラーメッセージを表示してエラー番号をクリア
      continue;           // 繰り返しの先頭へ戻ってやり直し
    }

    // 中間コードの並びが命令と判断される場合
    if (icom())           // 実行する
        error(false);     // エラーメッセージを表示してエラー番号をクリア
  } // 無限ループの末尾
}
