# This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
# Copyright (c) 2023 Aymeric Wibo

from datetime import datetime

AQUABSD_ALPS_WIN_DEVICE = "aquabsd.alps.win"
AQUABSD_ALPS_WIN_DEVICE_HEADER_INCLUDE = "aquabsd.alps/win/public.h"
AQUABSD_BLACK_WIN_DEVICE = "aquabsd.black.win"
AQUABSD_BLACK_WIN_DEVICE_HEADER_INCLUDE = "aquabsd.black/win/win.h"
AQUABSD_BLACK_WM_DEVICE = "aquabsd.black.wm"
AQUABSD_BLACK_WM_DEVICE_HEADER_INCLUDE = "aquabsd.black/wm/wm.h"
PACKED = "__attribute__((packed))"
CMD_SURFACE_FROM_WIN = "0x0000"
CMD_SURFACE_FROM_WM = "0x0001"
CMD_WGPU_BASE = 0x1000

with open("webgpu.h") as f:
	lines = map(str.rstrip, f.readlines())

count = CMD_WGPU_BASE

cmds = ""
impls = ""

c_types = ""
c_wrappers = ""

for line in lines:
	if not line.startswith("WGPU_EXPORT "):
		c_types += line + "\n"
		continue

	type_and_name, args = line.split('(')
	_, *return_type, name = type_and_name.split()
	return_type = ' '.join(return_type)

	raw_args = args.split(')')[0]
	args = raw_args.split(", ")
	arg_names = []

	for arg in args:
		*type_, arg_name = arg.split()
		arg_names.append(arg_name)

	cmd_id = f"0x{count:04x}"

	# device source

	cmds += f"\tCMD_{name} = {cmd_id},\n"

	args_struct = ";\n\t\t\t".join(args)
	args_call = ", ".join(map(lambda name: f"args->{name}", arg_names))
	ret = "" if return_type == "void" else f"return (uint64_t) "

	impls += f"""
	else if (cmd == CMD_{name}) {{
		struct {{
			{args_struct};
		}} {PACKED}* const args = data;

		{ret}{name}({args_call});
	}}
"""

	# C library source

	args_struct = ";\n\t\t".join(args)
	args_assign = ",\n\t\t".join(map(lambda name: f".{name} = {name}", arg_names))
	ret = "" if return_type == "void" else f"return ({return_type}) "

	c_wrappers += f"""
AQUA_C_FN {return_type} {name}({raw_args}) {{
	struct {{
		{args_struct};
	}} {PACKED} const args = {{
		{args_assign},
	}};

	{ret}send_device(wgpu_device, {cmd_id}, (void*) &args);
}}
"""

	count += 1

# device source

year = datetime.now().year

dev_out = f"""// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) {year} Aymeric Wibo

// this file is automatically generated by 'aqua-devices/aquabsd.black/wgpu/gen.py'
// if you need to update this, read the 'aqua-devices/aquabsd.black/wgpu/README.md' document

#include <stdint.h>
#include <string.h>

#include "webgpu.h"

#include <{AQUABSD_ALPS_WIN_DEVICE_HEADER_INCLUDE}>
#include <{AQUABSD_BLACK_WIN_DEVICE_HEADER_INCLUDE}>
#include <{AQUABSD_BLACK_WM_DEVICE_HEADER_INCLUDE}>

typedef enum {{
	CMD_SURFACE_FROM_WIN = {CMD_SURFACE_FROM_WIN},
	CMD_SURFACE_FROM_WM = {CMD_SURFACE_FROM_WM},

	// WebGPU commands

{cmds}}} cmd_t;

uint64_t (*kos_query_device) (uint64_t, uint64_t name);
void* (*kos_load_device_function) (uint64_t device, const char* name);
uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

static uint64_t aquabsd_alps_win_device = -1;

int load(void) {{
	aquabsd_alps_win_device = kos_query_device(0, (uint64_t) "{AQUABSD_ALPS_WIN_DEVICE}");

	if (aquabsd_alps_win_device != (uint64_t) -1) {{
		aquabsd_alps_win_get_draw_win = kos_load_device_function(aquabsd_alps_win_device, "get_draw_win");
	}}

	return 0;
}}

uint64_t send(uint16_t _cmd, void* data) {{
	cmd_t const cmd = _cmd;

	if (cmd == CMD_SURFACE_FROM_WIN) {{
		struct {{
			WGPUInstance instance;
			win_t* win;
		}} {PACKED}* const aquabsd_black_win_args = data;

		if (strcmp(aquabsd_black_win_args->win->aquabsd_black_win_signature, AQUABSD_BLACK_WIN_SIGNATURE) == 0) {{
			struct {{
				WGPUInstance instance;
				win_t* win;
			}} {PACKED}* const aquabsd_black_win_args = data;

			WGPUSurfaceDescriptorFromWaylandSurface const descr_from_wayland = {{
				.chain = (WGPUChainedStruct const) {{
					.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface,
				}},
				.display = aquabsd_black_win_args->win->display,
				.surface = aquabsd_black_win_args->win->surface,
			}};

			WGPUSurfaceDescriptor const descr = {{
				.nextInChain = (WGPUChainedStruct const*) &descr_from_wayland,
			}};

			WGPUSurface const surface = wgpuInstanceCreateSurface(aquabsd_black_win_args->instance, &descr);
			return (uint64_t) surface;
		}}

		struct {{
			WGPUInstance instance;
			aquabsd_alps_win_t* win;
		}} {PACKED}* const aquabsd_alps_win_args = data;

		WGPUSurfaceDescriptorFromXcbWindow const descr_from_xcb = {{
			.chain = (WGPUChainedStruct const) {{
				.sType = WGPUSType_SurfaceDescriptorFromXcbWindow,
			}},
			.connection = aquabsd_alps_win_args->win->connection,
			.window = aquabsd_alps_win_args->win->win,
		}};

		WGPUSurfaceDescriptor const descr = {{
			.nextInChain = (WGPUChainedStruct const*) &descr_from_xcb,
		}};

		WGPUSurface const surface = wgpuInstanceCreateSurface(aquabsd_alps_win_args->instance, &descr);
		return (uint64_t) surface;
	}}

	else if (cmd == CMD_SURFACE_FROM_WM) {{
		struct {{
			WGPUInstance instance;
			wm_t* wm;
		}} {PACKED}* const args = data;

		WGPUSurfaceDescriptorFromDrmFd const descr_from_drm_fd = {{
			.chain = (WGPUChainedStruct const) {{
				.sType = WGPUSType_SurfaceDescriptorFromDrmFd,
			}},
			.fd = args->wm->drm_fd,
		}};

		WGPUSurfaceDescriptor const descr = {{
			.nextInChain = (WGPUChainedStruct const*) &descr_from_drm_fd,
		}};

		WGPUSurface const surface = wgpuInstanceCreateSurface(args->instance, &descr);
		return (uint64_t) surface;
	}}

	{impls}
	return -1;
}}
"""

with open("main.c", "w") as f:
	f.write(dev_out)

# C library source

lib_out = f"""// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) {year} Aymeric Wibo

// this file is automatically generated by 'aqua-devices/aquabsd.black/wgpu/gen.py'
// if you need to update this, read the 'aqua-devices/aquabsd.black/wgpu/README.md' document

#pragma once

#if !defined(AQUABSD_ALPS_WIN) && !defined(AQUABSD_BLACK_WIN)
#error "You must first either include <aquabsd/alps/win.h> or <aquabsd/black/win.h> before including <aquabsd/black/wgpu.h>"
#endif

#include <root.h>

#include "wgpu_types.h"

static device_t wgpu_device = NO_DEVICE;

AQUA_C_FN int wgpu_init(void) {{
	wgpu_device = query_device("aquabsd.black.wgpu");

	if (wgpu_device == NO_DEVICE) {{
		return ERR_NO_DEVICE;
	}}

	return SUCCESS;
}}

AQUA_C_FN WGPUSurface wgpu_surface_from_win(WGPUInstance instance, win_t* win) {{
	struct {{
		WGPUInstance instance;
		void* win;
	}} {PACKED} const args = {{
		.instance = instance,
		.win = (void*) win->internal_win,
	}};

	return (WGPUSurface) send_device(wgpu_device, {CMD_SURFACE_FROM_WIN}, (void*) &args);
}}

#if defined(AQUABSD_BLACK_WM)
AQUA_C_FN WGPUSurface wgpu_surface_from_wm(WGPUInstance instance, wm_t* wm) {{
	struct {{
		WGPUInstance instance;
		void* wm;
	}} {PACKED} const args = {{
		.instance = instance,
		.wm = (void*) wm->internal_wm,
	}};

	return (WGPUSurface) send_device(wgpu_device, {CMD_SURFACE_FROM_WM}, (void*) &args);
}}
#endif
{c_wrappers}"""

with open("wgpu.h", "w") as f:
	f.write(lib_out)

with open("wgpu_types.h", "w") as f:
	f.write(c_types)
