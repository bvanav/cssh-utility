#ifndef __PING_H__
#define __PING_H

#include <iostream>
#include <cstring>
#include <vector>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/ip_icmp.h>       // icmp packet header
#include <sys/socket.h>            // socket functions
#include <netinet/in.h>            // sockaddr_in and IP protocols
#include <arpa/inet.h>             // inet_pton, inet_ntop...

class Ping {
    // constants
    private:
    const static int PING_PKT_S = 64;
    
    // member variables
    private:
    int _pingInterval; // in ms eg. 1000000 micro second = 1 second
    int _recvTimeOut; // in ms

    // helper struct
    struct ping_pkt {
        struct icmphdr hdr; // 8 bytes
        char msg[PING_PKT_S - sizeof(struct icmphdr)]; // 56 bytes
    };

    // member functions
    private:
    unsigned short checksum(void* b, int len){
        unsigned short* buf = (unsigned short*) b;
        unsigned int sum = 0;
        unsigned short result;
        // 1s complement
        // Increment and add 16bits 
        for (sum = 0; len > 1; len -= 2)
            sum += *buf++;
        if (len == 1)
            sum += *(unsigned char*)buf;
        // carry wrap-around - add upper 16bits to lower 16bits
        sum = (sum >> 16) + (sum & 0xFFFF);
        // another carry wrap-around - to handle if above opr. left any carry
        sum += (sum >> 16);
        result = ~sum;
        return result;
    }

    public:
    bool isIPV4Valid(const std::string& ); // TODO
    bool pingIp(const std::string& ping_ip, int max_try = 2, int add_recv_wait_ms = 0, int add_ping_gap_s = 0){
        bool rval = false;
        int ttl_val = 64;
        struct timeval tv_out;
        int msg_count = 0;
        int msg_received_count = 0;
        int flag = 1;
        struct ping_pkt pckt;
        auto ping_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if(ping_sockfd < 0) {
            std::cout << "Socket file descriptor not received try executing with sudo!!" << std::endl;
        }
        else{
            // set socket option - TTL/Hop Limit
            if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0){
                std::cout << "Setting socket options to TTL failed!" << std::endl;
            }
            else{
                tv_out.tv_sec = _recvTimeOut/1000000;
                tv_out.tv_usec = _recvTimeOut - (tv_out.tv_sec*1000000) + add_recv_wait_ms;
                // set socket option - receive timeout
                setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

                for (auto ping_loop = max_try; ping_loop > 0; ping_loop--) {
                    // flag is whether packet is sent or not
                    flag = 1;
                    std::memset(&pckt, 0, sizeof(pckt));
                    
                    // Construct ICMP packet
                    // fill ASCII for message
                    for (auto i = 0; i < sizeof(pckt.msg) - 1; i++)
                        pckt.msg[i] = i + '0';
                    // terminate last msg byte with null
                    pckt.msg[sizeof(pckt.msg) - 1] = 0;
                    pckt.hdr.type = ICMP_ECHO;
                    pckt.hdr.un.echo.id = getpid();
                    pckt.hdr.un.echo.sequence = msg_count++;
                    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
                    usleep(_pingInterval + add_ping_gap_s);

                    struct sockaddr_in ping_addr; // address structure for IPV_4
                    ping_addr.sin_family = AF_INET;
                    ping_addr.sin_port = htons(0);
                    inet_pton(AF_INET, ping_ip.c_str(), &ping_addr.sin_addr.s_addr);
                    if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&ping_addr, sizeof(ping_addr)) <= 0){
                        std::cout << "Packet Sending Failed!" << std::endl;
                        flag = 0;
                    }

                    // receive packet
                    struct sockaddr_in r_addr;
                    auto addr_len = sizeof(r_addr);
                    if (recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, (socklen_t*)&addr_len) <= 0 && msg_count > 1){
                        std::cout << " Packet receive failed!" << std::endl;
                    }
                    else {
                        char szAddr[16] = { 0 };
                        inet_ntop(AF_INET, &r_addr.sin_addr, szAddr, sizeof(szAddr));
                        std::cout << " Received bytes from (" << szAddr << ") msg_seq=" << msg_count << std::endl;
                        if( szAddr == ping_ip){
                            msg_received_count++;
                            break;
                        }
                        else{
                            continue;
                        }
                    }
                }
                rval = msg_received_count > 0;
            }
            close(ping_sockfd);
        }
        return (rval);
    }

    void pingSubnetIp(const std::string& network_addr){
        std::string subnetIp;
        std::string newNetIp =  network_addr + ".";
        for(int i = 2; i < 255; i++){
            subnetIp = newNetIp + std::to_string(i);
            std::cout << "Pinging Ip: " << subnetIp << std::endl;
            if(!pingIp(subnetIp))
                std::cout << " Ping Failed" << std::endl;
        }
    }

    private: 
    Ping()
        : _pingInterval(0) // dont wait before next retry
        , _recvTimeOut(100000) // .10 seconds time out for recv socket call
    { }

    public: // create singleton isntance
    static Ping& getInstance(){
        static Ping lping;
        return lping;
    }
};

// access methods using gping eg. gping.pingIp(pingIp)
Ping& gping = Ping::getInstance();
#endif