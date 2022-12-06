// C compilation

var cc = CC.new()

cc.add_opt("-I/usr/local/include")
cc.add_opt("-I..")
cc.add_opt("-fPIC")
cc.add_opt("-std=c99")
cc.add_opt("-Wall")
cc.add_opt("-Wextra")
cc.add_opt("-Werror")
// cc.add_opt("-DAQUABSD_ALPS_SVG_NEW_RSVG")

cc.add_lib("librsvg-2.0")

var src = File.list(".")
	.where { |path| path.endsWith(".c") }

src
	.each { |path| cc.compile(path) }

// create dynamic library

var linker = Linker.new(cc)
linker.link(src.toList, [], "aquabsd.alps.svg.vdev", true)

// TODO testing

class Tests {
}

var tests = []
