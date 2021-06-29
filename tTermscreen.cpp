//
// file:tTermscreen.cpp
// �^�[�~�i���X�N���[�����䃉�C�u���� for Arduino STM32
//  V1.0 �쐬�� 2017/03/22 by ���܋g����
//  �C���� 2017/03/26, �F����֘A�֐��̒ǉ�
//  �C���� 2017/03/30, moveLineEnd()�̒ǉ�,[HOME],[END]�̕ҏW�L�[�̎d�l�ύX
//  �C���� 2017/06/27, �ėp���̂��߂̏C��
//  �C���� 2018/08/22, KEY_F(n)��KEY_F1,KEY_F2 .. �̒�`�ɕύX�Ή�
//  �C���� 2018/08/23, SC_KEY_XXX ��KEY_XXX�ɕύX
//  �C���� 2018/08/23, �S�p����(SJIS)�Ή�
//  �C���� 2018/08/29 editLine()�i�S�p�Ή��Łj�̒ǉ�
//  �C���� 2018/09/14 edit() [F1]�ł̃N���A���A�z�[���߂�ǉ�

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include "tTermscreen.h"

// http://katsura-kotonoha.sakura.ne.jp/prog/c/tip00010.shtml
//*********************************************************
// ������ str �� str[nPos] �ɂ��āA
//   �O �c�c �P�o�C�g����
//   �P �c�c �Q�o�C�g�����̈ꕔ�i��P�o�C�g�j
//   �Q �c�c �Q�o�C�g�����̈ꕔ�i��Q�o�C�g�j
// �̂����ꂩ��Ԃ��B
//*********************************************************
#define jms1(c) (((0x81<=c)&&(c<=0x9F))||((0xE0<=c)&&(c<=0xFC))) 
#define jms2(c) ((0x7F!=c)&&(0x40<=c)&&(c<=0xFC))
int isJMS( uint8_t *str, uint16_t nPos ) {
	int i;
	int state; // { 0, 1, 2 }

	state = 0;
	for( i = 0; str[i] != '\0'; i++ )	{
		if      ( ( state == 0 ) && ( jms1( str[i] ) ) ) state = 1; // 0 -> 1
		else if ( ( state == 1 ) && ( jms2( str[i] ) ) ) state = 2; // 1 -> 2
		else if ( ( state == 2 ) && ( jms1( str[i] ) ) ) state = 1; // 2 -> 1
		else                                             state = 0; // 2 -> 0, ���̑�
		// str[nPos] �ł̏�Ԃ�Ԃ��B
		if ( i == nPos ) return state;
	}
	return 0;
}//isJMS

//******* mcurses�p�t�b�N�֐��̒�`(�J�n)  *****************************************
static tTermscreen* tsc = NULL;

// �V���A���o�R1�����o��

static void Arduino_putchar(uint8_t c) {
	putchar(c);
}

// �V���A���o�R1��������
static char Arduino_getchar() {
	return _getch();
}
//******* mcurses�p�t�b�N�֐��̒�`(�I��)  *****************************************

//****** �V���A���^�[�~�i���f�o�C�X�ˑ��̃����o�[�֐��̃I�[�o�[���C�h��`(�J�n) ****

// �J�[�\���̈ړ�
// pos_x,pos_y�͖{�֐��݂̂ł̂ݕύX�\
// �J�[�\���̕\�����s��
void tTermscreen::MOVE(uint8_t y, uint8_t x) {
  ::move(y,x);
  pos_x = x;
  pos_y = y;
};

// �����̕\��
void tTermscreen::WRITE(uint8_t x, uint8_t y, uint8_t c) {
  ::move(y,x);
  ::addch(c);
  ::move(pos_y, pos_x);
}
    
void tTermscreen::CLEAR() {
  ::clear();
}

// �s�̏���
void tTermscreen::CLEAR_LINE(uint8_t l) {
  ::move(l,0);  ::clrtoeol();  // �ˑ��֐�  
}

// �X�N���[���A�b�v
void tTermscreen::SCROLL_UP() {
  ::scroll();
}

// �X�N���[���_�E��
void tTermscreen::SCROLL_DOWN() {
  INSLINE(0);
}

// �w��s��1�s�}��(���X�N���[��)
void tTermscreen::INSLINE(uint8_t l) {
  ::move(l,0);
  ::insertln();
  ::move(pos_y,pos_x);
}

// �ˑ��f�o�C�X�̏�����
// �V���A���R���\�[�� mcurses�̐ݒ�
void tTermscreen::INIT_DEV() {
  // mcurses�̐ݒ�
  ::setFunction_putchar(Arduino_putchar);  // �ˑ��֐�
  ::setFunction_getchar(Arduino_getchar);  // �ˑ��֐�
  ::initscr();                             // �ˑ��֐�
  ::setscrreg(0,height-1);
  tsc = this;
}

// �L�[���̓`�F�b�N
uint8_t tTermscreen::isKeyIn() {	
 if (kbhit())
    return get_ch();
}

// ��������
uint8_t tTermscreen::get_ch() {
	uint8_t c = cgetch();
  return c;
}

// �L�[���̓`�F�b�N(�����Q��)
int16_t tTermscreen::peek_ch() {
	return kbhit();
}

// �J�[�\���̕\��/��\��
// flg: �J�[�\����\�� 0�A�\�� 1�A�����\�� 2
void tTermscreen::show_curs(uint8_t flg) {
    flgCur = flg;
    ::curs_set(flg);  // �ˑ��֐�
}

// �J�[�\���̏���
void tTermscreen::draw_cls_curs() {  

}

// �����F�w��
void tTermscreen::setColor(uint16_t fc, uint16_t bc) {
  static const uint16_t tbl_fcolor[]  =
     { F_BLACK,F_RED,F_GREEN,F_BROWN,F_BLUE,F_MAGENTA,F_CYAN,A_NORMAL,F_YELLOW};
  static const uint16_t tbl_bcolor[]  =
     { B_BLACK,B_RED,B_GREEN,B_BROWN,B_BLUE,B_MAGENTA,B_CYAN,B_WHITE,B_YELLOW};

  if ( fc <= 8 && bc <= 8 )
     attrset(tbl_fcolor[fc]|tbl_bcolor[bc]);  // �ˑ��֐�
}

// ��������
void tTermscreen::setAttr(uint16_t attr) {
  static const uint16_t tbl_attr[]  =
    { A_NORMAL, A_UNDERLINE, A_REVERSE, A_BLINK, A_BOLD };
  
  if ( attr <= 4 )
     attrset(tbl_attr[attr]);  // �ˑ��֐�
}


//****** �V���A���^�[�~�i���f�o�C�X�ˑ��̃����o�[�֐��̃I�[�o�[���C�h��`(�I��) ****

//****** �S�p�����Ή��̂��߂̊֐��Ē�`�E�ǉ�**************************************

// �����̎擾�i�V�t�gJIS�Ή�)
uint16_t tTermscreen::get_wch() {
  uint8_t ch1,ch2;           // ���͕���
  uint16_t wch;              // 2�o�C�g�����R�[�h
  ch1 = get_ch();
  if (isShiftJIS(ch1)) {
     ch2 = get_ch();
     wch = ch1<<8 | ch2;
  } else {
    wch = ch1;
  }
  return wch;
}

// �J�[�\����1���������Ɉړ�(�S�p�Ή�)
void tTermscreen::movePosNextChar() {
  if (pos_x+1 < width) {
    if ( IS_PRINT( VPEEK(pos_x ,pos_y)) ) {
      if ( isShiftJIS(VPEEK(pos_x ,pos_y)) ) {
        // ���݈ʒu���S�p1�o�C�g�ڂ̏ꍇ,2�o�C�g���ړ�����
        if (pos_x+2 < width) {
          MOVE(pos_y, pos_x+2);
        } else {
          if (pos_y+1 < height) {
            if ( IS_PRINT(VPEEK(0, pos_y + 1)) ) {
              MOVE(pos_y+1, 0);
            }
          }
        }
      } else { 
        MOVE(pos_y, pos_x+1);
      }
    }
  } else {
    if (pos_y+1 < height) {
      if ( IS_PRINT(VPEEK(0, pos_y + 1)) ) {
        MOVE(pos_y+1, 0);
      }
    }
  }
}

// �J�[�\�������s�Ɉړ�(�S�p�Ή�)
void tTermscreen::movePosNextLineChar() {
  if (pos_y+1 < height) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y + 1)) ) {
      // �J�[�\����^���Ɉړ�
      if ( (isJMS(&VPEEK(0,pos_y+1),pos_x) == 2) &&  (pos_x > 0) ) {
         // �^�����S�p2�o�C�g�ڂ̏ꍇ�A�S�p1�o�C�g�ڂɃJ�[�\�����ړ����� 
         MOVE(pos_y+1, pos_x-1);
      } else {
         MOVE(pos_y+1, pos_x);
      }
    } else {
      // �J�[�\�������s�̍s�������Ɉړ�
      int16_t x = pos_x;
      for(;;) {
        if (IS_PRINT(VPEEK(x, pos_y + 1)) ) 
           break;  
        if (x > 0)
          x--;
        else
          break;
      }
      if ( !isShiftJIS(VPEEK(x ,pos_y+1)) && x > 0 && isShiftJIS(VPEEK(x-1 ,pos_y+1)) ) {
         MOVE(pos_y+1, x-1);
       } else {
         MOVE(pos_y+1, x);      
       }
    }
  } else if (pos_y+1 == height) {
    edit_scrollUp();    
  }
}

// �J�[�\����1�������O�Ɉړ�(�S�p�Ή�)
void tTermscreen::movePosPrevChar() {
  if (pos_x > 0) {
    if ( IS_PRINT(VPEEK(pos_x-1 , pos_y)) ) {
        // 1�O�̕������S�p2�o�C�g�ڂ����`�F�b�N
        if ( (pos_x -2 >= 0) && (isJMS(&VPEEK(0,pos_y),pos_x-1) !=2 ) ) {
          MOVE(pos_y, pos_x-1);
        } else if ( (pos_x -2 >= 0) && isShiftJIS(VPEEK(pos_x-2 , pos_y)) ) {
          // �S�p�����Ή�
          MOVE(pos_y, pos_x-2);
        } else {
          MOVE(pos_y, pos_x-1);
        }
    }
  } else {
   if(pos_y > 0) {
      if ( IS_PRINT(VPEEK(width-1, pos_y-1)) ) {
        if ( isShiftJIS(VPEEK(width-2 , pos_y-1)) ) {
          // �S�p�����Ή�
          MOVE(pos_y-1, width - 2);
        } else {
          MOVE(pos_y-1, width - 1);
        }
      } 
    }
  }
}

// �J�[�\����O�s�Ɉړ�(�S�p�Ή�)
void tTermscreen::movePosPrevLineChar() {
  if (pos_y > 0) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y-1)) ) {
      // �J�[�\����^��Ɉړ�
      if ( (isJMS(&VPEEK(0,pos_y-1),pos_x) == 2) &&  (pos_x > 0) ) {
         // �^�オ�S�p2�o�C�g�ڂ̏ꍇ�A�S�p1�o�C�g�ڂɃJ�[�\�����ړ����� 
         MOVE(pos_y-1, pos_x-1);
      } else {
         MOVE(pos_y-1, pos_x);
      }
    } else {
      // �J�[�\���̐^��ɕ����������ꍇ�́A�O�s�̍s�������Ɉړ�����
      int16_t x = pos_x;
      for(;;) {
        if (IS_PRINT(VPEEK(x, pos_y - 1)) ) 
           break;  
        if (x > 0)
          x--;
        else
          break;
      }      
      if ( !isShiftJIS(VPEEK(x ,pos_y-1)) && (x > 0) && isShiftJIS(VPEEK(x-1 ,pos_y-1)) ) {
         // �s�����S�p2�o�C�g�ڂ̏ꍇ�A�J�[�\����S�p1�o�C�g�ڂɈړ�����
         MOVE(pos_y-1, x-1);
       } else {
         MOVE(pos_y-1, x);      
       }
    }
  } else if (pos_y == 0){
    edit_scrollDown();
  }
}

// �J�[�\�����s���Ɉړ�(�S�p�Ή�)
void tTermscreen::moveLineEnd() {
  int16_t x = width-1;
  for(;;) {
    if (IS_PRINT(VPEEK(x, pos_y)) ) 
       break;  
    if (x > 0)
      x--;
    else
      break;
  }
  if (x>1 && (isJMS(&VPEEK(0,pos_y),x) == 2) ) {
    // �V�t�gJIS�Q�o�C�g�ڂ̏ꍇ�A�J�[�\�����P�o�C�g�ڂɈړ����� 
    x--;
  }
  MOVE(pos_y, x);     
}

// �w��s���폜
void tTermscreen::deleteLine(uint16_t l) {
  if (l < height-1) {
    memmove(&VPEEK(0,l), &VPEEK(0,l+1), width*(height-1-l));
  }
  memset(&VPEEK(0,height-1), 0, width);
  refresh();
}

// ���݂̃J�[�\���ʒu�̕����폜(�S�p�Ή�)

void tTermscreen::delete_char() {
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* top = start_adr;
  uint16_t ln = 0;

  if (!*top) {
    if (pos_y < height-1 && pos_x == 0) {
       // �󔒍s���l�߂�
      deleteLine(pos_y);
      refresh();
      return;
    } else {
       return; // 0�����폜�s�\
    }
  }
    
  while( *top ) { ln++; top++; } // �s�[,��������
  if (isShiftJIS(*start_adr) && ln>=2) {
    memmove(start_adr, start_adr + 2, ln-2); // 2�����l�߂�
    *(top-1) = 0;
    *(top-2) = 0;
  } else if ( ln >=1 ) {
    memmove(start_adr, start_adr + 1, ln-1); // 1�����l�߂�
    *(top-1) = 0; 
  }

  for (uint8_t i=0; i < (pos_x+ln)/width+1; i++)
    refresh_line(pos_y+i);   
  MOVE(pos_y,pos_x);
  return;
}

// �s�̍ĕ\��
void tTermscreen::refresh_line(uint16_t l) {
  CLEAR_LINE(l);
  for (uint16_t j = 0; j < width; j++) {
    if( IS_PRINT( VPEEK(j,l) )) { 
      WRITE(VPEEK(j,l));
    }
  }
}

// �����̏o��
void tTermscreen::putch(uint8_t c) {
  VPOKE(pos_x, pos_y, c); // VRAM�ւ̏�����
  WRITE(c);
  movePosNextNewChar();
}

// �����̏o�́i�V�t�gJIS�Ή�)
void tTermscreen::putwch(uint16_t c) {
  if (c>0xff) { // 2�o�C�g����
   VPOKE(pos_x, pos_y, c>>8);     // VRAM�ւ̏�����
   VPOKE(pos_x+1, pos_y, c&0xff); // VRAM�ւ̏�����
   WRITE(c>>8); WRITE(c&0xff);
   
   movePosNextNewChar();
   movePosNextNewChar();
  } else {     // 1�o�C�g����
   VPOKE(pos_x, pos_y, c);        // VRAM�ւ̏�����
   WRITE(c);
   movePosNextNewChar();
  }  
}

// �����̑}��
void tTermscreen::Insert_char(uint16_t c) {  
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* last = start_adr;
  uint16_t ln = 0;
  uint8_t clen = (c>0xff) ? 2:1 ; // �����o�C�g��
  
  // ���͈ʒu�̊���������(�J�[�\���ʒu����̒���)�̎Q��
  while( *last ) {
    ln++;
    last++;
  }
  if (ln == 0 || flgIns == false) {
     // �����񒷂���0�܂��͏㏑�����[�h�̏ꍇ�A���̂܂�1�����\��
    if (pos_y + (pos_x+ln+clen)/width >= height) {
      // �ŏI�s�𒴂���ꍇ�́A�}���O��1�s��ɃX�N���[�����ĕ\���s���m��
      scroll_up();
      start_adr-=width;
      MOVE(pos_y-1, pos_x);
    } else  if ( (pos_x + ln >= width-1) && !VPEEK(width-1,pos_y) ) {
       // ��ʍ��[��1�����������ꍇ�ŁA���s�ƘA���łȂ��ꍇ�͉��̍s��1�s�󔒂�}������
       Insert_newLine(pos_y+(pos_x+ln)/width);       
    }
    putwch(c);
  } else {
     // �}���������K�v�̏ꍇ
    if (pos_y + (pos_x+ln+clen)/width >= height) {
      // �ŏI�s�𒴂���ꍇ�́A�}���O��1�s��ɃX�N���[�����ĕ\���s���m��
      scroll_up();
      start_adr-=width;
      MOVE(pos_y-1, pos_x);
    } else  if ( ((pos_x + ln +clen)%width == width-clen) && !VPEEK(pos_x + ln , pos_y) ) {
       // ��ʍ��[��1�����������ꍇ�ŁA���s�ƘA���łȂ��ꍇ�͉��̍s��1�s�󔒂�}������
          Insert_newLine(pos_y+(pos_x+ln)/width);
    }
    // 1�����}���̂��߂�1�������̃X�y�[�X���m��
    memmove(start_adr+clen, start_adr, ln);
    if (clen ==1) {
      *start_adr=c; // �m�ۂ����X�y�[�X��1�����\��
      movePosNextNewChar();

    } else {
      *start_adr     = (c>>8);   // �m�ۂ����X�y�[�X��1�o�C�g��
      *(start_adr+1) = c & 0xff; // �m�ۂ����X�y�[�X��2�o�C�g��
      movePosNextNewChar();
      movePosNextNewChar();
    }
    
    // �}�������s�̍ĕ\��
    for (uint8_t i=0; i < (pos_x+ln)/width+1; i++)
       refresh_line(pos_y+i);   
    MOVE(pos_y,pos_x);
  }
}

// ���C���ҏW�i�S�p�Ή��Łj
// ���f�̏ꍇ�A0��Ԃ�
uint8_t tTermscreen::editLine() {
  uint16_t basePos_x = pos_x;
  uint16_t basePos_y = pos_y;
  uint16_t ch;  // ���͕���  
  
  show_curs(true);
  for(;;) {
    ch = get_wch();
    switch(ch) {
      case KEY_CR:         // [Enter]�L�[
        show_curs(false);
        text = &VPEEK(basePos_x, basePos_y);
        return 1;
        break;
 
      case KEY_HOME:       // [HOME�L�[] �s�擪�ړ�
        locate(basePos_x, basePos_y);
        break;
        
      case KEY_F5:         // [F5],[CTRL_R] ��ʍX�V
        //beep();
        refresh();  break;

      case KEY_END:        // [END�L�[] �s�̉E�[�ړ�
         moveLineEnd();
         break;

      case KEY_IC:         // [Insert]�L�[
        flgIns = !flgIns;
        break;        

      case KEY_BACKSPACE:  // [BS]�L�[
        if (pos_x > basePos_x) {
          movePosPrevChar();
          delete_char();
        }
        break;        

      case KEY_RIGHT:      // [��]�L�[
        if (pos_x < width-1) {
          movePosNextChar();
        }
        break;

      case KEY_LEFT:       // [��]�L�[
        if (pos_x > basePos_x) {
          movePosPrevChar();
        }
        break;

      case KEY_DC:         // [Del]�L�[
      case KEY_CTRL_X:
        delete_char();
        break;        
    
      case KEY_CTRL_C:   // [CTRL_C] ���f
      case KEY_ESCAPE:
        return 0;

      default:               // ���̑�
      if (IS_PRINT(ch) && (pos_x <width-1) ) {
          Insert_char(ch);
        }  
        break;
    }
  }
}

// �X�N���[���ҏW
uint8_t tTermscreen::edit() {
  uint16_t ch;  // ���͕���  
  for(;;) {
    ch = get_wch();   
    show_curs(false);
    switch(ch) {
      case KEY_CR:         // [Enter]�L�[
        show_curs(true);
        return enter_text();
        break;

      case KEY_F1:        // [F1],[CTRL+L] ��ʃN���A
        cls();
        locate(0,0);
        break;
 
      case KEY_HOME:      // [HOME]�L�[ �s�擪�ړ�
        locate(0, pos_y);
        break;
        
      case KEY_NPAGE:      // [PageDown] �\���v���O�����ŏI�s�Ɉړ�
        if (pos_x == 0 && pos_y == height-1) {
          edit_scrollUp();
        } else {
          moveBottom();
        }
        break;
      
      case KEY_PPAGE:     // [PageUP] ���(0,0)�Ɉړ�
        if (pos_x == 0 && pos_y == 0) {
          edit_scrollDown();
        } else {
          locate(0, 0);
        }  
        break;
        
      case KEY_F5:         // [F5],[CTRL_R] ��ʍX�V
        beep();
        refresh();  break;

      case KEY_END:        // [END]�L�[ �s�̉E�[�ړ�
         moveLineEnd();
         break;

      case KEY_IC:         // [Insert]�L�[
        flgIns = !flgIns;
        break;        

      case KEY_BACKSPACE:  // [BS]�L�[
        movePosPrevChar();
        delete_char();
        break;        

      case KEY_DC:         // [Del]�L�[
      case KEY_CTRL_X:
        delete_char();
        break;        
      
      case KEY_RIGHT:      // [��]�L�[
        movePosNextChar();
        break;

      case KEY_LEFT:       // [��]�L�[
        movePosPrevChar();
        break;

      case KEY_DOWN:       // [��]�L�[
        movePosNextLineChar();
        break;
      
      case KEY_UP:         // [��]�L�[
        movePosPrevLineChar();
        break;

      case KEY_F3:        // [F3],[CTRL N] �s�}��
        Insert_newLine(pos_y);       
        break;

      case KEY_F2:        // [F2], [CTRL D] �s�폜
        clerLine(pos_y);
        break;

      case KEY_F7:        // [F7] �s�̕���
        splitLine();
        break;

      case KEY_F8:        // [F8] �s�̌���
        margeLine();
        break;
      
      default:            // ���̑� ���\�����͑}���\��
      
      if (IS_PRINT(ch)) {
        Insert_char(ch);
      }  
      break;
    }
    show_curs(true);
  };
   show_curs(true);
}

