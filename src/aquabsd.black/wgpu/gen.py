# This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
# Copyright (c) 2023 Aymeric Wibo

WIN_DEVICE = "aquabsd.alps.win"
PACKED = "__attribute__((packed))"
CMD_SURFACE_FROM_WIN = "0x0000"
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
static {return_type} {name}({raw_args}) {{
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

dev_out = f"""// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

// this file is automatically generated by 'aqua-devices/aquabsd.black/wgpu/gen.py'
// if you need to update this, read the 'aqua-devices/aquabsd.black/wgpu/README.md' document

#include <stdint.h>
#include "webgpu.h"

#include <{WIN_DEVICE}/public.h>

typedef enum {{
	CMD_SURFACE_FROM_WIN = {CMD_SURFACE_FROM_WIN},

	// WebGPU commands

{cmds}}} cmd_t;

uint64_t (*kos_query_device) (uint64_t, uint64_t name);
void* (*kos_load_device_function) (uint64_t device, const char* name);
uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

static uint64_t win_device = -1;

int load(void) {{
	win_device = kos_query_device(0, (uint64_t) "{WIN_DEVICE}");

	if (win_device != (uint64_t) -1) {{
		aquabsd_alps_win_get_draw_win = kos_load_device_function(win_device, "get_draw_win");
	}}

	return 0;
}}

uint64_t send(uint16_t _cmd, void* data) {{
	cmd_t const cmd = _cmd;

	if (cmd == CMD_SURFACE_FROM_WIN) {{
		struct {{
			WGPUInstance instance;
			aquabsd_alps_win_t* win;
		}} {PACKED}* const args = data;

		WGPUSurfaceDescriptorFromXcbWindow const descr_from_xcb = {{
			.chain = (WGPUChainedStruct const) {{
				.sType = WGPUSType_SurfaceDescriptorFromXcbWindow,
			}},
			.connection = args->win->connection,
			.window = args->win->win,
		}};

		WGPUSurfaceDescriptor const descr = {{
			.nextInChain = (WGPUChainedStruct const*) &descr_from_xcb,
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
// Copyright (c) 2023 Aymeric Wibo

// this file is automatically generated by 'aqua-devices/aquabsd.black/wgpu/gen.py'
// if you need to update this, read the 'aqua-devices/aquabsd.black/wgpu/README.md' document

#pragma once

#include <root.h>
#include <aquabsd/alps/win.h>

#include "wgpu_types.h"

static device_t wgpu_device = -1;

static int wgpu_init(void) {{
	wgpu_device = query_device("aquabsd.black.wgpu");

	if (wgpu_device == NO_DEVICE) {{
		return -ERR_NO_DEVICE;
	}}

	return SUCCESS;
}}

static WGPUSurface wgpu_surface_from_win(WGPUInstance instance, win_t* win) {{
	struct {{
		WGPUInstance instance;
		void* win;
	}} {PACKED} const args = {{
		.instance = instance,
		.win = (void*) win->internal_win,
	}};

	return (WGPUSurface) send_device(wgpu_device, {CMD_SURFACE_FROM_WIN}, (void*) &args);
}}
{c_wrappers}"""

with open("wgpu.h", "w") as f:
	f.write(lib_out)

with open("wgpu_types.h", "w") as f:
	f.write(c_types)