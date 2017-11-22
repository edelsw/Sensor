#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <hidapi/hidapi.h>
#include <termios.h>
#include <ncurses.h>
#include <panel.h>
#include <pthread.h>

#define PID 0x5750
#define VID 0x0483

#define oops(msg) { strerror(msg); return 0; }

    WINDOW *win[2];
    PANEL *pan[2];

void printProgress (WINDOW *win, double percentage, int len)
{
    char *str;
    int i;
    str = malloc((int)percentage);
    memset(str, 0, (int)percentage);
    for (i = 0; i <= (int)percentage>>len; i++)
    {
       strcat(str, "|");
    }
    mvwprintw(win, 3, 10,"\r%.2f %.*s", percentage, 100, str);
    free(str);
}

char* parsing(unsigned char* str, int wlen, unsigned char *p)
{
int i, len=0;
char *r;
    for ( i = 0; i < wlen; i++, p++)
        len+=snprintf(str+len, sizeof(str)-len, "%x", *p);
    r = strchr(str, 'a');
    if (r != NULL)
        *r = '-';
    r = strchr(str, 'b');
    if (r != NULL)
        *r = '.';
    r = strchr(str, 'c');
    if (r != NULL)
        *r = '0';
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
    char buf[80];
    int wlen;
    struct termios *tty;
    unsigned char *p, str[6];
    int i;

    if ((tty = malloc(sizeof(struct termios))) == NULL) {
        printf("Error of allocation\n");
        return 0;
    }
    memset(tty, 0, sizeof(struct termios));

    while((fd = open(arg, O_RDWR | O_NOCTTY | O_SYNC)) < 0)
    {
        sleep(1);
    }
    tty->c_cc[VMIN] = 1;
    tty->c_cc[VTIME] = 5;
    tty->c_iflag &= IGNBRK;

    mvwprintw(win[0], 0, 18, "Temperature");
    doupdate();

    for ( ;; )
    {
       int len = 0;
       char *r;
       change_speed(fd, tty, B38400);
       wlen = write(fd, "O \0", 4);
        if (wlen != 4) {
            printf("Error from write: %d, %d\n", wlen, errno);
        }
        tcdrain(fd);
        change_speed(fd, tty, B115200);
        wlen = read(fd, buf, 4);
        if (wlen == 4) {
           p = buf;
           if (*p == 0x00)
              ++p;
           update_panels();
           printProgress(win[0], atof(parsing(str, wlen, p)), 0);
           doupdate();
        }
          usleep(500000);
    }
    close(fd);
}

void* pressure(void *arg)
{
    int wlen;
    char com[3] = {0x02, 'S', 0x00};
    unsigned char str[8], buf[8], *p;
    hid_device *handle;
    int i;

    memset(str, 0, sizeof(str));
    memset(buf, 0, sizeof(buf));
    while ((handle = hid_open(VID, PID, NULL)) == NULL)
    {
       sleep(1);
    }

    mvwprintw(win[1], 0, 15,"Absolute pressure");
    doupdate();

    for ( ;; )
    {
       int len = 0;
       char *r;
       hid_set_nonblocking(handle, 1);
       wlen = hid_write(handle, com, 3);
       wlen = hid_read(handle, buf, 8);
       if (buf[1] >= 3)
       {
           p = buf+2;
           update_panels();
           printProgress(win[1], atof(parsing(str, buf[1], p)), 4);
           doupdate();
       }
       usleep(500000);
    }
}

int main(int argc, char* argv[])
{
    pthread_t temp, press;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    if (!has_colors())
    {
        endwin();
        printf("Error of initialization colors\n");
        return -1;
    }
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLUE);
    init_pair(2, COLOR_GREEN, COLOR_RED);
    init_pair(3, COLOR_GREEN, COLOR_BLUE);

    win[0] = newwin(10, 50, 2, 10);
    wbkgdset(win[0], COLOR_PAIR(3));
    wclear(win[0]);
    wrefresh(win[0]);
    pan[0] = new_panel(win[0]);
    update_panels();

    win[1] = newwin(10, 50, 15, 10);
    wbkgdset(win[1], COLOR_PAIR(3));
    wclear(win[1]);
    wrefresh(win[1]);
    pan[1] = new_panel(win[1]);
    update_panels();

    pthread_create(&temp, NULL, temperature, argv[1]);
    pthread_create(&press, NULL, pressure, NULL);
    pthread_join(temp, NULL);
    pthread_join(press, NULL);
    free(win[0]);
    free(win[1]);
    endwin();
return 0;
}
