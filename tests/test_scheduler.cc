/*
 * @Author: Orrero
 * @Date: 2024-12-17
 * @Description: 
 * 
 * Copyright (c) 2024 by Orrero, All Rights Reserved. 
 */
#include "sylar/sylar.h"
#include <iostream>
#include <thread>
#include <functional>
#include <atomic>
#include <chrono>
static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();




class Timer {
public:
    Timer() : running(false) {}

    // 启动定时器（单次执行）
    void startOnce(int delay_ms, std::function<void()> task) {
        stop();  // 停止已有的定时器
        running = true;

        timer_thread = std::thread([this, delay_ms, task]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            if (running) {
                task();
            }
        });
    }

    // 启动定时器（周期性执行）
    void startPeriodic(int interval_ms, std::function<void()> task) {
        stop();  // 停止已有的定时器
        running = true;

        timer_thread = std::thread([this, interval_ms, task]() {
            while (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                if (running) {
                    task();
                }
            }
        });
    }

    // 停止定时器
    void stop() {
        running = false;
        if (timer_thread.joinable()) {
            timer_thread.join();
        }
    }

    ~Timer() {
        stop();
    }

private:
    std::thread timer_thread; // 定时器线程
    std::atomic<bool> running; // 标志定时器是否在运行
};



void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
    Timer timer;
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    SYLAR_LOG_INFO(g_logger) << &*fiber;
    timer.startOnce(2000, [fiber]() {
        SYLAR_LOG_INFO(g_logger) << "Single-shot timer executed after 2 seconds!";
        SYLAR_LOG_INFO(g_logger) << &*fiber;
        sylar::Scheduler::GetThis()->schedule(fiber);
    });
    sylar::Fiber::YieldToHold();
    
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "main";
    sylar::Scheduler sc(3, false, "test");
    sc.start();
    sylar::Fiber::ptr fiber(new sylar::Fiber(&run_in_fiber));
    sc.schedule(fiber);
    while (true) {
        sleep(10);
    }
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}
