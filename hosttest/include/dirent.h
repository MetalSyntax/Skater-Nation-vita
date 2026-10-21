/* hosttest: en la newlib de VitaSDK struct dirent trae d_stat (SceIoStat), que
 * darwin.c usa para el tipo de entrada. Aquí readdir() la rellena con stat(). */
#ifndef HOSTTEST_DIRENT_H
#define HOSTTEST_DIRENT_H
#include_next <dirent.h>
#ifndef HOSTTEST_REAL_DIRENT
#include <psp2/io/stat.h>
struct hosttest_dirent { SceIoStat d_stat; char d_name[256]; };
struct hosttest_dirent *hosttest_readdir(DIR *dir);
#define dirent hosttest_dirent
#define readdir hosttest_readdir
#endif
#endif
