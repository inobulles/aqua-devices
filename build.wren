// get options from environment variables

var devset = Meta.getenv("DEVSET")

if (devset == null) {
	devset = File.read("devset")
}

if (devset == null) {
	devset = "core"
}

File.write("devset", devset)

// compile all devices
// combine this with the creation of the installation map

var install = {}

var devices = File.list(devset, 1)
	.where { |path| path.startsWith("%(devset)/") }

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

// TODO testing

class Tests {
}

var tests = []
