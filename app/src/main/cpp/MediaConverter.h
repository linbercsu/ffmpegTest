//
// Created by ZhaoLinlin on 2021/5/18.
//

#ifndef MXCORE_BUILD_MEDIACONVERTER_H
#define MXCORE_BUILD_MEDIACONVERTER_H

#include <memory>
#include <jni.h>

class OutputStream;
class InputStream;
class ProcessCallback;

class MediaConverter {

public:
    static void initClass(JNIEnv *pEnv, jclass pJclass);

    MediaConverter(ProcessCallback* callback, const char* sourcePath, const char* targetPath, const char* format);

    ~MediaConverter() noexcept;

    const char* convert();
    void cancel();

private:
    static const char TAG[];

    std::unique_ptr<OutputStream> target;
    std::unique_ptr<InputStream> inputStream;
};


#endif //MXCORE_BUILD_AUDIOCONVERTER_H
