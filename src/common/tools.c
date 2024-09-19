//
// Created by ajl on 2021/4/13.
//
#include "tools.h"

/**
 *
 * @param data
 * @param planeY
 * @param planeU
 * @param planeV
 * @param imWidth
 * @param imHeight
 * @param pitchY
 * @param pitchU
 * @param pitchV
 * @param Ulenth
 * @param pixPitchY
 * @param pixPitchUV
 */
void
YUV_420_888toNV21(t_yuvchar data, t_yuvchar planeY, t_yuvchar planeU, t_yuvchar planeV, int imWidth,
                  int imHeight,
                  int pitchY, int pitchU, int pitchV, int Ulenth, int pixPitchY, int pixPitchUV,
                  int rotate) {
//,new byte[ySize+ uvSize*2]
    if (rotate == 0) {
        int width = imWidth;
        int height = imHeight;
        int ySize = (width) * height;
        int uvSize = width * height >> 2;
        int vuwidth = width >> 1;
        int vuheigth = height >> 1;
        uint8_t *nv21 = (uint8_t *) data;

        int rowStride = pitchY;

        assert(pixPitchY == 1);

        int pos = 0;

        if (rowStride == width) { // likely
            memcpy(nv21, planeY, ySize);
            pos += ySize;
        } else {
            size_t yBufferPos = -rowStride; // not an actual position
            for (pos = 0; pos < ySize; pos += width) {
                yBufferPos += rowStride;
                memcpy(nv21 + pos, planeY + yBufferPos, width);
            }
        }

        rowStride = pitchV;
        int pixelStride = pixPitchUV;
        assert(rowStride == pitchU);

        if (pixelStride == 2 && rowStride == width && *planeU == *(planeV + 1)) {
// maybe V an U planes overlap as per NV21, which means vBuffer[1] is alias of uBuffer[0]
            char savePixel = *(planeV + 1);

            if (planeU == planeV + 1) {
                memcpy(nv21 + ySize, planeV, 1);
                memcpy(nv21 + ySize + 1, planeU, Ulenth);
                return; // shortcut
            }
        }

// other optimizations could check if (pixelStride == 1) or (pixelStride == 2),
// but performance gain would be less significant

        for (int row = 0; row < vuheigth; row++) {
            for (int col = 0; col < vuwidth; col++) {
                int vuPos = col * pixelStride + row * rowStride;
                nv21[pos++] = planeV[vuPos];
                nv21[pos++] = planeU[vuPos];
            }
        }

    } else if (rotate == 90) {
        int width = imWidth;
        int height = imHeight;
        int ySize = (width) * height;
        int uvSize = width * height >> 2;

        uint8_t *nv21 = (uint8_t *) data;

        int rowStride = pitchY;

        assert(pixPitchY == 1);

        int pos = 0;
        int yplaneBottom = (height - 1) * pitchY;
        int yplanePos = 0;

        int vuwidth = width >> 1;
        int vuheigth = height >> 1;
        int vuplaneBottom = (vuheigth - 1) * pitchU;
        int vuplanePos = 0;



        for (int col = 0; col < width; col++) {
            yplanePos = yplaneBottom + col;
            for (int row = 0; row < height; row++) {
                nv21[pos++] = planeY[yplanePos];
                //sprintf(s,"%#x ",planeY[yplanePos]);
                // fputs(s,f2);
                yplanePos -= pitchY;
            }
            // fputs("\n",f2);
        }
        // fclose(f2);


        for (int col = 0; col < vuwidth; col++) {
            vuplanePos = vuplaneBottom + col * pixPitchUV;
            for (int row = 0; row < vuheigth; row++) {
                nv21[pos++] = planeV[vuplanePos];
                nv21[pos++] = planeU[vuplanePos];
                vuplanePos -= pitchU;
            }
        }
    } else if (rotate == 270) {

        int width = imWidth;
        int height = imHeight;
        int ySize = (width) * height;
        int uvSize = width * height >> 2;

        uint8_t *nv21 = (uint8_t *) data;

        int rowStride = pitchY;

        assert(pixPitchY == 1);

        int pos = 0;
        int yImRight = width - pixPitchY;
        int yplanePos = 0;

        int vuwidth = width >> 1;
        int vuheigth = height >> 1;
        int vuImRight = width - pixPitchUV;
        int vuplanePos = 0;





        for (int col = 0; col < width; col++) {
            yplanePos = yImRight - col;
            for (int row = 0; row < height; row++) {
                nv21[pos++] = planeY[yplanePos];
                yplanePos += pitchY;
            }
        }


        for (int col = 0; col < vuwidth; col++) {
            vuplanePos = vuImRight - col * pixPitchUV;
            for (int row = 0; row < vuheigth; row++) {
                nv21[pos++] = planeV[vuplanePos];
                nv21[pos++] = planeU[vuplanePos];
                vuplanePos += pitchU;
            }
        }
    }
}

void
YUV_420_888toIyuv(t_yuvchar data, t_yuvchar planeY, t_yuvchar planeU, t_yuvchar planeV, int imWidth,
                  int imHeight,
                  int pitchY, int pitchU, int pitchV, int Ulenth, int pixPitchY, int pixPitchUV,
                  int rotate) {
//,new byte[ySize+ uvSize*2]
    if (rotate == 0) {
        int width = imWidth;
        int height = imHeight;
        int ySize = (width) * height;
        int uvSize = width * height >> 2;
        int vuwidth = width >> 1;
        int vuheigth = height >> 1;
        uint8_t *Iyuv = (uint8_t *) data;

        int rowStride = pitchY;

        assert(pixPitchY == 1);

        int pos = 0;
        int pos_1=0;
        unsigned int yBufferPos = -rowStride; // not an actual position
        for (pos = 0; pos < ySize; pos += width) {
            yBufferPos += rowStride;
            memcpy(Iyuv + pos, planeY + yBufferPos, width);
        }


        rowStride = pitchV;
        int pixelStride = pixPitchUV;
        assert(rowStride == pitchU);

// other optimizations could check if (pixelStride == 1) or (pixelStride == 2),
// but performance gain would be less significant

        for (int row = 0; row < vuheigth; row++) {
            for (int col = 0; col < vuwidth; col++) {
                int vuPos = col * pixelStride + row * rowStride;
                pos_1=pos++;
                Iyuv[pos_1] = planeU[vuPos];
                Iyuv[pos_1+uvSize] = planeV[vuPos];
            }
        }

    } else if (rotate == 90) {
        int width = imWidth;
        int height = imHeight;
        int ySize = (width) * height;
        int uvSize = width * height >> 2;

        uint8_t *Iyuv = (uint8_t *) data;

        int rowStride = pitchY;

        assert(pixPitchY == 1);

        int pos = 0;
        int pos_1=0;
        int yplaneBottom = (height - 1) * pitchY;
        int yplanePos = 0;

        int vuwidth = width >> 1;
        int vuheigth = height >> 1;
        int vuplaneBottom = (vuheigth - 1) * pitchU;
        int vuplanePos = 0;



        for (int col = 0; col < width; col++) {
            yplanePos = yplaneBottom + col;
            for (int row = 0; row < height; row++) {
                Iyuv[pos++] = planeY[yplanePos];
                yplanePos -= pitchY;
            }
            // fputs("\n",f2);
        }
        // fclose(f2);


        for (int col = 0; col < vuwidth; col++) {
            vuplanePos = vuplaneBottom + col * pixPitchUV;
            for (int row = 0; row < vuheigth; row++) {
                pos_1=pos++;

                Iyuv[pos_1] = planeU[vuplanePos];
                Iyuv[pos_1+uvSize] = planeV[vuplanePos];
                vuplanePos -= pitchU;
            }
        }
    } else if (rotate == 270) {

        int width = imWidth;
        int height = imHeight;
        int ySize = (width) * height;
        int uvSize = width * height >> 2;

        uint8_t *Iyuv = (uint8_t *) data;

        int rowStride = pitchY;

        assert(pixPitchY == 1);

        int pos = 0;
        int pos_1=0;
        int yImRight = width - pixPitchY;
        int yplanePos = 0;

        int vuwidth = width >>1;
        int vuheigth = height>>1;
        int vuImRight = width - pixPitchUV;
        int vuplanePos = 0;

        for (int col = 0; col < width; col++) {
            yplanePos = yImRight - col;
            for (int row = 0; row < height; row++) {
                Iyuv[pos++] = planeY[yplanePos];
                yplanePos += pitchY;
            }
        }


        for (int col = 0; col < vuwidth; col++) {
            vuplanePos = vuImRight - col * pixPitchUV;
            for (int row = 0; row < vuheigth; row++) {
                pos_1=pos++;
                Iyuv[pos_1] = planeU[vuplanePos];
                Iyuv[pos_1+uvSize] = planeV[vuplanePos];
                vuplanePos += pitchU;
            }
        }
    }
}
void toolsIntBigEndianV(uint8_t *d, uint8_t *val, uint32_t digits) {
    if (digits == 8) {
        *val=*(d++);
    } else if (digits == 16) {
        *(val+1)=*(d++);
        *(val)=*(d++);
    } else if (digits == 32) {
        *(val+3)=*(d++);
        *(val+2)=*(d++);
        *(val+1)=*(d++);
        *(val)=*(d++);
    }
}
//转成大端，位移移动的是高低字节的值，并不是存储，不管是大端还是小端，高低字节的值都是一样的，只是存储循序不同
//大端，显存大的值，小端，先存小的值，位运算不会受大小端影响
void toolsIntBigEndian(uint8_t *d, uint32_t val, uint32_t digits) {
    if (digits == 8) {
        *(d++) = (uint8_t)(val & 0xff);
    } else if (digits == 16) {
        *(d++) = (uint8_t)((val >> 8) & 0xff); //高位
        *(d++) = (uint8_t)(val & 0xff);//低位
    } else if (digits == 32) {
        *(d++) = (uint8_t)((val >> 24) & 0xff);//高位
        *(d++) = (uint8_t)((val >> 16) & 0xff);//低位
        *(d++) = (uint8_t)((val >> 8) & 0xff);//。。
        *(d++) = (uint8_t)(val & 0xff);
    }
}
void toolsIntLitEndian(uint8_t *d, uint32_t val, uint32_t digits) {
    if (digits == 8) {
        *(d++) = (uint8_t)(val & 0xff);
    } else if (digits == 16) {
        *(d++) = (uint8_t)(val & 0xff);//低位
        *(d++) = (uint8_t)((val >> 8) & 0xff); //高位
    } else if (digits == 24) {
        *(d++) = (uint8_t)(val & 0xff);
        *(d++) = (uint8_t)((val >> 8) & 0xff);//。。
        *(d++) = (uint8_t)((val >> 16) & 0xff);//低位
    } else if (digits == 32) {
        *(d++) = (uint8_t)(val & 0xff);
        *(d++) = (uint8_t)((val >> 8) & 0xff);//。。
        *(d++) = (uint8_t)((val >> 16) & 0xff);//低位
        *(d++) = (uint8_t)((val >> 24) & 0xff);//高位
    }
}
#define HG_SEQ_MAX_NUM 65535//不是最大的值而是总共的个数，0-65534
void init16Num(){
    // HG_SEQ_MAX_NUM=65536;//(65535-(65535%sess_cache_LEN));
}

int uint16Sub(uint16_t newd,uint16_t oldd){
    uint32_t ret;
    if(newd<oldd){
        ret= (newd-0)+1+(HG_SEQ_MAX_NUM-1)-oldd;
        if(ret>HG_SEQ_MAX_NUM/2){
            return -1;
        }else{
            return ret;
        }
    }else{
        ret=newd-oldd;
        if(ret>HG_SEQ_MAX_NUM/2){
            return -1;
        }else{
            return newd-oldd;
        }
    }

}

uint16_t uint16Add(uint16_t add1,uint16_t add2){
    return (uint16_t)((add1+add2)%(HG_SEQ_MAX_NUM));
}

int hgSemWait(sem_t * sem)
{
    int retval;

    if (!sem) {
        return -1;
    }

    do {
        retval = sem_wait(sem);
    } while (retval < 0 && errno == EINTR);

    if (retval < 0) {
        retval = -1;
    }
    return retval;
}
uint64_t hgetSysTimeMicros()
{
#ifdef _WIN32
    // 从1601年1月1日0:0:0:000到1970年1月1日0:0:0:000的时间(单位100ns)
#define HG_EPOCH_FILE_TIME   (116444736000000000UL)
    FILETIME ft;
    LARGE_INTEGER li;
    uint64_t tt = 0;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    // 从1970年1月1日0:0:0:000到现在的微秒数(UTC时间)
    tt = (li.QuadPart - HG_EPOCH_FILE_TIME) /10;
    return tt;
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
#endif // _WIN32
    return 0;
}
void  hg_nsleep(unsigned int miliseconds)
{

    usleep(miliseconds*1000);
//
//    struct timespec req, rem;
//
//    if(miliseconds > 999)
//    {
//        req.tv_sec = (int)(miliseconds / 1000);                            /* Must be Non-Negative */
//        req.tv_nsec = (miliseconds - ((long)req.tv_sec * 1000)) * 1000000; /* Must be in range of 0 to 999999999 */
//    }
//    else
//    {
//        req.tv_sec = 0;                         /* Must be Non-Negative */
//        req.tv_nsec = miliseconds * 1000000;    /* Must be in range of 0 to 999999999 */
//    }
//
//    nanosleep(&req , &rem);//好像有点争议，和版本有关
}


int checkLitEndian() {
    uint32_t i = 0x11223344;
    uint8_t *p;

    p = (uint8_t *) &i;
    if (*p == 0x44) {
        return 1;
    } else {
        return 0;
    }
}
void PrintBuffer(void* pBuff,uint32_t f1, uint32_t nLen)
{
    return;
    if (NULL == pBuff || 0 == nLen)
    {
        return;
    }

    const uint32_t nBytePerLine = 16;
    unsigned char* p = (unsigned char*)pBuff;
    char szHex[3*nBytePerLine+1] ;
    memset(szHex,0,3*nBytePerLine+1);
    if(f1==1){
        HG_ALOGI(1,"-----------------begin-------------------\n");
    }else{
        HG_ALOGI(0,"-----------------begin-------------------\n");
    }

    for ( uint32_t i=0; i<nLen; ++i)
    {
        uint32_t idx = 3 * (i % nBytePerLine);
        if (0 == idx)
        {
            memset(szHex, 0, sizeof(szHex));
        }
#ifdef WIN32
        sprintf_s(&szHex[idx], 4, "%02x ", p[i]);// buff长度要多传入1个字节
#else
        snprintf(&szHex[idx], 4, "%02x ", p[i]); // buff长度要多传入1个字节
#endif

        // 以16个字节为一行，进行打印
        if (0 == ((i+1) % nBytePerLine))
        {
            if(f1==1){
                HG_ALOGI(1,"%s\n", szHex);
            }else{
                HG_ALOGI(0,"%s\n", szHex);
            }

        }
    }

    // 打印最后一行未满16个字节的内容
    if (0 != (nLen % nBytePerLine))
    {
        if(f1==1){
            HG_ALOGI(1,"%s\n", szHex);
        }else{
            HG_ALOGI(0,"%s\n", szHex);
        }

    }
    if(f1==1){
        HG_ALOGI(1,"------------------end-------------------\n");
    }else{
        HG_ALOGI(0,"------------------end-------------------\n");
    }

}

#define HG_MAX_UINT32_NUM 4294967295 //0-4294967294
int uint32Sub(uint32_t newd,uint32_t oldd){
    uint32_t ret;
    if(newd<oldd){
        ret= (newd-0)+1+(HG_MAX_UINT32_NUM-1)-oldd;
        if(ret>HG_MAX_UINT32_NUM/2){
            return -1;
        }else{
            return ret;
        }
    }else{
        return newd-oldd;
    }
}

uint32_t uint32Add(uint32_t add1,uint32_t add2){
    return (uint32_t)((add1+add2)&(0xffffffff));
}

