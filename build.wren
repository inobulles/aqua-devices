// compile all devices
// combine this with the creation of the installation map
// TODO how should we be able to send custom options, such as the devset?

var install = {}
var devset = "core"

var devices = File.list(devset, 1)
	.where { |path| path.startsWith("%(devset)/%(devset).") }

devices.each { |path|
	if (Meta.instruction() != "install" && Meta.instruction() != "test" /* TODO technically this should work, problem is the devices obviously don't each have an installation map so... they don't know how to build a testing environment */) {
		if (File.bob(path, [Meta.instruction()]) != 0) {
			return
		}
	}

	var name = path.split("/")[-1]
	var filename = name + ".vdev"

	install[filename] = "%(Meta.prefix())/share/aqua/devices/%(filename)"
}

System.print(install)

// TODO testing

class Tests {
}

var tests = []
