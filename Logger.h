#ifndef __LOGGER_H_
#define __LOGGER_H_

#include <stdio.h>
#include "Utils.h"
#include <mutex>

enum LogLevel {DebugLevel, InfoLevel, WarnLevel, ErrorLevel, None};

class Logger {
    private:
    LogLevel logLevel =  InfoLevel;
    FILE *file = 0;
    std::mutex log_mutex;

    public:
    static void enableFileLogging(const char *file_path="log.txt") {
        Logger& logger_instance = get_instance();
        logger_instance.close_file();
        logger_instance.file = fopen(file_path, "a");
        if(logger_instance.file == 0 ){
            printf("Failed to open file %s", file_path);
        }
    }

    static void disableFileLogging(void) {
        Logger& logger_instance = get_instance();
        logger_instance.close_file();
    }

    static void setLogLevel(LogLevel log_level){
        Logger& logger_instance = get_instance();
        logger_instance.logLevel = log_level;
    }

    template<typename... Args>
    static void println(LogLevel log_level, const char *file_name, int line_number, const char* message, Args... args){
        Logger& logger_instance = get_instance();
        switch(log_level) {
            case DebugLevel:
                logger_instance.log(log_level, "DEBUG", file_name, line_number,message, args...);
                break;
            case InfoLevel:
                logger_instance.log(log_level, "INFO", file_name, line_number, message, args...);
                break;
            case WarnLevel:
                logger_instance.log(log_level, "WARN", file_name, line_number, message, args...);
                break;
            case ErrorLevel:
                logger_instance.log(log_level, "ERROR", file_name, line_number,message, args...);
                break;
            default:
                break;
        }
    }

    private:
    Logger() {}
    Logger& operator=(const Logger&) = delete;
    Logger(const Logger&) = delete;
    
    ~Logger(){
    	close_file();
    }

    static Logger& get_instance() {
        static Logger my_logger;
        return my_logger;
    }

    void close_file() {
        if(file){
            fclose(file);
            file = 0;
        }
    }

    template<typename... Args>
    void log(LogLevel msg_log_level, const char* log_level_str, const char* filename, int line_number, const char* message, Args... args) {
        if(logLevel <= msg_log_level) {

            const char* timestamp = TimeUtil::nowISOUTC();
            std::lock_guard<std::mutex> lock(log_mutex);
            printf("\n %s ", timestamp);
            printf(" [%s][%s:%d] ",log_level_str, filename, line_number);
            printf(message, args...);
            printf("\n");

            // WARNING: No Synchronisation provided in case of multiple processes scenario
            if(file) {
                fprintf(file, "\n %s ", timestamp);
                fprintf(file, " [%s][%s:%d] ",log_level_str, filename, line_number);
                fprintf(file, message, args...);
                fprintf(file, "\n");
            }
        }
    }

};

#define LOG_DEBUG(Message, ...)     (Logger::println(DebugLevel, __FILE__, __LINE__, Message, ##__VA_ARGS__))
#define Log_Debug(Message, ...)     (LOG_DEBUG(Message, ##__VA_ARGS__))
#define logd(Message, ...)          (LOG_DEBUG(Message, ##__VA_ARGS__))

#define LOG_INFO(Message, ...)      (Logger::println(InfoLevel, __FILE__, __LINE__, Message, ##__VA_ARGS__))
#define Log_Info(Message, ...)      (LOG_INFO(Message, ##__VA_ARGS__))
#define logi(Message, ...)          (LOG_INFO(Message, ##__VA_ARGS__))

#define LOG_WARN(Message, ...)      (Logger::println(WarnLevel, __FILE__, __LINE__, Message, ##__VA_ARGS__))
#define Log_Warn(Message, ...)      (LOG_WARN(Message, ##__VA_ARGS__))
#define logw(Message, ...)          (LOG_WARN(Message, ##__VA_ARGS__))

#define LOG_ERROR(Message, ...)     (Logger::println(ErrorLevel, __FILE__, __LINE__, Message, ##__VA_ARGS__))
#define Log_Error(Message, ...)     (LOG_ERROR(Message, ##__VA_ARGS__))
#define loge(Message, ...)          (LOG_ERROR(Message, ##__VA_ARGS__))

#endif


