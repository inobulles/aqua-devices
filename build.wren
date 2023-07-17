var DEVSET_ENV = "DEVSET"
var DEVSET_FILE = "devset"
var DEFAULT_DEVSET = "core"
var DEPS_FILE = "deps"

// get options from environment variables

var devset = Meta.getenv(DEVSET_ENV)
var just_read = false

if (devset == null) {
	devset = File.read(DEVSET_FILE)
	just_read = devset != null
}

if (devset == null) {
	devset = DEFAULT_DEVSET
}

devset = devset.trim()

if (!just_read) {
	File.write(DEVSET_FILE, devset + "\n")
}

// compile all devsets
// combine this with the creation of the installation map

var install = {}
var devset_stack = devset.split(",")

Meta.setenv("DEVSET_INC_PATH", "%(Meta.cwd())/src")

while (devset_stack.count > 0) {
	devset = devset_stack.removeAt(0) // TODO just make this .pop in Bob's dialect of Wren

	// read dependent devsets and add them to the stack

	var deps = File.read("src/%(devset)/%(DEPS_FILE)").trim().split(",")

	if (deps[0] != "") {
		devset_stack = devset_stack + deps // TODO allow +=
	}

	// compile all devices of devset

	var devices = File.list("src/%(devset)", 1)
		.where { |path| path.startsWith("src/%(devset)/") && !path.endsWith(DEPS_FILE) }

	devices.each { |path|
		if (File.bob(path, ["build"]) != 0) {
			return
		}

		var name = path.split("/")[-1]
		var filename = "%(devset).%(name).vdev"

		install[filename] = "share/aqua/devices/%(filename)"
	}
}

// TODO testing

class Tests {
}

var tests = []
