// compile : g++ main.cpp -o logger
// usage   : ./logger

#include "logger.h"

int main() {
    Logger::enableFileLogging("cssh-log.txt");
    Logger::setLogLevel(LogLevel::DebugLevel);

    // 3 types of usage
    LOG_DEBUG("Hello World - 1");
    Log_Debug("Hello World - %f", 2.0);
    logd("Hello World - %s", "3.0");

    // other log level and usage
    Logger::disableFileLogging();

    Log_Info("Hello World - %ld", 4);
    logi("Hello World - %s", "5.0");

    LOG_ERROR("Hello World - 6");
    loge("Hello World - %lf", 7.1);
    
    Log_Warn("Hello World - %c", '8');
    logw("End of program !!");
}

// Possible Output:
//  2025-10-05T17:58:54.279Z  [DEBUG][main.cpp:9] Hello World - 1
//  2025-10-05T17:58:54.281Z  [DEBUG][main.cpp:10] Hello World - 2.000000
//  2025-10-05T17:58:54.281Z  [DEBUG][main.cpp:11] Hello World - 3.0
//  2025-10-05T17:58:54.285Z  [INFO][main.cpp:16] Hello World - 4
//  2025-10-05T17:58:54.285Z  [INFO][main.cpp:17] Hello World - 5.0
//  2025-10-05T17:58:54.285Z  [ERROR][main.cpp:19] Hello World - 6
//  2025-10-05T17:58:54.285Z  [ERROR][main.cpp:20] Hello World - 7.100000
//  2025-10-05T17:58:54.285Z  [WARN][main.cpp:22] Hello World - 8
//  2025-10-05T17:58:54.285Z  [WARN][main.cpp:23] End of program !!
