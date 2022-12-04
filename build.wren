// install dependencies

Deps.git("https://github.com/inobulles/umber")
Deps.git("https://github.com/inobulles/iar")

// compile all devices
// combine this with the creation of the installation map

var install = {}
var devset = "core"

var devices = File.list(devset, 1)
	.where { |path| path.startsWith("%(devset)/%(devset).") }

devices.each { |path|
	if (File.bob(path, ["build"]) != 0) {
		return
	}

	var name = path.split("/")[-1]
	var filename = name + ".vdev"

	install[filename] = "%(OS.prefix())/share/aqua/devices/%(filename)"
}

System.print(install)

// TODO testing

class Tests {
}

var tests = []
