/* hosttest: ventana SDL2 en vivo (ver window_sdl.c). */
#ifndef HOSTTEST_WINDOW_SDL_H
#define HOSTTEST_WINDOW_SDL_H
#include <stdint.h>

int  hosttest_window_on(void);
void hosttest_window_frame(const uint32_t *abgr, int w, int h, int pitch);
void hosttest_window_pad(unsigned *buttons, uint8_t *lx, uint8_t *ly);
/* 1 si el ratón está pulsado; x, y en pantalla 960x544 */
int  hosttest_window_touch(int *x, int *y);

#endif
