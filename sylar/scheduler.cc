#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/*
 每个线程都会拥有自己独立的变量副本
 保证每个线程的静态指针指向不同
*/
//调度器
static thread_local Scheduler* t_scheduler = nullptr;

//调度协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) {
    SYLAR_ASSERT(threads > 0);

    //如果作为主线程
    if(use_caller) {
        //没有主线程的话，创建主线程
        sylar::Fiber::GetThis();

        --threads;

        //只能有一个作为协程调度器，由主线程作为
        SYLAR_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        sylar::Thread::SetName(m_name);

        //标记调度协程
        t_scheduler_fiber = m_rootFiber.get();
        
        m_rootThread = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {

    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    //线程池此时一定是空的
    SYLAR_ASSERT(m_threads.empty());

    //初始化线程池的数量
    m_threads.resize(m_threadCount);

    //SYLAR_LOG_DEBUG(g_logger) << "scheduler:::::: " << this;
    //添加m_threadCount的线程到线程池中
    for(size_t i = 0; i < m_threadCount; ++i) {
        //并且对每个线程绑定调度协程的run方法, 创建一个绑定到当前对象 (this) 的函数对象
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        //把线程池中的线程id放入到id数组中
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    //if(m_rootFiber) {
    //    //m_rootFiber->swapIn();
    //    m_rootFiber->call();
    //    SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    //}
}

void Scheduler::stop() {
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    //bool exit_on_this_fiber = false;
    if(m_rootThread != -1) {
        SYLAR_ASSERT(GetThis() == this);
    } else {
        SYLAR_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }

    if(m_rootFiber) {
        //while(!stopping()) {
        //    if(m_rootFiber->getState() == Fiber::TERM
        //            || m_rootFiber->getState() == Fiber::EXCEPT) {
        //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //        SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
        //        t_fiber = m_rootFiber.get();
        //    }
        //    m_rootFiber->call();
        //}
        if(!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
    //if(exit_on_this_fiber) {
    //}
}

void Scheduler::setThis() {
    t_scheduler = this;
}

//新创建的线程在run里面启动
void Scheduler::run() {
    /*
    使用hook的函数
    */
    set_hook_enable(true);
    setThis();

    //SYLAR_LOG_DEBUG(g_logger) << "t_scheduler value: " << t_scheduler;
    //SYLAR_LOG_DEBUG(g_logger) << "threadID: " << sylar::GetThreadId() << " m_rootthread: " << m_rootThread;
    //如果当前线程不是主线程的话
    if(sylar::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    //SYLAR_LOG_DEBUG(g_logger) << "t_scheduler_fiber: " << t_scheduler_fiber;
    //SYLAR_LOG_DEBUG(g_logger) << m_name << " run: m_name: " << (t_scheduler->m_name) << " rootid: " << (t_scheduler-> m_rootThread);
    //如果无事可做的话，执行idle
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

    //回调函数
    Fiber::ptr cb_fiber;


    FiberAndThread ft;
    //从任务队列中取出一个待执行的消息
    while(true) {
        //每次先重置一下
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;

        {
            //从协程消息队列取出一个任务去执行
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            /**
             * 主协程不需要去取出
            */
            while(it != m_fibers.end()) {

                //如果他有制定要去的线程的话，并且该线程id并不是当前线程，则重新寻找
                if(it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }


                //当前任务要么是fiber，要么是cb
                SYLAR_ASSERT(it->fiber || it->cb);
                //如果找到了当前的任务，并且任务的状态是执行中的话，该任务无需在调度器中调度
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                //此时说明协程调度器找到了需要调度的协程
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            //如果没找到需要执行的任务时
            tickle_me |= it != m_fibers.end();
        }


       
        if(tickle_me) {
            tickle();
        }

        

        /**
         * 要么是协程去调度，要么是任务去执行
        */
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) {//如果是协程的话，判断该协程的状态
            
            //唤醒
            ft.fiber->swapIn();
            --m_activeThreadCount;


            if(ft.fiber->getState() == Fiber::READY) {
                //如果是准备状态的话，重新加入到schedule中
                schedule(ft.fiber);
            } 
            else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                //否则被切换出来是阻塞状态
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();

        }
        else if(ft.cb) {//如果是回调函数的话，与上面同理
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);//重置fiber状态
            } 
            else {
                cb_fiber.reset(new Fiber(ft.cb));//刚创建时的cb_fiber为空，需要new一个表示指向
            }
            ft.reset();//重置一下ft

            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {//if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } 
        else {//如果该线程空闲的话
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            //空闲线程++
            ++m_idleThreadCount;
            //空闲idle放入执行，占用cpu，等待分配
            idle_fiber->swapIn();
            //空闲线程--
            --m_idleThreadCount;


            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        sylar::Fiber::YieldToHold();
    }
}

void Scheduler::switchTo(int thread) {
    SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
    if(Scheduler::GetThis() == this) {
        if(thread == -1 || thread == sylar::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target) {
    m_caller = Scheduler::GetThis();
    if(target) {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher() {
    if(m_caller) {
        m_caller->switchTo();
    }
}

}