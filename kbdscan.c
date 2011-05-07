/*
 * kbdscan.c
 *
 * Small program to set the FreeBSD console keyboard in raw scancode
 * mode and show scancodes of keys pressed and released.
 *
 * Five seconds of inactivity resets the console to the original mode
 * and quits the program.
 *
 * Copyright (c) 2010 Michael Cardell Widerkrantz <mc@hack.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/consio.h>
#include <sys/kbio.h>
#include <sys/select.h>
#include <sys/time.h>

/* Globals */
struct termios origtermios;

/* Functions */
int ttysave(int ttyfd);
int ttyreset(int ttyfd);
int xlatemode(void);
int scanmode(void);
void die(int status);
int ttyraw(int ttyfd);

/* Save original terminal settings. */
int ttysave(int ttyfd)
{
    if (0 < tcgetattr(ttyfd, &origtermios))
    {
        return -1;
    }

    return 0;
}

/* Reset original TTY mode. Be sure to call on exit. */
int ttyreset(int ttyfd)
{
    if (0 < tcsetattr(ttyfd, TCSAFLUSH, &origtermios))
    {
        return -1;
    }

    return 0;
}

int xlatemode(void)
{
    if (0 != ioctl(0, KDSKBMODE, K_XLATE))
    {
        perror("ioctl KDSKBMODE K_XLATE");
        return -1;        
    }

    return 0;
}

/* Set scancode mode on console keyboard. */
int scanmode(void)
{
    if (0 != ioctl(0, KDSKBMODE, K_CODE))
    {
        perror("ioctl KDSKBMODE K_CODE");
        return -1;
    }

    return 0;
}

/* Quit the program. Return the value status. */
void die(int status)
{
    /* Reset console to normal. */
    ttyreset(0);
    xlatemode();
    
    exit(status);
}

/*
 * Set terminal in raw mode, e.g. no processing of special characters,
 * no echo.
 */
int ttyraw(int ttyfd)
{
    struct termios raw;

    raw = origtermios;

    /* No break. */
    raw.c_iflag &= ~(BRKINT);

    /*
     * No echo, canonical treatment off, no extended function, no
     * signal characters.
     */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    /* Flush and turn on raw mode. */
    if (0 != tcsetattr(ttyfd, TCSAFLUSH, &raw))
    {
        perror("Can't set raw mode");
        return -1;
    }

    return 0;
}

int main(void)
{
    fd_set inset;
    struct timeval timeout;
    int ttyfd = 0; /* standard input */
    int key;
    
    /* FIXME: Set up signal handlers that calls ttyreset. */

    /* Save terminal setup. */
    if (0 < ttysave(ttyfd))
    {
        printf("Couldn't save terminal settings. Exiting...\n");
        die(-1);
    }
    
    /* Set terminal in raw mode, no echo. */
    if (0 != ttyraw(ttyfd))
    {
        die(-1);
    }

    /* Get raw scan codes from keyboard. */
    if (0 != scanmode())
    {
        printf("Please note that this program will only work in the FreeBSD "
               "console\n");
        fflush(stdout);
        die(-1);
    }
    
    for (;;)
    {
        int found;
        
        FD_ZERO(&inset);
        FD_SET(ttyfd, &inset);

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        found = select(ttyfd + 1, &inset, NULL, NULL, &timeout);
        if (-1 == found)
        {
            perror("select");
            die(-1);
        }

        if (0 == found)
        {
            /* Timeout. */
            printf("Timeout. Exiting...\n");
            die(0);
        }

        if (FD_ISSET(ttyfd, &inset))
        {
            ssize_t rc;

            if (0 > (rc = read(ttyfd, &key, 1)))
            {
                perror("read");
                die(-1);
            }
            else if (1 == rc)
            {
                /*
                 * The number of the key is transmitted as an 8 bit
                 * number with bit 7 as 0 when a key is pressed, and
                 * the number with bit 7 as 1 when released.
                */
                if (key & 0x80)
                {
                    printf("Scancode %u released.\r\n",
                           key & 0x7f);
                }
                else
                {
                    printf("Scancode %u pressed.\r\n", key & 0x7f);
                }

                fflush(stdout);
            }
        }

    } /* for */
    
    die(0);
}
