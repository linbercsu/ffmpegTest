//
// Created by ZhaoLinlin on 2021/7/1.
//

#include "Thread.h"
#include <list>
#include <mutex>
#include <unordered_map>

namespace concurrent {
    namespace {
        std::unordered_map<std::__thread_id, thread *> threadMap;
        std::mutex threadLock;
    }

    thread *concurrent::thread::current() {
        std::lock_guard<std::mutex> lock(threadLock);
        const std::__thread_id &id = std::this_thread::get_id();
        const std::unordered_map<std::__thread_id, thread *>::iterator &iterator = threadMap.find(
                id);
        return iterator->second;
    }

    thread::thread() : thread(nullptr) {

    }

    thread::thread(threadCallback *callback) : callback(callback) {
        innerThread = new std::thread(&thread::onExecuted, this);
    }

    thread::~thread() {
        delete innerThread;
    }

    void thread::run() {
        if (callback != nullptr) {
            callback->onThreadRun(this);
        }
    }

    void thread::onExecuted() {
        {
            std::lock_guard<std::mutex> lock(threadLock);
            const std::thread::id &id = innerThread->get_id();
            std::unordered_map<std::__thread_id, thread *>::value_type p(id, this);
            threadMap.insert(p);
        }

        run();

        std::lock_guard<std::mutex> lock(threadLock);

        const std::thread::id &id = innerThread->get_id();
        threadMap.erase(id);
    }

    void thread::interrupt() {
        interrupted.store(true, std::memory_order_seq_cst);
        condition_variable *v = currentWaiting.load(std::memory_order_seq_cst);
        if (v != nullptr) {
            v->notify_all();
        }
    }

    bool thread::isInterrupted() {
        return interrupted.load(std::memory_order_seq_cst);
//        bool expect = true;
//        return std::atomic_compare_exchange_strong(&interrupted, &expect, false);
    }

    void thread::join() {
        innerThread->join();
    }
}
