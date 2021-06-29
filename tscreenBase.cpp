// 
// �X�N���[�������{�N���X �w�b�_�[�t�@�C��(Arduino STM32)
// �쐬�� 2017/06/27 by ���܋g����
// �C���� 2017/08/05 �t�@���N�V�����L�[��NTSC�œ��l�ɗ��p�\�Ή�
// �C���� 2017/08/12 edit_scrollUp() �ōŏI�s��2�s�ȏ�̏ꍇ�̏����~�X�C��
// �C���� 2017/10/15 ��`�����̂���KEY_F1�AKEY_F(n)��KEY_Fn1�AKEY_Fn(n)�ύX
// �C���� 2018/01/07 [ENTER]�L�[�����p��KEY_LF��ǉ�
// �C���� 2018/08/22, KEY_F(n)��KEY_F1,KEY_F2 .. �̒�`�ɕύX�Ή�
// �C���� 2018/08/23, SC_KEY_XXX ��KEY_XXX�ɕύX
// �C���� 2018/08/29 editLine()�i���p���͔Łj�̒ǉ�
// �C���� 2018/09/14 �qtTermscreen�N���X��splitLine()�AmargeLine() ��{�N���X�����Ɉڍs
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tscreenBase.h"

#if DEPEND_TTBASIC == 1
  int16_t getPrevLineNo(int16_t lineno);
  int16_t getNextLineNo(int16_t lineno);
  char* getLineStr(int16_t lineno);
#endif

// �X�N���[���̏����ݒ�
// ����
//  w      : �X�N���[����������
//  h      : �X�N���[���c������
//  l      : 1�s�̍ő咷
//  extmem : �O���l���������A�h���X NULL:�Ȃ� NULL�ȊO ����
// �߂�l
//  �Ȃ�
void tscreenBase::init(uint16_t w, uint16_t h, uint16_t l,uint8_t* extmem) {
  width   = w;
  height  = h;
  maxllen = l;
  flgCur = 0;
  
  // �f�o�C�X�̏�����
  INIT_DEV();

  // ���O�̊l���������̊J��
  if (!flgExtMem) {
  	if (screen != NULL) {
      free(screen);
    }
  }

  // �X�N���[���p�o�b�t�@�̈�̐ݒ�
  if (extmem == NULL) {
    flgExtMem = 0;
    screen = (uint8_t*)malloc( width * height );
  } else {
     flgExtMem = 1;
  	 screen = extmem;
  }
  
  cls();
  show_curs(true);  
  MOVE(pos_y, pos_x);

  // �ҏW�@�\�̐ݒ�
  flgIns = true;
}

// �X�N���[�����p�I��
void tscreenBase::end() {
  
  // �f�o�C�X�̏I������
  END_DEV();

  // ���I�m�ۂ����������[�̊J��
  if (!flgExtMem) {
    if (screen != NULL) {
      free(screen);
      screen = NULL;
    }
  }
}

// �w��s��1�s���N���A
void tscreenBase::clerLine(uint16_t l) {
  memset(screen+width*l, 0, width);
  CLEAR_LINE(l);
  MOVE(pos_y, pos_x);
}

// �X�N���[���̃N���A
void tscreenBase::cls() {
  CLEAR();
  memset(screen, 0, width*height);
}

// �X�N���[�����t���b�V���\��
void tscreenBase::refresh() {
  for (uint16_t i = 0; i < height; i++)
    refresh_line(i);
  MOVE(pos_y, pos_x);
}

// �s�̃��t���b�V���\��
void tscreenBase::refresh_line(uint16_t l) {
  CLEAR_LINE(l);
  for (uint16_t j = 0; j < width; j++) {
    if( IS_PRINT( VPEEK(j,l) )) { 
      WRITE(j,l,VPEEK(j,l));
    }
  }
}

// 1�s���X�N���[���̃X�N���[���A�b�v
// Windows 10�ł�SCROLL_UP()�����삵�Ȃ����߁A��ʍĕ\���őΉ�
void tscreenBase::scroll_up() {
  memmove(screen, screen + width, (height-1)*width);
  draw_cls_curs();
//  SCROLL_UP();
  clerLine(height-1);
  MOVE(pos_y, pos_x);
  refresh();
}

// 1�s���X�N���[���̃X�N���[���_�E��
void tscreenBase::scroll_down() {
  memmove(screen + width, screen, (height-1)*width);
  draw_cls_curs();
  SCROLL_DOWN();
  clerLine(0);
  MOVE(pos_y, pos_x);
}

// �w��s�ɋ󔒍s�}��
void tscreenBase::Insert_newLine(uint16_t l) {
  if (l < height-1) {
    memmove(screen+(l+2)*width, screen+(l+1)*width, width*(height-1-l-1));
  }
  memset(screen+(l+1)*width, 0, width);
  INSLINE(l+1);
}

// ���݂̃J�[�\���ʒu�̕����폜
void tscreenBase::delete_char() {
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* top = start_adr;
  uint16_t ln = 0;

  if (!*top) // 0�����폜�s�\
    return;
  
  while( *top ) { ln++; top++; } // �s�[,��������
  if ( ln > 1 ) {
    memmove(start_adr, start_adr + 1, ln-1); // 1�����l�߂�
  }
  *(top-1) = 0; 
  for (uint8_t i=0; i < (pos_x+ln)/width+1; i++)
    refresh_line(pos_y+i);   
  MOVE(pos_y,pos_x);
  return;
}

// �����̏o��
void tscreenBase::putch(uint8_t c) {
 VPOKE(pos_x, pos_y, c); // VRAM�ւ̏�����
 WRITE(pos_x, pos_y, c); // �X�N���[���ւ̏�����
 movePosNextNewChar();
}


// ���݂̃J�[�\���ʒu�ɕ�����}��
void tscreenBase::Insert_char(uint8_t c) {  
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* last = start_adr;
  uint16_t ln = 0;  

  // ���͈ʒu�̊���������(�J�[�\���ʒu����̒���)�̎Q��
  while( *last ) {
    ln++;
    last++;
  }
  if (ln == 0 || flgIns == false) {
     // �����񒷂���0�܂��͏㏑�����[�h�̏ꍇ�A���̂܂�1�����\��
    if (pos_y + (pos_x+ln+1)/width >= height) {
      // �ŏI�s�𒴂���ꍇ�́A�}���O��1�s��ɃX�N���[�����ĕ\���s���m��
      scroll_up();
      start_adr-=width;
      MOVE(pos_y-1, pos_x);
    } else  if ( (pos_x + ln >= width-1) && !VPEEK(width-1,pos_y) ) {
       // ��ʍ��[��1�����������ꍇ�ŁA���s�ƘA���łȂ��ꍇ�͉��̍s��1�s�󔒂�}������
       Insert_newLine(pos_y+(pos_x+ln)/width);       
    }
    putch(c);
  } else {
     // �}���������K�v�̏ꍇ  
    if (pos_y + (pos_x+ln+1)/width >= height) {
      // �ŏI�s�𒴂���ꍇ�́A�}���O��1�s��ɃX�N���[�����ĕ\���s���m��
      scroll_up();
      start_adr-=width;
      MOVE(pos_y-1, pos_x);
    } else  if ( ((pos_x + ln +1)%width == width-1) && !VPEEK(pos_x + ln , pos_y) ) {
       // ��ʍ��[��1�����������ꍇ�ŁA���s�ƘA���łȂ��ꍇ�͉��̍s��1�s�󔒂�}������
          Insert_newLine(pos_y+(pos_x+ln)/width);
    }
    // 1�����}���̂��߂�1�������̃X�y�[�X���m��
    memmove(start_adr+1, start_adr, ln);
    *start_adr=c; // �m�ۂ����X�y�[�X��1�����\��
    movePosNextNewChar();
    
    // �}�������s�̍ĕ\��
    for (uint8_t i=0; i < (pos_x+ln)/width+1; i++)
       refresh_line(pos_y+i);   
    MOVE(pos_y,pos_x);
  }
}


// ���s
void tscreenBase::newLine() {
  int16_t x = 0;
  int16_t y = pos_y+1;
  if (y >= height) {
     scroll_up();
     y--;
   }    
   MOVE(y, x);
}

// �J�[�\�����P���������Ɉړ�
void tscreenBase::movePosNextNewChar() {
 int16_t x = pos_x;
 int16_t y = pos_y; 
 x++;
 if (x >= width) {
    x = 0;
    y++;        
   if (y >= height) {
      scroll_up();
      y--;
    }    
 }
 MOVE(y, x);
}

// �J�[�\����1�������O�Ɉړ�
void tscreenBase::movePosPrevChar() {
  if (pos_x > 0) {
    if ( IS_PRINT(VPEEK(pos_x-1 , pos_y))) {
       MOVE(pos_y, pos_x-1);
    }
  } else {
   if(pos_y > 0) {
      if (IS_PRINT(VPEEK(width-1, pos_y-1))) {
         MOVE(pos_y-1, width - 1);
      } 
   }    
  }
}

// �J�[�\����1���������Ɉړ�
void tscreenBase::movePosNextChar() {
  if (pos_x+1 < width) {
    if ( IS_PRINT( VPEEK(pos_x ,pos_y)) ) {
      MOVE(pos_y, pos_x+1);
    }
  } else {
    if (pos_y+1 < height) {
        if ( IS_PRINT( VPEEK(0, pos_y + 1)) ) {
          MOVE(pos_y+1, 0);
        }
    }
  }
}

// �J�[�\�������s�Ɉړ�
void tscreenBase::movePosNextLineChar() {
  if (pos_y+1 < height) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y + 1)) ) {
      // �J�[�\����^���Ɉړ�
      MOVE(pos_y+1, pos_x);
    } else {
      // �J�[�\�������s�̍s�������Ɉړ�
      int16_t x = pos_x;
      while(1) {
        if (IS_PRINT(VPEEK(x, pos_y + 1)) ) 
           break;  
        if (x > 0)
          x--;
        else
          break;
      }      
      MOVE(pos_y+1, x);      
    }
  } else if (pos_y+1 == height) {
    edit_scrollUp();    
  }
}

// �J�[�\����O�s�Ɉړ�
void tscreenBase::movePosPrevLineChar() {
  if (pos_y > 0) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y-1)) ) {
      // �J�[�\����^��Ɉړ�
      MOVE(pos_y-1, pos_x);
    } else {
      // �J�[�\����O�s�̍s�������Ɉړ�
      int16_t x = pos_x;
      while(1) {
        if (IS_PRINT(VPEEK(x, pos_y - 1)) ) 
           break;  
        if (x > 0)
          x--;
        else
          break;
      }      
      MOVE(pos_y-1, x);      
    }
  } else if (pos_y == 0){
    edit_scrollDown();
  }
}

// �J�[�\�����s���Ɉړ�
void tscreenBase::moveLineEnd() {
  int16_t x = width-1;
  while(1) {
    if (IS_PRINT(VPEEK(x, pos_y)) ) 
       break;  
    if (x > 0)
      x--;
    else
      break;
  }        
  MOVE(pos_y, x);     
}

// �X�N���[���\���̍ŏI�\���̍s�擪�Ɉړ�
void tscreenBase::moveBottom() {
  int16_t y = height-1;
  while(y) {
    if (IS_PRINT(VPEEK(0, y)) ) 
       break;
    y--;
  }
  MOVE(y,0);
}

// �J�[�\�����w��ʒu�Ɉړ�
void tscreenBase::locate(uint16_t x, uint16_t y) {
  if ( x >= width )  x = width-1;
  if ( y >= height)  y = height;
  MOVE(y, x);
}

// �J�[�\���ʒu�̕����R�[�h�擾
uint16_t tscreenBase::vpeek(uint16_t x, uint16_t y) {
  if (x >= width || y >= height) 
     return 0;
  return VPEEK(x,y);
}

// �s�f�[�^�̓��͊m��
uint8_t tscreenBase::enter_text() {

  // ���݂̃J�[�\���ʒu�̍s�擪�A�h���X�擾
  uint8_t *ptr = &VPEEK(0, pos_y); 
  if (pos_x == 0 && pos_y)
    ptr--;

  // �|�C���^�������̂ڂ��āA�O�s����̕�����̘A�����𒲂ׂ�
  // ���̕�����擪�A�h���X��top�ɃZ�b�g����
  uint8_t *top = ptr;
  while (top > screen && *top != 0 )
    top--;
  if ( top != screen ) top++;
 text = top;
  return true;
}

// �w��s�̍s�ԍ��̎擾
int16_t tscreenBase::getLineNum(int16_t l) {
  uint8_t* ptr = screen+width*l;
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
  else {
    if (*ptr==32 && *(ptr+1) > 0)
      rc = n;
    else 
      rc = -1;
  }
  return rc;
}

// �ҏW����ʂ��X�N���[���A�b�v����
uint8_t tscreenBase::edit_scrollUp() {
  static uint16_t prvlineNum = 0; // ���O�̏����s�̍s��
#if DEPEND_TTBASIC == 0
   scroll_up();
#else
  // 1�s���X�N���[���A�b�v�����݂�
  int16_t lineno,nm,len;
  char* text;
  lineno = getLineNum(height-1); // �ŏI�s�̕\���s�ԍ��̎擾
  if (lineno <= 0) {
    lineno = prvlineNum;
   }
  if (lineno > 0) {
    // �擾�o�����ꍇ�A���̍s�ԍ����擾����
    nm = getNextLineNo(lineno); 
    if (nm > 0) {
      // ���̍s�����݂���
      text = getLineStr(nm);
      len = strlen(text);
      for (uint8_t i=0; i < len/width+1; i++) {
        scroll_up();
      }
      strcpy((char*)&VPEEK(0,height-1-(len/width)),text);
      for (uint8_t i=0; i < len/width+1; i++)
         refresh_line(height-1-i);
      prvlineNum = nm; // ����̏��������s��ێ�
    } else {
      prvlineNum = 0; // �ێ����Ă��������s���N���A
      scroll_up();      
    }
  } else {
    scroll_up();    
  }
  MOVE(pos_y, pos_x);
#endif
  return 0;
}

// �ҏW����ʂ��X�N���[���_�E������
uint8_t tscreenBase::edit_scrollDown() {
#if DEPEND_TTBASIC == 0
  scroll_down();
#else
  // 1�s���X�N���[���_�E�������݂�
  int16_t lineno,prv_nm,len;
  char* text;
  lineno = getLineNum(0); // �ŏI�s�̕\���s�ԍ��̎擾
  if (lineno > 0) {
    prv_nm = getPrevLineNo(lineno);
    if (prv_nm > 0) {
      text = getLineStr(prv_nm);
      len = strlen(text);
      for (uint8_t i=0; i < len/width+1; i++) {
        scroll_down();
      }
      strcpy((char*)&VPEEK(0,0),text);
      //refresh();
      for (uint8_t i=0; i < len/width+1; i++)
         refresh_line(0+i);
    } else {
      scroll_down();      
    }
  } else {
    scroll_down();
  }
 MOVE(pos_y, pos_x);
#endif
  return 0;
}

// ���C���ҏW�i���p���͔Łj
// ���f�̏ꍇ�A0��Ԃ�
uint8_t tscreenBase::editLine() {
  uint16_t basePos_x = pos_x;
  uint16_t basePos_y = pos_y;
  uint16_t ch;  // ���͕���  
  
  show_curs(true);
  for(;;) {
    ch = get_ch();
    switch(ch) {
      case KEY_CR:          // [Enter]�L�[
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
// ���ݍs�̖����Ɏ��̍s����������
void tscreenBase::margeLine() {
  if (pos_y >= height-1)
    return;
  
  uint8_t* start_adr = &VPEEK(0,pos_y);       // ���݈ʒu�̃A�h���X�擾
  uint8_t* top = start_adr;                   // �s���̐擪
  uint16_t ln = 0;                            // �����敶���񒷂�
  while( *top ) { ln++; top++; }              // �s�[,��������
  
  if (ln > width)
    return;
  
  uint8_t* next_start_adr = &VPEEK(0,pos_y+1);   // ���̍s�̐擪�̃A�h���X�擾
  uint8_t* next_top = next_start_adr;            // ���̍s�̍s���̐擪
  uint16_t next_ln = 0;                          // ���̍s�̌������镶���񒷂�
  while( *next_top ) { next_ln++; next_top++; }  // �s�[,��������
  uint16_t offset = 0;
  if (ln + next_ln >= width) {
    offset = width - ln ;
  }  
    
  // �s�̌���
  memmove(top, next_start_adr, next_ln);
  
  // �������̃f�[�^����
  if (offset)
    memset(next_start_adr+next_ln-offset, 0,offset);
  else
    memset(next_start_adr, 0,+next_ln);
  refresh(); 
}

// �J�[�\���ʒu�ōs�𕪊�����
void tscreenBase::splitLine() {
  uint8_t* start_adr = &VPEEK(pos_x,pos_y); // ���݈ʒu�̃A�h���X�擾
  uint8_t* top = start_adr;                 // �������镶����̐擪
  uint16_t ln = 0;                          // �������镶���񒷂�
  uint16_t insLine = 0;                     // �}������s��
  
  if (!*top) // 0���������s�\
    return;
  
  while( *top ) { ln++; top++; } // �s�[,��������
  if (pos_x+ln > width-1) 
    ln = width - pos_x;
  
  insLine = ln/width + 1;        // ���ɑ}������s��
  if (pos_y + insLine > height-1) {
    return; // �����s�\
  }
  
  // ���̍s�ɋ󔒑}��
  for (uint8_t i = 0; i < insLine; i++) {
    Insert_newLine(pos_y);
  }
  
  // �����s�̈ړ�
  memmove(&VPEEK(0,pos_y+1), start_adr, ln);
  
  // �ړ����̏���
  memset(start_adr,0,ln);
  
  refresh(); 
}  
