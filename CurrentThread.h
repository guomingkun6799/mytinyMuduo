//
// Created by MingkunGuo on 2026/10/16.
//
#pragma once

namespace CurrentThread {
    extern __thread int t_cachedTid;

    void cachedTid();

    inline int tid() {
        if (__builtin_expect((t_cachedTid != 0), 0)) {
            cachedTid();
        }
        return t_cachedTid;
    }
};

