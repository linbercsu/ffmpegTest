//
// Created by Zhao, Linlin on 17/12/24.
//

#pragma once

#include <atomic>
#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>
#include <cstdint>

namespace next {

    int64_t currentTimestampMs();

    class MessageCallback;

    class Message {

    private:
        constexpr static int MESSAGE_ID_EMPTY = 0;
    public:
        constexpr static int MESSAGE_ID_USER = 0xff;

        constexpr static int MESSAGE_PRIORITY_NORMAL = 0;
        constexpr static int MESSAGE_PRIORITY_MAX = 0xffff;

    public:

        explicit Message(int id):mId(id), mPriority(MESSAGE_PRIORITY_NORMAL) {

        }
        explicit Message(int id, int priority):mId(id), mPriority(priority) {

        }

        Message& withCallback(MessageCallback* callback) {
            mMessageCallback = callback;
            return *this;
        }

        Message& delay(int64_t ms) {
            auto time = currentTimestampMs() + ms;
            this->mExecuteTime = time;
            return *this;
        }
        Message& priority(int p) {
            this->mPriority = p;
            return *this;
        }

        static Message emptyMessage() {
            return Message(MESSAGE_ID_EMPTY, MESSAGE_PRIORITY_MAX + 1);
        }

        static Message simpleMessage(int id) {
            return Message(id, MESSAGE_PRIORITY_NORMAL);
        }



        bool isEmpty() const {
            return mId == MESSAGE_ID_EMPTY;
        }

        void execute();

        int getId() const {
            return mId;
        }

        int getPriority() const {
            return mPriority;
        }

        Message& withData1(int data1) {
            this->mData1 = data1;
            return *this;
        }

        Message& withData2(int64_t data2) {
            this->mData2 = data2;
            return *this;
        }

        Message& withObject1(void* object1) {
            this->mObject1 = object1;
            return *this;
        }

        Message& withObject2(void* object2) {
            this->mObject2 = object2;
            return *this;
        }

        int getData1() const {
            return mData1;
        }
        int64_t getData2() const {
            return mData2;
        }

        void* getObject1() const {
            return mObject1;
        }

        void* getObject2() const {
            return mObject2;
        }

        int64_t executeTime() const {
            return mExecuteTime;
        }

    private:
        int mId;
        int mPriority;
        MessageCallback* mMessageCallback {nullptr};
        int64_t mExecuteTime{0};

        int mData1{0};
        int64_t mData2{0};
        void* mObject1{nullptr};
        void* mObject2{nullptr};
    };



    class MessageQueue {
    public:
        Message next();
        //message queue will take the ownership of message.
        void pushBack(Message&& message);
        //message queue will take the ownership of message.
        void pushBack(Message& message);

        void removeMessageById(int id);
    private:
        std::mutex mLock;
        std::condition_variable mCondition;
        std::list<Message> mMessages;
    };

    class MessageCallback {
    public:
        virtual void handleMessage(const Message& message) = 0;
    };

    class MessageThreadCallback {
    public:
        virtual void onThreadEnded() = 0;
    };

    class MessageThread {
    public:
        explicit MessageThread(MessageThreadCallback* messageThreadCallback);
        ~MessageThread();

        MessageQueue& messageQueue() {
            return mMessageQueue;
        }

        void stop();
        void join();

        void run();
    private:
        std::atomic_bool mStopped{false};
        std::thread* mThread;
        MessageQueue mMessageQueue;
        MessageThreadCallback* mMessageThreadCallback;

    };
}
