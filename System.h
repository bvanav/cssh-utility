#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <cstdio>
#include "Logger.h"
#include <cerrno>
#include <cstring>
#include <sys/wait.h>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>


struct ArpOut {
    char ip[16];
    char mac[18];
};

class System {
    public:
        static int m_port;

    private:
    static bool get_my_ip(char* ip_addr, const char* interface="wlan0"){ // usage: get_host_ip(ip-addr, "eth0")
        logi("Enter get_my_ip ip: %s, interface: %s", ip_addr, interface);
        int sock;
        // declare ifreq struct for querying ioctl
        struct ifreq ifr; 
        errno = 0;

        if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
            loge("get_host_ip - Socket creation failed errno: %d", errno);
            return false;
        }

        strcpy(ifr.ifr_name, interface);
        // use ioctl command SIOCGIFADDR - Socket IOCTL Get InterFace Address
        if(ioctl(sock, SIOCGIFADDR, &ifr) < 0){
            if(errno == 99) {
                fprintf(stderr, " Pls check if RPi is connected to wlan...\n"); // user msg
                loge("IP Address not assigned for requested interface");
            }
                
            loge("get_my_ip - ioctl failed errno: %d", errno);
            return false;
        }

        // convert ip addr into human readable form
        struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
        inet_ntop(AF_INET, &(addr->sin_addr), ip_addr, INET_ADDRSTRLEN);

        close(sock);
        return true;
    }

    public:
    static bool arp(ArpOut* cmdout, uint8_t &count){
        logi("Enter arp");
        errno = 0;
        ArpOut* resarray = cmdout;
        FILE *pipe; 
        int status;
        int exitcode;
        char buffer[256];
        char ip[16];
        char mac[18];

        pipe = popen("arp -a", "r");
        if(pipe == NULL){
            loge("popen failed for cmd \'arp\' errno: %d", errno);
            return false;
        }

        while(fgets(buffer, sizeof(buffer), pipe) != NULL){
            sscanf(buffer, "? (%15[^)]) at %18s", ip, mac);
            if(mac[0] != '<'){
                ++count;
                // logd("Valid mac: %s ip: %s", mac, ip);
                std::strcpy(resarray->ip, ip);
                std::strcpy(resarray->mac, mac);
                resarray++;
            }
        }
        
        status = pclose(pipe);
        if(status == -1){
            loge("pclose failed with errno: %d", errno);
            return false;
        }
 
        if(WIFEXITED(status)){
            if((exitcode = WEXITSTATUS(status)) != 0){
                loge("arp command failed with exit code: %d", exitcode);
                return false;
            }
        }

        return true;
    }

    static bool getPmi(const char *ip, char* cmdout){
        logi("Enter getPmi ip: %s", ip);
        errno = 0;
        char cmd[256];
        FILE *pipe;
        char buffer[256];
        char *token = nullptr;
        const char delimiter[] = ":_";
        int status, exitcode;

        // sprintf(cmd, "ssh -o \"UserKnownHostsFile=/dev/null\" -o \"StrictHostKeyChecking=no\" -o \"ConnectTimeout=3\" -p 10022 -t root@%s \"head -n1 /version.txt\"", ip);
        sprintf(cmd, "ssh -o \"UserKnownHostsFile=/dev/null\" -o \"StrictHostKeyChecking=no\" -o \"ConnectTimeout=2\" -p %d -qt root@%s \"head -n1 /version.txt\"", m_port, ip);
        logd("Executing command: %s", cmd);
        pipe = popen(cmd, "r");
        
        fgets(buffer, sizeof(buffer), pipe);
        // parse pmi
        strtok(buffer, delimiter);
        token = strtok(NULL, delimiter);
        if(token){
            strcpy(cmdout, token);
            logd("Pmi is %s for ip: %s", token, ip);
        }
        else{
            loge("Pmi extraction failed for ip: %s", ip);
        }
        status = pclose(pipe);
        if((exitcode = WEXITSTATUS(status)) != 0){
            loge("ssh cmd to extract device pmi failed with exit code: %d", exitcode);
            return false;
        }
        return true;
    }

    static bool logOut(pid_t process_id){
        logi("Enter logOut");
        errno = 0;
        if(kill(process_id, SIGTERM) == 0){
            return true;
        }
        else{
            if(errno == ESRCH){
                logw("kill sys call no such process %d", process_id);
            }
            else if(errno != 0 ){
                loge("logOut - kill failed");
            }
        }

        return false;
    }

    static bool execSsh(const char* target_ip){
        logi("Enter execSsh");
        errno = 0;
        char my_ip[16];
        if(!get_my_ip(my_ip))
            return false;

        system("clear");
        execlp("ssh", "ssh", "-p", std::to_string(m_port).c_str(), "-o", (std::string("BindAddress=")+my_ip).c_str(), (std::string("root@")+target_ip).c_str(), nullptr);

        // This region will execute only when exec call fails
        loge("execSsh - execlp failed errno: %d", errno);
        return false;
    }
};

int System::m_port = 10022;
#endif