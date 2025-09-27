// compile : g++ main.cpp -o myping
// usage   : sudo ./myping -p 10.0.0.61
// stat    : time to ping single Ip             best case: ~0.03s worst case: ~0.25s 
// stat    : time to all ip's in a subnet net   best case: ~8s worst case: ~42s
#include "ping.h"

int main(int argc, char* argv[]) {
    std::string pingIp;
    if( argc < 3){
        std::cout 
                << "Usage: \n"
                << " " << argv[0] << "-p <ip>  eg. 10.0.0.61\n"
                << " " << argv[0] << "-sp <network addr>   eg. -sp 10.0.0\n" << std::endl;
    }
    else {
        pingIp = argv[2];
        if(std::strcmp(argv[1], "-p") == 0){ 
            std::cout << "Pinging Ip: " << pingIp << std::endl;
            gping.pingIp(pingIp);
            
        }
        else if(std::strcmp(argv[1], "-sp") == 0) {
            std::cout << "Pinging network addr: " << pingIp << std::endl;
            gping.pingSubnetIp(pingIp);
        }
    }
    return 0;
}