//
// Created by Zhao, Linlin on 17/12/24.
//

#include <unistd.h>
#include "MessageThread.h"

namespace next {

    void Message::execute() {
        mMessageCallback->handleMessage(*this);
    }

    Message MessageQueue::next() {
        std::unique_lock<std::mutex> lk(mLock);
        while (mMessages.empty())
            mCondition.wait(lk);

        Message &message = mMessages.front();
        mMessages.pop_front();

        return message;
    }

    void MessageQueue::pushBack(Message &&message) {
        std::unique_lock<std::mutex> lk(mLock);
        mMessages.push_back(message);
    }

    void MessageQueue::pushBack(Message &message) {
        std::unique_lock<std::mutex> lk(mLock);
        mMessages.push_back(message);
    }


    /////////////////////////////////////////

    void MessageThread::run() {
        while (!mStopped.load(std::memory_order_acquire)) {
            Message message = mMessageQueue.next();

            if (message.isEmpty()) {
                continue;
            }

            message.execute();

        }

        if (mMessageThreadCallback != nullptr) {
            mMessageThreadCallback->onThreadEnded();
        }
    }

    void MessageThread::stop() {
        mStopped.store(true, std::memory_order_release);
        mMessageQueue.pushBack(Message::emptyMessage());
    }

    void MessageThread::join() {
        mThread->join();
    }

    MessageThread::MessageThread(MessageThreadCallback* messageThreadCallback):mMessageThreadCallback(messageThreadCallback) {
        mThread = new std::thread(&MessageThread::run, this);
    }

    MessageThread::~MessageThread() {
        delete mThread;
    }


}
