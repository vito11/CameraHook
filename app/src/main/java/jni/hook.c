
#include <stdio.h>
#include <string.h>
#include "camera_log.h"
#include <errno.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "libyuv.h"


typedef struct {
    int w;
    int h;
} supported;

double currentVersion = -1;
int picWidth;
int picHeight;
int previewWidth = 0;
int previewHeight = 0;
int previewSize = 0;
int supportedListLength;
supported **supportedList = 0;
int picSize;
uint8 *picBuffer = 0;
uint8 *y = 0;
uint8 *u = 0;
uint8 *v = 0;

uint8 *_y = 0;
uint8 *_u = 0;
uint8 *_v = 0;

int _y_size;
int _uv_size;
int _img_size;
int _last_index;

int readInt(int f);

double readDouble(int f);

int readBytes(int f, void *b, int size);

void *my_memcpy(void *_dst, const void *_src, unsigned len);

int update = 0;

int readInt(int f) {
    int value = 0;
    int ret = read(f, &value, 4);
    LOGE("int %d", ret);
    if (ret < 0) {
        LOGE("readInt failed");
        return -1;
    }
    return value;
}

double readDouble(int f) {
    double value = 0;
    int ret = read(f, &value, 8);
    LOGE("double %d", ret);
    if (ret < 0) {
        LOGE("readDouble failed");
        return -1;
    }
    return value;
}

int readBytes(int f, void *b, int size) {
    int ret = read(f, b, size);
    LOGE("bytes %d", ret);
    if (ret < 0) {
        LOGE("readBytes failed");
        return -1;
    }
    return 0;
}

void *my_memcpy(void *_dst, const void *_src, unsigned len) {
    LOGE("I hook here:%d", len);

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
    int f = open("/system/myResource/fake020", O_RDONLY);
    if (f == -1) {
        LOGE("open file failed:%s", strerror(errno));
        return memcpy(_dst, _src, len);
    }

    double time = readDouble(f);

    LOGE("currentVersion:%ll", currentVersion);

    if (currentVersion != time) {
        //update
        currentVersion = time;
        if (supportedList) {
            for (int i = 0; i < supportedListLength; i++) {
                free(supportedList[i]);
            }
            free(supportedList);
            supportedList = 0;
        }

        if (picBuffer) {
            free(picBuffer);
            free(y);
            free(u);
            free(v);
            picBuffer = 0;
        }

        picWidth = readInt(f);
        picHeight = readInt(f);

        LOGE("picWidth:%d,picHeight:%d", picWidth, picHeight);
        picSize = readInt(f);
        LOGE("picSize:%d", picSize);

        picBuffer = (uint8 *) malloc(picSize);
        readBytes(f, picBuffer, picSize);


        int y_size = picWidth * picHeight;
        int uv_size = picWidth * picHeight / 4;

        y = (uint8 *) malloc(y_size);
        u = (uint8 *) malloc(uv_size);
        v = (uint8 *) malloc(uv_size);

        memcpy(y, picBuffer, y_size);
        for (int i = 0; i < uv_size; i++) {
            u[i] = picBuffer[y_size + i * 2 + 0];
            v[i] = picBuffer[y_size + i * 2 + 1];
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
    if (previewSize != len) {
        for (int i = 0; i < supportedListLength; i++) {
            supported *s = supportedList[i];
            int _size = s->w * s->h * 3 / 2;
            LOGE("supportedSize: %d", _size);
            if (_size == len) {
                previewWidth = s->w;
                previewHeight = s->h;
                break;
            }
        }

        if (previewWidth == 0 || previewHeight == 0) {
            LOGE("can not get preview size!");
            return memcpy(_dst, _src, len);
        }

        _y_size = previewWidth * previewHeight;
        _uv_size = _y_size / 4;

        if (_y && _u && _v) {
            free(_y);
            free(_u);
            free(_v);
            _y = 0;
            _u = 0;
            _v = 0;
        }

        _y = (uint8 *) malloc(_y_size);
        _u = (uint8 *) malloc(_uv_size);
        _v = (uint8 *) malloc(_uv_size);

        update = 1;
        previewSize = len;
    }


    //update _y,_u,_v
    if (update) {
        int result = I420Scale(y, picWidth, u, picWidth / 2, v, picWidth / 2, picWidth, picHeight,
                               _y, previewWidth, _u, previewWidth / 2, _v, previewWidth / 2,
                               previewWidth, previewHeight, kFilterNone);
        //int result = I420Scale(y,previewHeight,u,previewHeight/2,v,previewHeight/2,previewHeight,previewWidth,_y,previewHeight,_u,previewHeight/2,_v,previewHeight/2,previewHeight,previewWidth,kFilterNone);
        LOGE("Image Scale result %d", result);
        update = 0;
    }

    LOGE("begin to change image");

    //set yuv420sp buffer
    uint8 *to = (uint8 *) _dst;
    memcpy(to, _y, _y_size);
    for (int i = 0; i < _uv_size; i++) {
        to[_y_size + i * 2] = _u[i];
        to[_y_size + i * 2 + 1] = _v[i];
    }
    return to;
}
