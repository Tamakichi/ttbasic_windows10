//
// �L�l�GTiny BASIC for Arduino STM32 �V���A���f�o�C�X����
// 2017/07/19 by ���܋g����
//


#ifndef __tSerialDev_h__
#define __tSerialDev_h__
#include "Arduino.h"

class tSerialDev {
  protected:
    uint8_t serialMode;         // �V���A���|�[�g���[�h
    uint8_t allowCtrl;          // �V���A������̓��͐��䋖��

  public:
    // �V���A���|�[�g����
	  void    Serial_open(uint32_t baud){};  // �V���A���|�[�g�I�[�v��
	  void    Serial_close(){};               // �V���A���|�[�g�N���[�Y
	  void    Serial_write(uint8_t c){};      // �V���A��1�o�C�g�o��
	  int16_t Serial_read(){};                // �V���A��1�o�C�g����
	  uint8_t Serial_available(){};           // �V���A���f�[�^���M�`�F�b�N
	  void    Serial_newLine(){};             // �V���A�����s�o��
    void    Serial_mode(uint8_t c, uint32_t b);    // �V���A���|�[�g���[�h�ݒ�
	uint8_t getSerialMode() { return serialMode;}; // �V���A���|�[�g���[�h�̎Q��
};

#endif

