//
// Created by ZhaoLinlin on 2021/5/18.
//

#ifndef MXCORE_BUILD_AUDIOCONVERTER_H
#define MXCORE_BUILD_AUDIOCONVERTER_H

#include <memory>
#include <jni.h>

class OutputStream;
class InputStream;

class AudioConverter {

public:
    static void initClass(JNIEnv *pEnv, jclass pJclass);

    AudioConverter(const char* sourcePath, const char* targetPath, const char* format);

    ~AudioConverter() noexcept;

    void convert();
    void cancel();

private:
    std::unique_ptr<OutputStream> target;
    std::unique_ptr<InputStream> inputStream;
};


#endif //MXCORE_BUILD_AUDIOCONVERTER_H
