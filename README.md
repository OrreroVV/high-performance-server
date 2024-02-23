# 开发环境

```
ubuntu 18.04
gcc 9.1.0
cmake

Boost
wget https://boostorg.jfrog.io/artifactory/main/release/1.84.0/source/boost_1_84_0.tar.gz
tar -xzvf boost_1_84_0.tar.gz
构建：sudo ./bootstrap.sh 
安装：sudo ./b2 install

yaml-cpp 安装
wget https://codeload.github.com/jbeder/yaml-cpp/tar.gz/refs/tags/0.8.0
mkdir build && cd build && cmake .. && make install
```

# 项目路径

```shell
bin --二进制输出文件	
build --中间文件路径
cmake 	--cmake定义脚本文件夹
CmakeLists.txt	 --cmake
lib	--库的输出路径
# Makefile	--
sylar	--源代码路径
tests	--测试路径
logs 	--日志存放路径
```





# 日志系统

## Log4J

```
Logger（定义日志类别）

​	|

​		Formatter（日志格式）

​	|

Appender（日志输出）
```


# 配置系统

```shell
config -> Yaml

YAML使用
node.IsMap()
YAML::Node node = YAML::LoadFile(filename)
node.IsSequence()
vector
```





# 配置系统原则

```c++
template<T, FromStr, ToStr>

vector<T> -> T

//容器偏特化
//vector list map unordered_map unordered_set class

//配置类型转换
LexicalCast<F, T>
//实现偏特化->解析自定义类型

定义LogDefine LogAppenderDefine ->偏特化	LexicalCast
实现日志解析
```

# 配置事件机制

当一个配置项发生改变时，可以反向通知对应的代码，设置回调函数
对每一个$logger$添加回调函数

# 日志系统整合配置

$log.yml$大纲

```yaml
logs:
  - name: root
    level: (debug, info, warn, error, fatal)
    formatter: '%d%T%m%n'
    appenders:
      - type: (FileLogAppender, StdoutLogAppender)
      #  file: /apps/logs/sylar/root.txt
```



```cpp
sylar::Logger g_logger = sylar::LoggerMgr::GetInstance()->getLogger(name);
#define: ->   SYLAR_LOG_NAME(name)
获取logger


```



系统日志

```cpp
static Logger::ptr system_log = SYLAR_LOG_NAME("system");//名为system的logger，方便调整输出格式等

```



# 线程

```cpp
//局部锁模板
ScopedLockImpl
ReadScopedLockImpl
WriteScopedLockImpl

//互斥量
typedef ScopedLockImpl<Mutex> Lock;
//空锁
typedef ScopedLockImpl<NullMutex> Lock;
//读写锁	
typedef ReadScopedLockImpl<RWMutex> ReadLock;
typedef WriteScopedLockImpl<RWMutex> WriteLock;
//自旋锁
typedef ScopedLockImpl<Spinlock> Lock;
//原子锁
typedef ScopedLockImpl<CASLock> Lock;
```


# 协程库封装


## 定义协程接口
```
ucontext_t
macro
```

```shell
Fiber::GetThis() # 得到当前协程

Thread -> main_fiber <--> sub_fiber
```

##协程调度模块

```
        1  -   N   1 -  M
scheduler ->  thread -> fiber
        N       -       M
1. 线程池  分配一组线程
2. 协程调度器 将协程制定到相应的线程上去执行
```
作用：开辟一组线程池，对想要执行的协程找到一个对应的线程，放到该线程队列中去执行


协程调度器
```
m_fibers  --> (function<void()> || fiber) 需要thread_id
```

start()

stop()

//调度核心，协程调度器如何去协助工作

run()
1. 设置当前线程的协程调度(scheduler)
2. 设置当前的线程的run， fiber
3. 协程调度
    1. 协程消息队列里面是否有任务
    2. 无任务时执行idle



```
IOManager(epoll) --> Scheduler
        |
        |
        |
        |
        v
        idle(epoll_wait)

PutMessage(msg.) 信号量+1
message_queue
        |
        |---Thread
        |---Thread
                wait()信号量-1， RecvMessage
异步IO，等待数据返回 -->epoll_wait
```

定时器
```
针对IOmanager创建定时器
Timer -> addTimer() --> cancel()
获取当前定时器触发离现在的时间差
返回当前要触发的定时器，执行回调函数
```


```
[Fiber] --> [Thread] --> [Scheduler]
                             |
                             |
                      [IOmanager(epoll)]
                             |
                             |
                         [timeManager]
                             |
                             |
                          [Timer]

```

## HOOK
```
sleep

usleep

socket相关(socket, connect, accept...)
io相关(read, write, send, recv...)
fd相关(fcntl, ioctl...)
```

# socker函数库
                               | -|IPv4Address|
|Address| --- |IPAddress|   ---|
                               | -|IPv6Address|

    |
|Socket|


```c++

/*
family:协议族
AF_INT(IPv4)
AF_INT6(IPv6)
AF_UNIX(unix)

type : 在网络编程中常见的两种套接字类型，分别用于基于流的传输和数据报传输。
SOCK_STREAM 用于创建基于流的套接字，通常对应于 TCP 协议。
提供面向连接的、可靠的、双向的字节流通信。
保证数据按发送顺序到达目的地，不存在数据丢失或乱序问题。
数据传输过程中会进行错误检测和重传，保证数据的可靠性。

SOCK_DGRAM 用于创建基于数据报的套接字，通常对应于 UDP 协议。
提供无连接的、不可靠的数据报服务。
每个数据报是独立的、短小的消息单元，不存在数据的拆分和合并。
不保证数据的可靠传输，可能存在数据丢失或乱序现象。


protocol协议：两个协议标识符，用于指定套接字所使用的传输层协议。
IPPROTO_TCP
TCP协议

IPPROTO_UDP
UDP协议

*/

/*
sa_family 字段指定了地址家族（IPv4、IPv6 等），而 sa_data 则存储具体的地址信息。
*/
struct sockaddr {
    unsigned short sa_family;    // 地址家族（Address Family），AF_INET 或 AF_INET6 等
    char sa_data[14];            // 地址数据，长度为 14 字节
};

/*
sin_family 指定了地址家族（通常为 AF_INET）。
sin_port 存储了端口号。
sin_addr 包含了 IPv4 地址信息。
sin_zero 用于填充以保证结构体大小对齐。
*/

struct sockaddr_in {
    short sin_family;           // 地址家族，AF_INET
    unsigned short sin_port;    // 端口号
    struct in_addr sin_addr;    // IPv4 地址
    char sin_zero[8];           // 预留字段，通常用 0 填充
};
struct in_addr {
    unsigned long s_addr;       // IPv4 地址
};
/*
sizeof(sockaddr) == sizeof(sockaddr_in)
可以互相指针转换
*/
```



# http协议开发

# 分布式协议