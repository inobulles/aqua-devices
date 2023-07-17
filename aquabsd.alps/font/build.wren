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

cc.add_lib("cairo")
cc.add_lib("pango")
cc.add_lib("pangocairo")

var src = File.list(".")
	.where { |path| path.endsWith(".c") }

src
	.each { |path| cc.compile(path) }

// create dynamic library

var linker = Linker.new()

linker.add_lib("cairo")
linker.add_lib("pango")
linker.add_lib("pangocairo")

linker.link(src.toList, [], "aquabsd.alps.font.vdev", true)

// TODO testing

class Tests {
}

var tests = []
