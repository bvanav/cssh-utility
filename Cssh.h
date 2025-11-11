#ifndef __CSSH_H__
#define __CSSH_H__

#include <cstring>
#include <vector>
#include <limits>
#include "Device.h"

class Cssh : public Device {

    public:
    Cssh(std::string& ntid, std::string& model_name, int port)
        : Device(model_name, ntid)
    { 
        System::m_port = port;
    }

    Cssh(std::string& ntid, std::string& model_name)
        : Device(model_name, ntid)
    { }

    Cssh(std::string& ntid)
        : Device(ntid)
    { }

    Cssh()
        : Device()
    { }

    ~Cssh(){
        cleanUp();
    }

    inline void clearInputBuffer(void){
        int c;
        while((c = getchar() != '\n') && c != EOF);
    }

    void updateCacheIp(int port = 10022){
        logi("Enter updateCacheIp port: %d", port);
        displayDeviceCache();
        fprintf(stderr, " Enter device index (0 to quit): ");
        size_t dev_index;
        std::cin >> dev_index;
        if(dev_index != 0){
            //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            clearInputBuffer();
            std::string ip;
            fprintf(stderr, " Enter device ip: ");
            std::getline(std::cin, ip);
            if(isDeviceReachable(ip, port)){
                changeDeviceCacheIp(dev_index-1, ip, port);
                fprintf(stderr, " Ip addr change successful !\n");
            }
            else{
                fprintf(stderr, " Ip addr change failed - device not reachable !\n");
            }
        }
    }

    void display_user_device(void){
        logi("Enter display_user_device");
        if(!loadUserDeviceInfo()){
            fprintf(stderr, " Oops some issue in finding user specific device information, Exiting...\n");
        }
        displayUserDeviceInfo();
    }

    void close(void){
        logi("Enter close");
        if(loadUserDeviceInfo()){
            displayUserDeviceInfo();
            fprintf(stderr, " Enter device index (0 to quit): ");
            size_t dev_index;
            std::cin >> dev_index;

            if(dev_index > 0 && dev_index <= m_user_devices.size() && dev_index != 0){
                setCloseConnectionUserRequest(dev_index-1);
                if(updateUserAccess()){
                    fprintf(stderr, " User logout successfull\n");
                }
                else{
                    fprintf(stderr, " Oops some issue in updating user access log, Exiting...\n");
                }
            }
        }
        else{
            fprintf(stderr, " Oops some issue in finding user specific device information, Exiting...\n");
        }
    }

    void connect(void) {
        logi("Enter connect");
        enum State{
            CACHE_CHECK,
            CACHE_CREATE,
            DEVICE_INFO,
            USER_REQUEST,
            CONNECT,
            END
        } state = CACHE_CHECK;

        // state machine
        while(state != END){
            switch(state){
                case CACHE_CHECK:
                    logi("Enter state CACHE_CHECK");
                    if(!isModelNameFileExist()){
                        fprintf(stderr, " Make sure you have friendly_names.config file to continue...\n");
                        state = END;
                        break;
                    }
                    if(!isDeviceCacheFileExist()){
                        fprintf(stderr, " Opps device-info cache unavailable, would you like to scan the network & create one(y/n)? ");
                        char ch = toupper(getchar());
                        (ch == 'Y') ? (state = CACHE_CREATE) : (state = END);
                        break;
                    }
                    if(!isKnownPmi()){
                        fprintf(stderr, " Opps provided device model name does not exist in friendly_names.config !!\n");
                        state = END;
                        break;
                    }
                    state = DEVICE_INFO;
                    break;
                
                case DEVICE_INFO:
                    logi("Enter state DEVICE_INFO");
                    // find user requested device info in the device info cache
                    if(!loadNewConnectionDeviceInfo()){
                        fprintf(stderr, " Opps some issue in finding device, would you like to update cache and try again(y/n)? ");
                        char ch = toupper(getchar());
                        (ch == 'Y') ? (state = CACHE_CREATE) : (state = END);
                        // TODO: try wake on lan
                        break;
                    }
                    state = USER_REQUEST;
                    break;

                case CACHE_CREATE:
                    logi("Enter state CACHE_CREATE");
                    fprintf(stderr, " Scanning network and parsing date will take less than 2 minutes, please wait...\n");
                    if(!createDeviceCache()){
                        fprintf(stderr, " Some issue with creating device cache, exiting...\n");
                        state = END;
                        break;
                    }
                    state = DEVICE_INFO;
                    break;

                case USER_REQUEST:
                    logi("Enter state USER_REQUEST");
                    displayConnectionInfo();
                    fprintf(stderr, " Enter device index (0 to quit): "); // q qill result in infinite loop
                    size_t dev_index;
                    std::cin >> dev_index;
                    if(dev_index > 0 && dev_index <= m_available_devices.size() && dev_index != 0){
                        setNewConnectionUserRequest(dev_index-1);
                        if(isDeviceBingUsed()){ // if get_my_ip fail it will worng consider forc login for user
                            // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			                clearInputBuffer();
                            fprintf(stderr, " WARNING: This might logout %s session\n",m_available_devices[dev_index-1].ntid);
                            fprintf(stderr, " Confirm to force connect(y/n)? ");
                            char ch = toupper(getchar()); 
                            if(ch != 'Y'){
                                state = END;
                                break;
                            }
                        }
                        state = CONNECT;
                    }
                    else if(dev_index == 0){
                        state = END;
                    }
                    else{
                        fprintf(stderr, " Please enter valid device index...\n");
                        continue;
                    }
                    break;

                case CONNECT:
                    logi("Enter state CONNECT");
                    if(isDeviceReachable()){
                        updateUserAccess();
                        cleanUp();
                        sshDevice();
                    }
                    else{
                        clearInputBuffer();
                        fprintf(stderr, " Opps unbale to connect to device, would you like to scan the network and create cache(y/n): ");
                        char ch = toupper(getchar());
                        if(ch == 'Y'){
                            state = CACHE_CREATE;
                            break;
                        }
                    }
                    state = END;
                    break;
                default:
                    loge("Captured invalid state: %d, Exiting...", state);
                    state = END;
                    break;
            }
        }
    }
};

#endif
