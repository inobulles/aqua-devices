// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

// C compilation

var cc = CC.new()

var inc_path = Meta.getenv("DEVSET_INC_PATH")
cc.add_opt("-I%(inc_path)")

cc.add_opt("-DWM_WITH_RENDERER_WGPU")

cc.add_opt("-I/usr/local/include")
cc.add_opt("-I/usr/local/wlroots-devel/include")
cc.add_opt("-fPIC")
cc.add_opt("-std=c99")
cc.add_opt("-Wall")
cc.add_opt("-Wextra")
cc.add_opt("-Werror")
cc.add_opt("-g")

cc.add_lib("libdrm")
cc.add_lib("pixman-1")

var src = File.list(".")
	.where { |path| path.endsWith(".c") }

src
	.each { |path| cc.compile(path) }

// create dynamic library

var linker = Linker.new()
linker.link(src.toList, ["m", "wayland-client", "drm", "wlroots"], "aquabsd.black.wm.vdev", true)

// TODO testing

class Tests {
}

var tests = []
