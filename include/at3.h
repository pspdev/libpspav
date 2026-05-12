#ifndef PSPAV_AT3_H
#define PSPAV_AT3_H

#ifdef __cplusplus
extern "C" {
#endif

void pspavSetAt3Data(char* data, int size, int* abortVar, int delay);
void pspavResetAt3Data();
unsigned pspavGetAT3Frequency();
int pspavPlayAT3(SceSize argc, void* argv);

#ifdef __cplusplus
}
#endif

#endif
