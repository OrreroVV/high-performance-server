#include "address.h"
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "endian.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/*
是后缀，即子网掩码的取反
形如000011111 此时bits = 5
*/
template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

/*
统计value中后缀连续1的个数
即求反子网掩码1的个数
*/
template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::LookupAny(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        //for(auto& i : result) {
        //    std::cout << i->toString() << std::endl;
        //}
        for(auto& i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v) {
                return v;
            }
        }
    }
    return nullptr;
}

bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char* service = NULL;
    /*
    解析主机名/域名中可能包含的 IPv6 地址或端口信息。
    调用 getaddrinfo 函数获取地址信息。
    遍历查找到的地址信息列表，调用 Create 函数创建 Address 对象并将其添加到 result 向量中。
    释放 addrinfo 结果列表的内存空间，并返回是否成功找到地址信息。
    */


    /*
    检查 ipv6address serivce   [2001:db8::1]:8080
    如果是类似ipv6格式的地址的话
    */
    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            //TODO check out of range
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //检查 node serivce
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        /*
        如果有：，说明可能是ipv4地址
        */
        if(service) {
            /*
            如果没有找到第二个：，说明是一个ipv4的地址，node存储ip，service存储端口号
            */
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
                SYLAR_LOG_DEBUG(g_logger) << "node: " << node << "  service: " << service;
            }
        }
    }


    //如果node是空的话，说明host就是域名，比如baidu.com
    if(node.empty()) {
        node = host;
    }
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        SYLAR_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << gai_strerror(error);
        return false;
    }

    next = results;
    while(next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        SYLAR_LOG_DEBUG(g_logger) << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return !result.empty();
}



bool Address::GetInterfaceAddresses(std::multimap<std::string
                    ,std::pair<Address::ptr, uint32_t> >& result,
                    int family) {
    struct ifaddrs *next, *results;

    /*
    <eth0, 192.168.1.10, 24> 表示网卡名为 eth0，IP 地址为 192.168.1.10，子网掩码位数为 24。
    获取所有网卡保存到results中
    */
    if(getifaddrs(&results) != 0) {
        SYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    try {
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            //判断网卡的family
            switch(next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        /*
                        从 next 结构体中获取网络掩码（ifa_netmask）
                        将网络掩码转换为 32 位整数 uint32_t 类型，存储在 netmask 变量中
                        
                        */
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        //调用 CountBytes 函数，将网络掩码中设置为 1 的字节计数作为前缀长度 prefix_len
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }

            if(addr) {
                result.insert(std::make_pair(next->ifa_name,
                            std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !result.empty();
}

bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family) {

    /*
    如果网卡名为空或者是空配符*
    */
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            //0.0.0.0
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string
          ,std::pair<Address::ptr, uint32_t> > results;

    if(!GetInterfaceAddresses(results, family)) {
        return false;
    }

    //its.first -> its.second
    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty();
}

int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
    if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

bool Address::operator<(const Address& rhs) const {
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());

    //比较两个地址的内容，最大比较长度为 minlen。
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if(result < 0) {
        return true;
    } 
    else if(result > 0) {
        return false;
    } 
    else if(getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address& rhs) const {
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
    return !(*this == rhs);
}

IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    //地址参数为数字格式
    hints.ai_flags = AI_NUMERICHOST;
    //表示不限定地址族
    hints.ai_family = AF_UNSPEC;

    /*
    使用提供的地址和提示信息调用 getaddrinfo 来查找地址信息
    把结果保存到result中
    */
    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        SYLAR_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

/*
IPv4使用的是大端,小端需要转成大端
*/

IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    /*
    address为IP地址
    rt->m_addr.sin_addr为输出地址
    将点分十进制的 IPv4 地址转换为二进制形式
    */
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0) {
        SYLAR_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address) {
    m_addr = address;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    //int转成ipv4地址
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    //拷贝m_addr
    sockaddr_in baddr(m_addr);

    /*
    IP地址 | 子网掩码取反
    */
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}


IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    /*
    IP地址 & 子网掩码
    */
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    /*
    子网掩码 = CreateMask取反
    */
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}





IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0) {
        SYLAR_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address) {
    m_addr = address;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

sockaddr* IPv6Address::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    if(!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

void UnixAddress::setAddrLen(uint32_t v) {
    m_length = v;
}

sockaddr* UnixAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}

std::string UnixAddress::getPath() const {
    std::stringstream ss;
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) {
    m_addr = addr;
}

sockaddr* UnknownAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}

socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}

}