//
// Created by linlin zhao on 2024/5/24.
//

#include "AudioRender.h"
#include "VideoPackageQueue.h"
#include "Exception.h"
#include <aaudio/AAudio.h>
#include "Log.h"
#include "Releasable.h"
#include <chrono>
#include "sonic.h"
#include "define.h"

using namespace std::chrono;

bool lastRender = false;
extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
}

class AndroidAAudioDataCallback {
public:
    virtual aaudio_data_callback_result_t onAndroidAAudioDataCallback(
            AAudioStream *stream,
            void *audioData,
            int32_t numFrames) = 0;
};

static aaudio_data_callback_result_t audioStream_dataCallback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {
//    auto render = (next::AudioRender*) userData;
//    return render->audioStream_dataCallback(stream, audioData, numFrames);
    auto render = (AndroidAAudioDataCallback*) userData;
    return render->onAndroidAAudioDataCallback(stream, audioData, numFrames);
}

namespace next {
    namespace {
        const int MESSAGE_ID_RENDER_IDLE = Message::MESSAGE_ID_USER + 1;
        const int MESSAGE_ID_INIT_RENDER = MESSAGE_ID_RENDER_IDLE + 1;
        const int MESSAGE_ID_INIT_AUDIO_DEVICE = MESSAGE_ID_INIT_RENDER + 1;
        const int MESSAGE_ID_INIT_DECODER = MESSAGE_ID_INIT_AUDIO_DEVICE + 1;
        const int MESSAGE_ID_PROCESS_PACKAGE = MESSAGE_ID_INIT_DECODER + 1;
        const int MESSAGE_ID_RESEND_PACKAGE = MESSAGE_ID_PROCESS_PACKAGE + 1;
        const int MESSAGE_ID_RECEIVE_FRAME = MESSAGE_ID_RESEND_PACKAGE + 1;
        const int MESSAGE_ID_PRE_SEEK = MESSAGE_ID_RECEIVE_FRAME + 1;
        const int MESSAGE_ID_PAUSE = MESSAGE_ID_PRE_SEEK + 1;
        const int MESSAGE_ID_START = MESSAGE_ID_PAUSE + 1;

        const int MESSAGE_PRIORITY_RENDER = Message::MESSAGE_PRIORITY_NORMAL + 1;
        const int MESSAGE_PRIORITY_PRE_SEEK = Message::MESSAGE_PRIORITY_NORMAL + 1;
        const int MESSAGE_PRIORITY_INIT = MESSAGE_PRIORITY_PRE_SEEK + 1;


    }

    AudioOutput::~AudioOutput() {

    }

    class AndroidAAudioOutput : public AudioOutput, AndroidAAudioDataCallback {
    public:
        ~AndroidAAudioOutput() {

        }

        void start() override {
            AAudioStream_requestStart(mStream);
        }

        void pause() override {
            AAudioStream_requestPause(mStream);
        }

        void close() override {
            if (mStream != nullptr) {
                AAudioStream_close(mStream);
                mStream = nullptr;
            }
        }

        int getSampleRate() override {
            return outputSampleRate;
        }

        int getChannelLayout() override {
            return outputChannelLayout;
        }

        int getChannelCount() override {
            return outputChannelCount;
        }

        int getFormat() override {
            return outputSampleFormat;
        }

        void open(next::AudioRender *render) override {
            this->audioRender = render;

            AAudioStreamBuilder *builder;
            aaudio_result_t result = AAudio_createStreamBuilder(&builder);
            AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
            AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
            AAudioStreamBuilder_setDataCallback(builder, audioStream_dataCallback, static_cast<AndroidAAudioDataCallback*>(this));
            next_log_tag("Audio", "open %d %d", result, __LINE__);
// Setup stream any way you want.
//            AAudioStreamBuilder_setChannelCount(builder, numChannels);
//            AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT); // or PCM16


            result = AAudioStreamBuilder_openStream(builder, &mStream);
            next_log_tag("Audio", "open %d %d", result, __LINE__);
            AAudioStreamBuilder_delete(builder);
            if (result != AAUDIO_OK) {
                throw std::bad_alloc();
            }

//            int32_t framesPerBurst = AAudioStream_getFramesPerBurst(stream);
            outputSampleRate = AAudioStream_getSampleRate(mStream);
//            samplesPerFrame = AAudioStream_getSamplesPerFrame(stream);
//            samplesPerFrame = sampleRate / 50;
//            samplesPerFrame = framesPerBurst;
            outputChannelCount = AAudioStream_getChannelCount(mStream);

            // Common default mappings
            switch (outputChannelCount) {
                case 1: // Mono
                    outputChannelLayout = AV_CH_LAYOUT_MONO;
                    break;
                case 2: // Stereo (Left, Right)
                    outputChannelLayout = AV_CH_LAYOUT_STEREO;
                    break;
                case 3:
                    outputChannelLayout = AV_CH_LAYOUT_2POINT1;
                    break;
                case 4:
                    outputChannelLayout = AV_CH_LAYOUT_3POINT1;
                    break;
                case 5:
                    outputChannelLayout = AV_CH_LAYOUT_4POINT1;
                    break;
                case 6: // 5.1 surround
                    outputChannelLayout = AV_CH_LAYOUT_5POINT1;
                    break;
                default:
                    throw std::bad_cast();
                    // etc.
            }

            auto format = AAudioStream_getFormat(mStream);

            if (format == AAUDIO_FORMAT_PCM_I16) {
                outputSampleFormat = AV_SAMPLE_FMT_S16;
            } else if (format == AAUDIO_FORMAT_PCM_FLOAT) {
                outputSampleFormat = AV_SAMPLE_FMT_FLT;
            } else {
                throw std::bad_cast();
            }

            result = AAudioStream_requestStart(mStream);
            if (result != AAUDIO_OK){
                throw std::bad_alloc();
            }
        }

        aaudio_data_callback_result_t onAndroidAAudioDataCallback(AAudioStream *stream, void *audioData, int32_t numFrames) override {
            return audioRender->audioStream_dataCallback(stream, audioData, numFrames);
        }

    private:
        int outputSampleFormat;
        int outputChannelLayout;
        int outputChannelCount;
        int outputSampleRate;
        AudioRender* audioRender{nullptr};
        AAudioStream *mStream{nullptr};
    };



    class FrameAutoRelease {
    public:
        FrameAutoRelease(AVFrame * frame): mRef(frame) {

        }

        ~FrameAutoRelease() {
            if (mRef != nullptr) {
                av_frame_free(&mRef);
                mRef = nullptr;
            }
        }

    private:
        AVFrame * mRef;
    };

    namespace {
        int64_t nowMicro() {
            microseconds ms = duration_cast< microseconds >(
                    system_clock::now().time_since_epoch()
            );

            return ms.count();
        }
    }

    AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                               uint64_t channel_layout, int channelCount,
                               int sample_rate, int nb_samples) {
        AVFrame *frame = av_frame_alloc();
        int ret;

        if (!frame) {
            throw std::bad_alloc();
        }

        frame->format = sample_fmt;
        frame->channels = channelCount;
        frame->channel_layout = channel_layout;
        frame->sample_rate = sample_rate;
        frame->nb_samples = nb_samples;

        if (nb_samples) {
            ret = av_frame_get_buffer(frame, 0);
            if (ret < 0) {
                throw std::bad_alloc();
            }
        }

        return frame;
    }

    int sampleSize(AVFrame * frame, int count) {
        if (frame->format == AV_SAMPLE_FMT_S16) {
            return count * 2 * frame->channels;
        } else if (frame->format == AV_SAMPLE_FMT_FLT) {
            return count * 4 * frame->channels;
        }
        throw std::bad_cast();
    }

    int persampleSizeInByte(AVFrame * frame) {
        if (frame->format == AV_SAMPLE_FMT_S16) {
            return  2 * frame->channels;
        } else if (frame->format == AV_SAMPLE_FMT_FLT) {
            return 4 * frame->channels;
        }
        throw std::bad_cast();
    }




    class AudioFrameBuffer {

    public:
        AVFrame* buffer = {nullptr};
        AVFrame* bufferTmp = {nullptr};
        int SIZE = 1024 * 4;
        int count = 0;
        int64_t firstPts{0};

        ~AudioFrameBuffer() {
            if (buffer != nullptr) {
                av_frame_free(&buffer);
                av_frame_free(&bufferTmp);
            }
        }

        void sendFrame(AVFrame* pFrame, int pCount) {
            if (buffer == nullptr) {
                buffer = alloc_audio_frame((enum AVSampleFormat)pFrame->format, pFrame->channel_layout, pFrame->channels, pFrame->sample_rate, SIZE);
                bufferTmp = alloc_audio_frame((enum AVSampleFormat)pFrame->format, pFrame->channel_layout, pFrame->channels, pFrame->sample_rate, SIZE);
            }

            av_frame_make_writable(buffer);
            av_frame_make_writable(bufferTmp);

            auto oldSize = sampleSize(pFrame, count);

            if (pCount + count > SIZE) {
                SIZE = pCount + count;
                AVFrame * buffer1 = alloc_audio_frame((enum AVSampleFormat)pFrame->format, pFrame->channel_layout, pFrame->channels, pFrame->sample_rate, SIZE);
                AVFrame* bufferTmp1 = alloc_audio_frame((enum AVSampleFormat)pFrame->format, pFrame->channel_layout, pFrame->channels, pFrame->sample_rate, SIZE);
                av_frame_make_writable(buffer1);
                av_frame_make_writable(bufferTmp1);


                if (count > 0) {
                    memcpy(buffer1->data[0], buffer->data[0], oldSize);

                    //planar need copy more data[1], data[2]
                }

                av_frame_free(&buffer);
                av_frame_free(&bufferTmp);

                buffer = buffer1;
                bufferTmp = bufferTmp1;
//                throw ConvertException("buffer internal error");
            }

            auto size = sampleSize(pFrame, pCount);
            memcpy(buffer->data[0] + oldSize, pFrame->data[0], size);
//            memcpy(buffer->data[1] + count * 4, pFrame->data[1], pCount * 4);
            count += pCount;
        }

        bool canReceive(int pCount) {
            return count >= pCount;
        }

        int receiveFrame(AVFrame* out, int pCount) {
            if (count < pCount)
                return AVERROR(EAGAIN);

            out->nb_samples = pCount;
            auto size = sampleSize(out, pCount);
            memcpy(out->data[0], buffer->data[0], size);
//            memcpy(out->data[1], buffer->data[1], pCount * 4);

            int remain = count - pCount;
            count = remain;
            if (remain == 0) {
                return 0;
            }
            auto remainSize = sampleSize(out, remain);
            memcpy(bufferTmp->data[0], buffer->data[0] + size, remainSize);
//            memcpy(bufferTmp->data[1], buffer->data[1] + size, remain * 4);

            memcpy(buffer->data[0], bufferTmp->data[0], remainSize);
//            memcpy(buffer->data[1], bufferTmp->data[1], remain * 4);
            return 0;
        }

        void clear() {
            count = 0;
        }
    };

    class AudioDataContext {
    public:
        ~AudioDataContext() {

            if (decoderContext != nullptr) {
                avcodec_free_context(&decoderContext);
                decoderContext = nullptr;
            }
        }

        AVCodecContext* decoderContext{nullptr};
        AVCodec* decoder{nullptr};
        AVPacket* resendPkt{nullptr};
        AudioFrameBuffer frameBuffer;
        AVRational timebase;
        bool seek{false};
    };

    class AudioConverter {
    public:
        AudioConverter(int sourceChannelCount, int sourceChannelLayout, int sourceSampleRate, int sourceSampleFormat, int targetChannelCount, int targetChannelLayout, int targetSampleRate, int targetSampleFormat):
        mSourceSampleRate(sourceSampleRate),
        mTargetSampleRate(targetSampleRate),
        mTargetFormat(targetSampleFormat),
        mTargetChannelCount(targetChannelCount),
        mTargetChannelLayout(targetChannelLayout) {

            swr_ctx = swr_alloc();
            /* set options */
            av_opt_set_int(swr_ctx, "in_channel_layout", sourceChannelLayout, 0);
//            av_opt_set_int(swr_ctx, "out_channel_layout", mSourceChannelLayout, 0);
            av_opt_set_int(swr_ctx, "in_channel_count", sourceChannelCount, 0);
            av_opt_set_int(swr_ctx, "out_channel_count", targetChannelCount, 0);
            av_opt_set_int(swr_ctx, "out_channel_layout", targetChannelLayout, 0);
            av_opt_set_int(swr_ctx, "in_sample_rate", sourceSampleRate, 0);
            av_opt_set_int(swr_ctx, "out_sample_rate", targetSampleRate, 0);
            av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (enum AVSampleFormat) sourceSampleFormat, 0);
//            if (format == AAUDIO_FORMAT_PCM_I16) {
//                targetFormat = AV_SAMPLE_FMT_S16;
            av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (enum AVSampleFormat)targetSampleFormat, 0);
//                av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", targetFormat, 0);


            auto ret = 0;
            /* initialize the resampling context */
            if ((ret = (swr_init(swr_ctx))) < 0) {
                throw std::bad_alloc();
            }
        }

        ~AudioConverter() {
            if (mSonicStream != nullptr) {
                sonicDestroyStream(mSonicStream);
                mSonicStream = nullptr;
            }

            if (swr_ctx != nullptr) {
                swr_free(&swr_ctx);
                swr_ctx = nullptr;
            }
        }

        AVFrame *getAudioFrame(int size) const {
            return alloc_audio_frame((enum AVSampleFormat)mTargetFormat, mTargetChannelLayout, mTargetChannelCount,
                                     mTargetSampleRate, size);
        }

        void convert(AVFrame *oldFrame, LockFrameQueue &queue, AudioFrameBuffer &audioFrameBuffer,
                     int pSpeed) {
            int ret;
            int dst_nb_samples;

            dst_nb_samples = av_rescale_rnd(
                    swr_get_delay(swr_ctx, mSourceSampleRate) +
                    oldFrame->nb_samples,
                    mTargetSampleRate, mSourceSampleRate, AV_ROUND_UP);
//            __android_log_print(6, "AudioConverter", "resample %d, %d, %d, %d, %d, %d", sourceSampleFormat, codecContext->sample_fmt, sourceSample_rate, codecContext->sample_rate, audioFrame->nb_samples, dst_nb_samples);

            auto targetSize = dst_nb_samples * 2;
            auto frame = getAudioFrame(targetSize);
//            FrameAutoRelease r(frame);
            ret = av_frame_make_writable(frame);
            if (ret < 0) {
                throw std::bad_alloc();
            }


            ret = swr_convert(swr_ctx,
                              frame->data, dst_nb_samples,
                              (const uint8_t **) oldFrame->data, oldFrame->nb_samples);
            if (ret < 0) {
                throw std::bad_cast();
            }

            frame->pts = oldFrame->pts;
            frame->nb_samples = ret;
            frame->width = 0;

            int speed = pSpeed;
            if (mSonicStream == nullptr) {
                mSonicStream = sonicCreateStream(mTargetSampleRate, mTargetChannelCount);
                sonicSetQuality(mSonicStream, 1);
            }
            sonicSetSpeed(mSonicStream, (float)speed);

//            auto targetCount = frame->nb_samples * 2;
            if (mTargetFormat == AV_SAMPLE_FMT_S16) {
                sonicWriteShortToStream(mSonicStream, (const short *)(frame->data[0]), frame->nb_samples);
                ret = sonicReadShortFromStream(mSonicStream, (short *) (frame->data[0]),
                                               targetSize);

            } else if (mTargetFormat == AV_SAMPLE_FMT_FLT) {
                sonicWriteFloatToStream(mSonicStream, (const float *)(frame->data[0]), frame->nb_samples);
                ret = sonicReadFloatFromStream(mSonicStream, (float *) (frame->data[0]),
                                               targetSize);
            } else {
                throw std::bad_cast();
            }

            if (ret > 0) {
                frame->nb_samples = ret;
                queue.pushFrame(frame);
                return;
            } else {
                av_frame_free(&frame);
                return;
            }

        }


    private:
        int mSourceSampleRate;
        int mTargetSampleRate;
        int mTargetFormat;
        int mTargetChannelCount;
        int mTargetChannelLayout;
        struct SwrContext* swr_ctx;
        sonicStream mSonicStream{nullptr};
    };

    class AudioDevice {
    public:
        AudioDevice(AudioRender* render):mRenderRef(render){

        }

        void pause() {
            AAudioStream_requestPause(stream);
        }

        void start() {
            AAudioStream_requestStart(stream);
        }

        void clear() {
//            AAudioStream_requestStop(stream);
            if (stream != nullptr) {
                AAudioStream_close(stream);
                stream = nullptr;
            }
            if (mSonicStream != nullptr) {
                sonicDestroyStream(mSonicStream);
                mSonicStream = nullptr;
            }
            if (swr_ctx != nullptr) {
                swr_free(&swr_ctx);
                swr_ctx = nullptr;
            }
        }
        void reset() {
//            AAudioStream_requestStop(stream);
            if (mSonicStream != nullptr) {
                sonicDestroyStream(mSonicStream);
                mSonicStream = nullptr;
            }
        }

        void withSourceCodecParameter(AVCodecParameters *parameters) {
            mSourceChannelLayout = parameters->channel_layout;
            mSourceFormat = parameters->format;
            mSourceSampleRate = parameters->sample_rate;
            mSourceChannelCount = parameters->channels;

            next_log_tag("Audio", "withSourceCodecParameter %d, %d, %d, %d %d,", mSourceFormat, mSourceChannelCount, mSourceSampleRate, mSourceChannelLayout, __LINE__);

        }
        
        void closeStreamSync() {
            if (stream == nullptr)
                return;

            std::atomic_bool &closed = mRenderRef->mStreamClosed;
            if (closed.load()) {
                return;
            }

            closed.store(true);

            for (int i = 0; i < 5; i++) {
                auto state = AAudioStream_getState(stream);
                if (state == AAUDIO_STREAM_STATE_STOPPED || AAUDIO_STREAM_STATE_CLOSED == state) {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            AAudioStream_close(stream);
            stream = nullptr;
        }

        void openStream() {
            std::atomic_bool &closed = mRenderRef->mStreamClosed;
            if (!closed.load()) {
                return;
            }

            closed.store(false);
            openStreamInternal();
        }

        void openStreamInternal() {
            AAudioStreamBuilder *builder;
            aaudio_result_t result = AAudio_createStreamBuilder(&builder);
            AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
            AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
            AAudioStreamBuilder_setDataCallback(builder, audioStream_dataCallback, mRenderRef);
            next_log_tag("Audio", "open %d %d", result, __LINE__);
// Setup stream any way you want.
//            AAudioStreamBuilder_setChannelCount(builder, numChannels);
//            AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT); // or PCM16


            result = AAudioStreamBuilder_openStream(builder, &stream);
            next_log_tag("Audio", "open %d %d", result, __LINE__);
            AAudioStreamBuilder_delete(builder);
            if (result != AAUDIO_OK) {
                throw std::bad_alloc();
            }

            int32_t framesPerBurst = AAudioStream_getFramesPerBurst(stream);
            sampleRate = AAudioStream_getSampleRate(stream);
//            samplesPerFrame = AAudioStream_getSamplesPerFrame(stream);
//            samplesPerFrame = sampleRate / 50;
            samplesPerFrame = framesPerBurst;
            channelCount = AAudioStream_getChannelCount(stream);
            format = AAudioStream_getFormat(stream);

            result = AAudioStream_requestStart(stream);
            if (result != AAUDIO_OK){
                throw std::bad_alloc();
            }
        }

        void open() {
            openStreamInternal();
//            next_log_tag("Audio", "open %d %d", result, __LINE__);

            swr_ctx = swr_alloc();
            /* set options */
            av_opt_set_int(swr_ctx, "in_channel_layout", mSourceChannelLayout, 0);
//            av_opt_set_int(swr_ctx, "out_channel_layout", mSourceChannelLayout, 0);
            av_opt_set_int(swr_ctx, "in_channel_count", mSourceChannelCount, 0);
            av_opt_set_int(swr_ctx, "out_channel_count", channelCount, 0);
            av_opt_set_int(swr_ctx, "in_sample_rate", mSourceSampleRate, 0);
            av_opt_set_int(swr_ctx, "out_sample_rate", sampleRate, 0);
            av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (enum AVSampleFormat) mSourceFormat, 0);
            if (format == AAUDIO_FORMAT_PCM_I16) {
                targetFormat = AV_SAMPLE_FMT_S16;
                av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", targetFormat, 0);
            } else if (format == AAUDIO_FORMAT_PCM_FLOAT) {
                targetFormat = AV_SAMPLE_FMT_FLT;
                av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", targetFormat, 0);
            } else {
                throw std::bad_cast();
            }


            auto ret = 0;
            /* initialize the resampling context */
            if ((ret = (swr_init(swr_ctx))) < 0) {
                throw std::bad_alloc();
            }
//            next_log_tag("Audio", "frames %d, sampleRate %d, channels %d, format %d, %d",
//                         framesPerBurst, sampleRate, channelCount, format, samplesPerFrame);
//            AAudioStream_getSamplesPerFrame(stream);

        }



        AVFrame *getAudioFrame(int size) {
            return alloc_audio_frame(targetFormat, mSourceChannelLayout, channelCount,
                                          sampleRate, size);
        }

        void convert(AVFrame *oldFrame, LockFrameQueue &queue, AudioFrameBuffer &audioFrameBuffer,
                     int pSpeed) {
            int ret;
            int dst_nb_samples;

            dst_nb_samples = av_rescale_rnd(
                    swr_get_delay(swr_ctx, mSourceSampleRate) +
                    oldFrame->nb_samples,
                    sampleRate, mSourceSampleRate, AV_ROUND_UP);
//            __android_log_print(6, "AudioConverter", "resample %d, %d, %d, %d, %d, %d", sourceSampleFormat, codecContext->sample_fmt, sourceSample_rate, codecContext->sample_rate, audioFrame->nb_samples, dst_nb_samples);

            auto targetSize = dst_nb_samples * 2;
            auto frame = getAudioFrame(targetSize);
//            FrameAutoRelease r(frame);
            ret = av_frame_make_writable(frame);
            if (ret < 0) {
                throw std::bad_alloc();
            }


            ret = swr_convert(swr_ctx,
                              frame->data, dst_nb_samples,
                              (const uint8_t **) oldFrame->data, oldFrame->nb_samples);
            if (ret < 0) {
                throw std::bad_cast();
            }

            frame->pts = oldFrame->pts;
            frame->nb_samples = ret;
            frame->width = 0;

            int speed = pSpeed;
            if (mSonicStream == nullptr) {
                mSonicStream = sonicCreateStream(sampleRate, channelCount);
                sonicSetQuality(mSonicStream, 1);
            }
            sonicSetSpeed(mSonicStream, (float)speed);

//            auto targetCount = frame->nb_samples * 2;
            if (targetFormat == AV_SAMPLE_FMT_S16) {
                sonicWriteShortToStream(mSonicStream, (const short *)(frame->data[0]), frame->nb_samples);
                ret = sonicReadShortFromStream(mSonicStream, (short *) (frame->data[0]),
                                               targetSize);

            } else if (targetFormat == AV_SAMPLE_FMT_FLT) {
                sonicWriteFloatToStream(mSonicStream, (const float *)(frame->data[0]), frame->nb_samples);
                ret = sonicReadFloatFromStream(mSonicStream, (float *) (frame->data[0]),
                                               targetSize);
            } else {
                throw std::bad_cast();
            }

            if (ret > 0) {
                frame->nb_samples = ret;
                queue.pushFrame(frame);
                return;
            } else {
                av_frame_free(&frame);
                return;
            }
        }

        int write(AVFrame* frame) {
                auto ret = AAudioStream_write(stream, frame->data[0], frame->nb_samples, 500000000);

                return ret;
//                next_log_tag("Audio", "write: error %d %d, %d", frame->nb_samples, ret, __LINE__);
        }

        void setStartPts(int64_t pts) {
            startPts = pts;
            samples_count = 0;
        }


        AudioRender* mRenderRef;
        AAudioStream *stream;
        int32_t sampleRate;
        int32_t channelCount;
        int32_t format;
        AVSampleFormat targetFormat;

        struct SwrContext *swr_ctx{};

        int mSourceFormat;
        int mSourceSampleRate;
        int mSourceChannelCount;
        int mSourceChannelLayout;
        int64_t samples_count{0};
        int32_t samplesPerFrame;
        int64_t startPts{0};
        sonicStream mSonicStream{nullptr};
        int mSpeed{DEFAULT_SPEED};
    };
    

    AudioRender::AudioRender(next::VideoPackageQueue *pQueue, MediaClock* clock) : mThread(this), mQueueRef(pQueue),
                                                                mFrameQueue(1024 * 8), mMediaClockRef(clock) {
        mAudioOutput = new AndroidAAudioOutput();
        sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
    }

    AudioRender::~AudioRender() {
        next_log_tag("audio", "delete AudioRender %d", __LINE__);
        delete mAudioOutput;
    }

    void AudioRender::release() {
//        mAudioDevice->closeStreamSync();

//        mAudioDevice->clear();
        mAudioOutput->close();

        if (mAudioConverter != nullptr) {
            delete mAudioConverter;
            mAudioConverter = nullptr;
        }

        mFrameQueue.clear();

        if (mDataContext != nullptr) {
            delete mDataContext;
            mDataContext = nullptr;
        }

        if (currentFrame != nullptr) {
            av_frame_free(&currentFrame);
            currentFrame = nullptr;
        }

        if (reusedAudioFrame != nullptr) {
            av_frame_free(&reusedAudioFrame);
            reusedAudioFrame = nullptr;
        }
    }

    void AudioRender::onFrame(AVFrame *frame, AVRational timebase, int speed) {
        int64_t p = av_frame_get_best_effort_timestamp(frame);

        p = av_rescale_q(p,
                         timebase,
                         AV_TIME_BASE_Q);

        frame->pts = p;

        mAudioConverter->convert(frame, mFrameQueue, mDataContext->frameBuffer, speed);
//        newFrame->pts = p;
    }

    void AudioRender::stop() {
//        mAudioDevice->closeStreamSync();
//        mAudioOutput->close();
//        if (mAudioConverter != nullptr) {
//            delete mAudioConverter;
//            mAudioConverter = nullptr;
//        }
        mStopped.store(true);
        mThread.stop();
        mThread.join();
    }

    bool AudioRender::isStopped() {
        return mStopped.load();
    }

    void AudioRender::pause() {
        if (mPaused.load()) {
            return;
        }

        mPaused.store(true);
        sendMessage(MESSAGE_ID_PAUSE);
    }

    void AudioRender::start() {
        if (!mPaused.load()) {
            return;
        }

        mPaused.store(false);
        sendMessage(MESSAGE_ID_START);
    }

    bool AudioRender::isPaused() {
        return mPaused.load();
    }

    void AudioRender::preSeek() {
        sendMessage(MESSAGE_ID_PRE_SEEK, MESSAGE_PRIORITY_PRE_SEEK);
    }

    void AudioRender::handleMessage(const next::Message &message) {
        switch (message.getId()) {
            case MESSAGE_ID_INIT_DECODER: {
                onMessageInit();
                break;
            }

            case MESSAGE_ID_INIT_AUDIO_DEVICE: {
                onMessageInitAudioDevice();
                break;
            }

            case MESSAGE_ID_PROCESS_PACKAGE: {
                onMessageProcessPkt();
                break;
            }

            case MESSAGE_ID_RESEND_PACKAGE: {
                onMessageResendPkt();
                break;
            }

            case MESSAGE_ID_RECEIVE_FRAME: {
                onMessageReceiveFrame();
                break;
            }

            case MESSAGE_ID_PRE_SEEK: {
//                onMessagePreSeek();
                break;
            }

            case MESSAGE_ID_PAUSE: {
                onMessagePause();
                break;
            }

            case MESSAGE_ID_START: {
                onMessageStart();
                break;
            }

            default:{
                break;
            }
        }
    }

    void AudioRender::onMessageInit()  {

        int ret = 0;

        auto codecParameters = mQueueRef->getCodecParameters();

        if (codecParameters == nullptr) {
            sendMessageDelay(MESSAGE_ID_INIT_RENDER, 100);
            return;
        }

        if (mDataContext != nullptr) {
            delete mDataContext;
            mDataContext = nullptr;
        }

        mDataContext = new AudioDataContext();
        AVRational timeBase = mQueueRef->getTimebase();
        mDataContext->timebase = timeBase;

        auto decoder = avcodec_find_decoder(codecParameters->codec_id);

        auto dec_ctx = avcodec_alloc_context3(decoder);
        mDataContext->decoderContext = dec_ctx;
        ret = avcodec_parameters_to_context(dec_ctx, codecParameters);
        dec_ctx->pkt_timebase = timeBase;

        AVDictionary *opts = nullptr;
        ret = avcodec_open2(dec_ctx, decoder, &opts);

        reusedAudioFrame = av_frame_alloc();

        sendMessage(MESSAGE_ID_INIT_AUDIO_DEVICE);
        sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
    }

    void AudioRender::onMessageInitAudioDevice() {
        auto codecParameters = mQueueRef->getCodecParameters();
//        mAudioDevice->withSourceCodecParameter(codecParameters);
//        mAudioDevice->open();
        mAudioOutput->open(this);



        mAudioConverter = new AudioConverter(codecParameters->channels,
                                             codecParameters->channel_layout,
                                             codecParameters->sample_rate,
                                             codecParameters->format,
                                             mAudioOutput->getChannelCount(),
                                             mAudioOutput->getChannelLayout(),
                                             mAudioOutput->getSampleRate(),
                                             mAudioOutput->getFormat());
    }

    void AudioRender::onMessageProcessPkt()  {
        bool clear = false;

        AVPacket *pkt = mQueueRef->getPkt(&clear);
        if (pkt == nullptr) {
            sendMessageDelay(MESSAGE_ID_PROCESS_PACKAGE, 10);
            return;
        }

        if (pkt->stream_index == NEXT_INDEX_TRACK_CHANGED) {
            av_packet_free(&pkt);
            sendMessage(MESSAGE_ID_INIT_DECODER, MESSAGE_PRIORITY_INIT);
            return;
        } else if (pkt->stream_index == NEXT_INDEX_END) {
            av_packet_free(&pkt);
            pkt = nullptr;

            mDataContext->resendPkt = nullptr;
            onMessageResendPkt();
        } else if (pkt->stream_index == NEXT_INDEX_SEEK) {
            mDataContext->seek = false;
            mFrameQueue.clear();
            auto dec = mDataContext->decoderContext;
            avcodec_flush_buffers(dec);
            sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
        } else {
            mDataContext->resendPkt = pkt;
            onMessageResendPkt();
        }
    }

    void AudioRender::onMessageResendPkt()  {
        auto pkt = mDataContext->resendPkt;
        auto dec = mDataContext->decoderContext;
        int action = mQueueRef->getTopAction();
        if (action == NEXT_INDEX_SEEK || action == NEXT_INDEX_TRACK_CHANGED) {
            mThread.messageQueue().removeMessageById(MESSAGE_ID_RECEIVE_FRAME);
            mDataContext->resendPkt = nullptr;
            av_packet_free(&pkt);
            avcodec_flush_buffers(dec);
            sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
            return;
        }

        int ret = 0;

        ret = avcodec_send_packet(dec, pkt);

        if (ret != AVERROR(EAGAIN) && ret < 0 && ret != AVERROR_EOF) {
            throw DecoderException(ret);
        }

        if (ret == AVERROR(EAGAIN)) {
            mDataContext->resendPkt = pkt;
            sendMessage(MESSAGE_ID_RESEND_PACKAGE);
        } else {
            mDataContext->resendPkt = nullptr;
            av_packet_free(&pkt);
            sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
        }

        onMessageReceiveFrame();
    }
    void AudioRender::onMessageReceiveFrame()  {
        if (mFrameQueue.isFull()) {
            sendMessageDelay(MESSAGE_ID_RECEIVE_FRAME, 10);
            return;
        }
        int ret = 0;
        auto dec = mDataContext->decoderContext;
        auto videoFrame = reusedAudioFrame;

        ret = avcodec_receive_frame(dec, videoFrame);

        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            //fixme throw
        }

        if (ret == AVERROR_EOF) {
            return;
        }

        if (ret == AVERROR(EAGAIN)) {
            return;
        }

        sendMessage(MESSAGE_ID_RECEIVE_FRAME);

        onFrame(videoFrame, dec->pkt_timebase, mMediaClockRef->getSpeed());

        av_frame_unref(videoFrame);

    }
    void AudioRender::onMessagePreSeek()  {
        mThread.messageQueue().removeMessageById(MESSAGE_ID_RECEIVE_FRAME);
        mThread.messageQueue().removeMessageById(MESSAGE_ID_RESEND_PACKAGE);
        mThread.messageQueue().removeMessageById(MESSAGE_ID_PROCESS_PACKAGE);

        if (mDataContext->resendPkt != nullptr) {
            av_packet_free(&mDataContext->resendPkt);
            mDataContext->resendPkt = nullptr;
        }

        mDataContext->seek = true;
        sendMessageDelay(MESSAGE_ID_PROCESS_PACKAGE, 10);
    }

    void AudioRender::onMessagePause() {
        mAudioOutput->pause();
    }

    void AudioRender::onMessageStart() {
        mAudioOutput->start();
    }

    void AudioRender::onThreadEnded() {
        release();
    }

    aaudio_data_callback_result_t
    AudioRender::audioStream_dataCallback(AAudioStream *stream, void *audioData,
                                          int32_t numFrames) {
        if (mStreamClosed.load()) {
            return AAUDIO_CALLBACK_RESULT_STOP;
        }

        if (currentFrame == nullptr) {
            currentFrame = mFrameQueue.pop();
            if (currentFrame != nullptr) {
                if (currentFrame->nb_samples > 0) {
                    mMediaClockRef->resetPts(currentFrame->pts);
                } else {
                    //end
                    av_frame_free(&currentFrame);
                    currentFrame = nullptr;
                    return AAUDIO_CALLBACK_RESULT_STOP;
                }
            }
        }

        if (currentFrame == nullptr) {
            return AAUDIO_CALLBACK_RESULT_CONTINUE;
        }

        auto sampleSize = persampleSizeInByte(currentFrame);
        auto numBytes = numFrames * sampleSize;
        auto targetPtr = (char*)audioData;
        auto targetPosition = 0;

        while (true) {
            auto position = currentFrame->width;
            auto remain = currentFrame->nb_samples * sampleSize - position;
            if (remain == 0) {
                av_frame_free(&currentFrame);
                currentFrame = nullptr;
                currentFrame = mFrameQueue.pop();
                if (currentFrame == nullptr) {
                    break;
                }

                if (currentFrame->nb_samples > 0) {
                    mMediaClockRef->resetPts(currentFrame->pts);
                    continue;
                } else {
                    //end
                    av_frame_free(&currentFrame);
                    currentFrame = nullptr;
                    return AAUDIO_CALLBACK_RESULT_STOP;
                }
            }

            auto canWrite = std::min(remain, numBytes);
            memcpy(targetPtr + targetPosition, currentFrame->data[0] + position, canWrite);
            currentFrame->width += canWrite;
            targetPosition += canWrite;
            numBytes -= canWrite;

            if (numBytes == 0) {
                break;
            }

            if (numBytes < 0) {
                throw std::bad_exception();
            }
        }

        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }


    void AudioRender::sendMessage(int id) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).withCallback(this));
    }

    void AudioRender::sendMessage(int id, int priority) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).priority(priority).withCallback(this));
    }

    void AudioRender::sendMessageDelay(int id, int delayMs) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).delay(delayMs).withCallback(this));
    }
}
