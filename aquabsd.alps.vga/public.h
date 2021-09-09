#if !defined(__AQUABSD_ALPS_VGA)
#define __AQUABSD_ALPS_VGA

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 'aquabsd_alps_vga_mode_t' struct must be identical to the 'vga_mode_t' struct in 'aqua-lib' ('c/aquabsd/alps/vga.h')

typedef struct {
	uint64_t id;

	uint64_t text;

	uint64_t width, height;
	uint64_t bpp, fps;
} aquabsd_alps_vga_mode_t;

static int (*aquabsd_alps_vga_mode_count) (void);
static aquabsd_alps_vga_mode_t* (*aquabsd_alps_vga_modes) (void);

static void (*aquabsd_alps_vga_set_mode) (aquabsd_alps_vga_mode_t* mode);
static void* (*aquabsd_alps_vga_framebuffer) (void);

static int (*aquabsd_alps_vga_flip) (void);

#endif
