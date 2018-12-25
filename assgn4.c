#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <time.h>
#include <poll.h>
#include <pthread.h>
#include <inttypes.h>

#define MAX_BUF 64


#define SPIDEVICE "/dev/spidev1.0"

pthread_mutex_t lock;
pthread_t HCSR_thread;    //Ultrasonic sensor thread
pthread_t LED_thread;     //LED Matrix sensor

int spi1;
int distance;
long delay;
double previous = 0, current=0;
int direction = 0;


static __inline__ unsigned long long rdtsc(void)        // Converting 32 bit timestamp output into 64 bits
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

uint8_t array[2];

uint8_t array_w1 [] = {                                                 //pattern
		0x0F, 0x00,
    		0x0C, 0x01,
    		0x09, 0x00,
   		0x0A, 0x0F,
		0x0B, 0x07,
		0x01, 0x3C,
  		0x02, 0x42,
  		0x03, 0x95,
  		0x04, 0xA1,
  		0x05, 0xA1,
  		0x06, 0x95,
  		0x07, 0x42,
  		0x08, 0x3C,

    		
};

uint8_t array_r1 [] = {                                   //pattern
		0x0F, 0x00,
    		0x0C, 0x01,
    		0x09, 0x00,
   		0x0A, 0x0F,
		0x0B, 0x07,
		0x01, 0x3C,
		0x02, 0x42,
		0x03, 0x95,
		0x04, 0x91,
		0x05, 0xB1,
		0x06, 0xB5,
		0x07, 0x42,
		0x08, 0x3C,

    		
};
uint8_t array_w2 [] = {                                 //pattern
	0x0F, 0x00,                       
    	0x0C, 0x01,
    	0x09, 0x00,
   	0x0A, 0x0F,
	0x0B, 0x07,
	0x01, 0x3C,
	0x02, 0x42,
	0x03, 0xA5,
	0x04, 0x91,
	0x05, 0x91,
	0x06, 0xA5,
	0x07, 0x42,
	0x08, 0x3C,

  	
};

uint8_t array_r2 [] = {                                       //pattern
	0x0F, 0x00,
    	0x0C, 0x01,
    	0x09, 0x00,
   	0x0A, 0x0F,
	0x0B, 0x07,
	0x01, 0x3C,
	0x02, 0x42,
	0x03, 0x85,
	0x04, 0xB1,
	0x05, 0xB1,
	0x06, 0x85,
	0x07, 0x42,
	0x08, 0x3C,
  	
};

int gpio_export(int gpionum)					//Function for exporting gpio pins
{
FILE *fd;
fd=fopen("/sys/class/gpio/export","w");
if(fd == NULL)
{
printf("Error in exporting");
}
fprintf(fd,"%d",gpionum);
fclose(fd);
return 0;
}

int gpio_unexport(int gpionum)					//Function for unexporting the exported pins
{
FILE *fd;
fd=fopen("/sys/class/gpio/unexport","w");
if(fd == NULL)
{
printf("Error in unexporting");
}
fprintf(fd,"%d",gpionum);
fclose(fd);
return 0;
}

void gpio_direc(int gpionum,int direc)					//Function for setting the direction of gpio pins to output
{
int fd;
char buf[256];
snprintf(buf,sizeof(buf),"/sys/class/gpio/gpio%d/direction",gpionum);
fd=open(buf,O_WRONLY);
if(fd==-1)
{
printf("Error in setting the direction");
}
if(direc)
	write(fd,"out",3);
else
	write(fd,"in",2);
close(fd);


}

int gpio_setvalue(int gpionum, int value)			//Function to set the value of gpio pins
{
int fd;

char buf[256];

snprintf(buf,sizeof(buf),"/sys/class/gpio/gpio%d/value",gpionum);


fd = open(buf,O_WRONLY);
	if (fd < 0) {
		perror("Setting value failed");
		return fd;
	}

	if (value)
		write(fd, "1", 1);
	else
		write(fd, "0", 1);

close(fd);
return 0;

}


void sensormux()                 // Muxing the GPIO pins for Ultrasonic Sensor
{

gpio_export(34);
gpio_export(77);
gpio_export(13);
gpio_export(16);
gpio_export(76);
gpio_export(14);
gpio_export(64);

gpio_direc(34,1);
gpio_direc(16,1);
gpio_direc(13,1);
gpio_direc(14,0);

gpio_setvalue(34,0);
gpio_setvalue(77,0);
gpio_setvalue(13,0);
gpio_setvalue(16,1);
gpio_setvalue(76,0);
gpio_setvalue(14,0);
gpio_setvalue(64,0);

}


void ledmux()                         // Muxing the GPIO pins for Led Matrix
{

gpio_export(24);
gpio_export(44);
gpio_export(5);
gpio_export(15);
gpio_export(42);
gpio_export(7);
gpio_export(30);
gpio_export(46);
gpio_export(25);
gpio_export(43);
gpio_export(31);

gpio_direc(24,1);
gpio_direc(44,1);
gpio_direc(5,1);
gpio_direc(15,1);
gpio_direc(42,1);
gpio_direc(7,1);
gpio_direc(30,1);
gpio_direc(46,1);
gpio_direc(25,1);
gpio_direc(43,1);
gpio_direc(31,1);


gpio_setvalue(24,0);
gpio_setvalue(44,1);
gpio_setvalue(5,1);
gpio_setvalue(15,0);
gpio_setvalue(42,0);
gpio_setvalue(7,0);
gpio_setvalue(30,0);
gpio_setvalue(46,1);
gpio_setvalue(25,0);
gpio_setvalue(43,0);
gpio_setvalue(31,0);

}

void direc_func(double dist ){                             //Calculating delay and direction
    current =dist;
    double dist_interval= current - previous;
    double distance_min = current / 10.0;
    if(dist_interval > distance_min){
        direction=1;
        delay=30000;
    }
    else if(dist_interval < (-distance_min)){
        direction=0;
        delay=10000;
    }
    previous = current;
}


void* led_mat(void* arg)                         //Thread function for Led Matrix
{
int i,j;

ledmux();
struct spi_ioc_transfer tr =
	{
		.tx_buf = (unsigned long)array,
		.rx_buf = 0,
		.len = 2,
		.delay_usecs = 1,
		.speed_hz = 10000000,
		.bits_per_word = 8,
		.cs_change = 1,
	};
	spi1= open(SPIDEVICE,O_RDWR);

	if(spi1==-1)
	{
     	printf("error in opening file %s\n", SPIDEVICE);
     	exit(-1);
	}

while(1)
{
i=0;
j=0;
pthread_mutex_lock(&lock);
current = distance;
pthread_mutex_unlock(&lock);



		while (i < 26 && direction==1)			
		{
			array[0] = array_w1 [i];
			array[1] = array_w1 [i+1];
			gpio_setvalue(15,0);
			ioctl(spi1, SPI_IOC_MESSAGE (1), &tr);
			gpio_setvalue(15,1);
			i = i + 2;
			printf("1st sequence");
		}
		usleep(delay);

		while (j < 26 && direction==1 )
		{
			array[0] = array_r1 [j];
			array[1] = array_r1 [j+1];
			gpio_setvalue(15,0);
			ioctl(spi1, SPI_IOC_MESSAGE (1), &tr);
			gpio_setvalue(15,1);
			j = j + 2;
			printf("2nd sequence");
		}
		usleep(delay);
		

		while (i < 26 && direction==0)					//changed order//
		{
			
			array[0] = array_w2 [i];
			array[1] = array_w2 [i+1];
			gpio_setvalue(15,0);
		  	ioctl(spi1, SPI_IOC_MESSAGE (1), &tr);
			gpio_setvalue(15,1);
			i = i + 2;
			printf("3rd sequence");
		}
		usleep(delay);

		while (j < 26 && direction==0)
		{
			
			array[0] = array_r2 [j];
			array[1] = array_r2 [j+1];
			gpio_setvalue(15,0);
			ioctl(spi1, SPI_IOC_MESSAGE (1), &tr);
			gpio_setvalue(15,1);
			j = j + 2;
			printf("4th sequence");
		}
		usleep(delay);
		previous = current; 
	}
	close (spi1);
}

void* ultra_sensor(void* arg)                   //Thread Function for Sensor function
{

int fd, fd1,ret;
int timeout = 2000;
unsigned long long rise_t=0,fall_t=0,diff_time;
char *buf[MAX_BUF];
double local;
long abc;
struct pollfd fd_p;
sensormux();
fd = open("sys/class/gpio/gpio14/edge", O_WRONLY);
fd1 = open("/sys/class/gpio/gpio14/value", O_RDONLY| O_NONBLOCK);
fd_p.fd = fd1;
fd_p.events = POLLPRI|POLLERR;
fd_p.revents = 0;
while(1)
{
	printf("inside while loop");
	lseek(fd1, 0, SEEK_SET);
	write(fd, "rising", 6);
	gpio_setvalue(13,0);
	usleep(2);
	gpio_setvalue(13,1);
	
	usleep(14);
	gpio_setvalue(13,0);
	ret = poll(&fd_p, 1 , timeout);
	printf("After poll");
	if(ret<0)
	{
	printf("Error");
	}
	else if(ret>0)
		{	
		if(fd_p.revents & POLLPRI)
		  {
			rise_t = rdtsc();
			printf("Polling encounter");
			read(fd1, buf, 1);
		  }
		  }
			lseek(fd1, 0, SEEK_SET);
			write(fd, "falling", 7);
			ret = poll(&fd_p, 1, timeout);
	if(ret>0)
		{
		if(fd_p.revents & POLLPRI)
		  {
			fall_t = rdtsc();
			read(fd1,buf,1);
		  }
		}	
			pthread_mutex_lock(&lock);	
   			diff_time = fall_t - rise_t;
                        abc = (long)diff_time;
                        local = (abc * 340.00) / (2.0 * 4000000);
			//printf("local = %f",local);
			//distance = local;
			direc_func(local);	
			pthread_mutex_unlock(&lock);
		  	//printf("distance = %d",distance);
		
			
		  
	usleep(600000);
}
return 0;
			
}


int main()                                     //Main Function
{

//int ret, ret1;
printf("Inside main");
if(pthread_mutex_init(&lock, NULL)!=0)
{
	printf("Mutex init failed");
	return 1;
}
pthread_create(&HCSR_thread, NULL, &ultra_sensor, NULL);             //Creating Ultrasonic Sensor thread
//if(ret==0)
//{
printf("thread created");
//}
pthread_create(&LED_thread, NULL, &led_mat, NULL);                  //Creating LED Matrix thread
//if(ret1 ==0)
//{
printf("LeD thread created");
//}
pthread_join(LED_thread, NULL);
pthread_join(HCSR_thread, NULL);
pthread_mutex_destroy(&lock);

gpio_unexport(34);
gpio_unexport(77);
gpio_unexport(13);
gpio_unexport(16);
gpio_unexport(76);
gpio_unexport(14);
gpio_unexport(64);
gpio_unexport(24);
gpio_unexport(44);
gpio_unexport(5);
gpio_unexport(15);
gpio_unexport(42);
gpio_unexport(7);
gpio_unexport(30);
gpio_unexport(46);
gpio_unexport(25);
gpio_unexport(43);
gpio_unexport(31);

return 0;

}



