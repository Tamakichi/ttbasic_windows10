// 
// �X�N���[�������{�N���X �w�b�_�[�t�@�C��
// �쐬�� 2017/06/27 by ���܋g����
// �C���� 2017/09/15 IsCurs()  �J�[�\���\���L���̎擾�̒ǉ�
// �C���� 2017/10/15 ��`�����̂���KEY_F1�AKEY_F(n)��KEY_Fn1�AKEY_Fn(n)�ύX
// �C���� 2018/01/07 [ENTER]�L�[�����p��KEY_LF��ǉ�
// �C���� 2018/08/23 �L�[�����R�[�h��mcurses�̒�`�ɓ���
// �C���� 2018/08/29 editLine()�i���p���͔Łj�̒ǉ�
// �C���� 2018/09/14 �qtTermscreen�N���X��splitLine()�AmargeLine() ��{�N���X�����Ɉڍs

#ifndef __tscreenBase_h__
#define __tscreenBase_h__

#define DEPEND_TTBASIC           1     // �L�l�GTinyBASIC�ˑ������p�̗L�� 0:���p���Ȃ� 1:���p����

#include "Arduino.h"
#include "tSerialDev.h"
#include "mcurses.h"

// VRAM�Q�ƃ}�N����`
#define VPEEK(X,Y)      (screen[width*(Y)+(X)])
#define VPOKE(X,Y,C)    (screen[width*(Y)+(X)]=C)

class tscreenBase /*: public tSerialDev*/ {
  protected:
    uint8_t* screen;            // �X�N���[���p�o�b�t�@
    uint16_t width;             // �X�N���[�����T�C�Y
    uint16_t height;            // �X�N���[���c�T�C�Y
    uint16_t maxllen;           // 1�s�ő咷��
    uint16_t pos_x;             // �J�[�\�����ʒu
    uint16_t pos_y;             // �J�[�\���c�ʒu
    uint8_t*  text;             // �s�m�蕶����
    uint8_t flgIns;             // �ҏW���[�h
    uint8_t dev;                // �������̓f�o�C�X
    uint8_t flgCur;             // �J�[�\���\���ݒ�
    uint8_t flgExtMem;          // �O���m�ۃ��������p�t���O
	
protected:
    virtual void INIT_DEV() = 0;                              // �f�o�C�X�̏�����
	  virtual void END_DEV() {};                                // �f�o�C�X�̏I��
    virtual void MOVE(uint8_t y, uint8_t x) = 0;              // �L�����N�^�J�[�\���ړ�
    virtual void WRITE(uint8_t x, uint8_t y, uint8_t c) = 0;  // �����̕\��
    virtual void CLEAR() = 0;                                 // ��ʑS����
    virtual void CLEAR_LINE(uint8_t l)  = 0;                  // �s�̏���
    virtual void SCROLL_UP()  = 0;                            // �X�N���[���A�b�v
    virtual void SCROLL_DOWN() = 0;                           // �X�N���[���_�E��
    virtual void INSLINE(uint8_t l) = 0;                      // �w��s��1�s�}��(���X�N���[��)
    
  public:
	  virtual void beep() {};                              // BEEP���̔���
    virtual void show_curs(uint8_t flg) =0;               // �J�[�\���̕\��/��\��
    virtual void draw_cls_curs() = 0;                        // �J�[�\���̏���
    inline  uint8_t IsCurs() { return flgCur; };         // �J�[�\���\���L���̎擾
    virtual void putch(uint8_t c);                       // �����̏o��
    virtual uint8_t get_ch() = 0;                            // �����̎擾
    virtual uint8_t isKeyIn() = 0;                           // �L�[���̓`�F�b�N
	  virtual void setColor(uint16_t fc, uint16_t bc) {};  // �����F�w��
	  virtual void setAttr(uint16_t attr) {};              // ��������
	  virtual void set_allowCtrl(uint8_t flg) {};          // �V���A������̓��͐��䋖�ݒ�

	//virtual int16_t peek_ch();                           // �L�[���̓`�F�b�N(�����Q��)
    virtual inline uint8_t IS_PRINT(uint8_t ch) {
      //return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0)); 
     return ch;
    };
    void init(uint16_t w=0,uint16_t h=0,uint16_t ln=128, uint8_t* extmem=NULL); // �X�N���[���̏����ݒ�
	  virtual void end();                               // �X�N���[�����p�I��
    void clerLine(uint16_t l);                        // 1�s���N���A
    void cls();                                       // �X�N���[���̃N���A
    void refresh();                                   // �X�N���[�����t���b�V���\��
    virtual void refresh_line(uint16_t l);            // �s�̍ĕ\��
    void scroll_up();                                 // 1�s���X�N���[���̃X�N���[���A�b�v
    void scroll_down();                               // 1�s���X�N���[���̃X�N���[���_�E�� 
    void delete_char() ;                              // ���݂̃J�[�\���ʒu�̕����폜
    inline uint8_t getDevice() {return dev;};         // �������͌��f�o�C�X��ʂ̎擾        ***********
    void Insert_char(uint8_t c);                      // ���݂̃J�[�\���ʒu�ɕ�����}��
    void movePosNextNewChar();                        // �J�[�\�����P���������Ɉړ�
    void movePosPrevChar();                           // �J�[�\����1�������O�Ɉړ�
    void movePosNextChar();                           // �J�[�\����1���������Ɉړ�
    void movePosNextLineChar();                       // �J�[�\�������s�Ɉړ�
    void movePosPrevLineChar();                       // �J�[�\����O�s�Ɉړ�
    void moveLineEnd();                               // �J�[�\�����s���Ɉړ�
    void moveBottom();                                // �X�N���[���\���̍ŏI�\���̍s�擪�Ɉړ� 
    void locate(uint16_t x, uint16_t y);              // �J�[�\�����w��ʒu�Ɉړ�
    virtual uint8_t edit() = 0;                       // �X�N���[���ҏW
    virtual uint8_t editLine();                       // ���C���ҏW�i���p���͔Łj
    uint8_t enter_text();                             // �s���͊m��n���h��
    virtual void newLine();                           // ���s�o��
    void Insert_newLine(uint16_t l);                  // �w��s�ɋ󔒑}�� 
    uint8_t edit_scrollUp();                          // �X�N���[�����đO�s�̕\��
    uint8_t edit_scrollDown();                        // �X�N���[�����Ď��s�̕\��
    uint16_t vpeek(uint16_t x, uint16_t y);           // �J�[�\���ʒu�̕����R�[�h�擾
    
    inline uint8_t *getText() { return &text[0]; };   // �m����͂̍s�f�[�^�A�h���X�Q��
    inline uint8_t *getScreen() { return screen; };   // �X�N���[���p�o�b�t�@�A�h���X�Q��
    inline uint16_t c_x() { return pos_x;};           // ���݂̃J�[�\�����ʒu�Q��
    inline uint16_t c_y() { return pos_y;};           // ���݂̃J�[�\���c�ʒu�Q��
    inline uint16_t getWidth() { return width;};      // �X�N���[�������擾
    inline uint16_t getHeight() { return height;};    // �X�N���[���c���擾
    inline uint16_t getScreenByteSize() {return width*height;}; // �X�N���[���̈�o�C�g�T�C�Y
    int16_t getLineNum(int16_t l);                    // �w��s�̍s�ԍ��̎擾
    void splitLine();                                 // �J�[�\���ʒu�ōs�𕪊�����
    void margeLine();                                 // ���ݍs�̖����Ɏ��̍s����������
};

#endif

