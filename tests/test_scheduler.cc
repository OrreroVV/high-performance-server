#include "sylar/sylar.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


void test_fiber() {
    //该函数执行五次
    static int s_count = 5;
    SYLAR_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        //调度线程执行
        sylar::Scheduler::GetThis()->schedule(&test_fiber);
        //固定线程执行
        //sylar::Scheduler::GetThis()->schedule(&test_fiber, sylar::GetThreadId());
    }
}

/**
 * 创建大小为3的线程池
 * 执行协程调用
 * 把需要做的任务通过协程调度进行工作
 * 
*/

int main(int argc, char** argv) {

    // YAML::Node root = YAML::LoadFile("/home/hzh/workspace/high-performance-server/bin/conf/log_scheduler.yml");
    // sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(g_logger) << "main";
    sylar::Scheduler sc(3, false, "test");
    sc.start();

    sleep(2);
    SYLAR_LOG_INFO(g_logger) << "schedule";
    //协程调度执行test_fiber，创建一个新的协程，去执行该函数
    sc.schedule(&test_fiber);
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}