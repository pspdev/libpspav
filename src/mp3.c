#include <string.h>
#include <malloc.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspmp3.h>
#include <pspaudio.h>
#include <psputility.h>
#include <psputility_avmodules.h>

#include "mp3.h"

#define SAMPLE_PER_FRAME 1152
#define MP3BUF_SIZE (8*1024)
#define PCMBUF_SIZE (SAMPLE_PER_FRAME*2*4)

//static char mp3Buf[MP3BUF_SIZE]  __attribute__((aligned(64)));
//static short pcmBuf[PCMBUF_SIZE]  __attribute__((aligned(64)));

static bool running = false;
static bool eof = false;
static bool paused = false;

bool pspavIsMP3Active(){
    return running;
}

bool pspavIsMP3Paused(){
    return paused;
}

void pspavStopMP3Playback(){
    running = false;
}

void pspavResumeOrPauseMP3Playback(){
    paused = !paused;
}

void pspavPauseMP3Playback(){
    paused = true;
}

void pspavResumeMP3Playback(){
    paused = false;
}

static bool fillStreamBuffer(int fd, int handle, void* buffer, int buffer_size)
{
    bool res = 0;
    char* dst;
    SceInt32 write;
    SceInt32 pos;

    if (eof) return false;

    // Get Info on the stream (where to fill to, how much to fill, where to fill from)
    int status = sceMp3GetInfoToAddStreamData(handle, (SceUChar8**)&dst, &write, &pos);
    if (status < 0){
        return 0;
    }

    // read from file
    if (fd >= 0){

        // Seek file to position requested
        status = sceIoLseek32( fd, pos, SEEK_SET );
        if (status < 0)
            return false;

        // Read the amount of data
        int read = sceIoRead( fd, dst, write );
        if (read <= 0){
            // End of file?
            eof = true;
            return false;
        }
        write = read;
        res = (pos > 0);
    }
    // read from memory buffer
    else{
        
        if (pos >= buffer_size){
            eof = true;
            return false;
        }

        if (pos + write > buffer_size){
            write = buffer_size-pos;
            eof = true;
        }
        memcpy(dst, (u8*)buffer+pos, write);
        res = (pos > 0);
    }

    // Notify mp3 library about how much we really wrote to the stream buffer
    status = sceMp3NotifyAddStreamData( handle, write);
    if (status < 0){
        return false;
    }
    return res;
}

static u32 findMP3StreamStart(int file_handle, void* buffer, int buffer_size, unsigned char* tmp_buf){
    u8* buf = (u8*)buffer;
    if (file_handle>=0){
        buf = (u8*)tmp_buf;
        sceIoRead(file_handle, buf, MP3BUF_SIZE);
    }
    if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3'){
        u32 header_size = (buf[9] | (buf[8]<<7) | (buf[7]<<14) | (buf[6]<<21));
        return header_size+10;
    }
    else if (buf[0] == 'A' && buf[1] == 'P' && buf[2] == 'E'){
        u32 header_size = (buf[12] | (buf[13]<<8) | (buf[14]<<16) | (buf[15]<<24));
        return header_size+32;
    }
    return 0;
}

void pspavPlayMP3File(char* filename, void* buffer, int buffer_size)
{

    if (filename == NULL && buffer == NULL){
        running = false;
        return;
    }

    int file_handle = -1;
    int mp3_handle = -1;
    int channel = -1;
    unsigned char* mp3Buf = NULL;
    unsigned char* pcmBuf = NULL;

    int status;

    eof = false;

    if (filename != NULL){
        file_handle = sceIoOpen(filename, PSP_O_RDONLY, 0777 );
        if(file_handle < 0) {
            running = false;
            return;
        }
    }

    // load the needed utilities if not done already
    int mod_loaded1 = sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);
    int mod_loaded2 = sceUtilityLoadModule(PSP_MODULE_AV_MP3);

    status = sceMp3InitResource();
    if(status < 0) {
        goto mp3_terminate;
        return;
    }

    // Reserve a mp3 handle for our playback
    SceMp3InitArg mp3Init;
    mp3Buf = (unsigned char*)memalign(64, MP3BUF_SIZE);
    pcmBuf = (unsigned char*)memalign(64, PCMBUF_SIZE);
    memset(mp3Buf, 0, MP3BUF_SIZE);
    memset(pcmBuf, 0, PCMBUF_SIZE);
    memset(&mp3Init, 0, sizeof(mp3Init));
    mp3Init.mp3StreamStart = findMP3StreamStart(file_handle, buffer, buffer_size, mp3Buf);
    mp3Init.mp3StreamEnd = (file_handle >= 0)? sceIoLseek32( file_handle, 0, SEEK_END ) : buffer_size;
    mp3Init.mp3Buf = mp3Buf;
    mp3Init.mp3BufSize = MP3BUF_SIZE;
    mp3Init.pcmBuf = pcmBuf;
    mp3Init.pcmBufSize = PCMBUF_SIZE;

    int samplingRate, numChannels, max_sample;

    mp3_handle = sceMp3ReserveMp3Handle( &mp3Init );

    if (mp3_handle < 0){
        goto mp3_terminate;
    }

    // Fill the stream buffer with some data so that sceMp3Init has something to
    // work with
    fillStreamBuffer(file_handle, mp3_handle, buffer, buffer_size);
    
    status = sceMp3Init( mp3_handle );
    
    if (status < 0){
        goto mp3_terminate;
    }

    if (!running)
        running = true;

    sceMp3SetLoopNum(mp3_handle, 0);

    samplingRate = sceMp3GetSamplingRate(mp3_handle);
    numChannels = sceMp3GetMp3ChannelNum(mp3_handle);
    max_sample = sceMp3GetMaxOutputSample(mp3_handle);

    sceAudioSRCChRelease();
    channel = sceAudioSRCChReserve(max_sample, samplingRate, numChannels);

    if (channel < 0) goto mp3_terminate;

    while (running) {

        if (paused){
            sceKernelDelayThread(10000);
            continue;
        }
        
         // Check if we need to fill our stream buffer
        if (sceMp3CheckStreamDataNeeded( mp3_handle ) > 0){
            if (!fillStreamBuffer(file_handle, mp3_handle, buffer, buffer_size)){
                break;
            }
        }

        // Decode some samples
        short* buf;
        unsigned int bytesDecoded;

        bytesDecoded = sceMp3Decode(mp3_handle, &buf);

        // Nothing more to decode? Must have reached end of input buffer
        if (bytesDecoded <= 0)
        {
            break;
        }

        // Output the decoded samples and accumulate the 
        // number of played samples to get the playtime
        sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, buf );
    }

    mp3_terminate:

    // Cleanup time...
    if (channel >= 0){
        while (sceAudioSRCChRelease() < 0){ // wait for the audio to be outputted
            sceKernelDelayThread(100);
        }
    }

    if (mp3_handle >= 0) sceMp3ReleaseMp3Handle( mp3_handle );

    sceMp3TermResource();

    if (mod_loaded2>=0) sceUtilityUnloadModule(PSP_MODULE_AV_MP3);
    if (mod_loaded1>=0) sceUtilityUnloadModule(PSP_MODULE_AV_AVCODEC);

    if (file_handle >= 0)
        status = sceIoClose( file_handle );

    running = false;
    free(mp3Buf);
    free(pcmBuf);
}
