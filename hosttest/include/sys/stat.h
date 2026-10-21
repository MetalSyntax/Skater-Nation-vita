/* hosttest: glibc define st_mtime como st_mtim.tv_sec; en los archivos que usan
 * SceIoStat (CMakeLists: HOSTTEST_SCE_STAT) esos macros rompen sus campos. */
#include_next <sys/stat.h>
#ifdef HOSTTEST_SCE_STAT
#undef st_atime
#undef st_mtime
#undef st_ctime
#endif
