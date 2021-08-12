# aqua-devices

This repository contains the source code for all core AQUA devices for a range of different platforms.
Each branch corresponds to a set of devices which are meant to work together on a specific platform or configuration.

Using `aqua-unix`, you can switch to a different device branch by running:

```shell
% aqua-unix --update --devbranch different-branch
```

Here is a quick overview of each device branch:

## Core devices

`core` is the branch in which core devices are gathered.
Core devices are devices which are meant to be common between all branches, and can practically be assumed to exist on any platform.

## Devices for aquaBSD 1.0 Alps

`aquabsd.alps` is where all the devices needed for aquaBSD 1.0 Alps are gathered.
They are kept in a seperate branch as many devices depend on one another and some are platform-specific.
