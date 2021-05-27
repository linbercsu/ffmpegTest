//
// Created by changping.yang on 2021/4/25.
//
#ifndef VIDEOEDIT_NXANDROIDDEF_H
#define VIDEOEDIT_NXANDROIDDEF_H

#include <jni.h>

struct _com_nx_BanubaEffect
{
    jmethodID method_createBanubaSdkManagerObject;
    jmethodID initSdk;
    jmethodID closeSdk;
    jmethodID makeBanubaCurrent;
    jmethodID makeSavedCurrent;
    jmethodID getOutTexture;
    jmethodID updateTextureImage;
    jmethodID getTimestamp;
    jmethodID getTexMatrix;
    jmethodID loadEffect;
    jmethodID surfaceCreated;
    jmethodID surfaceChanged;
    jmethodID surfaceDestroyed;
    jmethodID pushFrameWithNumber;
    jmethodID drawFrame;
    jmethodID hasBanubaEffect;

    jfieldID data_field;
    jfloatArray m_jMatrixArray;

    jobject BanubaSdkManager;
};
typedef struct _com_nx_BanubaEffect com_nx_BanubaEffect;
#endif //VIDEOEDIT_NXANDROIDDEF_H
