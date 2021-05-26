/*
 * YXAndroidAllClass.c
 *
 *  Created on: 2016-11-3
 *      Author:
 */



#include "NXAndroidAllClass.h"

int YX_LoadAll__catchAll(JNIEnv *env)
{
    int ret = 0;

    YX_LOAD_CLASS(android_os_Build);

fail:
    return ret;
}
