
#include <stdio.h>
#include <string.h>
#include "camera_log.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "libyuv.h"


typedef struct {
    int w;
    int h;
} supported;

int currentVersion = -1;
int picWidth;
int picHeight;
int previewWidth = 0;
int previewHeight = 0;
int previewSize = 0;
int supportedListLength;
supported **supportedList = 0;
int picSize;
int picsLength;
uint8 **picBuffer = 0;
uint8 *y = 0;
uint8 *u = 0;
uint8 *v = 0;

uint8 *_y = 0;
uint8 *_u = 0;
uint8 *_v = 0;

int _y_size;
int _uv_size;
int _last_index;

int readInt(int f);

double readDouble(int f);

int readBytes(int f, void *b, int size);

void *my_memcpy(void *_dst, const void *_src, unsigned len);

int update = 0;

int readInt(int f) {
    int value = 0;
    int ret = read(f, &value, 4);
    if (ret < 0) {
        LOGE("readInt failed");
        return -1;
    }
    return value;
}

double readDouble(int f) {
    double value = 0;
    int ret = read(f, &value, 8);
    if (ret < 0) {
        LOGE("readDouble failed");
        return -1;
    }
    return value;
}

long long readLongLong(int f) {
    long long value = 0;
    int ret = read(f, &value, 8);
    if (ret < 0) {
        LOGE("readLongLong failed");
        return -1;
    }
    return value;
}

int readBytes(int f, void *b, int size) {
    int ret = read(f, b, size);
    if (ret < 0) {
        LOGE("readBytes failed");
        return -1;
    }
    return 0;
}

void *my_memcpy(void *_dst, const void *_src, unsigned len) {
    LOGE("I hook here:%d", len);
    //len = 460800

//    int fop = open("/system/myIndex", O_RDONLY);
//    if (fop == -1)
//    {
//        LOGE("open index file failed:%s", strerror(errno));
//        return memcpy(_dst, _src, len);
//    }
//
//    _img_size = readInt(fop);
//    char buff[30];
//    _last_index = _last_index % _img_size + 1;
//    sprintf(buff, "/system/myResource/fake%03d", _last_index);
//    LOGE("image size:%d lastIndex:%d fileName:%s", _img_size, _last_index, buff);
//    int f = open(buff, O_RDONLY);
//    if (f == -1) {
//        LOGE("open image file failed:%s", strerror(errno));
//        return memcpy(_dst, _src, len);
//    }
    int f = open("/system/hook", O_RDONLY);
    if (f == -1) {
        LOGE("open file failed:%s", strerror(errno));
        return memcpy(_dst, _src, len);
    }

    int hashcode = readInt(f);

    LOGE("currentVersion:%d hashcode:%d", currentVersion, hashcode);

    if (currentVersion != hashcode) {
        //update
        currentVersion = hashcode;
        if (supportedList) {
            for (int i = 0; i < supportedListLength; i++) {
                free(supportedList[i]);
            }
            free(supportedList);
            supportedList = 0;
        }

        if (picBuffer) {
            for (int i = 0; i < picsLength; ++i) {
                free(picBuffer[i]);
            }
            free(picBuffer);
            free(y);
            free(u);
            free(v);
            picBuffer = 0;
        }

        picWidth = readInt(f);
        picHeight = readInt(f);

        LOGE("picWidth:%d,picHeight:%d", picWidth, picHeight);
        picsLength = readInt(f);
        LOGE("picsLength:%d", picsLength);

        picBuffer = (uint8 **) malloc(sizeof(uint8 *) * picsLength);

        for (int i = 0; i < picsLength; ++i) {
            picSize = readInt(f);
            LOGE("picSize:%d", picSize);
            picBuffer[i] = (uint8 *) malloc(sizeof(uint8) * picSize);
            readBytes(f, picBuffer[i], picSize);

//            int y_size = picWidth * picHeight;
//            int uv_size = picWidth * picHeight / 4;
//
//            y = (uint8 *) malloc(y_size);
//            u = (uint8 *) malloc(uv_size);
//            v = (uint8 *) malloc(uv_size);
//
//            memcpy(y, picBuffer[i], y_size);
//            for (int i = 0; i < uv_size; i++) {
//                u[i] = picBuffer[y_size + i * 2 + 0];
//                v[i] = picBuffer[y_size + i * 2 + 1];
//            }
        }

        supportedListLength = readInt(f);
        LOGE("supportedListLength:%d", supportedListLength);

        supportedList = (supported **) malloc(supportedListLength * sizeof(supported *));
        for (int i = 0; i < supportedListLength; i++) {
            int w = readInt(f);
            int h = readInt(f);

            LOGE("supportedList w:%d, h:%d", w, h);

            supported *s = (supported *) malloc(sizeof(supported));
            s->w = w;
            s->h = h;
            supportedList[i] = s;
        }

        update = 1;
    }
    close(f);

    //if previewSize changed, remalloc _y,_u,_v
    if (460800 != len) {
        LOGE("not support dynamic preview size");
        return memcpy(_dst, _src, len);
//        for (int i = 0; i < supportedListLength; i++) {
//            supported *s = supportedList[i];
//            int _size = s->w * s->h * 3 / 2;
//            LOGE("supportedSize: %d", _size);
//            if (_size == len) {
//                previewWidth = s->w;
//                previewHeight = s->h;
//                break;
//            }
//        }
//
//        if (previewWidth == 0 || previewHeight == 0) {
//            LOGE("can not get preview size!");
//            return memcpy(_dst, _src, len);
//        }
//
//        _y_size = previewWidth * previewHeight;
//        _uv_size = _y_size / 4;
//
//        if (_y && _u && _v) {
//            free(_y);
//            free(_u);
//            free(_v);
//            _y = 0;
//            _u = 0;
//            _v = 0;
//        }
//
//        _y = (uint8 *) malloc(_y_size);
//        _u = (uint8 *) malloc(_uv_size);
//        _v = (uint8 *) malloc(_uv_size);
//
//        update = 1;
//        previewSize = len;
    }


    //update _y,_u,_v
//    if (update) {
//        int result = I420Scale(y, picWidth, u, picWidth / 2, v, picWidth / 2, picWidth, picHeight,
//                               _y, previewWidth, _u, previewWidth / 2, _v, previewWidth / 2,
//                               previewWidth, previewHeight, kFilterNone);
//        //int result = I420Scale(y,previewHeight,u,previewHeight/2,v,previewHeight/2,previewHeight,previewWidth,_y,previewHeight,_u,previewHeight/2,_v,previewHeight/2,previewHeight,previewWidth,kFilterNone);
//        LOGE("Image Scale result %d", result);
//        update = 0;
//    }


    _last_index = (_last_index + 1) % picsLength;

    if (y) {
        free(y);
        y = 0;
    }
    if (u) {
        free(u);
        u = 0;
    }
    if (v) {
        free(v);
        v = 0;
    }
    if (_y) {
        free(_y);
        _y = 0;
    }
    if (_u) {
        free(_u);
        _u = 0;
    }
    if (_v) {
        free(_v);
        _v = 0;
    }

    if (picWidth == 0 || picHeight == 0) {
        LOGE("can not get preview size!");
        return memcpy(_dst, _src, len);
    }
    if (!picBuffer) {
        LOGE("can not malloc buffer!");
        return memcpy(_dst, _src, len);
    }

    LOGE("begin to change image");

    int y_size = picWidth * picHeight;
    int uv_size = picWidth * picHeight / 4;

    y = (uint8 *) malloc(y_size);
    u = (uint8 *) malloc(uv_size);
    v = (uint8 *) malloc(uv_size);

    _y_size = y_size;
    _uv_size = uv_size;
    _y = (uint8 *) malloc(_y_size);
    _u = (uint8 *) malloc(_uv_size);
    _v = (uint8 *) malloc(_uv_size);

    memcpy(y, picBuffer[_last_index], y_size);
    for (int i = 0; i < uv_size; i++) {
        u[i] = picBuffer[_last_index][y_size + i * 2 + 0];
        v[i] = picBuffer[_last_index][y_size + i * 2 + 1];
    }

    previewWidth = 640;
    previewHeight = 480;
    int result = I420Scale(y, picWidth, u, picWidth / 2, v, picWidth / 2, picWidth, picHeight,
                           _y, previewWidth, _u, previewWidth / 2, _v, previewWidth / 2,
                           previewWidth, previewHeight, kFilterNone);
    LOGE("Image Scale result %d", result);

    //set yuv420sp buffer
    uint8 *to = (uint8 *) _dst;
    memcpy(to, _y, _y_size);
    for (int i = 0; i < uv_size; i++) {
        to[_y_size + i * 2] = _u[i];
        to[_y_size + i * 2 + 1] = _v[i];
    }

    return to;
}
