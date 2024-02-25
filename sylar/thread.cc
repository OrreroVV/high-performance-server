#include "thread.h"
#include "log.h"

namespace sylar{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count){
    if (sem_init(&m_semaphore, 0, count)){
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify(){
    if (sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}

//thread

/*
 静态变量&函数
 每个线程都会拥有自己独立的变量副本
 保证每个线程的静态指针指向不同
*/
static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

Thread* Thread::GetThis(){
    return t_thread;
}
const std::string& Thread::GetName(){
    return t_thread_name;
}

void Thread::SetName(const std::string &name){
    if (t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb), m_name(name){
    if (name.empty()){
        m_name = "UNKNOW";
    }

    //线程创建，并且绑定run方法，并且执行run方法，其中run的参数是this，并且以默认的创建线程方式创建，把返回的线程id赋值给m_thread上
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    //SYLAR_LOG_DEBUG(g_logger) << "thread created name: " << m_name << "threadId: " << m_thread;
    if (rt){
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
                                  << " name=" << name;
        throw std::logic_error("pthread_create error");
    }

    m_semaphore.wait();
}

Thread::~Thread(){
    if (m_thread){
        pthread_detach(m_thread);
    }
}

void Thread::join(){
    if (m_thread){
        int rt = pthread_join(m_thread, nullptr);
        if (rt){
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                                      << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void *Thread::run(void *arg){

    Thread *thread = (Thread *)arg;

    //SYLAR_LOG_DEBUG(g_logger) << "threadId: " << thread->m_thread;

    //函数指针赋值
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();

    /*
    设置可读的名字
    */
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();
    cb();
    
    return 0;
}

pid_t Thread::getId() const { return m_id; }
const std::string& Thread::getName() const { return m_name; }

}