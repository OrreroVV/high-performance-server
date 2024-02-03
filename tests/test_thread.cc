#include "sylar/sylar.h"
#include <unistd.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count;

sylar::Mutex m_mutex;

void fun1(){
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << " this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id: " << sylar::Thread::GetThis()->getId();

    for (int i = 0; i < 1000000; i++){
        sylar::Mutex::Lock lock(m_mutex);
        ++count;
    }
}
void fun2() {
    int cnt = 1000;
    while(cnt--) {
        SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}
void fun3() {
    int cnt = 1000;
    while(cnt--) {
        SYLAR_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv){
    YAML::Node root = YAML::LoadFile("/home/hzh/workspace/high-performance-server/bin/conf/log_thread.yml");
    sylar::Config::LoadFromYaml(root);


    std::vector<sylar::Thread::ptr> thrs;

    for (int i = 0; i < 2; i++){
        sylar::Thread::ptr thr1(new sylar::Thread(&fun2, "name_ " + std::to_string(i * 2)));
        sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_ " + std::to_string(i * 2 + 1)));

        thrs.push_back(thr1);
        thrs.push_back(thr2);
    }

    // for (int i = 0; i < 5; i++){
    //     sylar::Thread::ptr thr(new sylar::Thread(&fun1, "name_ " + std::to_string(i)));
    //     thrs.push_back(thr);
    // }
    for (int i = 0; i < 4; i++){
        thrs[i]->join();
    }

    SYLAR_LOG_INFO(g_logger) << "thread test end";
    //SYLAR_LOG_INFO(g_logger) << "count= " << count;
    return 0;
}