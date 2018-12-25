#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <sched.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <pthread.h>
#include <poll.h>

#define SPI_DEVICE "/dev/spidev1.34"
#define MAX_BUF 64

pthread_mutex_t mutex;
//pthread_mutex_t lock;
pthread_mutex_t lock1;

typedef unsigned long long ticks;

int i; 

int fd13,fd13v,fd14,fd14_val,fd14_edge;


uint8_t seq[16];


uint8_t seq1[16] = {0,250,1,250,2,250,1,250,0,250,1,250,3,250,0,0};
uint8_t seq2[16] = {4,250,5,250,6,250,4,250,5,250,6,250,4,250,0,0};

	
uint8_t sequence[10][24]= {
	
        {0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x1F, 0x02, 0x21, 0x03, 0x42, 0x04, 0x84, 0x05, 0x84, 0x06, 0x42, 0x07, 0x21, 0x08, 0x1F,},
	{0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x7E, 0x02, 0xC3, 0x03, 0xA5, 0x04, 0x99, 0x05, 0x99, 0x06, 0xA5, 0x07, 0xC3, 0x08, 0x7E,},
	{0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x7E, 0x02, 0x81, 0x03, 0xA5, 0x04, 0xA1, 0x05, 0xA1, 0x06, 0xA5, 0x07, 0x81, 0x08, 0x7E,},
	{0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x3E, 0x02, 0x81, 0x03, 0xCD, 0x04, 0xA1, 0x05, 0xA1, 0x06, 0xCD, 0x07, 0x81, 0x08, 0x7E,},
        {0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x7E, 0x02, 0x81, 0x03, 0x85, 0x04, 0x81, 0x05, 0x81, 0x06, 0x85, 0x07, 0x85, 0x08, 0x7E,}, 
	{0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x98, 0x02, 0x3c, 0x03, 0x7e, 0x04, 0x3c, 0x05, 0x7e, 0x06, 0xff, 0x07, 0x18, 0x08, 0x18,},
	{0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x19, 0x02, 0x3c, 0x03, 0x7e, 0x04, 0x3c, 0x05, 0x7e, 0x06, 0xff, 0x07, 0x18, 0x08, 0x18,},
	{0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x00, 0x02, 0x07, 0x03, 0x02, 0x04, 0x7e, 0x05, 0x66, 0x06, 0x55, 0x07, 0x55, 0x08, 0x66,},
	{0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x00, 0x02, 0x07, 0x03, 0x02, 0x04, 0x7e, 0x05, 0x66, 0x06, 0x99, 0x07, 0x99, 0x08, 0x66,},
        {0x0C, 0x01, 0x09, 0x00, 0x0A, 0x0F, 0x0B, 0x07, 0x01, 0x00, 0x02, 0x07, 0x03, 0x02, 0x04, 0x7e, 0x05, 0x66, 0x06, 0xaa, 0x07, 0xaa, 0x08, 0x66,},
};


int dir=1; 
double current,previous=0;
pthread_mutex_t lock; // mutex for locking critical section
long delay=1000000;
int fd_spi;


static inline ticks timestamp(void){
      unsigned a, d;
      asm("cpuid");
      asm volatile("rdtsc" : "=a" (a), "=d" (d));
      return (((ticks)a) | (((ticks)d) << 32));
}


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


void sensormux()
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


void direc_func(double dist ){
    current =dist;
    double interval= current - previous;
    double min = current / 10.0;
   printf("\n Distance is = %f\n", current);
    if(interval > min){
        dir=3;
        delay=300000;
    }
    else if(interval < (-min)){
        dir=1;
        delay=10000;
    }
    previous = current;
}

void* sensor_func(void* arg){
    int ret;
    int timeout = 2000;
    double rise_t=0; // rising edge time
    double fall_t=0; // falling edge time
    long diff_t;
    struct pollfd edge_poll;
    char* buff[64];

   sensormux();

   
  fd14 = open("/sys/class/gpio/gpio14/direction", O_WRONLY);
    if (fd14 < 0)
	printf("\n gpio14 direction file could not be opened");
    if (0> write(fd14,"in",2))
	printf(" \n in direction writing error fd14");
    fd14_val = open("/sys/class/gpio/gpio14/value", O_RDWR);
    if (fd14_val<0)
	   printf("\n gpio14 value file could not be opened");
	fd14_edge = open("/sys/class/gpio/gpio14/edge", O_WRONLY);
	if (fd14_edge< 0)
		printf("\n gpio14 value file could not be opened");
	fd13v = open("/sys/class/gpio/gpio13/value", O_WRONLY);
	if (fd13v< 0)
		printf("\n gpio13 value file could not be opened");
	
	edge_poll.fd = fd14_val;
	edge_poll.events = POLLPRI|POLLERR;
	edge_poll.revents=0;

    while(1){
	        fall_t=0;
	        rise_t=0;
			lseek(fd14_val,0,SEEK_SET);
			write(fd14_edge,"rising",6);
		        gpio_setvalue(13,0);
			usleep(14);
                        gpio_setvalue(13,1);
			ret=poll(&edge_poll,1,timeout);
	        if(ret>0){
			    if (edge_poll.revents & POLLPRI){
				     rise_t = timestamp();
				     read(fd14_val,buff,1);
			    }
	           
			}
		 lseek(fd14_val,0,SEEK_SET);
	 	      	write(fd14_edge,"falling",7);
			ret = poll(&edge_poll,1,timeout);
	        if(ret>0){
			if (edge_poll.revents & POLLPRI){
	        		fall_t= timestamp();
				read(fd14_val,buff,1);
	        }
	       
		}
  
    pthread_mutex_lock(&lock);
    diff_t = (long)(fall_t-rise_t);
    double local =(diff_t*340.00)/(4000000.00*2.00);
       direc_func(local);

    if(local<70)
			{
                   	for(i=0;i<16;i++)
				{
					seq[i] = seq1[i];
				}
			}
			else
			{
				for(i=0;i<16;i++)
				{
					seq[i] = seq2[i];
}
}




    pthread_mutex_unlock(&lock);
    usleep(500000);
    }
    return 0;
}

void * led_function()
{		
	int fd;
	fd= open(SPI_DEVICE, O_RDWR);
	
	ioctl(fd,1,sequence);
   
	

    while(1)
    {
    	pthread_mutex_lock(&lock1);
 
       write(fd,(void*)seq , sizeof(seq));
		
		pthread_mutex_unlock(&lock1);
	}
	close(fd);
	return 0;
}



int main()
{
pthread_t sensor_id;
pthread_t led_id;
pthread_mutex_init(&mutex, NULL);
pthread_create(&sensor_id, NULL, &sensor_func, NULL);
pthread_create(&led_id, NULL, &led_function, NULL);
pthread_join(sensor_id, NULL);
pthread_join(led_id, NULL);
return 0;
}



