#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <hidapi/hidapi.h>
#include <ncurses.h>
#include <panel.h>

#define PID 0x5750
#define VID 0x0483

#define oops(msg) { strerror(msg); return -1; }

void printProgress (WINDOW *win, double percentage)
{
    char *str;
    int i;
    str = malloc((int)percentage);
    memset(str, 0, (int)percentage);
    for (i = 0; i <= (int)percentage>>4; i++)
    {
       strcat(str, "|");
    }
    mvwprintw(win, 3, 10,"\r%.2f %.*s", percentage, 100, str);
    free(str);
}

int main(int argc, char* argv[])
{

    int wlen;
    char com[3] = {0x02, 'S', 0x00};
    unsigned char str[8], buf[8], *p;
    hid_device *handle;
    int i;
    WINDOW *win;
    PANEL *pan;
    memset(str, 0, sizeof(str));
    memset(buf, 0, sizeof(buf));
    if ((handle = hid_open(VID, PID, NULL)) == NULL)
       oops(errno);
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    if (!has_colors())
    {
        endwin();
        printf("Error of initialization colors\n");
        return 1;
    }
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLUE);
    init_pair(2, COLOR_GREEN, COLOR_RED);
    init_pair(3, COLOR_BLUE, COLOR_GREEN);

    win = newwin(10, 106, 2, 10);
    wbkgdset(win, COLOR_PAIR(3));
    wclear(win);
    wrefresh(win);
    pan = new_panel(win);
    update_panels();
    mvwprintw(win, 0, 48,"Absolute pressure");
    doupdate();

    for ( ;; )
    {
       int len=0;
       char *r;
       hid_set_nonblocking(handle, 1);
       wlen = hid_write(handle, com, 3);
       wlen = hid_read(handle, buf, 8);
       if (buf[1] >= 3)
       {
           p = buf+2;
           for ( i = 0; i < buf[1]; i++, p++)
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
           update_panels();
           printProgress(win, atof(str));
           doupdate();
       }
       sleep(1);
    }
    free(win);
    endwin();
return 0;
}
