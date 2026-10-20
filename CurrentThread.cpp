//
// Created by MingkunGuo on 2026/10/16.
//

#include "CurrentThread.h"
#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread {
    __thread int t_cachedTid;

    void cachedTid() {
        if (t_cachedTid == 0) {
            //通过linux系统调用，获取当前线程tid的数值
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}

