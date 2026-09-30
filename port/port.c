/*
 * port.c -- replaces the LSI-11 hardware that ISP talked to.
 * #included at the end of isp.c so it can see ISP's globals.
 *
 *   PARALLEL channel (DR-11 link to the host PDP-11/45)
 *       input  <- the rule file (re-read from the top on "@s" restart)
 *       output -> stdout      (synthesis lists / parameter tracks)
 *   SERIAL channel (console DLV-11)
 *       input  <- stdin       (utterances and @ directives)
 *       output -> stderr      (messages)
 *   Synthesizer interrupts (every 10 ms of speech)
 *       runsynth() calls the interrupt handler in a loop instead.
 *       For the PAT synthesizer each 10 ms frame is also rendered to a
 *       WAV file by a small parallel-formant synthesizer (patsynth below),
 *       standing in for the PAT hardware itself.
 */
#include <math.h>
#include <string.h>

#undef ldiv

struct devregs patdev, icldev;

static FILE *rulefp;
static const char *wavname = "isp.wav";

/* ---------------------------------------------------------------- */
/* character I/O                                                    */

char getdr(dev)
int dev;
{
    int c = getc(rulefp);
    if (c == EOF) {         /* host "resends" the table on the next read */
        rewind(rulefp);
        return '\0';
    }
    return (char)c;
}

putdr(w, dev)
char w;
int dev;
{
    putchar(w);
}

char jgetchar(channel)
int channel;
{
    int c;
    if (channel == PARALLEL) return getdr(0);
    c = getchar();
    if (c == EOF) {         /* end of input: finish cleanly */
        fflush(stdout);
        exit(0);
    }
    return (char)(c & 0177);
}

jputchar(c, channel)
char c;
int channel;
{
    if (c == '\r') return;  /* the terminals wanted CR LF */
    if (channel == PARALLEL) putdr(c, 0);
    else fputc(c, stderr);
}

/* ---------------------------------------------------------------- */
/* PDP-11 odds and ends                                             */

static int psw;
int mfps() { return psw; }
void mtps(p) int p; { psw = p; }

setprior(prior)
int prior;
{
    int old = psw & 0200;
    psw = prior ? (psw | 0200) : (psw & 0177);
    return old;
}

/* 32/16-bit divide: (hi<<16 | lo) / b, remainder left in ldivr */
int isp_ldiv(hi, lo, b)
int hi, lo, b;
{
    long n = ((long)hi << 16) | (lo & 0xffff);
    ldivr = (int)(n % b);
    return (int)(n / b);
}

/* ---------------------------------------------------------------- */
/* patsynth: render PAT frames to audio                             */
/*
 * A PAT frame, after patparm()'s shuffle, is 8 six-bit values:
 *     [0] f0  log scale 33..203 Hz      [4] f3  1550..3500 Hz
 *     [1] av  voicing amplitude         [5] as  sibilance amplitude
 *     [2] f1  100..1050 Hz              [6] fh  2000..7000 Hz
 *     [3] f2  500..2500 Hz              [7] ah  aspiration amplitude
 * (ranges from patcal[] in isp.c; amplitudes linear 0..63 per the paper).
 * The real PAT's internals are not given in the paper, so this is a
 * conventional cascade F1-F3 voice/aspiration branch plus a single
 * sibilance resonator at fh.
 */
#define SRATE 16000
#define FRAME (SRATE / 100)
#define PI 3.14159265358979

struct reson { double a, b, c, y1, y2; };

static void setres(struct reson *r, double f, double bw, int unitpeak)
{
    double t = 1.0 / SRATE;
    r->c = -exp(-2 * PI * bw * t);
    r->b = 2 * exp(-PI * bw * t) * cos(2 * PI * f * t);
    r->a = unitpeak ? (1 - exp(-PI * bw * t)) : 1 - r->b - r->c;
}

static double res(struct reson *r, double x)
{
    double y = r->a * x + r->b * r->y1 + r->c * r->y2;
    r->y2 = r->y1;
    r->y1 = y;
    return y;
}

static float *wav;
static long nwav, wavcap;
static double prev[NPHYS];
static int haveprev;
static struct reson r1, r2, r3, rh, gl1;
static double phase, lastout;
static unsigned long seed = 12345;

static double noise(void)
{
    seed = seed * 1103515245 + 12345;
    return ((seed >> 8) & 0xffff) / 32768.0 - 1.0;
}

static void emit(double x)
{
    if (nwav == wavcap) {
        wavcap = wavcap ? 2 * wavcap : SRATE * 10;
        wav = realloc(wav, wavcap * sizeof *wav);
        if (!wav) { fputs("out of memory\n", stderr); exit(1); }
    }
    wav[nwav++] = (float)x;
}

static void patframe(int *p)
{
    int n, k;
    double cur[NPHYS];

    for (k = 0; k < NPHYS; k++) cur[k] = p[k];
    if (!haveprev) { memcpy(prev, cur, sizeof prev); haveprev = 1; }
    for (n = 0; n < FRAME; n++) {
        double v[NPHYS], f0, av, f1, f2, f3, as, fh, ah, src, out;
        for (k = 0; k < NPHYS; k++) v[k] = prev[k] + (cur[k] - prev[k]) * n / FRAME;
        f0 = 33.0 * pow(203.0 / 33.0, v[0] / 64.0);
        av = v[1] / 63.0;
        f1 = 100 + v[2] * (1050 - 100) / 64.0;
        f2 = 500 + v[3] * (2500 - 500) / 64.0;
        f3 = 1550 + v[4] * (3500 - 1550) / 64.0;
        as = v[5] / 63.0;
        fh = 2000 + v[6] * (7000 - 2000) / 64.0;
        ah = v[7] / 63.0;
        if (n % 16 == 0) {  /* update filters every 1 ms */
            setres(&r1, f1, 60, 0);
            setres(&r2, f2, 90, 0);
            setres(&r3, f3, 150, 0);
            setres(&rh, fh, 900, 1);
            setres(&gl1, 0, 100, 0);    /* glottal pulse shaping, -12 dB/oct */
        }
        /* glottal source: pulse train through a 2-pole low-pass */
        phase += f0 / SRATE;
        src = 0;
        if (phase >= 1) { phase -= 1; src = 1; }
        src = res(&gl1, src * av * 2000);
        src += ah * noise() * 0.6;
        out = res(&r3, res(&r2, res(&r1, src)));
        src = out;
        out -= lastout;             /* lip radiation (first difference) */
        lastout = src;
        out +=res(&rh, as * noise()) * 1.5;
        emit(out);
    }
    memcpy(prev, cur, sizeof prev);
}

static void writewav(void)
{
    FILE *f;
    long i;
    double peak = 1e-9, g;
    unsigned char h[44];
    unsigned long datalen = nwav * 2;

    if (nwav == 0) return;
    for (i = 0; i < nwav; i++) if (fabs(wav[i]) > peak) peak = fabs(wav[i]);
    g = 0.9 * 32767 / peak;
    if ((f = fopen(wavname, "wb")) == 0) { perror(wavname); return; }
    memcpy(h, "RIFF\0\0\0\0WAVEfmt \20\0\0\0\1\0\1\0\0\0\0\0\0\0\0\0\2\0\20\0data\0\0\0\0", 44);
#define PUT32(o, v) (h[o] = (v) & 255, h[o+1] = ((v) >> 8) & 255, h[o+2] = ((v) >> 16) & 255, h[o+3] = ((v) >> 24) & 255)
    PUT32(4, 36 + datalen);
    PUT32(24, SRATE);
    PUT32(28, SRATE * 2);
    PUT32(40, datalen);
    fwrite(h, 1, 44, f);
    for (i = 0; i < nwav; i++) {
        long s = (long)(wav[i] * g);
        fputc(s & 255, f);
        fputc((s >> 8) & 255, f);
    }
    fclose(f);
    fprintf(stderr, "wrote %s (%.2f s)\n", wavname, (double)nwav / SRATE);
}

/* ---------------------------------------------------------------- */
/* interrupt emulation                                              */

void runsynth(s)
int s;
{
    int n;
    while (pd[s].state) {
        if (s == PATTYP && pd[s].clock >= -1) patframe(pd[s].parms);
        switch (s) {
        case PATTYP: pat(); break;
        case ICLTYP: icl(); break;
        case COMTYP: computalker(); break;
        }
    }
    if (s == PATTYP) {  /* 150 ms of silence between utterances */
        int z[NPHYS] = {0};
        for (n = 0; n < 15; n++) patframe(z);
        haveprev = 0;
    }
}

/* ---------------------------------------------------------------- */

#undef main
int main(argc, argv)
int argc;
char **argv;
{
    const char *rulename = "rule";
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r") && i + 1 < argc) rulename = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) wavname = argv[++i];
        else {
            fprintf(stderr, "usage: isp [-r rulefile] [-o out.wav] < utterances\n");
            return 2;
        }
    }
    if ((rulefp = fopen(rulename, "rb")) == 0) { perror(rulename); return 1; }
    atexit(writewav);
    isp_main();
    return 0;
}
