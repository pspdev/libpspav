#ifndef PSPAV_H
#define PSPAV_H

#ifdef __cplusplus
extern "C"{
#endif

#include "pspav_entry.h"

unsigned char pspavPlayGamePMF(PSPAVEntry* e, PSPAVCallbacks* callbacks, int x, int y);
void pspavPlayVideoFile(const char* path, PSPAVCallbacks* callbacks);

#ifdef __cplusplus
}
#endif

#endif
