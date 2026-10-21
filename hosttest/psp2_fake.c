/* hosttest (Skater Nation): las llamadas sce* y kubridge del loader iOS sobre
 * Linux ARM32 softfp (Debian armel), para arrancar el mismo loader en Docker
 * sin consola. Ver ../hosttest/run.sh y "Prueba-ARM32-sin-consola.md".
 *
 * Rutas: el loader usa "ux0:data/skaternation/..." tal cual; run.sh crea el
 * enlace /ux0:data -> /data y trabaja con cwd=/ (Linux admite ':' en nombres).
 */
/* psp2 antes que glibc: <sys/stat.h> define st_atime como macro. */
#include <psp2/apputil.h>
#include <psp2/audioout.h>
#include <psp2/common_dialog.h>
#include <psp2/ctrl.h>
#include <psp2/ime_dialog.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/rtc.h>
#include <psp2/system_param.h>
#include <psp2/touch.h>
#include <kubridge.h>

#define HOSTTEST_REAL_DIRENT   /* este archivo implementa la versión de include/dirent.h */
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#include "window_sdl.h"

/* glibc define st_mtime como st_mtim.tv_sec: estorba en SceIoStat */
#undef st_atime
#undef st_mtime
#undef st_ctime

static uint64_t now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* ── archivos ─────────────────────────────────────────────── */

static void fill_stat(SceIoStat *st, const struct stat *s)
{
    struct tm tm;
    memset(st, 0, sizeof *st);
    st->st_size = s->st_size;
    st->st_mode = (S_ISDIR(s->st_mode) ? SCE_S_IFDIR : SCE_S_IFREG) | SCE_S_IRUSR | SCE_S_IWUSR;
    gmtime_r(&s->st_mtim.tv_sec, &tm);
    st->st_mtime.year = tm.tm_year + 1900; st->st_mtime.month = tm.tm_mon + 1; st->st_mtime.day = tm.tm_mday;
    st->st_mtime.hour = tm.tm_hour; st->st_mtime.minute = tm.tm_min; st->st_mtime.second = tm.tm_sec;
}

int sceIoMkdir(const char *dir, SceMode mode) { return mkdir(dir, mode) == 0 || errno == EEXIST ? 0 : -1; }
int sceIoRmdir(const char *dir) { return rmdir(dir) == 0 ? 0 : -1; }
int sceIoRemove(const char *file) { return unlink(file) == 0 ? 0 : -1; }
int sceIoGetstat(const char *file, SceIoStat *st)
{
    struct stat s;
    if (stat(file, &s) != 0) return -1;
    fill_stat(st, &s);
    return 0;
}

SceUID sceIoOpen(const char *file, int flags, SceMode mode)
{
    int f = 0, fd;
    if ((flags & SCE_O_RDWR) == SCE_O_RDWR) f = O_RDWR;
    else if (flags & SCE_O_WRONLY) f = O_WRONLY;
    if (flags & SCE_O_CREAT) f |= O_CREAT;
    if (flags & SCE_O_TRUNC) f |= O_TRUNC;
    if (flags & SCE_O_APPEND) f |= O_APPEND;
    fd = open(file, f, mode ? mode : 0666);
    return fd < 0 ? -1 : fd + 1;   /* SceUID 0 no es válido */
}
int sceIoClose(SceUID fd) { return close(fd - 1); }
int sceIoRead(SceUID fd, void *buf, SceSize n) { return (int)read(fd - 1, buf, n); }
int sceIoWrite(SceUID fd, const void *buf, SceSize n) { return (int)write(fd - 1, buf, n); }

SceUID sceIoDopen(const char *dirname)
{
    DIR *d = opendir(dirname);
    return d ? (SceUID)(intptr_t)d : -1;
}
int sceIoDread(SceUID fd, SceIoDirent *dir)
{
    struct dirent *e;
    DIR *d = (DIR *)(intptr_t)fd;
    struct stat s;
    char path[1024];
    do {
        if (!(e = readdir(d))) return 0;
    } while (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."));
    memset(dir, 0, sizeof *dir);
    snprintf(dir->d_name, sizeof dir->d_name, "%s", e->d_name);
    snprintf(path, sizeof path, "/proc/self/fd/%d/%s", dirfd(d), e->d_name);
    if (stat(path, &s) == 0) fill_stat(&dir->d_stat, &s);
    return 1;
}
int sceIoDclose(SceUID fd) { return closedir((DIR *)(intptr_t)fd); }

/* include/dirent.h: struct dirent con d_stat, como la newlib de VitaSDK */
struct hosttest_dirent { SceIoStat d_stat; char d_name[256]; };
struct hosttest_dirent *hosttest_readdir(DIR *d)
{
    static __thread struct hosttest_dirent out;
    if (!sceIoDread((SceUID)(intptr_t)d, (SceIoDirent *)&out)) return NULL;
    return &out;
}

/* ── sistema ──────────────────────────────────────────────── */

void *sceClibMemset(void *dst, int ch, SceSize len) { return memset(dst, ch, len); }

int scePowerSetArmClockFrequency(int f) { (void)f; return 0; }
int scePowerSetBusClockFrequency(int f) { (void)f; return 0; }
int scePowerSetGpuClockFrequency(int f) { (void)f; return 0; }
int scePowerSetGpuXbarClockFrequency(int f) { (void)f; return 0; }

int sceAppUtilInit(SceAppUtilInitParam *i, SceAppUtilBootParam *b) { (void)i; (void)b; return 0; }
int sceAppUtilSystemParamGetInt(unsigned int id, int *value)
{
    *value = id == SCE_SYSTEM_PARAM_ID_ENTER_BUTTON ? SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS : 1; /* 1 = inglés */
    return 0;
}
int sceCommonDialogSetConfigParam(const SceCommonDialogConfigParam *p) { (void)p; return 0; }

/* El teclado del sistema: siempre "cancelado" (no hay IME). */
int sceImeDialogInit(const SceImeDialogParam *p) { (void)p; return 0; }
SceCommonDialogStatus sceImeDialogGetStatus(void) { return SCE_COMMON_DIALOG_STATUS_FINISHED; }
int sceImeDialogGetResult(SceImeDialogResult *r) { memset(r, 0, sizeof *r); r->button = SCE_IME_DIALOG_BUTTON_CLOSE; return 0; }
int sceImeDialogTerm(void) { return 0; }

int sceRtcGetCurrentClockLocalTime(SceDateTime *t)
{
    struct timespec ts;
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);
    t->year = tm.tm_year + 1900; t->month = tm.tm_mon + 1; t->day = tm.tm_mday;
    t->hour = tm.tm_hour; t->minute = tm.tm_min; t->second = tm.tm_sec;
    t->microsecond = ts.tv_nsec / 1000;
    return 0;
}

SceUInt64 sceKernelGetProcessTimeWide(void) { return now_us(); }
int sceKernelDelayThread(SceUInt delay) { usleep(delay); return 0; }
int sceKernelExitProcess(int res)
{
    fprintf(stderr, "[HOSTTEST] sceKernelExitProcess(%d)\n", res);
    fflush(NULL);
    _exit(res);
}

/* ── hilos y semáforos ───────────────────────────────────── */

typedef struct { pthread_t t; SceKernelThreadEntry entry; SceSize stack; void *args; SceSize arglen; } fake_thread;

static void *thread_main(void *arg)
{
    fake_thread *ft = arg;
    return (void *)(intptr_t)ft->entry(ft->arglen, ft->args);
}
SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int prio, SceSize stack,
                             SceUInt attr, int affinity, const SceKernelThreadOptParam *opt)
{
    fake_thread *ft = calloc(1, sizeof *ft);
    (void)name; (void)prio; (void)attr; (void)affinity; (void)opt;
    ft->entry = entry;
    ft->stack = stack;
    return (SceUID)(intptr_t)ft;
}
int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp)
{
    fake_thread *ft = (fake_thread *)(intptr_t)thid;
    pthread_attr_t a;
    /* la Vita copia los argumentos a la pila del hilo */
    ft->arglen = arglen;
    if (arglen && argp) { ft->args = malloc(arglen); memcpy(ft->args, argp, arglen); }
    pthread_attr_init(&a);
    pthread_attr_setstacksize(&a, ft->stack < 65536 ? 65536 : ft->stack);
    return pthread_create(&ft->t, &a, thread_main, ft) ? -1 : 0;
}
int sceKernelWaitThreadEnd(SceUID thid, int *stat, SceUInt *timeout)
{
    void *ret;
    (void)timeout;
    pthread_join(((fake_thread *)(intptr_t)thid)->t, &ret);
    if (stat) *stat = (int)(intptr_t)ret;
    return 0;
}
int sceKernelDeleteThread(SceUID thid) { free((void *)(intptr_t)thid); return 0; }
int sceKernelGetThreadId(void) { return (int)(pthread_self() & 0x7FFFFFFF); }

SceUID sceKernelCreateSema(const char *name, SceUInt attr, int init, int max, SceKernelSemaOptParam *opt)
{
    sem_t *s = malloc(sizeof *s);
    (void)name; (void)attr; (void)max; (void)opt;
    sem_init(s, 0, (unsigned)init);
    return (SceUID)(intptr_t)s;
}
int sceKernelWaitSema(SceUID id, int need, SceUInt *timeout)
{
    (void)timeout;
    while (need-- > 0) sem_wait((sem_t *)(intptr_t)id);
    return 0;
}
int sceKernelSignalSema(SceUID id, int n)
{
    while (n-- > 0) sem_post((sem_t *)(intptr_t)id);
    return 0;
}
int sceKernelDeleteSema(SceUID id) { sem_destroy((sem_t *)(intptr_t)id); free((void *)(intptr_t)id); return 0; }

/* ── kubridge: memoria ejecutable en una dirección fija ─────── */

/* SceUID es int: una dirección >= 0x80000000 sería "error". Índice en una tabla. */
static void *s_blocks[64];
static int s_nblocks;
int sceKernelGetMemBlockBase(SceUID uid, void **base)
{
    if (uid < 1 || uid > s_nblocks) return -1;
    *base = s_blocks[uid - 1];
    return 0;
}

SceUID kuKernelAllocMemBlock(const char *name, SceKernelMemBlockType type, SceSize size,
                             SceKernelAllocMemBlockKernelOpt *opt)
{
    void *want = opt && (opt->attr & 1) ? (void *)(uintptr_t)opt->field_C : NULL;
    void *p = mmap(want, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS | (want ? MAP_FIXED_NOREPLACE : 0), -1, 0);
    (void)type;
    if (p == MAP_FAILED) {
        fprintf(stderr, "[HOSTTEST] kuKernelAllocMemBlock(%s, %p, 0x%X): %s\n", name, want, size, strerror(errno));
        return -1;
    }
    if (s_nblocks == 64) return -1;
    s_blocks[s_nblocks++] = p;
    return s_nblocks;
}
int kuKernelCpuUnrestrictedMemcpy(void *dst, const void *src, SceSize len) { memcpy(dst, src, len); return 0; }
void kuKernelFlushCaches(const void *ptr, SceSize len) { __builtin___clear_cache((char *)ptr, (char *)ptr + len); }

/* ── mando y táctil (ventana: teclado y ratón) ─────────────── */

int sceCtrlSetSamplingMode(SceCtrlPadInputMode mode) { (void)mode; return 0; }
int sceCtrlPeekBufferPositive(int port, SceCtrlData *pad, int count)
{
    (void)port; (void)count;
    memset(pad, 0, sizeof *pad);
    pad->lx = pad->ly = pad->rx = pad->ry = 128;
    hosttest_window_pad(&pad->buttons, &pad->lx, &pad->ly);
    return 1;
}
int sceTouchSetSamplingState(SceUInt32 port, SceTouchSamplingState state) { (void)port; (void)state; return 0; }
int sceTouchPeek(SceUInt32 port, SceTouchData *data, SceUInt32 n)
{
    int x, y;
    (void)n;
    memset(data, 0, sizeof *data);
    data->timeStamp = now_us();
    if (port == SCE_TOUCH_PORT_FRONT && hosttest_window_touch(&x, &y)) {
        data->reportNum = 1;
        data->report[0].id = 1;
        data->report[0].x = (uint16_t)(x * 2);   /* panel 1920x1088 = pantalla x2 */
        data->report[0].y = (uint16_t)(y * 2);
    }
    return 1;
}

/* ── audio: se descarta al ritmo real ─────────────────────── */

static int s_audio_len[8], s_audio_freq[8];
int sceAudioOutOpenPort(SceAudioOutPortType type, int len, int freq, SceAudioOutMode mode)
{
    static int next;
    int port = next++ & 7;
    (void)type; (void)mode;
    s_audio_len[port] = len;
    s_audio_freq[port] = freq;
    return port;
}
int sceAudioOutOutput(int port, const void *buf)
{
    (void)buf;
    usleep((useconds_t)(1000000LL * s_audio_len[port & 7] / (s_audio_freq[port & 7] ? s_audio_freq[port & 7] : 48000)));
    return 0;
}
int sceAudioOutReleasePort(int port) { (void)port; return 0; }

/* ── crashes: lo que en la Vita da el .psp2dmp ──────────────── */

static void on_crash(int sig, siginfo_t *si, void *uctx)
{
    ucontext_t *uc = uctx;
    unsigned long pc = uc->uc_mcontext.arm_pc, lr = uc->uc_mcontext.arm_lr;
    Dl_info a = {0}, b = {0};
    dladdr((void *)pc, &a);
    dladdr((void *)lr, &b);
    fprintf(stderr, "[CRASH] señal %d pc=0x%08lX (%s) lr=0x%08lX (%s) addr=%p\n", sig,
            pc, a.dli_fname ? a.dli_fname : "?", lr, b.dli_fname ? b.dli_fname : "?", si->si_addr);
    /* Código del juego iOS: Mach-O cargado en 0x98000000 (MACHO_SLIDE) */
    if (pc >= 0x98000000u) fprintf(stderr, "  pc en el Mach-O: 0x%08lX\n", pc - 0x98000000u);
    if (lr >= 0x98000000u) fprintf(stderr, "  lr en el Mach-O: 0x%08lX\n", lr - 0x98000000u);
    fprintf(stderr, "  r0=%08lX r1=%08lX r2=%08lX r3=%08lX sp=%08lX\n", uc->uc_mcontext.arm_r0,
            uc->uc_mcontext.arm_r1, uc->uc_mcontext.arm_r2, uc->uc_mcontext.arm_r3, uc->uc_mcontext.arm_sp);
    fflush(NULL);
    _exit(128 + sig);
}

__attribute__((constructor)) static void install_crash_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_crash;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}
