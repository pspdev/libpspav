#include <stdlib.h>
#include <string.h>
#include <pspsdk.h>
#include <psputility.h>
#include <pspjpeg.h>

void pspavGetJpegInfo(unsigned char* data, int data_size, int* out_w, int* out_h){
    int w = 0, h = 0;
    const uint8_t * buf = &data[0];
    for (int i = 2; i < data_size;) {
        if (buf[i] == 0xFF) {
            i++;
            switch(buf[i]){
                case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC5: case 0xC6: case 0xC7: case 0xC9: case 0xCA: case 0xCB: case 0xCD: case 0xCE: case 0xCF:
                    i += 4;
                    h = (buf[i] << 8) | (buf[i+1]);
                    w = (buf[i+2] << 8) | (buf[i+3]);
                    i = data_size; break;
                case 0xDA: case 0xD9: break;
                default:
                    i += ((buf[i+1] << 8) | (buf[i+2])) + 1;
                    break;
            }
        } else i++;
    }
    *out_w = w;
    *out_h = h;
}

int pspavLoadJpegBuffer(void* jpegbuf, unsigned long jpeg_size, int w, int h, int w2, int h2, unsigned char* tex_data, int stride){

    void* bufRGB = NULL;
    int res = -1;

    if (w <= 0 || h <= 0 || !jpegbuf || !jpeg_size || !tex_data){
        //printf("bad image: %dx%d\n", w, h);
        return -1;
    }

    int utility_loaded = sceUtilityLoadModule(PSP_MODULE_AV_MPEGBASE);
    sceJpegInitMJpeg();

    bufRGB = malloc(4*w2*h2);
    if (!bufRGB){
        //printf("alloc FAILED: %p, %p\n", bufRGB, texture);
        goto pspav_load_jpeg_error;
    }

    if ((res=sceJpegCreateMJpeg(w2, h2))<0) {
        //printf("sceJpegCreateMJpeg FAILED: %p\n", res);
        goto pspav_load_jpeg_error;
    }

    if ((res=sceJpegDecodeMJpeg(jpegbuf, jpeg_size, bufRGB, 0)) < 0){
        //printf("sceJpegCreateMJpeg FAILED: %p\n", res);
        goto pspav_load_jpeg_error;
    }

    unsigned char* buf = bufRGB;
    unsigned int wb = w2*4;
    for (int i=0; i<h2; i++){
        memcpy(tex_data, buf, wb);
        buf += wb;
        tex_data += stride;
    }

    res = 0;
    goto pspav_load_jpeg_finish;

    pspav_load_jpeg_error:
    res = -1;

    pspav_load_jpeg_finish:
    free(bufRGB);
    sceJpegDeleteMJpeg();
    sceJpegFinishMJpeg();
    if (utility_loaded>=0) sceUtilityUnloadModule(PSP_MODULE_AV_MPEGBASE);

    return res;
}
