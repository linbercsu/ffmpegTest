//
// Created by linlin zhao on 2024/5/23.
//

#pragma once
#include <android/log.h>

#define next_log(format, ...) __android_log_print(6, "next", format, __VA_ARGS__)
#define next_log_tag(TAG, format, ...) __android_log_print(6, TAG, format, __VA_ARGS__)