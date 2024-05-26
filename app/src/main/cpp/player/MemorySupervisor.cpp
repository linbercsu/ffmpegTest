//
// Created by linlin zhao on 2024/5/26.
//

#include "MemorySupervisor.h"
#include "Log.h"

namespace next {
    std::size_t bh_hash::operator()(const void * s) const noexcept {
        int64_t p = (int64_t)s;
        std::hash<int64_t> h;
        return h(p);
    }

    bool bh_equal_to::operator()(const void *x, const void *y) const {
        int64_t p = (int64_t)x;
        int64_t p2 = (int64_t)y;
        return p == p2;
    }
    
    MemorySupervisor::MemorySupervisor() {

    }

    void MemorySupervisor::on_posix_memalign(void *p, size_t size) {
        if (p != nullptr) {
            std::lock_guard<std::mutex> l(lock);
            map.insert({p, new MemoryItem(p, size)});
        }
    }

    void MemorySupervisor::on_free(void *p) {
        if (p == nullptr)
            return;

        std::lock_guard<std::mutex> l(lock);
        const auto &iterator = map.find(p);
        if (iterator != map.end()) {
//            next_log_tag("mem", "free %zu", iterator->second->size);
            map.erase(iterator);
        }
    }

    void MemorySupervisor::on_realloc_(void *p, size_t size) {
        if (p != nullptr) {
//            std::lock_guard<std::mutex> l(lock);
            on_free(p);
        }
    }

    void MemorySupervisor::on_post_realloc_(void *p, size_t size) {
        if (p != nullptr) {
            std::lock_guard<std::mutex> l(lock);
            map.insert({p, new MemoryItem(p, size)});
        }
    }

    void MemorySupervisor::calculate() {
        std::lock_guard<std::mutex> l(lock);

        size_t size = 0;
        for (auto ite : map) {
            size += ite.second->size;
            next_log_tag("mem", "calculate1 %p, %zu", ite.second->p, ite.second->size);
        }

        next_log_tag("mem", "calculate %zu, %d", size, map.size());
    }

}
