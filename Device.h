#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <string>
#include <map>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <algorithm>
#include <iostream>
#include <algorithm>
#include "Logger.h"
#include "System.h"
#include "Utils.h"
#include "Network.h"
#include <pwd.h>

// NOTE: Given user can access any number of devices, but can only extablish one session with one device type

enum FError : uint32_t {
    NO_ERROR                = 0,
    NO_FILE                 = ENOENT,       // No such file or directory
    PERMISSION_DENIED       = EACCES,       // Permission denied
    TOO_MANY_FILES          = EMFILE,       // Too many open files
    NAME_TOO_LONG           = ENAMETOOLONG, // File name too long
    IO_ERROR                = EIO,          // I/O error
    INVALID_ARG             = EINVAL,       // Invalid argument
    NO_SPACE                = ENOSPC,       // No space left on device
    READ_ONLY               = EROFS,        // Read-only file system
    INTERRUPTED             = EINTR         // Interrupted system call
};


struct DeviceInfo {
    char pmi[16];
    char ip[16];
    char mac[18];
};

struct DeviceInUseInfo {
    char pmi[16];
    char ip[16];
    char mac[18];
    char ntid[10];
    char startTime[20];
    pid_t processId;
};

struct ConnectionInfo{
    char ip[16];
    char mac[18];
    char ntid[10];
    char startTime[20];
    uint8_t isBeingUsed;
    pid_t processId;
};

struct UserDeviceInfo{
    char pmi[16];
    char ip[16];
    char mac[18];
    char startTime[20];
};

struct LoginRecordInfo{
    char ntid[10];
    char pmi[16];
    char ip[16];
    char mac[18];
    char startTime[20];
    char endTime[20];
    char logoutType[7];
};

class Device {
    char* homeDir = std::getenv("homeDir");
    std::string prefixPath = (homeDir) ? std::string(std::string(homeDir) + "/cssh/") : "./";
    char* m_device_cache_filename = strdup(std::string(prefixPath  + "device_scanned.dat").c_str());
    char* m_device_in_use_filename = strdup(std::string(prefixPath + "device_being_used.dat").c_str());
    char* m_login_record_filename = strdup(std::string(prefixPath  + "device_login_record.csv").c_str());
    char* m_login_record_sno_filename = strdup(std::string(prefixPath + "device_login_record_sno.txt").c_str());
    char* m_model_names_filename = strdup(std::string(prefixPath + "friendly_names.config").c_str());

    private:
    std::string m_friendly_name;
    std::string m_pmi;
    std::string m_ntid;
    
    DeviceInfo* m_device_cache_ptr = nullptr;
    size_t m_device_cache_size = 0;

    DeviceInUseInfo* m_device_in_use_ptr = nullptr;
    size_t m_device_in_use_size = 0;

    enum RequestType{
        UNKNOWN = 0,
        NEW_CONNECTION,
        CLOSE_CONNECTION,
        LIST
    } m_request_type = RequestType::UNKNOWN;

    size_t m_user_requested_index = -1;

    std::map<std::string, std::string> m_model_name_pmi_map;

    protected:
    std::vector<ConnectionInfo> m_available_devices;
    std::vector<UserDeviceInfo> m_user_devices; // ntid specific device information
    std::string m_ip;

    private:

    void toLower(std::string &s){
        std::transform(s.begin(), s.end(), s.begin(), 
        [](char c){
            if(std::isalpha(c))
                return static_cast<char>(std::tolower(c));
            else
                return c;
        });
    }

    void toUpper(std::string &s){
        std::transform(s.begin(), s.end(), s.begin(), 
        [](char c){
            if(std::isalpha(c))
                return static_cast<char>(std::toupper(c));
            else
                return c;
        });
    }

    template <typename T>
    uint32_t serialize(const char* file_name, T* outptr, size_t size){
        logi("Enter serialize file_name: %s, size: %d", file_name, size);
        errno = 0;
        FILE* fileptr = std::fopen(file_name, "wb");
        if(fileptr == 0){
            loge("serialize - Failed to open file: %s errno: %d errorstr: %s", file_name, errno, std::strerror(errno));
            return errno;
        }
        
        if(size == 0 || outptr == nullptr)
            loge("serialize - Invalid size(%d) or outptr", size);

        int fd = fileno(fileptr);
        if (flock(fd, LOCK_EX) == -1) // lock is automatically released after fclose
            loge("serialize - Exclusive file lock failed errno: %d", errno);

        if(std::fwrite(&size, sizeof(size), 1, fileptr) != 1) {
            loge("serialize - fwrite failed - errno: %d", errno);
            std::fclose(fileptr);
            return errno;
        }

        if(std::fwrite(outptr, sizeof(T), size, fileptr) != size){
            loge("serialize - fwrite failed - errno: %d", errno);
            std::fclose(fileptr);
            return errno;
        }
        std::fclose(fileptr);
        return errno;
    }

    template<typename T>
    uint32_t deserialize(const char* file_name, T** inptr, size_t& size ){
        logi("Enter deserialize file_name: %s", file_name);
        errno = 0;
        FILE *fileptr = std::fopen(file_name, "rb");
        if(fileptr == 0) {
            loge("deserialize - Failed to open file: %s errno: %d errorstr: %s", file_name, errno, std::strerror(errno));
            return errno;
        }

        int fd = fileno(fileptr);
        if (flock(fd, LOCK_SH) == -1) // lock is automatically released after fclose
            loge("deserialize - Shared file lock failed errno: %d", errno);

        size = 0;
        if(std::fread(&size, sizeof(size), 1, fileptr) != 1){
            loge("deserialize - fread failed - errno: %d", errno);
            std::fclose(fileptr);
            return errno;
        }

        *inptr = new T[size];
        if(std::fread(*inptr, sizeof(T), size, fileptr) != size){
            loge("deserialize - fread failed - errno: %d", errno);
            std::fclose(fileptr);
            delete[] *inptr;
            *inptr = nullptr;
            return errno;
        }
        std::fclose(fileptr);
        return errno; 
    }

    bool getSno(uint32_t &sno){
        logi("Enter getSno");
        errno = 0;
        FILE* user_record_sno_fileptr = std::fopen(m_login_record_sno_filename, "rb");
        if(user_record_sno_fileptr == 0){
            loge("getSno - fopen failed with errno: %d", errno);
            std::fclose(user_record_sno_fileptr);
            return false;
        }

        int fd = fileno(user_record_sno_fileptr);
        if (flock(fd, LOCK_EX) == -1) // lock is automatically released after fclose
            loge("getSno - Exclusive file lock failed errno: %d", errno);

        if(std::fread(&sno, sizeof(sno), 1, user_record_sno_fileptr) != 1){
            loge("getSno - fread failed with errno: %d", errno);
            std::fclose(user_record_sno_fileptr);
            return false;
        }
        std::fclose(user_record_sno_fileptr);  
        return true;  
    }

    bool setSno(uint32_t &sno){
        logi("Enter getSno sno: %d", sno);
        errno = 0;
        FILE* user_record_sno_fileptr = std::fopen(m_login_record_sno_filename, "wb");
        if(user_record_sno_fileptr == 0){
            loge("setSno - fopen failed with errno: %d", errno);
            std::fclose(user_record_sno_fileptr);
            return false;
        }

        int fd = fileno(user_record_sno_fileptr);
        if (flock(fd, LOCK_EX) == -1) // lock is automatically released after fclose
            loge("setSno - Exclusive file lock failed errno: %d", errno);

        if(std::fwrite(&sno, sizeof(sno), 1, user_record_sno_fileptr) != 1){
            loge("setSno - fwrite failed with errno: %d", errno);
            std::fclose(user_record_sno_fileptr);
            return false;
        }
        std::fclose(user_record_sno_fileptr);
        return true;
    }

    bool is_file_exists(const char* filename){
        logi("Enter is_file_exists: %s", filename);
        errno = 0;
        FILE* fileptr = std::fopen(filename, "r");
        bool file_exist = true;
        if(!fileptr && errno == FError::NO_FILE)
            file_exist = false;
        if(fileptr)
            std::fclose(fileptr);
        return file_exist;
    }

    bool updateEntry(LoginRecordInfo& entry){
        logi("Enter updateEntry");
        FILE* login_record_fileptr = 0;
        uint32_t sno;
        bool file_exist = is_file_exists(m_login_record_filename);

        errno = 0;
        login_record_fileptr = std::fopen(m_login_record_filename, "a");
        if(login_record_fileptr){
            if(!file_exist){
                std::fprintf(login_record_fileptr, "SNo,NTID,IP,MAC,StartTime(UTC),EndTime(UTC),LogOutType\n");
                sno = 0;
                setSno(sno);
            }

            int fd = fileno(login_record_fileptr);
            if (flock(fd, LOCK_EX) == -1) // lock is automatically released after fclose
                loge("updateEntry - Exclusive file lock failed errno: %d", errno);
            getSno(sno);
            sno++;
            setSno(sno);
            std::fprintf(login_record_fileptr, "%d,%s,%s,%s,%s,%s,%s\n", sno, entry.ntid, entry.ip, entry.mac, entry.startTime, entry.endTime, entry.logoutType);
        }
        else{
            loge("updateEntry - frpintf failed with errno: %d", errno);
            fclose(login_record_fileptr);
            return false;
        }
        fclose(login_record_fileptr);
        return true;
    }

    public:
    Device(std::string& device_name, std::string& ntid)
        : m_friendly_name(device_name)
        , m_ntid(ntid)
    {
        parseModelNames();
        if( m_model_name_pmi_map.find(m_friendly_name) != m_model_name_pmi_map.end() )
            m_pmi = m_model_name_pmi_map[m_friendly_name];
        
        logi("Device Ctor pmi: %s homeDir: %s, prefixPath: %s, cacheFilename: %s, inuseFilename: %s, loginRecordFilename: %s, loginRecordSnoFilename: %s, friendlyConfigFilename: %s", m_pmi.c_str(), homeDir, prefixPath.c_str(), m_device_cache_filename, m_device_in_use_filename, m_login_record_filename, m_login_record_sno_filename, m_model_names_filename);
    }

    Device(std::string& ntid)
        : m_ntid(ntid)
    { 
        parseModelNames();
        logi("Device Ctor pmi: %s homeDir: %s, prefixPath: %s, cacheFilename: %s, inuseFilename: %s, loginRecordFilename: %s, loginRecordSnoFilename: %s, friendlyConfigFilename: %s", m_pmi.c_str(), homeDir, prefixPath.c_str(), m_device_cache_filename, m_device_in_use_filename, m_login_record_filename, m_login_record_sno_filename, m_model_names_filename);
    }

    Device(){
        parseModelNames();
        logi("Device Ctor pmi: %s homeDir: %s, prefixPath: %s, cacheFilename: %s, inuseFilename: %s, loginRecordFilename: %s, loginRecordSnoFilename: %s, friendlyConfigFilename: %s", m_pmi.c_str(), homeDir, prefixPath.c_str(), m_device_cache_filename, m_device_in_use_filename, m_login_record_filename, m_login_record_sno_filename, m_model_names_filename);
    }

    void cleanUp(void){
        logi("Enter cleanUp");
        if(m_device_cache_filename){
            free((void *)m_device_cache_filename);
            m_device_cache_filename = nullptr;
        }
        
        if(m_device_in_use_filename){
            free((void *)m_device_in_use_filename);
            m_device_in_use_filename = nullptr;
        }

        if(m_login_record_filename){
            free((void *)m_login_record_filename);
            m_login_record_filename = nullptr;
        }

        if(m_login_record_sno_filename){
            free((void *)m_login_record_sno_filename);
            m_login_record_sno_filename = nullptr;
        }

        if(m_model_names_filename){
            free((void *)m_model_names_filename);
            m_model_names_filename = nullptr;
        }

        if(m_device_cache_ptr){
            delete[] m_device_cache_ptr;
            m_device_cache_ptr = nullptr;
        }

        if(m_device_in_use_ptr){
            delete[] m_device_in_use_ptr;
            m_device_cache_ptr = nullptr;
        }
    }   

    inline bool isKnownPmi(void){
        return (m_model_name_pmi_map.find(m_friendly_name) != m_model_name_pmi_map.end());
    }

    inline bool isDeviceCacheFileExist(void){
        return is_file_exists(m_device_cache_filename);
    }

    inline bool isDeviceInUseFileExist(void){
        return is_file_exists(m_device_in_use_filename);
    }

    inline bool isLoginRecordFileExist(void){
        return is_file_exists(m_login_record_filename);
    }

    inline bool isModelNameFileExist(void){
        return is_file_exists(m_model_names_filename);
    }

    inline bool isDeviceReachable(void){
        logi("Enter isDeviceReachable");
        char cmdout[256];
        if(!m_available_devices.empty()){
            return System::getPmi(m_available_devices[m_user_requested_index].ip, cmdout);
        }
        return false;
    }

    inline bool isDeviceReachable(std::string ip, int port = 10022){
        logi("Enter isDeviceReachable ip: %s, port: %d", ip.c_str(), port);
        char cmdout[256];
        System::m_port = port;
        return System::getPmi(ip.c_str(), cmdout);
    }
    
    inline void setNewConnectionUserRequest(size_t index){
        logi("Enter setNewConnectionUserRequest index: %d", index);
        m_user_requested_index = index;
        m_request_type = RequestType::NEW_CONNECTION;
    }

    inline void setCloseConnectionUserRequest(size_t index){
        logi("Enter setCloseConnectionUserRequest index: %d", index);
        m_user_requested_index = index;
        m_request_type = RequestType::CLOSE_CONNECTION;
    }

    bool parseModelNames(void){
        logi("Enter parseModelNames");
        if(!isModelNameFileExist()){
            loge("model name file: %s do not exist", m_model_names_filename);
            return false;
        }

        std::ifstream modelNameConfig(m_model_names_filename);
        std::string line;
        int line_num = 0;
        std::string key;
        std::string value;
        size_t first_quote, second_quote, third_quote, forth_quote;

        while(std::getline(modelNameConfig, line)){
            line_num++;

            if(line.empty())
                continue;

            // check number of double quotes 
            int count = std::count(line.begin(), line.end(), '"');
            if(count != 4){
                loge("Line %d not in expected format continuing...", line_num);
                continue;
            }

            first_quote = line.find('"');
            if(line[first_quote+1] == '"'){
                loge("Line %d has empty model name as key continuing...", line_num);
                continue;
            }
            second_quote = line.find('"', first_quote + 1);

            third_quote = line.find('"', second_quote + 1);
            if(line[third_quote+1] == '"'){
                loge("Line %d has empty pmi value continuing...", line_num);
                continue;
            }
            forth_quote = line.find('"', third_quote + 1);

            key = line.substr(first_quote+1, second_quote - first_quote-1);
            value = line.substr(third_quote+1, forth_quote - third_quote-1);
            toLower(key);
            toUpper(value);
            
            logd("Key: %s, value: %s\n", key.c_str(), value.c_str());

            if(m_model_name_pmi_map.find(key) == m_model_name_pmi_map.end())
                m_model_name_pmi_map.insert(std::make_pair(key, value));
            else
                logw("Found duplicate key: %s", key.c_str());
        }

        if(m_model_name_pmi_map.empty()){
            loge("No valid entry in %s", m_model_names_filename);
            return false;
        }
        return true;
    }

    bool isDeviceBingUsed(void){
        logi("Enter isDeviceBingUsed");
        if(!m_available_devices.empty()){
            return m_available_devices[m_user_requested_index].isBeingUsed;
        }
        return false;
    }

    bool sshDevice(void){
        logi("Enter sshDevice");
        if(!m_available_devices.empty()){
            return System::execSsh(m_available_devices[m_user_requested_index].ip);
        }
        return false;
    }
    
    bool updateUserAccess(void){
        logi("Enter updateUserAccess");
        if(m_user_requested_index == (size_t)-1){
            loge("no device requested by user");
            return false;
        }

        errno = 0;
        LoginRecordInfo entry;
        DeviceInUseInfo* device_in_use_ptr = nullptr;
        const char* timeStamp = TimeUtil::nowUTC();

        if(m_request_type == RequestType::NEW_CONNECTION){
            if(m_available_devices.empty()){
                loge("no device found");
                return false;
            }

            if(m_available_devices[m_user_requested_index].isBeingUsed){

                strcpy(entry.ntid, m_available_devices[m_user_requested_index].ntid); // previous user of the device
                strcpy(entry.pmi, m_pmi.c_str());
                strcpy(entry.ip, m_available_devices[m_user_requested_index].ip);
                strcpy(entry.mac, m_available_devices[m_user_requested_index].mac);
                strcpy(entry.startTime, m_available_devices[m_user_requested_index].startTime);
                strcpy(entry.endTime, timeStamp);
                strcpy(entry.logoutType, "FORCED");

                if(m_device_in_use_ptr){
                    for(size_t i = 0; i < m_device_in_use_size; i++){
                        // if device is already available in device in use cache, update current user id,start time and process id
                        if(!strcmp((m_device_in_use_ptr+i)->mac, m_available_devices[m_user_requested_index].mac)){
                            strcpy((m_device_in_use_ptr+i)->ntid, m_ntid.c_str());
                            strcpy((m_device_in_use_ptr+i)->startTime, timeStamp);
                            (m_device_in_use_ptr+i)->processId = getpid();
                            break;
                        }
                    }
                }
                
                logw("Killing ssh session for user: %s", m_available_devices[m_user_requested_index].ntid);
                if(!System::logOut(m_available_devices[m_user_requested_index].processId)){
                    logw("Killing ssh session (pid: %d) user: %s failed !", m_available_devices[m_user_requested_index].processId, m_available_devices[m_user_requested_index].ntid);
                    // return false;
                }

                if(serialize<DeviceInUseInfo>(m_device_in_use_filename, m_device_in_use_ptr, m_device_in_use_size) != FError::NO_ERROR){
                    loge("updateUserAccess - serialize failed");
                    return false;
                }

                if(!updateEntry(entry)){
                    loge("updateUserAccess - updateEntry failed");
                    return false;
                }
            }
            else{
                // update device in use cache
                device_in_use_ptr = (DeviceInUseInfo* )std::realloc(m_device_in_use_ptr, (m_device_in_use_size+1)*sizeof(DeviceInUseInfo));

                if(device_in_use_ptr == NULL){
                    loge("updateUserAccess - realloc failed errno: %d", errno);
                    return false;
                }

                strcpy((device_in_use_ptr+m_device_in_use_size)->pmi, m_pmi.c_str());
                strcpy((device_in_use_ptr+m_device_in_use_size)->ntid, m_ntid.c_str());
                strcpy((device_in_use_ptr+m_device_in_use_size)->ip, m_available_devices[m_user_requested_index].ip);
                strcpy((device_in_use_ptr+m_device_in_use_size)->mac, m_available_devices[m_user_requested_index].mac);
                strcpy((device_in_use_ptr+m_device_in_use_size)->startTime, timeStamp);
                (device_in_use_ptr+m_device_in_use_size)->processId = getpid();

                if(serialize<DeviceInUseInfo>(m_device_in_use_filename, device_in_use_ptr, m_device_in_use_size+1) != FError::NO_ERROR){
                    loge("updateUserAccess - serialize failed");
                    free(device_in_use_ptr);
                    return false;
                }
            }
        }

        if(m_request_type == RequestType::CLOSE_CONNECTION){
            if(m_user_devices.empty()){
                loge("no device found");
                return false;
            }

            strcpy(entry.ntid, m_ntid.c_str());
            strcpy(entry.pmi, m_user_devices[m_user_requested_index].pmi);
            strcpy(entry.ip, m_user_devices[m_user_requested_index].ip);
            strcpy(entry.mac, m_user_devices[m_user_requested_index].mac);
            strcpy(entry.startTime, m_user_devices[m_user_requested_index].startTime);
            strcpy(entry.endTime, timeStamp);
            strcpy(entry.logoutType, "NORMAL");

            // remove this device info from m_device_in_use
            device_in_use_ptr = (DeviceInUseInfo* )std::malloc((m_device_in_use_size-1)*sizeof(DeviceInUseInfo));
            
            if(device_in_use_ptr == NULL){
                loge("updateUserAccess - malloc failed errno: %d", errno);
                return false;
            }

            int  j = 0;
            for(size_t i = 0; i < m_device_in_use_size; i++){
                if(strcmp((m_device_in_use_ptr+i)->mac, m_user_devices[m_user_requested_index].mac) != 0){
                    strcpy((device_in_use_ptr+i)->pmi, (m_device_in_use_ptr+j)->pmi);
                    strcpy((device_in_use_ptr+i)->ip, (m_device_in_use_ptr+j)->ip);
                    strcpy((device_in_use_ptr+i)->mac, (m_device_in_use_ptr+j)->mac);
                    strcpy((device_in_use_ptr+i)->ntid, (m_device_in_use_ptr+j)->ntid);
                    strcpy((device_in_use_ptr+i)->startTime, (m_device_in_use_ptr+j)->startTime);
                    (device_in_use_ptr+i)->processId = (m_device_in_use_ptr+j)->processId;
                    j++;
                }
            }

            if(serialize<DeviceInUseInfo>(m_device_in_use_filename, device_in_use_ptr, m_device_in_use_size-1) != FError::NO_ERROR){
                loge("updateUserAccess - serialize failed");
                return false;
            }

            if(!updateEntry(entry)){
                loge("updateUserAccess - updateEntry failed");
                return false;
            }
        }
        
        if(device_in_use_ptr)
		free(device_in_use_ptr);
        return true;
    }

    // scan for available devices, and update their information in device cache file 
    bool createDeviceCache(void){
        logi("Enter createDeviceCache");
        uint8_t device_count = 0;
        size_t device_pmi_count = 0;
        char pmi[16] = {'\0'};
        ArpOut* scanned_devicesptr = nullptr;
        DeviceInfo* cacheptr = nullptr;

        // scan subnet for pingable devices
        // device_count = gping.pingSubnetIp("10.0.0");
        // device_count = gping.pingSubnetIp("192.168.0");
        gping.pingSubnetIp("10.0.0");

        scanned_devicesptr = new ArpOut[253];
        
        // get the scanned devices info from arp output
        System::arp(scanned_devicesptr, device_count);
        
        if(device_count == 0){
            logw("No devices found while scanning !!");
            return false;
        }
	    fprintf(stderr, " Devices Found: %d\n", device_count);
        cacheptr = new DeviceInfo[device_count];
	    ProgressBar bar(" Fetching PMI...", device_count);
	    bar.start();
        // attempt to get pmi info
        for(int i=0; i < device_count; i++){
            // reset pmi
            pmi[0] = '\0';
            // ssh device and extract pmi from build name
            System::getPmi((scanned_devicesptr+i)->ip, pmi); // **what if it fail ? say a mobile phone is conencted to a network
            if(pmi[0] != '\0'){
                strcpy((cacheptr+device_pmi_count)->pmi, pmi);
                strcpy((cacheptr+device_pmi_count)->ip, (scanned_devicesptr+i)->ip);
                strcpy((cacheptr+device_pmi_count)->mac, (scanned_devicesptr+i)->mac);
                device_pmi_count++;
            }
	        bar.display();
        }
	    bar.end();	

        if(device_count > device_pmi_count){
            logw("Valid devices from apr cmd - %d > Valid devices with pmi - %d", device_count, device_pmi_count);
        }

        // store the info into cache file
        FError result = static_cast<FError>(serialize<DeviceInfo>(m_device_cache_filename, cacheptr, device_pmi_count));

        delete[] scanned_devicesptr;
        delete[] cacheptr;

        if( result != FError::NO_ERROR){
            loge("createDeviceCache - serialize failed - %d", result);
            return false;
        }
        return true;
    }

    void changeDeviceCacheIp(size_t index, std::string newip, int port = 10022){
        logi("Enter changeDeviceCacheIp idnex: %d, newIp: %s, port: %d", index, newip.c_str(), port);
        if(m_device_cache_ptr){
            if(index < m_device_cache_size){
                if(isDeviceReachable((m_device_cache_ptr + index)->ip, port)){
                    FError result = FError::NO_ERROR;
                    strcpy((m_device_cache_ptr + index)->ip, newip.c_str());
                    result = static_cast<FError>(serialize<DeviceInfo>(m_device_cache_filename, m_device_cache_ptr, m_device_cache_size));
                    if(result != FError::NO_ERROR){
                        fprintf(stderr, " Oops some issue in serializing device cache\n");
                        loge("changeDeviceCacheIp - serialize device in use failed - %d", result);
                    }
                    else{
                        fprintf(stderr, " Successfully update ip %s for index %ld !!\n", newip.c_str(), index+1);
                    }
                }
                else{
                    fprintf(stderr, " Oops device is not reachable\n");
                }
            }
            else{
                fprintf(stderr, " Pls specify valid device index...\n");
            }
        }
    }

    void displayConnectionInfo(void){
        logi("Enter displayConnectionInfo");
        if(m_pmi.empty() || m_available_devices.empty()){
            logw("pmi is empty or no device found for new conenction");
            return;
        }
            
        char hyphens[81];
        memset(hyphens, '-', 80);
        hyphens[80] = '\0';

        fprintf(stderr, "\n List of %s/%s Available:\n", m_friendly_name.c_str(), m_pmi.c_str());
        fprintf(stderr, " %s\n", hyphens); 
        fprintf(stderr, " %-4s %-18s %-16s %-7s %-10s %-20s\n", "SNo", "MAC", "IP", "InUse", "NTID", "StartTime(UTC)");
        fprintf(stderr, " %s\n", hyphens); 

        int count = 0;
        for(ConnectionInfo device : m_available_devices){
            count++;
            fprintf(stderr, " %-4s %-18s %-16s %-7s %-10s %-20s\n", std::to_string(count).c_str(), device.mac, device.ip, device.isBeingUsed ? "Yes" : "No", (device.ntid[0] == '\0') ? "NA" : device.ntid, (device.startTime[0] == '\0') ? "NA" : device.startTime);
        }
        fprintf(stderr, " %s\n\n", hyphens); 

    }

    void displayUserDeviceInfo(void){
        logi("Enter displayUserDeviceInfo");
        if(m_ntid.empty() || m_user_devices.empty()){
            logw("ntid is empty or no device in use by user");
            return;
        }

        char hyphens[81];
        memset(hyphens, '-', 80);
        hyphens[80] = '\0';

        fprintf(stderr, "\n List of Devices in use for %s:\n", m_ntid.c_str());
        fprintf(stderr, " %s\n", hyphens); 
	    fprintf(stderr, " %-4s %-16s %-16s %-18s %-20s\n", "SNo", "PMI", "IP", "MAC", "StartTime(UTC)");
        fprintf(stderr, " %s\n", hyphens); 

        int count = 0;
        for(UserDeviceInfo device : m_user_devices){
            count++;
	    fprintf(stderr, " %-4s %-16s %-16s %-18s %-20s\n", std::to_string(count).c_str(), device.pmi, device.ip, device.mac, device.startTime);
        }
        fprintf(stderr, " %s\n\n", hyphens); 
    }

    void displayDeviceCache(void){
        logi("Enter displayDeviceCache");
        FError result = FError::NO_ERROR;
        // load device-cache-file 
        logi("Device cache file name: %s, homeDir: %s, prefixPath: %s", m_device_cache_filename, homeDir, prefixPath.c_str());
        result = static_cast<FError>(deserialize<DeviceInfo>(m_device_cache_filename, &m_device_cache_ptr, m_device_cache_size));
        if(result != FError::NO_ERROR && result != FError::NO_FILE){
            loge("displayDeviceCache - deserialize device cache failed - %d", result);
        }

        char hyphens[81];
        memset(hyphens, '-', 80);
        hyphens[80] = '\0';

        fprintf(stderr, "\n Listing details of scanned devices:\n");
        fprintf(stderr, " %s\n", hyphens); 
        fprintf(stderr, " %-4s %-18s %-16s %-10s\n", "SNo", "PMI", "IP", "MAC");
        fprintf(stderr, " %s\n", hyphens); 

        for(size_t i = 0; i < m_device_cache_size; i++){
            fprintf(stderr, " %-4s %-18s %-16s %-10s\n", std::to_string(i+1).c_str(), (m_device_cache_ptr+i)->pmi, (m_device_cache_ptr+i)->ip, (m_device_cache_ptr+i)->mac);
        }

        fprintf(stderr, " %s\n", hyphens); 
    }

    void displayDeviceInUseCache(void){
        logi("Enter displayDeviceInUseCache");
        FError result = FError::NO_ERROR;
        // load device-in-use file
        result = static_cast<FError>(deserialize<DeviceInUseInfo>(m_device_in_use_filename, &m_device_in_use_ptr, m_device_in_use_size));
        if(result != FError::NO_ERROR && result != FError::NO_FILE){
            loge("displayDeviceInUseCache - deserialize device in use failed - %d", result);
        }

        char hyphens[121];
        memset(hyphens, '-', 120);
        hyphens[120] = '\0';
        fprintf(stderr, "\n Listing details of devices already being used:\n");
        fprintf(stderr, " %s\n", hyphens); 
        fprintf(stderr, " %-4s %-18s %-16s %-18s %-11s %-21s %-8s\n", "SNo", "PMI", "IP", "MAC", "NTID", "StartTime(UTC)", "SSH-PID");
        fprintf(stderr, " %s\n", hyphens); 

        for(size_t i = 0; i < m_device_in_use_size; i++){
            fprintf(stderr, " %-4s %-18s %-16s %-18s %-11s %-21s %-8s\n", std::to_string(i+1).c_str(), (m_device_in_use_ptr+i)->pmi
                , (m_device_in_use_ptr+i)->ip, (m_device_in_use_ptr+i)->mac, (m_device_in_use_ptr+i)->ntid, (m_device_in_use_ptr+i)->startTime
                , std::to_string((m_device_in_use_ptr+i)->processId).c_str());
        }

        fprintf(stderr, " %s\n", hyphens); 
    }

    bool loadNewConnectionDeviceInfo(void){
        logi("Enter loadNewConnectionDeviceInfo");
        FError result = FError::NO_ERROR;
        std::vector<size_t> interested_device_indices;

        // load device-cache-file 
        result = static_cast<FError>(deserialize<DeviceInfo>(m_device_cache_filename, &m_device_cache_ptr, m_device_cache_size));
        if(result != FError::NO_ERROR){
            loge("loadNewConnectionDeviceInfo - deserialize device cache failed - %d", result);
            return false;
        }

        // load device-in-use file
        result = static_cast<FError>(deserialize<DeviceInUseInfo>(m_device_in_use_filename, &m_device_in_use_ptr, m_device_in_use_size));
        if(result != FError::NO_ERROR && result != FError::NO_FILE){
            loge("loadNewConnectionDeviceInfo - deserialize device in use failed - %d", result);
            return false;
        }

        if(m_pmi.empty()){
            loge("loadNewConnectionDeviceInfo - requested device model's pmi info not found");
            return false;
        }

        // filter requested pmi info from device cache
        if(m_device_cache_ptr){
            for(size_t i=0; i < m_device_cache_size; i++){
                if(!std::strcmp((m_device_cache_ptr+i)->pmi, m_pmi.c_str())){
                    interested_device_indices.push_back(i);
                }
            }
        }

        if(interested_device_indices.empty()){
            logw("loadNewConnectionDeviceInfo - requested device model not found in device cache");
        }

        // filter devices that are already in use
        for(int i : interested_device_indices){
            ConnectionInfo device;

            device.ntid[0] = '\0';
            strcpy(device.ip, (m_device_cache_ptr+i)->ip);
            strcpy(device.mac, (m_device_cache_ptr+i)->mac);

            device.startTime[0] = '\0';
            device.isBeingUsed = 0;
            device.processId = 0;

            for(size_t j = 0; j < m_device_in_use_size; j++){
                // find if any of the user requested models is already in use
                if(!strcmp((m_device_in_use_ptr+j)->mac, (m_device_cache_ptr+i)->mac)){
                    device.isBeingUsed = 1;
                    // from when it being used
                    strcpy(device.startTime, (m_device_in_use_ptr+i)->startTime);
                    // who is using the device
                    strcpy(device.ntid, (m_device_in_use_ptr+i)->ntid);
                    // session id
                    device.processId = (m_device_in_use_ptr+i)->processId;
                    break;
                }
            }
            m_available_devices.push_back(device);
        }

        if(m_available_devices.empty()){
            loge("No device found");
            return false;
        }

        return true;
    }

    bool loadUserDeviceInfo(void){
        logi("Enter loadUserDeviceInfo");
        FError result = FError::NO_ERROR;
        // load device-in-use file
        result = static_cast<FError>(deserialize<DeviceInUseInfo>(m_device_in_use_filename, &m_device_in_use_ptr, m_device_in_use_size));
        if(result != FError::NO_ERROR){
            loge("loadUserDeviceInfo - deserialize device in use failed - %d", result);
            return false;
        }

        if(m_ntid.empty()){
            loge("loadUserDeviceInfo - user ntid info not found");
            return false;
        }

        for(size_t j = 0; j < m_device_in_use_size; j++){
            // filter user's currently in use devices
            UserDeviceInfo device;
            if(!strcmp((m_device_in_use_ptr+j)->ntid, m_ntid.c_str())){
		        strcpy(device.pmi, (m_device_in_use_ptr+j)->pmi);
                strcpy(device.ip, (m_device_in_use_ptr+j)->ip);
                strcpy(device.mac, (m_device_in_use_ptr+j)->mac);
                strcpy(device.startTime, (m_device_in_use_ptr+j)->startTime);
                m_user_devices.push_back(device);
            }
        }

        if(m_user_devices.empty()){
            logw("No in use devices found for ntid: %s", m_ntid.c_str());
            return false;
        }

        return true;
    }

};

#endif
