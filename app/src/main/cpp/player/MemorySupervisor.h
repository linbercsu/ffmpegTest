//
// Created by linlin zhao on 2024/5/26.
//

#pragma once

#include <stddef.h>
#include <unordered_map>
#include <mutex>

namespace next {
    struct bh_hash
    {
        std::size_t
        operator()(const void* __s) const noexcept;
    };

    struct bh_equal_to
    {
        bool operator()(const void* __x, const void* __y) const;
    };

    struct MemoryItem {
        MemoryItem(void* pp, size_t s): p(pp), size(s) {

        }
        void* p;
        size_t size;
    };

    class MemorySupervisor {
    public:
        MemorySupervisor();

        void on_posix_memalign(void *p, size_t size);
        void on_free(void *p);
        void on_realloc_(void *p, size_t size);

        void calculate();

        void on_post_realloc_(void *pVoid, size_t i);

    private:
        std::unordered_map<void*, MemoryItem*, bh_hash, bh_equal_to> map;
        std::mutex lock;
    };
}


