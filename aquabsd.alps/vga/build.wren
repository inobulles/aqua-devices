// C compilation

var cc = CC.new()

var inc_path = Meta.getenv("DEVSET_INC_PATH")
cc.add_opt("-I%(inc_path)")

cc.add_opt("-I/usr/local/include")
cc.add_opt("-fPIC")
cc.add_opt("-std=c99")
cc.add_opt("-Wall")
cc.add_opt("-Wextra")
cc.add_opt("-Werror")
cc.add_opt("-D_DEFAULT_SOURCE")

if (Meta.getenv("WITHOUT_X11") != null) {
	cc.add_opt("-DWITHOUT_X11")

} else { // TODO putting the if's closing brace on the same line as the else isn't allowed in Wren - fix this
	cc.add_lib("xcb")
	cc.add_lib("xcb-icccm")
	cc.add_lib("xcb-image")
	cc.add_lib("xcb-shm")
	cc.add_lib("xcb-xfixes")
	cc.add_lib("xcb-xinput")
}

var src = File.list(".")
	.where { |path| path.endsWith(".c") }

src
	.each { |path| cc.compile(path) }

// create dynamic library

var linker = Linker.new()
linker.link(src.toList, [], "aquabsd.alps.vga.vdev", true)

// TODO testing

class Tests {
}

var tests = []
