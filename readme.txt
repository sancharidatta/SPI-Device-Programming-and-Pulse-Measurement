Team no :22
Team Members :
1. Bhavana Santhoshi Siga - 1214945459
2. Sanchari Datta - 1215306118


Files :
Device driver code   : led_driver.c
User level code      : userprog.c
Makefiles     	     : Makefile

Prerequisites:
Linux kernel (preferably linux 2.6.19 and above) GNU (preferably gcc 4.5.0 and above)

Galileo Set up : The Boot Image is flashed into an SD card and inserted into the Board. A Static IP address is set to the board and a connection is established between the host machine and Galileo. Then the toolchain SDK is downloaded and extracted into a directory on your host machine.

Connection between host and Galileo : We can either use the shell screen to establish communication between the host machine and the Galileo or use Putty. Find the ip address of the  ethernet connection and the Galileo, which can be set according to us by using the command
"ifconfig  enp0s20f6 192.168.1.5  netmask  255.255.0.0  up"

The SDK is extracted to the directory : "/opt/iot-devkit/1.7.2/sysroots"

Installing GCC compiler :

"sudo apt-get install gcc" - this would install gcc compiler on hostmachine.

The code for device driver and the user space program can be found in the zip file wherein the codes are commented properly. 

Compile the make file using "make" command, two executable files are created in the same directory.


After setting up the Galileo Board,copy the two executable files i.e. Led_driver.ko module and user file from host machine to the board : "scp  root@192.168.1.5:/home" and "scp user root@192.168.1.5:/home"

Open the shell terminal and navigate to "home" directory.Install the module by using "insmod Led_driver.ko" command. Once the driver is initialised, we can see that there is a device named "spidev" which has been created in /dev directory.Now, make the user exec file executable by entering "chmod 777 /home/userprog".Then, enter : "./userprog". Now, 5 different patterns are displayed on the LED matrix and as the distance changes, the patterns keep changing.
