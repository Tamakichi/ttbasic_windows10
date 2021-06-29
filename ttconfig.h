//
// �L�l�GTiny BASIC for Arduino STM32 �\�z�R���t�B�O���[�V����
// �쐬�� 2021/06/18 ���܋g����
//

#ifndef __ttconfig_h__
#define __ttconfig_h__

// ** (1)�N�����R���\�[����ʎw�� 0:�V���A���^�[�~�i�� 1:�f�o�C�X�X�N���[��(NTSC�EOLED�ETFT)
#define USE_SCREEN_MODE 0  // �V���A���R���\�[���ŗ��p�ł͕K��0���w��(�f�t�H���g:1)

// ������(2)�`(4)�͔r���I��(�S��0�܂��́A�ǂꂩ1��1)

#define USE_NTSC    0
#define USE_OLED    0
#define USE_TFT     0

// ** �^�[�~�i�����[�h���̃f�t�H���g �X�N���[���T�C�Y  *************************
// �� �����ł́AWIDTH�R�}���h�ŕύX�\  (�f�t�H���g:80x24)
#define TERM_W       80
#define TERM_H       24

// ** Serial1�̃f�t�H���g�ʐM���x *********************************************
#define GPIO_S1_BAUD    115200 // // (�f�t�H���g:115200)

// ** �f�t�H���g�̃^�[�~�i���p�V���A���|�[�g�̎w�� 0:USB�V���A�� 1:GPIO UART1
// �� �����ł́ASMODE�R�}���h�ŕύX�\
#define DEF_SMODE     0 // (�f�t�H���g:0)

// ** �N������BOOT1�s��(PB2)�ɂ��V���A���^�[�~�i�����[�h�N���I���̗L��
#define FLG_CHK_BOOT1  1 // 0:�Ȃ�  1:���� // (�f�t�H���g:1)

// ** I2C���C�u�����̑I�� 0:Wire(�\�t�g�G�~�����[�V����) 1:HWire  *************
#define I2C_USE_HWIRE  1 // (�f�t�H���g:1)

// ** ����RTC�̗��p�w��   0:���p���Ȃ� 1:���p���� *****************************
#define USE_INNERRTC   0 // (�f�t�H���g:1) �� SD�J�[�h���p���͕K��1�Ƃ���

// ** SD�J�[�h�̗��p      0:���p���Ȃ� 1:���p���� *****************************
#define USE_SD_CARD    0 // (�f�t�H���g:1)

#endif
