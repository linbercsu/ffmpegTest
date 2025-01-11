//
// Created by Zhao, Linlin on 17/12/24.
//

#include <unistd.h>
#include "MessageThread.h"
#include <chrono>

namespace next {
    int64_t currentTimestampMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }


    void Message::execute() {
        mMessageCallback->handleMessage(*this);
    }

    Message MessageQueue::next() {
        std::unique_lock<std::mutex> lk(mLock);
        while (true) {
            while (mMessages.empty())
                mCondition.wait(lk);

            int64_t nextWakeup = 0x7fffffffffffffffL;
            int64_t current = 0;
            for (auto begin = mMessages.begin(); begin != mMessages.end(); begin++) {
                auto executeTime = begin->executeTime();
                if (executeTime == 0) {
                    auto message = *begin;
                    mMessages.erase(begin);
                    return message;
                } else {
                    if (current == 0) {
                        current = currentTimestampMs();
                    }

                    if (current >= executeTime) {
                        auto message = *begin;
                        mMessages.erase(begin);
                        return message;
                    }

                    if (nextWakeup > executeTime) {
                        nextWakeup = executeTime;
                    }

                }
            }

            //todo assert nextWakeup != 0x7fffffffffffffffL;

            auto wait = nextWakeup - current;
            mCondition.wait_for(lk, std::chrono::milliseconds(wait));

        }
//        Message &message = mMessages.front();
//        mMessages.pop_front();
//
//        return message;
    }

    void MessageQueue::pushBack(Message &&message) {
        std::unique_lock<std::mutex> lk(mLock);
        auto insert = false;
        for (auto begin = mMessages.rbegin(); begin != mMessages.rend(); begin++) {

            auto nextPriority = begin->getPriority();
            auto priority = message.getPriority();
            if (priority <= nextPriority) {
                mMessages.insert(begin.base(), message);
                insert = true;
                break;
            }
        }

        if (!insert) {
            mMessages.push_front(message);
        }
//        mMessages.push_back(message);
    }

    void MessageQueue::pushBack(Message &message) {
        std::unique_lock<std::mutex> lk(mLock);

        auto insert = false;
        for (auto begin = mMessages.rbegin(); begin != mMessages.rend(); begin++) {

            auto nextPriority = begin->getPriority();
            auto priority = message.getPriority();
            if (priority <= nextPriority) {
                mMessages.insert(begin.base(), message);
                insert = true;
                break;
            }
        }
        
        if (!insert) {
            mMessages.push_front(message);
        }

//        mMessages.push_back(message);
    }

    void MessageQueue::removeMessageById(int id) {
        std::unique_lock<std::mutex> lk(mLock);
        for (auto begin = mMessages.begin(); begin != mMessages.end();) {
            if (begin->getId() == id) {
                begin = mMessages.erase(begin);
            } else {
                begin++;
            }
        }
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
