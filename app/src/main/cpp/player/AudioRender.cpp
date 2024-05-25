//
// Created by linlin zhao on 2024/5/24.
//

#include "AudioRender.h"
#include "VideoPackageQueue.h"
#include "Exception.h"
#include <aaudio/AAudio.h>
#include "Log.h"
#include <chrono>

using namespace std::chrono;

bool lastRender = false;
extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
}

namespace next {
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

    class DataContext {
    public:
        ~DataContext() {
            if (decoderContext != nullptr) {
                avcodec_free_context(&decoderContext);
                decoderContext = nullptr;
            }
        }

        AVCodecContext* decoderContext{nullptr};
        AVCodec* decoder{nullptr};
        AudioFrameBuffer frameBuffer;
    };

    class AudioDevice {
    public:
        AudioDevice() {

        }

        void clear() {
//            AAudioStream_requestStop(stream);
            AAudioStream_close(stream);

            if (swr_ctx != nullptr) {
                swr_free(&swr_ctx);
                swr_ctx = nullptr;
            }
        }

        void withSourceCodecParameter(AVCodecParameters *parameters) {
            mSourceChannelLayout = parameters->channel_layout;
            mSourceFormat = parameters->format;
            mSourceSampleRate = parameters->sample_rate;
            mSourceChannelCount = parameters->channels;

            next_log_tag("Audio", "withSourceCodecParameter %d, %d, %d, %d %d,", mSourceFormat, mSourceChannelCount, mSourceSampleRate, mSourceChannelLayout, __LINE__);

        }

        void open() {
            AAudioStreamBuilder *builder;
            aaudio_result_t result = AAudio_createStreamBuilder(&builder);
            AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);

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
            next_log_tag("Audio", "frames %d, sampleRate %d, channels %d, format %d, %d",
                         framesPerBurst, sampleRate, channelCount, format, samplesPerFrame);
//            AAudioStream_getSamplesPerFrame(stream);

        }



        AVFrame *getAudioFrame(int size) {
            return alloc_audio_frame(targetFormat, mSourceChannelLayout, channelCount,
                                          sampleRate, size);
        }

        void convert(AVFrame *oldFrame, LockFrameQueue& queue, AudioFrameBuffer& audioFrameBuffer) {
            int ret;
            int dst_nb_samples;

            dst_nb_samples = av_rescale_rnd(
                    swr_get_delay(swr_ctx, mSourceSampleRate) +
                    oldFrame->nb_samples,
                    sampleRate, mSourceSampleRate, AV_ROUND_UP);
//            __android_log_print(6, "AudioConverter", "resample %d, %d, %d, %d, %d, %d", sourceSampleFormat, codecContext->sample_fmt, sourceSample_rate, codecContext->sample_rate, audioFrame->nb_samples, dst_nb_samples);

            auto targetSize = std::max(samplesPerFrame, dst_nb_samples);
            auto frame = getAudioFrame(targetSize);
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

            frame->nb_samples = ret;

            audioFrameBuffer.sendFrame(frame, ret);

            while (true) {
                if (!audioFrameBuffer.canReceive(samplesPerFrame)) {
                    break;
                }
                auto newFrame = getAudioFrame(samplesPerFrame);
                audioFrameBuffer.receiveFrame(newFrame, samplesPerFrame);

                int64_t pts = av_rescale_q(samples_count,
                                           (AVRational) {1, sampleRate},
                                           AV_TIME_BASE_Q);

                int64_t duration = av_rescale_q(newFrame->nb_samples,
                                           (AVRational) {1, sampleRate},
                                           AV_TIME_BASE_Q);

                newFrame->pts = pts + startPts;
                newFrame->pkt_duration = duration;
                samples_count += newFrame->nb_samples;

                queue.pushFrame(newFrame);
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

        friend class AudioOutput;

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
    };
    
    class AudioOutput {
    public:
        AudioOutput(AudioRender* render, AudioDevice *pDevice, LockFrameQueue *pQueue, MediaClock* clock):mAudioRenderRef(render),
        mAudioDeviceRef(pDevice),
                                                                  mFrameQueueRef(pQueue),
                                                                  mMediaClockRef(clock){
            mThread = new std::thread(&AudioOutput::run, this);
        }

        ~AudioOutput() {
            delete mThread;
            mThread = nullptr;
        }

        void stop() {
            mStopped.store(true);
            mThread->join();
        }

        bool isStopped() {
            return mStopped.load();
        }

        void run() {
            AVFrame *frame = nullptr;
            bool resetPts = true;
            int64_t lastFramePts = 0;
            int64_t lastFrameDuration = 0;
            while (!isStopped()) {
                if (frame != nullptr) {
                    av_frame_free(&frame);
                    frame = nullptr;
                }

                if (mAudioRenderRef->isPaused()) {
                    while (true) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        if (!mAudioRenderRef->isPaused() || mAudioRenderRef->isStopped()) {
                            break;
                        }
                    }

                    resetPts = true;

                    if (isStopped()) {
                        break;
                    }
                }

                frame = mFrameQueueRef->pop();
                if (frame == nullptr) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                } else {
                    //seek backward
                    if (lastFramePts > frame->pts) {
                        resetPts = true;
                    } else if (std::abs(lastFramePts + lastFrameDuration - frame->pts) > 10000) {//10 millsecond
                        //seek forward
                        resetPts = true;
                    }

                    lastFramePts = frame->pts;
                    lastFrameDuration = frame->pkt_duration;
                    if (resetPts) {
                        next_log_tag("audio", "reset pts %ld, %d", frame->pts, __LINE__);
                        mMediaClockRef->resetStartPts(frame->pts);
                        resetPts = false;
                    }
                    if (!isStopped()) {
//                        auto now = nowMicro();

                        if (mFirstFrame) {
                            mFirstFrame = false;
//                            startTime = now;
//                            mMediaClockRef->resetStartPts(0, nowMicro());
//                            next_log_tag("Audio", "output, first pts %ld, %d", frame->pts, __LINE__);
                        }
                        //may be write partical
                        auto written = 0;
                        while (!isStopped()) {
                            auto ret = AAudioStream_write(mAudioDeviceRef->stream, frame->data[0] + written, frame->nb_samples - written, 500000000);
                            mMediaClockRef->calculatePts();
                            if (ret < 0) {
                                next_log_tag("Audio", "AudioOutput, ret = %d %d", ret, __LINE__);
                                throw std::bad_function_call();
                                break;
                            }

                            if (ret == 0) {
                                next_log_tag("Audio", "AudioOutput, ret = 0 %d", __LINE__);
                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            } else {

                                //ret > 0
                                written += ret;
                                if (written == frame->nb_samples) {
                                    break;
                                }

                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            }
//                            mMediaClockRef->calculatePtsWithTime(now);
                        }
//                        auto ret = mAudioDeviceRef->write(frame);


                    }
                }
            }
        }

        int64_t calculateDurationInMicro(int sampleCount, int sampleRate) {
            return (((int64_t)sampleCount) * 1000000)/ sampleRate;
        }
        
    private:
        std::atomic_bool mStopped{false};
        std::thread* mThread;
        bool mFirstFrame{true};
        MediaClock* mMediaClockRef;
        AudioDevice* mAudioDeviceRef;
        LockFrameQueue* mFrameQueueRef;
        AudioRender* mAudioRenderRef;
        int mSampleCount{0};
        int64_t startPts{0};
        int64_t currentPts{0};
        int64_t startTime{0};
    };

    AudioRender::AudioRender(next::VideoPackageQueue *pQueue, MediaClock* clock) : mQueueRef(pQueue),
                                                                mFrameQueue(1024 * 1024 * 10), mMediaClockRef(clock) {
        mAudioDevice = new AudioDevice();
        mThread = new std::thread(&AudioRender::run, this);

        mAudioOutput = new AudioOutput(this, mAudioDevice, &mFrameQueue, mMediaClockRef);
    }

    void AudioRender::run() {
        mDataContext = new DataContext();
        runInternal();

        mAudioDevice->clear();

        mFrameQueue.clear();

        delete mDataContext;
        mDataContext = nullptr;

        if (reusedAudioFrame != nullptr) {
            av_frame_free(&reusedAudioFrame);
            reusedAudioFrame = nullptr;
        }
    }
    void AudioRender::runInternal() {
        int ret = 0;
        AVCodecParameters *codecParameters = nullptr;
        AVRational timeBase;

        while (!isStopped()) {
            codecParameters = mQueueRef->getCodecParameters();

            if (codecParameters == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            timeBase = mQueueRef->getTimebase();

            break;
        }

        if (isStopped()) {
            return;
        }

        mAudioDevice->withSourceCodecParameter(codecParameters);
        mAudioDevice->open();

        auto decoder = avcodec_find_decoder(codecParameters->codec_id);

        auto dec_ctx = avcodec_alloc_context3(decoder);
        mDataContext->decoderContext = dec_ctx;
        ret = avcodec_parameters_to_context(dec_ctx, codecParameters);
        dec_ctx->pkt_timebase = timeBase;

        AVDictionary *opts = nullptr;
        ret = avcodec_open2(dec_ctx, decoder, &opts);

        reusedAudioFrame = av_frame_alloc();
        bool resetFirstPts = true;
        while (!isStopped()) {
            if (mQueueRef->getClearFlagAndClear()) {
                mFrameQueue.clear();

                avcodec_flush_buffers(dec_ctx);
                mDataContext->frameBuffer.clear();
                resetFirstPts = true;
            }

            AVPacket *pkt = mQueueRef->getPkt();

            if (pkt == nullptr) {
//                next_log_tag("audio", "waiting pkt %d", __LINE__);
//                render();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (resetFirstPts) {
                resetFirstPts = false;

                int64_t pts = av_rescale_q(pkt->pts,
                                           timeBase,
                                           AV_TIME_BASE_Q);

                mAudioDevice->setStartPts(pts);
            }

//            next_log_tag("audio", "got pkt %d", __LINE__);

            decode(dec_ctx, pkt, reusedAudioFrame);

//            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            av_packet_unref(pkt);
            av_packet_free(&pkt);

//            render();
        }
    }

    void AudioRender::decode(AVCodecContext *dec, const AVPacket *package, AVFrame *videoFrame) {

        int ret;
        ret = avcodec_send_packet(dec, package);

//        __android_log_print(6, "MediaConverter", "on video pkt %ld %d", package->pts, ret);
        if (ret != AVERROR(EAGAIN) && ret < 0) {
            throw DecoderException(ret);
        }


        while (ret == AVERROR(EAGAIN)) {
            while (true) {
                ret = avcodec_receive_frame(dec, videoFrame);
                if (ret < 0) {
                    if (ret == AVERROR_EOF)
                        return;

                    if (ret == AVERROR(EAGAIN)) {
                        ret = avcodec_send_packet(dec, package);
                        if (ret == 0)
                            break;

                        if (ret < 0 && ret != AVERROR(EAGAIN)) {
                            throw DecoderException(ret);
                        }
                        continue;
                    }

                    throw DecoderException(ret);
                }

                onFrame(videoFrame, dec->pkt_timebase);

                av_frame_unref(videoFrame);
                ret = avcodec_send_packet(dec, package);
                break;
            }
        }

        if (ret < 0) {
            throw DecoderException(ret);
        }


        // get all the available frames from the decoder
        while (true) {
            ret = avcodec_receive_frame(dec, videoFrame);
            if (ret < 0) {
                // those two return values are special and mean there is no output
                // frame available, but there were no errors during decoding
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    return;

                throw DecoderException(ret);
            }

            onFrame(videoFrame, dec->pkt_timebase);

            av_frame_unref(videoFrame);
        }

    }

    void AudioRender::onFrame(struct AVFrame *frame, AVRational timebase) {
//        int64_t p = av_frame_get_best_effort_timestamp(frame);

//        p = av_rescale_q(p,
//                         timebase,
//                         AV_TIME_BASE_Q);
//

        mAudioDevice->convert(frame, mFrameQueue, mDataContext->frameBuffer);
//        newFrame->pts = p;
    }

    void AudioRender::render() {
        if (mFrameQueue.isEmpty()) {
            if (mQueueRef->isEnd()) {
                if (!lastRender) {
                    lastRender = true;
                    next_log_tag("render", "last frame %d", __LINE__);
                }
            }
            return;
        }


        do {
            AVFrame *pFrame = mFrameQueue.pop();
            mAudioDevice->write(pFrame);
        } while (mFrameQueue.isFull());

        /*
        if (mCurrentFrame == nullptr) {
            setCurrentFrame();
            //render first frame
            mClock.display();
            next_log_tag("render", "first frame %ld, %d", mCurrentFrame->pts, __LINE__);
            return;
        }

        while (true) {
            auto next = mFrameQueue.first();
            auto duration = next->pts - mCurrentFrame->pts;

            auto last = mClock.lastDisplayMicro();
            auto now = mClock.nowMicro();

            auto elapse = now - last;
            //
            if (elapse < duration) {
                if (mFrameQueue.isFull()) {
                    std::this_thread::sleep_for(std::chrono::microseconds(duration - elapse));
                    continue;
                } else {
                    break;
                }
            }

            releaseAndSetCurrentFrame();

//            next_log_tag("render", "render frame %ld, %d", mCurrentFrame->pts, __LINE__);
            mClock.display();
            if (mFrameQueue.isFull()) {
                duration = mFrameQueue.first()->pts - mCurrentFrame->pts;
                std::this_thread::sleep_for(std::chrono::microseconds(duration));
                continue;
            } else {
                break;
            }
        }
         */
    }

    void AudioRender::stop() {
        mAudioOutput->stop();
        mStopped.store(true);
        mThread->join();
    }

    bool AudioRender::isStopped() {
        return mStopped.load();
    }

    void AudioRender::pause() {
        mPaused.store(true);
    }

    void AudioRender::start() {
        mPaused.store(false);
    }

    bool AudioRender::isPaused() {
        return mPaused.load();
    }
}
