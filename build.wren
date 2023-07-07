// get options from environment variables

var devset = Meta.getenv("DEVSET")
var just_read = false

if (devset == null) {
	devset = File.read("devset")
	just_read = devset != null
}

if (devset == null) {
	devset = "core"
}

devset = devset.trim()

if (!just_read) {
	File.write("devset", devset + "\n")
}

// compile all devices
// combine this with the creation of the installation map

var install = {}
var devset_stack = devset.split(",")

while (devset_stack.count > 0) {
	devset = devset_stack.removeAt(0) // TODO just make this .pop in Bob's dialect of Wren

	var devices = File.list(devset, 1)
		.where { |path| path.startsWith("%(devset)/") }

	Meta.setenv("DEVSET_INC_PATH", "%(Meta.cwd())/%(devset)")

	devices.each { |path|
		if (File.bob(path, ["build"]) != 0) {
			return
		}

		var name = path.split("/")[-1]
		var filename = name + ".vdev"

		install[filename] = "share/aqua/devices/%(filename)"
	}
}

// TODO testing

class Tests {
}

var tests = []
