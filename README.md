# CSSH Utility
###### CSSH is a wrapper around the well-known "ssh" utility, designed to deliver a seamless and enhanced user experience within development ecosystems where multiple devices are accessed by multiple users. It comes equipped with powerful features such as device scanning, device management, and user management, among others.

### Why CSSH ?
- No more memorizing IPs or MACs — CSSH automatically scans devices within the WLAN subnet and extracts their IP, MAC, and PMI information.
- No more cable clutter — All managed devices are maintained in a WLAN environment, eliminating the need for Ethernet cables or worrying about cable length.
- No more network switching — Stream specific content seamlessly, as devices operate within a unified WLAN DOCSIS network.
- No more confusion over usage — Every device session is logged and maintained, so you always know who is currently using a device and who used it previously.
- No more access issues — Whether working remotely or on-premises, CSSH ensures the same consistent user experience across all devices.

### Getting Started:
This utility is designed to run on a Raspberry Pi. Users log in to the RPi and use the tool to access other devices. Create one-time ecosystem setup as illustrated below. In this setup, the Raspberry Pi is connected to the office LAN and simultaneously acts as a client to a WLAN router.

![](https://github.com/bvanav/cssh-utility/blob/main/images/cssh-ecosystem.png)

###### To connect the Raspberry Pi to a WLAN from the command line:
> sudo raspi-config 
> → System Options 
> → Wireless LAN 
> → Enter SSID and Password

###### Make One-Time SSH Configuration Change:
- Open the SSH config file:
```sh
sudo vi /etc/ssh/ssh_config
```
- Comment out line 54 (PermitRootLogin): 
```sh
# PermitRootLogin yes
```
- Restart the SSH service:
```sh
sudo systemctl restart sshd
```

###### Clone the repository.
###### Build the CSSH binary.
```sh
cd cssh-utility
make
```
###### Add an alias to your ~/.bashrc.
```sh
alias cssh='sudo homeDir="~" ~/cssh/cssh'
```
###### If its first time make to reload shell configuration.
```sh
source ~/.bashrc
```

### Usage:
- Open git bash and log in to RPi and use below command.
- If the user wishes to assign a custom name to a device for easier identification later, they can do so by adding an entry to the friendly_names.config file in the following format "friendly name" = "device PMI". Also multiple entries can be provided for the same device PMI if needed.
```sh
vi ~/cssh/friendly_names.config
add a custom entry in format: "<friendly name>" = "<device PMI>"
```
- To view user-device history
```sh
cat ~/cssh/device_login_record.csv
```

> commads args are case-insensitive, Enjoy !
- To SSH device: (default port is 10022)
```sh
cssh -n <ntid> -d <device model name>
```
- To SSH device with specific port:
```sh
cssh -n <ntid> -d <device model name> -p <port number>
```
- To geracefully close SSH connection:
```sh
cssh -c <ntid>
```
- To list user specific device info:
```sh
cssh -t list -n <ntid>
```
- To list device cache:
```sh
cssh -t list -o cache
```
- To list device being used:
```sh
cssh -t list -o inuse
```
- To scan network and update device cache:
```sh
cssh -t scan
```
- To change ip addr of any device in cache: ( default port is 10022)
```sh
cssh -t mod -o cache
```
- To change ip addr of any cache device that operate at specific port:
```sh
cssh -t mod -o cache -p <port number>
```
- To set verbose level: (debugging purpose)
```sh
cssh <any of above cmds> -v [dbg/info/warn/err]
```
> | 'n'tid | 'd'evice | 'c'lose | 't'ype | 'o'utput | 'i'p | 'p'ort |

### Upcoming Features Planned:
- Port Forwarding — Access device VNC servers and the AppServiced gateway seamlessly.
- Wake-on-LAN — Wake devices from deep sleep remotely with ease.
- Multiple SSH Sessions — Allow a given user to establish multiple SSH sessions on a single device.

