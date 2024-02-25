# 线程相关
```c++
POSIX 线程库

互斥锁（Mutex Lock）：
互斥锁是一种最基本的同步原语，用于保护共享资源不被并发访问。它确保在任意时刻只有一个线程可以获得锁，在获取到锁之前，其他线程都会被阻塞。
互斥锁是一种二进制锁，也称为互斥量，因为它只允许一个线程进入临界区。
原子锁（Atomic Lock）：
原子锁是一种更高级别的同步机制，通常是在硬件层面实现的，用于确保对共享资源的操作是原子的，即不可分割的。
原子锁可以保证某段代码执行期间不会被中断，从而避免了竞态条件。
读写锁（Read-Write Lock）：
读写锁允许多个线程同时读取共享数据，但在有写操作时，所有其他读操作和写操作都会被阻塞。
读写锁可以提高读操作的并发性能，适用于读操作频繁、写操作较少的情况。


int sem_init(sem_t *sem, int pshared, unsigned int value);
// sem_init 用于初始化一个未命名的信号量。
// 参数 pshared 指定信号量的类型，如果为 0，则线程间共享；非零值表示在进程间共享。
// value 是信号量的初始值。

int sem_destroy(sem_t *sem);
// sem_destroy 用于销毁一个信号量。
// 在使用完信号量后，应该调用该函数来释放资源。

int sem_wait(sem_t *sem);
// sem_wait 函数用于等待（阻塞）信号量，并在信号量可用时将信号量减一。
// 如果信号量的值大于零（表示可用资源），则将信号量减一并立即返回；否则将阻塞线程直到信号量变为可用为止。

int sem_post(sem_t *sem);
// sem_post 函数用于释放信号量。
// 调用 sem_post 会增加信号量的值，表示一个资源已经被释放，其他等待该信号量的线程可以继续执行。


int sem_getvalue(sem_t *sem, int *sval);
// sem_getvalue 用于获取信号量的当前值，并将其存储到 sval 中。
// 这个函数可以帮助你查看当前信号量的状态。

int sem_trywait(sem_t *sem);
// sem_trywait 尝试对信号量进行减操作，如果信号量的值大于零，则减一并立即返回；否则会立即返回错误，而不会阻塞线程。


int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
/*
sem_timedwait 类似于 sem_wait，但可以设置一个超时时间，在超时前等待信号量可用。
abs_timeout 是一个绝对时间，指定等待的最长时间。
*/


static thread_local Thread *t_thread = nullptr;
// thread_local： 这个关键字用于指定变量为线程局部存储（TLS），每个线程都会拥有自己独立的变量副本。这意味着每个线程可以拥有不同的 t_thread 变量，并且每个线程对该变量的修改不会影响其他线程的变量值。


int rt = pthread_create(&m_thread, nullptr, &Thread::run, this); 
/*
pthread_create 函数： 这个函数用于创建一个新的线程。
&m_thread： &m_thread 是一个指向 pthread_t 类型变量 m_thread 的指针，pthread_create 函数将会把新线程的标识符存储到 m_thread 中。
nullptr： 在这里表示不设置线程属性，使用默认属性。
&Thread::run： &Thread::run 是指向线程执行函数的指针，表示新创建的线程将执行 Thread 类的成员函数 run。
this： this 关键字代表当前对象的指针，用于传递给线程执行函数作为参数，以便在线程中访问对象的成员变量和方法。
int rt： rt 是一个整数变量，用于存储 pthread_create 函数的返回值，通常用来检查线程创建是否成功。
*/


int pthread_join(pthread_t thread, void **status);
/*
thread: 要等待的线程的标识符。
status: 用于存储被等待线程的返回值。
返回值：
如果成功，返回值为 0；如果出现错误，则返回相应的错误码。
功能：
主线程调用 pthread_join 来等待指定线程结束执行。在这种情况下，主线程将阻塞，直到指定线程结束。
status 参数允许指定线程向主线程返回一个指向返回值的指针，以便主线程获取线程的退出状态。
注意事项：
使用 pthread_join 可以避免创建的线程成为僵尸线程（zombie thread），它们已经退出但其资源仍未被释放。
每个线程只能被另外一个线程等待（即只能调用一次 pthread_join）
*/


int pthread_setname_np(pthread_t thread, const char *name);
/*
thread: 要设置名称的线程的标识符。
name: 要为线程设置的名称，以字符串形式表示。

如果成功，返回值为 0；如果出现错误，则返回相应的错误码。
pthread_setname_np 允许为指定线程设置一个人类可读的名称，以方便调试和诊断。
设置线程名称可以有助于在调试器中更轻松地区分和识别不同的线程。
这个函数不是 POSIX 标准的一部分，并且不是所有平台都支持该函数。在使用之前，请确保你的平台支持此函数。
线程名称的长度可能会受到限制，具体取决于实现。
*/

```

# socket相关
```c++

struct timeval
{
  __time_t tv_sec;		/* Seconds. 秒 */
  __suseconds_t tv_usec;	/* Microseconds.  微秒 */
};


/*

sockfd：套接字描述符，标识要操作的套接字。
level：指定选项所属的协议级别，如 SOL_SOCKET、IPPROTO_TCP 等。
optname：具体选项的名称，用于指定要查询的选项。
optval：用来存储获取到的选项值的缓冲区。
optlen：传入一个整数变量的指针，表示 optval 缓冲区的大小，并且会返回实际获取的选项值的长度。
*/
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);



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