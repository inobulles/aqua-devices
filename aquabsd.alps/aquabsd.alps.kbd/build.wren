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

var src = File.list(".")
	.where { |path| path.endsWith(".c") }

src
	.each { |path| cc.compile(path) }

// create dynamic library

var linker = Linker.new()
linker.link(src.toList, [], "aquabsd.alps.kbd.vdev", true)

// TODO testing

class Tests {
}

var tests = []
