#ifndef __UTILS_H__
#define __UTILS_H__

#include <ctime>
#include <cstdio>
#include <sys/time.h>
#include <cstring>
#include <iostream>
#include <map>
#include <algorithm>
#include <cstring>

class TimeUtil { // Logger class functionality cannot be used inside TimeUtil instead use cout/printf
    private: 
    static struct timeval tv;
    static struct tm *utc_tm;
    static char timestamp[40];

    public:
    TimeUtil() = delete;
    TimeUtil(const TimeUtil&) = delete;
    TimeUtil& operator=(const TimeUtil&) = delete;

    static const char* nowISOUTC(void){
        // reset values
        memset(&tv, 0, sizeof(tv));
        utc_tm = nullptr;
        timestamp[0] = '\0';

        // get current time
        gettimeofday(&tv, NULL);
        // Convert into UTC time stamp
        utc_tm = gmtime(&tv.tv_sec);
        // ISO 8601 formating - YYYY-MM-DDTHH:MM:SS.NNNZ
        snprintf(timestamp, sizeof(timestamp),
            "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
            utc_tm->tm_year + 1900,
            utc_tm->tm_mon + 1,
            utc_tm->tm_mday,
            utc_tm->tm_hour,
            utc_tm->tm_min,
            utc_tm->tm_sec,
            tv.tv_usec / 1000
        );

        return timestamp;
    }

    static const char* nowUTC(void){
        // reset values
        memset(&tv, 0, sizeof(tv));
        utc_tm = nullptr;
        timestamp[0] = '\0';

        // get current time
        gettimeofday(&tv, NULL);
        // Convert into UTC time stamp
        utc_tm = gmtime(&tv.tv_sec);
        // Formating - YYYY-MM-DDTHH:MM:SS
        snprintf(timestamp, sizeof(timestamp),
            "%04d-%02d-%02dT%02d:%02d:%02d",
            utc_tm->tm_year + 1900,
            utc_tm->tm_mon + 1,
            utc_tm->tm_mday,
            utc_tm->tm_hour,
            utc_tm->tm_min,
            utc_tm->tm_sec
        );

        return timestamp;
    }
};

#include "Logger.h" // Logger will expect TimeUtil to be declared

class ProgressBar {
    private:
    std::string m_description;
    int m_total;
    int m_step;
    int m_bar_width;
    int m_current;
    float m_percentage;
    int m_filled ;

    public:
    ProgressBar(std::string des, int total = 100, int step = 1, int width = 50)
        : m_description(des), m_total(total), m_step(step), m_bar_width(width)
    {
        m_current = 0;
        m_percentage = 0.0;
        m_filled = 0;
    }

    void start(void){
        std::cerr << m_description << std::endl;
        display(true);
    }

    void display(bool isStart=false){
        if(!isStart)
            m_current = m_current + m_step;

        m_percentage = static_cast<float>(m_current)/m_total;
        m_filled = static_cast<int>(m_percentage*m_bar_width);

        std::cerr << "\r [";
        for(int i = 0; i < m_bar_width; i++){
            if( i < m_filled)
                std::cerr << "=";
            else
                std::cerr << " ";
        }

        std::cerr << "] " << static_cast<int>(m_percentage*100) << "%";
        // std::cout.flush();
    }
    
    void end(void){
        m_current = 0;
        m_percentage = 0.0;
        m_filled = 0;
        std::cout << std::endl;
    }
};

class ArgParser {
    private:
    bool m_valid;
    std::string m_options;
    std::map<char, std::string> m_console_option;

    void toLower(std::string &s){
        std::transform(s.begin(), s.end(), s.begin(), 
        [](char c){
            if(std::isalpha(c))
                return static_cast<char>(std::tolower(c));
            else
                return c;
        });
    }

    void parse(int count, char* arg[]){
        logi("Enter parse");
        std::string value;
        for(int i = 1; i < count; i = i + 2){
            m_valid = true;
            // odd index arg should start with '-'
            if(arg[i][0] == '-' && m_options.find(arg[i][1]) != std::string::npos){
                value = arg[i+1];
                toLower(value);
                m_console_option.insert(std::make_pair(arg[i][1], value));
                logd("parsing console args - make_pair(arg[i][1]: %c, arg[i+1]: %s)", arg[i][1], value.c_str());
                value.clear();
            }
            else{
                m_valid = false;
            }

            if(!m_valid)
                break;
        }
    }

    public:
    ArgParser() = delete;
    ArgParser(int argc, char* argv[])
        : m_valid(false)
        , m_options("ndctoiv")
    {
        // always count should be a odd value
        if(argc % 2 != 0)
            parse(argc, argv);
    }

    inline bool isValid(void){
        return m_valid;
    }

    inline bool hasOption(char opt){
        return (m_console_option.find(opt) != m_console_option.end()) ? true : false;
    }

    inline std::string getOption(char opt){
        if(hasOption(opt)){
            return m_console_option[opt];
        }   
        return "";
    }

    void displayHelp(void) {
        char hypens[61];
        memset(hypens, '-', 60);
        hypens[60] = '\0';

        fprintf(stderr, " %s\n", hypens);
        fprintf(stderr, " U-S-A-G-E: \n");
        fprintf(stderr, " %s\n", hypens);

        fprintf(stderr, " To SSH device: (default port is 10022)\n");
        fprintf(stderr, " \tcssh -n <ntid> -d <device model name>\n");

        fprintf(stderr, " To SSH device with specific port:\n");
        fprintf(stderr, " \tcssh -n <ntid> -d <device model name> -p <port number>\n");

        fprintf(stderr, " To geracefully close SSH connection:\n");
        fprintf(stderr, " \tcssh -c <ntid>\n");

        fprintf(stderr, " To list user specific device info: \n");
        fprintf(stderr, " \tcssh -t list -n <ntid>\n");

        fprintf(stderr, " To list device cache: \n");
        fprintf(stderr, " \tcssh -t list -o cache\n");

        fprintf(stderr, " To list device being used: \n");
        fprintf(stderr, " \tcssh -t list -o inuse\n");

        fprintf(stderr, " To scan network and update device cache: \n");
        fprintf(stderr, " \tcssh -t scan\n");

        fprintf(stderr, " To change ip addr of any device in cache: ( default port is 10022)\n");
        fprintf(stderr, " \tcssh -t mod -o cache\n");

        fprintf(stderr, " To change ip addr of any cache device that operate at specific port: \n");
        fprintf(stderr, " \tcssh -t mod -o cache -p <port number>\n");

        fprintf(stderr, " To set verbose level: (debugging purpose) \n");
        fprintf(stderr, " \tcssh <any of above cmds> -v [dbg/info/warn/err]\n");

        fprintf(stderr, " To view user-device history \n");
        fprintf(stderr, " \tcat ~/cssh/device_login_record.csv\n");

        fprintf(stderr, "\n *commads are case-insensitive\n");
        fprintf(stderr, "\n | 'n'tid, 'd'evice, 'c'lose, 't'ype, 'o'utput, 'i'p  'p'ort |\n");

        fprintf(stderr, " %s\n", hypens);
    }
};

// definig static members
struct timeval TimeUtil::tv;
struct tm* TimeUtil::utc_tm;
char TimeUtil::timestamp[40];

#endif
