#ifndef FONTLIB_H
#define FONTLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern const unsigned char F6X8[];           /* ASCII 6x8 font, offset 32 */
extern const unsigned char F8X16[];          /* ASCII 8x16 font, offset 32 */
extern const unsigned char Num20X40[][100];  /* Large digit 20x40 */
extern const unsigned char TestBMP[];        /* 128x64 test bitmap */

#ifdef __cplusplus
}
#endif

#endif /* FONTLIB_H */
