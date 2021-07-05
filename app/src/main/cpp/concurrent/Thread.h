//
// Created by ZhaoLinlin on 2021/7/1.
//

#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace mx {
    namespace concurrent {

        class thread;

        class condition_variable;

        class threadCallback {
        public:
            virtual void onThreadRun(thread *thread) = 0;
        };

        class thread {

        public:
            thread(threadCallback *callback);

            thread();

            virtual ~thread();

            bool isInterrupted();

            void interrupt();

            void run();

            void join();

        public:
            static thread *current();

        private:
            void onExecuted();


            std::atomic_bool interrupted{false};
            std::thread *innerThread{nullptr};
            threadCallback *callback{nullptr};

            friend class condition_variable;

            std::atomic<condition_variable *> currentWaiting{nullptr};
        };

        class condition_variable : public std::condition_variable {

            template<class _Predicate>
            _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
            bool waitInterruptibly(std::unique_lock<std::mutex> &__lk, _Predicate __pred);
        };

        template<class _Predicate>
        bool
        condition_variable::waitInterruptibly(std::unique_lock<std::mutex> &__lk,
                                              _Predicate __pred) {
            thread *currentThread = thread::current();
            currentThread->currentWaiting = this;
            while (true) {
                if (currentThread->isInterrupted()) {
                    currentThread->currentWaiting = nullptr;
                    return true;
                }

                if (!__pred()) {
                    wait(__lk);
                } else {
                    break;
                }
            }

            currentThread->currentWaiting = nullptr;

            if (currentThread->isInterrupted()) {
                return true;
            }

            return false;
        }
    }
}
