# aqua-devices

This repository contains the source code for all AQUA device sets and devices for a range of different platforms.
Each top-level directory corresponds to a set of devices (a "device set" or "devset") which are meant to work together on a specific platform or configuration.

## Building

This repo will likely only ever serve as a pure dependency (e.g. for [`aqua-unix`](https://github.com/inobulles/aqua-unix)), but you can still build it yourself with [Bob the Builder](https://github.com/inobulles/bob) as such:

```console
bob build
```

This will build the `core` device set by default.
You can use the `DEVSET` environment variable to specify which device set you'd like to use:

```console
DEVSET=aquabsd.alps bob build
```

You can also use multiple devsets at once by separating each one with a comma (`,`):

```console
DEVSET=aquabsd.alps,aquabsd.black bob build
```

## Device sets

Detailed descriptions of device sets can be found in their respective README's, under the `src` directory.
The device sets provided by this repo include `core`, `aquabsd.alps`, and `aquabsd.black`.

### aquaBSD devices

These device sets are where all the devices needed for aquaBSD are gathered
They are kept in a separate device set as many devices interdepend on eachother and some are platform-specific (though most should work on Linux too).
