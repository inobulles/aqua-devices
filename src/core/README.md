### Core devices

`core` is the device set in which core devices are gathered.
Core devices are devices which are meant to be common between all other device sets, and can practically be assumed to exist on any platform.
This is what each device does:

|Name|Description|
|-|-|
|`core.fs`|Handles interactions with the filesystem.|
|`core.log`|Provides logging functionality through [Umber](https://github.com/inobulles/umber).|
|`core.math`|Provides complex mathematical functions.|
|`core.pkg`|Provides an interface to retrieve information about installed AQUA apps and read metadata from them.|
|`core.time`|Gives the time.|
