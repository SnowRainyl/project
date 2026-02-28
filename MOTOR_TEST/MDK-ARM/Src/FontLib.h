#ifndef __FONTLIB_H__
#define __FONTLIB_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>

//常用ASCII表
//偏移量32
//低位在前面

extern const unsigned char F6X8[];          //英文字符F6X8
extern const unsigned char F8X16[];         //英文字符F8X16
extern const unsigned char FontCN[][32];    //汉字16X16
extern const unsigned char Num20X40[][100]; //数字20X40
extern const unsigned char TestBMP[];       //全屏测试BMP图片(128 X 64)

#ifdef __cplusplus
}
#endif

#endif //FontLib.h
