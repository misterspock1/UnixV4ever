/* Simple text editor for Unix V4 */

struct sgttyb {
    char sg_ispeed;
    char sg_ospeed;
    char sg_erase;
    char sg_kill;
    int sg_flags;
};

char *lineptrs[200];
int nlines;
int curx, cury;
int dirty;
char *filename;
struct sgttyb old_tty;

strlen(s)
char *s;
{
    int n;
    n = 0;
    while (s[n])
        n++;
    return n;
}

strcpy(to, from)
char *to, *from;
{
    while (*from) {
        *to = *from;
        to++;
        from++;
    }
    *to = 0;
}

strdup(s)
char *s;
{
    char *p;
    int len;
    
    len = strlen(s);
    p = sbrk(len + 1);
    if (p == -1)
        return 0;
    strcpy(p, s);
    return p;
}

putch(c)
char c;
{
    write(1, &c, 1);
}

puts(s)
char *s;
{
    write(1, s, strlen(s));
}

itoa(n, s)
int n;
char *s;
{
    int i, sign, j;
    char tmp[20];
    
    sign = n;
    if (sign < 0)
        n = -n;
    
    i = 0;
    while (1) {
        tmp[i] = n % 10 + '0';
        i++;
        n = n / 10;
        if (n == 0)
            break;
    }
    
    if (sign < 0) {
        tmp[i] = '-';
        i++;
    }
    
    j = 0;
    while (i > 0) {
        i--;
        s[j] = tmp[i];
        j++;
    }
    s[j] = 0;
}

clearscreen()
{
    putch(033);
    putch('[');
    putch('2');
    putch('J');
    putch(033);
    putch('[');
    putch('H');
}

moveto(x, y)
int x, y;
{
    char buffer[20];
    char numstr[10];
    int pos;
    
    buffer[0] = 033;
    buffer[1] = '[';
    itoa(y + 1, numstr);
    pos = 2;
    strcpy(buffer + pos, numstr);
    pos = strlen(buffer);
    buffer[pos] = ';';
    pos++;
    itoa(x + 1, numstr);
    strcpy(buffer + pos, numstr);
    pos = strlen(buffer);
    buffer[pos] = 'H';
    pos++;
    buffer[pos] = 0;
    puts(buffer);
}

cleareol()
{
    putch(033);
    putch('[');
    putch('K');
}

setupterm()
{
    int flags[3];
    
    gtty(0, flags);
    
    old_tty.sg_ispeed = flags[0] & 0377;
    old_tty.sg_ospeed = (flags[0] >> 8) & 0377;
    old_tty.sg_erase = flags[1] & 0377;
    old_tty.sg_kill = (flags[1] >> 8) & 0377;
    old_tty.sg_flags = flags[2];
    
    flags[2] = flags[2] & ~010;
    flags[2] = flags[2] | 040;
    
    stty(0, flags);
}

restoreterm()
{
    int flags[3];
    
    flags[0] = old_tty.sg_ispeed | (old_tty.sg_ospeed << 8);
    flags[1] = old_tty.sg_erase | (old_tty.sg_kill << 8);
    flags[2] = old_tty.sg_flags;
    
    stty(0, flags);
}

redraw()
{
    int i;
    char numstr[10];
    
    clearscreen();
    i = 0;
    while (i < nlines && i < 23) {
        moveto(0, i);
        if (lineptrs[i])
            puts(lineptrs[i]);
        cleareol();
        i++;
    }
    
    moveto(0, 23);
    puts("Line ");
    itoa(cury + 1, numstr);
    puts(numstr);
    puts("/");
    itoa(nlines, numstr);
    puts(numstr);
    puts("  ");
    puts(filename);
    if (dirty)
        puts(" [Mod]");
    puts("  ^S=Save ^Q=Quit");
    cleareol();
    
    moveto(curx, cury);
}

loadfile(fname)
char *fname;
{
    int fd, n, i, llen;
    char readbuf[512];
    char linebuf[256];
    
    fd = open(fname, 0);
    if (fd < 0)
        return;
    
    nlines = 0;
    llen = 0;
    
    while (1) {
        n = read(fd, readbuf, 512);
        if (n <= 0)
            break;
        i = 0;
        while (i < n) {
            if (readbuf[i] == 10) {
                linebuf[llen] = 0;
                if (nlines < 200)
                    lineptrs[nlines] = strdup(linebuf);
                nlines++;
                llen = 0;
            } else {
                if (llen < 255) {
                    linebuf[llen] = readbuf[i];
                    llen++;
                }
            }
            i++;
        }
    }
    
    if (llen > 0) {
        linebuf[llen] = 0;
        if (nlines < 200)
            lineptrs[nlines] = strdup(linebuf);
        nlines++;
    }
    
    close(fd);
}

savefile()
{
    int fd, i;
    char nl;
    
    fd = creat(filename, 0644);
    if (fd < 0) {
        moveto(0, 23);
        puts("Error!");
        cleareol();
        moveto(curx, cury);
        return;
    }
    
    nl = 10;
    i = 0;
    while (i < nlines) {
        if (lineptrs[i])
            write(fd, lineptrs[i], strlen(lineptrs[i]));
        write(fd, &nl, 1);
        i++;
    }
    
    close(fd);
    dirty = 0;
}

insertchar(c)
char c;
{
    char *oldptr, *newptr;
    int slen, ix, jx, idx;
    
    while (cury >= nlines && nlines < 200) {
        idx = nlines;
        lineptrs[idx] = strdup("");
        nlines++;
    }
    
    idx = cury;
    oldptr = lineptrs[idx];
    slen = 0;
    if (oldptr)
        slen = strlen(oldptr);
    
    newptr = sbrk(slen + 2);
    if (newptr == -1)
        return;
    
    ix = 0;
    while (ix < curx) {
        if (ix < slen) {
            newptr[ix] = oldptr[ix];
        } else {
            newptr[ix] = ' ';
        }
        ix++;
    }
    
    newptr[curx] = c;
    
    jx = curx;
    while (jx < slen) {
        ix = jx + 1;
        newptr[ix] = oldptr[jx];
        jx++;
    }
    
    ix = slen + 1;
    newptr[ix] = 0;
    
    lineptrs[idx] = newptr;
    curx++;
    dirty = 1;
}

deletechar()
{
    char *oldptr, *newptr;
    int slen, ix, idx, src, dst;
    
    if (cury >= nlines || curx == 0)
        return;
    
    idx = cury;
    oldptr = lineptrs[idx];
    if (oldptr == 0)
        return;
    
    slen = strlen(oldptr);
    if (curx > slen)
        return;
    
    newptr = sbrk(slen);
    if (newptr == -1)
        return;
    
    ix = 0;
    dst = 0;
    while (ix < curx - 1) {
        newptr[dst] = oldptr[ix];
        ix++;
        dst++;
    }
    
    src = curx;
    while (src < slen) {
        newptr[dst] = oldptr[src];
        src++;
        dst++;
    }
    
    newptr[dst] = 0;
    
    lineptrs[idx] = newptr;
    curx--;
    dirty = 1;
}

main(argc, argv)
int argc;
char **argv;
{
    int ch;
    char charbuf[2];
    
    if (argc < 2) {
        write(2, "Usage: edit filename\n", 21);
        exit(1);
    }
    
    filename = argv[1];
    nlines = 0;
    curx = 0;
    cury = 0;
    dirty = 0;
    
    loadfile(filename);
    if (nlines == 0) {
        lineptrs[0] = strdup("");
        nlines = 1;
    }
    
    setupterm();
    redraw();
    
    charbuf[1] = 0;
    while (1) {
        if (read(0, charbuf, 1) <= 0)
            break;
        ch = charbuf[0] & 0377;
        
        if (ch == 021) {
            break;
        } else if (ch == 023) {
            savefile();
            redraw();
        } else if (ch == 0177 || ch == 010) {
            deletechar();
            redraw();
        } else if (ch == 012 || ch == 015) {
            cury++;
            curx = 0;
            if (cury >= nlines && nlines < 200) {
                lineptrs[nlines] = strdup("");
                nlines++;
            }
            redraw();
        } else if (ch == 020) {
            if (cury > 0)
                cury--;
            redraw();
        } else if (ch == 016) {
            if (cury < nlines - 1)
                cury++;
            redraw();
        } else if (ch == 002) {
            if (curx > 0)
                curx--;
            redraw();
        } else if (ch == 006) {
            curx++;
            redraw();
        } else if (ch >= 32 && ch < 127) {
            insertchar(ch);
            redraw();
        }
    }
    
    clearscreen();
    moveto(0, 0);
    restoreterm();
    exit(0);
}