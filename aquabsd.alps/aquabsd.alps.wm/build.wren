// C compilation

var cc = CC.new()

var inc_path = Meta.getenv("DEVSET_INC_PATH")
cc.add_opt("-I%(inc_path)")

cc.add_opt("-fPIC")
cc.add_opt("-std=c99")
cc.add_opt("-Wall")
cc.add_opt("-Wextra")
cc.add_opt("-Werror")

cc.add_lib("x11")
cc.add_lib("x11-xcb")
cc.add_lib("xcb")
cc.add_lib("xcb-composite")
cc.add_lib("xcb-ewmh")
cc.add_lib("xcb-icccm")
cc.add_lib("xcb-randr")
cc.add_lib("xcb-shape")
cc.add_lib("xcb-xfixes")

var src = File.list(".")
	.where { |path| path.endsWith(".c") }

src
	.each { |path| cc.compile(path) }

// create dynamic library

var linker = Linker.new()
linker.link(src.toList, [], "aquabsd.alps.wm.vdev", true)

// TODO testing

class Tests {
}

var tests = []
