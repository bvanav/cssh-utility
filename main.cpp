#include <cstring>
#include "Utils.h"
#include "Logger.h"
#include "Cssh.h"

// NOTE: All message that intended to be visible to user are cooded with fprintf(stderr)

int main(int argc, char* argv[]){
    // init logging - used for debugging pupose
    Logger::setLogLevel(LogLevel::None); 
    Logger::disableFileLogging();

    ArgParser console_opt(argc, argv);

    if(console_opt.isValid()) {
        if(console_opt.hasOption('v')){
            std::string verboseLevel = console_opt.getOption('v');
            logi("ConsoleArgs -v: %s",verboseLevel.c_str());
            if(verboseLevel == "dbg")
                Logger::setLogLevel(LogLevel::DebugLevel);
            else if(verboseLevel == "info")
                Logger::setLogLevel(LogLevel::InfoLevel);
            else if(verboseLevel == "warn")
                Logger::setLogLevel(LogLevel::WarnLevel);
            else if(verboseLevel == "err")
                Logger::setLogLevel(LogLevel::ErrorLevel);
            else
                console_opt.displayHelp();
        }

        if(console_opt.hasOption('n') && console_opt.hasOption('d')){
            std::string ntid =  console_opt.getOption('n');
            std::string model =  console_opt.getOption('d');
            logi("ConsoleArgs -n: %s; -d: %s",ntid.c_str(), model.c_str());
            Cssh _cssh(ntid, model);
            _cssh.connect();
        }
        else if(console_opt.hasOption('c')){
        std::string ntid =  console_opt.getOption('c');
        logi("ConsoleArgs -c: %s",ntid.c_str());
        Cssh _cssh(ntid);
        _cssh.close();
        }
        else if(console_opt.hasOption('t')){
            std::string type_value = console_opt.getOption('t');
            logi("ConsoleArgs -t: %s",type_value.c_str());
            if(type_value == "list"){
                if(console_opt.hasOption('n')){
                    std::string ntid =  console_opt.getOption('n');
                    logi("ConsoleArgs -n: %s",ntid.c_str());
                    Cssh _cssh(ntid);
                    _cssh.display_user_device();
                }
                else if(console_opt.hasOption('o')){
                    std::string output_type = console_opt.getOption('o');
                    logi("ConsoleArgs -o: %s",output_type.c_str());
                    Cssh _cssh;
                    if(output_type == "cache"){
                        _cssh.displayDeviceCache();
                    }
                    else if(output_type == "inuse"){
                        _cssh.displayDeviceInUseCache();
                    }
                    else
                        console_opt.displayHelp();
                }
            }
            else if(type_value == "scan"){
                Cssh _cssh;
                if(!_cssh.createDeviceCache()){
                    fprintf(stderr, " Some issue with creating device cache, exiting...\n");
                }
                _cssh.cleanUp();
            }
            else if(type_value == "mod"){
                if(console_opt.hasOption('o')){
                    if(console_opt.getOption('o') == "cache"){
                        Cssh _cssh;
                        if(console_opt.hasOption('p'))
                            _cssh.updateCacheIp(std::stoi(console_opt.getOption('p')));
                        else
                            _cssh.updateCacheIp();
                    }
                    else
                        console_opt.displayHelp();
                }
            }
            else
                console_opt.displayHelp();
        }
    }
    else{
        console_opt.displayHelp();   
    }
    
    return 0;
}
