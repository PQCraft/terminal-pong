#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include <wchar.h>
#include <locale.h>

#ifndef ASCII
    /*
    Uncomment the following line or use `-D ASCII' when
    compiling to use ASCII instead of UTF-8.
    */
    //#define ASCII 
#endif

char VER[] = "0.2";

#ifndef ASCII
wchar_t* logo[] = {
    (wchar_t[]){L"████▄ ▗████▖ █▖  █ ▗███▙"},
    (wchar_t[]){L"█   █ ████▒█ ██▖ █ █▘"},
    (wchar_t[]){L"████▀ ██████ █▝█▖█ █  ██"},
    (wchar_t[]){L"█     ██████ █ ▝██ █▖  █"},
    (wchar_t[]){L"█     ▝████▘ █  ▝█ ▝███▛"}
};
#else
wchar_t* logo[] = {
    (wchar_t[]){L"####   @@@@  #   #  ####"},
    (wchar_t[]){L"#   # @@@@#@ ##  # #"},
    (wchar_t[]){L"####  @@@@@@ # # # #  ##"},
    (wchar_t[]){L"#     @@@@@@ #  ## #   #"},
    (wchar_t[]){L"#      @@@@  #   #  ####"},
};
#endif

struct timeval time1;

void delay(int ms) {
    uint64_t cms = 0;
    gettimeofday(&time1, NULL);
    uint64_t bms = cms = time1.tv_sec * 1000 + time1.tv_usec / 1000;
    while (cms < bms + ms) {
        gettimeofday(&time1, NULL);
        cms = time1.tv_sec * 1000 + time1.tv_usec / 1000;
    }
}

uint64_t timems() {
    uint64_t cms = 0;
    gettimeofday(&time1, NULL);
    return time1.tv_sec * 1000 + time1.tv_usec / 1000;
}

bool rawMode = false;

struct termios term, restore;
static struct termios orig_termios;

void enableRawMode() {
    if (rawMode) return;
    tcgetattr(0, &term);
    tcgetattr(0, &restore);
    term.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(0, TCSANOW, &term);
    rawMode = true;
}

void disableRawMode() {
    if (!rawMode) return;
    tcsetattr(0, TCSANOW, &restore);
    rawMode = false;
}

void c_exit(int err) {
    disableRawMode();
    exit(err);
}

void z_exit() {
    c_exit(0);
}

int height, width;

void getTermSize() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    height = w.ws_row;
    width = w.ws_col;
}

int kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;
    if (!initialized) {
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

char kbin[256];

void kbget() {
    int tmp;
    kbin[0] = 0;
    if (!(tmp = kbhit())) return;
    int obp = 0;
    while (obp < tmp) {
        kbin[obp] = 0;
        kbin[obp] = getchar();
        if (kbin[obp] == 0) {break;}
        obp++;
    }
    kbin[obp] = 0;
    /*
    for (int i = 0; i < obp; i++) {
        printf("[%d]: [%d]/{%c} ", i, kbin[i], kbin[i]);
        putchar('\n');
    }
    */
}

#ifndef ASCII
wchar_t boxchars[] = {L'─', L'│', L'┌', L'┐', L'└', L'┘'};
wchar_t bchar = L'●';
wchar_t pchar = L'█';
#else
wchar_t boxchars[] = {L'-', L'|', L'.', L'.', L'\'', L'\''};
wchar_t bchar = L'O';
wchar_t pchar = L'$';
#endif

void putlogo(int xp, int yp) {
    if (!xp || !yp) return;
    int size = sizeof(logo) / sizeof(wchar_t*);
    int yh = yp + size;
    printf("\e[s\e[%d;%dH", yp, xp);
    FILE* ret = freopen(NULL, "w", stdout);
    (void)ret;
    for (int i = 0; i < size; i++) {
        fputws(logo[i], stdout);
        yp++;
        wprintf(L"\e[%d;%dH", yp, xp);
    }
    ret = freopen(NULL, "w", stdout);
    (void)ret;
    printf("\e[u");
    fflush(stdout);
}

void putbox(int xp, int yp, int xs, int ys) {
    if (!xp || !yp || !xs || !ys) return;
    int xh = xp + xs, yh = yp + ys;
    printf("\e[s\e[%d;%dH", yp, xp);
    FILE* ret = freopen(NULL, "w", stdout);
    (void)ret;
    putwchar(boxchars[2]);
    for (int i = xp; i < xh - 2; i++) {
        putwchar(boxchars[0]);
    }
    putwchar(boxchars[3]);
    while (yp < yh - 2) {
        yp++;
        wprintf(L"\e[%d;%dH", yp, xp);
        putwchar(boxchars[1]);
        wprintf(L"\e[%d;%dH", yp, xh - 1);
        putwchar(boxchars[1]);        
    }
    yp++;
    wprintf(L"\e[%d;%dH", yp, xp);
    putwchar(boxchars[4]);
    for (int i = xp; i < xh - 2; i++) {
        putwchar(boxchars[0]);
    }
    putwchar(boxchars[5]);
    ret = freopen(NULL, "w", stdout);
    (void)ret;
    printf("\e[u");
    fflush(stdout);
}

void fancyClear() {
    bool* ridline = calloc(height * sizeof(bool), sizeof(bool));
    for (int i = 0; i < height; i++) {
        int rnum = -1;
        while (rnum == -1 || ridline[rnum - 1]) {rnum = (rand() % height) + 1;}
        ridline[rnum - 1] = true;
        printf("\e[s\e[%d;1H\e[2K\e[u", rnum);
        fflush(stdout);
        delay(50);
    }
    fflush(stdout);
    free(ridline);
}

int ppos[] = {0, 0};
double bx, by, obx, oby, bxs, bys;
int phpercent, pheight, speed, sdelay, score[] = {0, 0};

void drawScreen() {
    int pmax = ppos[0] + pheight + 2;
    FILE* ret = freopen(NULL, "w", stdout);
    (void)ret;
    wprintf(L"\e[s\e[%d;%dH ", (int)(oby + 2), (int)(obx + 2));
    wprintf(L"\e[%d;%dH", (int)(by + 2), (int)(bx + 2), bchar);
    putwchar(bchar);
    obx = bx;
    oby = by;
    wprintf(L"\e[2;6H");
    for (int i = 2; i < height; i++) {
        if (i - 1 > ppos[0] && i < pmax) {
            putwchar(pchar);
        } else {
            putwchar(L' ');
        }
        wprintf(L"\e[D\e[B");
    }
    pmax = ppos[1] + pheight + 2;
    wprintf(L"\e[2;%dH", width - 5);
    for (int i = 2; i < height; i++) {
        if (i - 1 > ppos[1] && i < pmax) {
            putwchar(pchar);
        } else {
            putwchar(L' ');
        }
        wprintf(L"\e[D\e[B");
    }
    ret = freopen(NULL, "w", stdout);
    (void)ret;
    printf("\e[u");
    fflush(stdout);
}

void setVars() {
    speed = 2;
    phpercent = 25;
    pheight = ((double)height - 2) / (100 / (double)phpercent);
    obx = bx = (width - 2) / 2 - 1;
    if (!(width % 2)) obx = bx += rand() % 2;
    oby = by = (height - 2) / 2 - 1;
    if (!(height % 2)) oby = by += rand() % 2;
    sdelay = 100 / speed;
    ppos[0] = ppos[1] = (height - 2) / 2 - (pheight + 1) / 2;
    bxs = ((rand() % 2) * 2 - 1) * ((rand() % 4) + 2);
    bys = (6 - abs(bxs)) * ((rand() % 2) * 2 - 1);
}

bool key(char k) {
    return (kbin[0] == k && !kbin[1]);
}

bool chkp(int p) {
    return (by >= ppos[p] && by <= ppos[p] + pheight);
}

int main(int argc, char* argv[]) {
    if (system("tty -s 1> /dev/null 2> /dev/null")) {
        char command[32768];
        strcpy(command, "xterm -T Pong -b 0 -bg black -bcn 200 -bcf 200 -e 'echo -e -n \"\x1b[\x33 q\" && ");
        strcat(command, argv[0]);
        strcat(command, "'");
        exit(system(command));
    }
    bool exit = false;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
                if (argc > 2) {fputs("Incorrect number of options passed.\n", stderr); c_exit(EINVAL);}
                printf("Terminal Pong version %s\n", VER);
                puts("Copyright (C) 2021 PQCraft");
                exit = true;
            } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
                if (argc > 2) {fputs("Incorrect number of options passed.\n", stderr); c_exit(EINVAL);}
                printf("Usage: %s [options] [{--file|-f}] [file] [options]\n", argv[0]);
                puts("Options:");
                puts("  -h, --help          Shows this help text.");
                puts("  -v, --version       Shows the version and copyright information.");
                exit = true;
            } else {
                fprintf(stderr, "Invalid option '%s'\n", argv[i]); c_exit(EINVAL);
            }
        } else {
            fprintf(stderr, "Invalid data '%s'\n", argv[i]); c_exit(EINVAL);
        }
    }
    fflush(stdout);
    if (exit) c_exit(0);
    getTermSize();
    if (width < 80 || height < 24) {
        fprintf(stderr, "\e[2K\e[0GSorry, you must have a terminal size of at least 80x24   \e[%dG:(\n", width - 1);
        c_exit(EIO);
    }
    setlocale(LC_CTYPE, "");
    signal(SIGINT, z_exit);
    gettimeofday(&time1, NULL);
    srand(time1.tv_usec + time1.tv_sec);
    enableRawMode();
    fancyClear();
    fflush(stdout);
    putbox(1, 1, width, height);
    putbox(width / 2 - 20, 4, 3, 3);
    putbox(width / 2 - 19, 5, 4, 4);
    putlogo(width / 2 - 11, 4);
    putbox(width / 2 + 17, 4, 3, 3);
    putbox(width / 2 + 18, 5, 4, 4);
    char* mtxt[] = {
        "ENTER = Play, ESC = Exit, BACKSPACE = Toggle Pause",
        "P1: q = Up, z = Down, Q = Up 1, Z = Down 1, a/A = Stop",
        "P2: p = Up, , = Down, P = Up 1, < = Down 1, l/L = Stop"
    };
    printf("\e[%d;%dH%s", height / 2 - 2, (int)(width / 2 - strlen(mtxt[0]) / 2) + 1, mtxt[0]);
    printf("\e[%d;%dH%s", height / 2 - 1, (int)(width / 2 - strlen(mtxt[1]) / 2) + 1, mtxt[1]);
    printf("\e[%d;%dH%s", height / 2, (int)(width / 2 - strlen(mtxt[2]) / 2) + 1, mtxt[2]);
    printf("\e[1;1H");
    fflush(stdout);
    while (1) {
        delay(25);
        kbget();
        if (kbin[0] == '\e' && !kbin[1]) {fancyClear(); c_exit(0);}
        if (kbin[strlen(kbin) - 1] == 10 || kbin[strlen(kbin) - 1] == 13) break;
    }
    fancyClear();
    putbox(1, 1, width, height);
    //cur:
    //A=up B=down D=left C=right
    setVars();
    drawScreen();
    uint64_t tval = timems() + sdelay;
    int d[] = {0, 0};
    int c[] = {0, 0};
    while (1) {
        kbget();
        if (key('\e')) break;
        if (key('Q') && ppos[0] > 0) {d[0] = -1; c[0] = 0;}
        if (key('Z') && ppos[0] + pheight < height - 2) {d[0] = 1; c[0] = 0;}
        if ((key('A') || key('a') && !kbin[1])) c[0] = 0;
        if ((key('q') || c[0] == 1) && ppos[0] > 0) {d[0] = -1; c[0] = 1;}
        if ((key('z') || c[0] == 2) && ppos[0] + pheight < height - 2) {d[0] = 1; c[0] = 2;}
        if (key('P') && ppos[1] > 0) {d[1] = -1; c[1] = 0;}
        if (key('<') && ppos[1] + pheight < height - 2) {d[1] = 1; c[1] = 0;}
        if (key('l') || key('L')) c[1] = 0;
        if ((key('p') || c[1] == 1) && ppos[1] > 0) {d[1] = -1; c[1] = 1;}
        if ((key(',') || c[1] == 2) && ppos[1] + pheight < height - 2) {d[1] = 1; c[1] = 2;}
        if (timems() > tval + sdelay) {
            ppos[0] += d[0];
            ppos[1] += d[1];
            d[0] = d[1] = 0;
            bx += (bxs / 5);
            if (bx <= 5 && chkp(0) && bxs < 0) {bxs *= -1; bx = 5; c[0] = 0;}
            if (bx > width - 8 && chkp(1) && bxs > 0) {bxs *= -1; bx = width - 8; c[1] = 0;}
            if (bx <= 0) {bxs *= -1; bx = 0;}
            if (bx > width - 3) {bxs *= -1; bx = width - 3;}
            by += (bys / 10);
            if (by <= 0) {bys *= -1; by = 0;}
            if (by > height - 3) {bys *= -1; by = height - 3;}
            drawScreen();
            tval = timems();
        }
    }
    fancyClear();
    c_exit(0);
}