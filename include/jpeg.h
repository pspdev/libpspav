#ifndef PSPAV_JPEG_H
#define PSPAV_JPEG_H

#ifdef __cplusplus
extern "C" {
#endif

void pspavGetJpegInfo(unsigned char* data, int data_size, int* out_w, int* out_h);
int pspavLoadJpegBuffer(void* jpegbuf, unsigned long jpeg_size, int w, int h, int w2, int h2, unsigned char* tex_data, int stride);

#ifdef __cplusplus
}
#endif

#endif
