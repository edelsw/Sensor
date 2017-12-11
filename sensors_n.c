#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <hidapi/hidapi.h>
#include <termios.h>
#include <pthread.h>

#define PID 0x5750
#define VID 0x0483

struct param {
unsigned char name[80];
unsigned char trace[8];
};

const char speed[17] = "С 10 FREQ=10000\0";

char* parsing(unsigned char* str, int lenght, int wlen, unsigned char *p)
{
int i, len=0;
char *r;
    for ( i = 0; i < wlen; i++, p++)
        len+=snprintf(str+len, lenght-len, "%x", *p);
    r = strchr(str, 'a');
    if (r != NULL)
        *r = '-';
    r = strchr(str, 'b');
    if (r != NULL)
        *r = '.';
    r = strchr(str, 'c');
    if (r != NULL)
        *r = '0';
    r = strstr(str, "cc");
    if (r != NULL)
        sprintf(str, ".0");
return str;
}

void change_speed(int fd, struct termios *tty, speed_t speed)
{
    cfsetospeed(tty, speed);
    cfsetispeed(tty, speed);
    if (tcsetattr(fd, TCSANOW, tty) != 0) {
        printf("Error from tcsetattr\n");
        return;
    }
}

void* temperature(void* arg)
{
    int fd;
    char buf[8];
    int wlen;
    struct termios *tty;
    unsigned char *p, str[6];
    int i;
    struct param *tempP = (struct param *)arg;

    while((tty = malloc(sizeof(struct termios))) == NULL)
        usleep(500000);
    memset(tty, 0, sizeof(struct termios));

    while((fd = open(tempP->name, O_RDWR | O_NOCTTY | O_SYNC)) < 0)
        sleep(1);
    tty->c_cc[VMIN] = 1;
    tty->c_cc[VTIME] = 5;
    tty->c_iflag &= IGNBRK;

    change_speed(fd, tty, B38400);
    wlen = write(fd, speed, sizeof(speed));
    tcdrain(fd);

    wlen = write(fd, "I \0", 4);
    tcdrain(fd);
    wlen = write(fd, "S \0", 4);
    tcdrain(fd);

    for ( ;; )
    {
        change_speed(fd, tty, B115200);
        wlen = read(fd, buf, 6);
        if (wlen > 0 && wlen <= 6)
        {
           p = buf;
           if (*p == 0x00)
               ++p;
           if (!strcmp(tempP->trace, "trace"))
           {
               int i, len = 0;
               unsigned char string[7], *pointer;
               pointer = p;

               for ( i = 0; i < wlen; i++, pointer++)
                    len+=snprintf(string+len, sizeof(string)-len, "%x", *pointer);
               printf("\r%30.2f  %s", atof(parsing(str, sizeof(str), wlen, p)), string);
           }
           else
               printf("\r%30.2f", atof(parsing(str, sizeof(str), wlen, p)));
        }
           usleep(10000);
    }
    close(fd);
    free(tty);
}

void* pressure(void *arg)
{
    const char command[3] = {0x02, 'S', 0x00};
    int writelen;
    struct param *preP = (struct param *)arg;
    unsigned char string[8], buffer[8], *pointer;
    hid_device *handle;
    int count;

    memset(string, 0, sizeof(string));
    memset(buffer, 0, sizeof(buffer));
    while ((handle = hid_open(VID, PID, NULL)) == NULL)
    {
       sleep(1);
    }
       hid_set_nonblocking(handle, 1);
       writelen = hid_write(handle, speed, sizeof(speed));
       writelen = hid_write(handle, command, 3);

    for ( ;; )
    {
       writelen = hid_read(handle, buffer, sizeof(buffer));
       if (buffer[1] >= 3)
       {
           pointer = buffer+2;
           if (!strcmp(preP->trace, "trace"))
           {
               int i, len = 0;
               unsigned char str[8], *p;
               p = pointer;
               for ( i = 0; i < writelen; i++, p++)
                    len+=snprintf(str+len, sizeof(str)-len, "%x", *p);
               printf("\r%3.2f  %s", atof(parsing(string, sizeof(string), writelen, pointer)), str);
           }
           else
               printf("\r%3.2f", atof(parsing(string, sizeof(string), writelen, pointer)));
       }
       fflush(stdout);
       usleep(10000);
    }
    hid_close(handle);
}

int main(int argc, char* argv[])
{

struct param par;
    (argv[1]!= NULL) ? strcpy(par.name, argv[1]) : NULL;
    (argv[2]!=NULL) ? strcpy(par.trace, argv[2]) : NULL;
    pthread_t temp, press;
    printf("Absolute pressure     Temperature\n");
    pthread_create(&temp, NULL, temperature, &par);
    pthread_create(&press, NULL, pressure, &par);
    pthread_join(temp, NULL);
    pthread_join(press, NULL);
return 0;
}
