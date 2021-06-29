//
// file: tTermscreen.h
// �^�[�~�i���X�N���[�����䃉�C�u���� �w�b�_�t�@�C�� for Arduino STM32
// V1.0 �쐬�� 2017/03/22 by ���܋g����
//  �C���� 2017/03/26, �F����֘A�֐��̒ǉ�
//  �C���� 2017/03/30, moveLineEnd()�̒ǉ�,[HOME],[END]�̕ҏW�L�[�̎d�l�ύX
//  �C���� 2017/04/02, getScreenByteSize()�̒ǉ�
//  �C���� 2017/04/03, getScreenByteSize()�̕s��Ή�
//  �C���� 2017/04/06, beep()�̒ǉ�
//  �C���� 2017/06/27, �ėp���̂��߂̏C��
//  �C���� 2018/08/29 editLine()�i�S�p�Ή��Łj�̒ǉ�
//  �C���� 2018/09/14 splitLine()�AmargeLine() ��e�N���XtscreenBase�����Ɉڍs
//

#ifndef __tTermscreen_h__
#define __tTermscreen_h__

#include "Arduino.h"
#include "tscreenBase.h"
//#include "tSerialDev.h"
#include "mcurses.h"
//#undef erase()

//class tTermscreen : public tscreenBase, public tSerialDev {
class tTermscreen : public tscreenBase  {
	protected:
    void INIT_DEV();                             // �f�o�C�X�̏�����
    void MOVE(uint8_t y, uint8_t x);             // �L�����N�^�J�[�\���ړ�
    void WRITE(uint8_t x, uint8_t y, uint8_t c); // �����̕\��
    void CLEAR();                                // ��ʑS����
    void CLEAR_LINE(uint8_t l);                  // �s�̏���
    void SCROLL_UP();                            // �X�N���[���A�b�v
    void SCROLL_DOWN();                          // �X�N���[���_�E��
    void INSLINE(uint8_t l);                     // �w��s��1�s�}��(���X�N���[��)
    
  public:
    void beep() {addch(0x07);};                  // BEEP���̔���
    void show_curs(uint8_t flg);                 // �J�[�\���̕\��/��\��
    void draw_cls_curs();                        // �J�[�\���̏���
    void setColor(uint16_t fc, uint16_t bc);     // �����F�w��
    void setAttr(uint16_t attr);                 // ��������
    uint8_t get_ch();                            // �����̎擾
    uint8_t isKeyIn();                           // �L�[���̓`�F�b�N
    int16_t peek_ch();                           // �L�[���̓`�F�b�N(�����Q��)

    // �S�p�����Ή��̂��߂̃x�[�X�N���X�����o�֐��̍Ē�`�E�ǉ�
    uint8_t isShiftJIS(uint8_t  c) {             // �V�t�gJIS1�o�C�g�ڃ`�F�b�N
      return (((c>=0x81)&&(c<=0x9f))||((c>=0xe0)&&(c<=0xfc)))?1:0;
    };
  
    void moveLineEnd();                          // �J�[�\�����s���Ɉړ��i�V�t�gJIS�Ή�)
    void movePosNextChar();                      // �J�[�\����1���������Ɉړ�(�S�p�Ή�)
    void movePosNextLineChar();                  // �J�[�\�������s�Ɉړ�(�S�p�Ή�)
    void movePosPrevLineChar();                  // �J�[�\����O�s�Ɉړ�(�S�p�Ή�)
    void movePosPrevChar();                      // �J�[�\����1�������O�Ɉړ�(�S�p�Ή�)
    void refresh_line(uint16_t l);               // �s�̍ĕ\��
    void deleteLine(uint16_t l);                 // �w��s���폜(�S�p�Ή�)
    void delete_char();                          // ���݂̃J�[�\���ʒu�̕����폜(�S�p�Ή�)
    uint16_t get_wch();                          // �����̎擾�i�V�t�gJIS�Ή�)
    void putch(uint8_t c);                       // �����̏o��
    void putwch(uint16_t c);                     // �����̏o�́i�V�t�gJIS�Ή�)
    void Insert_char(uint16_t c);                // �����̑}��
    uint8_t edit();                              // �X�N���[���ҏW
    virtual uint8_t editLine();                  // ���C���ҏW    
    void WRITE(uint8_t c) { addch(c);};          // �����̕\��
};

#endif
