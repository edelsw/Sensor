#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ncurses.h>
#include <panel.h>
#include <form.h>
#include <menu.h>
#include <stdint.h>

void printProgress (WINDOW *win, double percentage)
{
    char *str;
    int i;
    str = malloc((int)percentage);
    memset(str, 0, (int)percentage);
    for (i = 0; i <= (int)percentage; i++)
    {
       strcat(str, "|");
    }
    mvwprintw(win, 3, 10,"\r%.2f %.*s", percentage, 100, str);
    free(str);
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
int main(int argc, char* argv[])
{
    int fd;
    char buf[80];
    int wlen;
    struct termios *tty;
    unsigned char *p, str[6];
    WINDOW *win;
    PANEL *pan;

    if ((tty = malloc(sizeof(struct termios))) == NULL) {
        printf("Error of allocation\n");
        return -1;
    }
    memset(tty, 0, sizeof(struct termios));
    fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", argv[1], strerror(errno));
        return -1;
    }
    tty->c_cc[VMIN] = 1;
    tty->c_cc[VTIME] = 5;
    tty->c_iflag &= IGNBRK;

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
    mvwprintw(win, 0, 48,"Temperature");
    doupdate();

    for ( ;; )
    {
       change_speed(fd, tty, B38400);
       wlen = write(fd, "O \0", 4);
        if (wlen != 4) {
            printf("Error from write: %d, %d\n", wlen, errno);
        }
        tcdrain(fd);    /* delay for output */
        change_speed(fd, tty, B115200);
        wlen = read(fd, buf, 4);
        if (wlen == 4) {
           p = buf;
           if (*p == NULL)
              ++p;
           sprintf(str, "%x.%x%x", *(p), *(p+1)&=~0xB0, *(p+2)>>4);
           update_panels();
           printProgress(win, atof(str));
           doupdate();
        }
          usleep(500000);
    }
    close(fd);
    free(win);
    endwin();
return 0;
}
