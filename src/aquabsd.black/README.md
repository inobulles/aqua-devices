### aquaBSD 2.0 Black Forest

Note that aquaBSD 1.0 Alps also depends on this now, not just 2.0 Black Forest.
This is what each devices does:

|Name|Description|
|-|-|
|`aquabsd.black.iraster`|Maybe?|
|`aquabsd.black.ivector`|Maybe?|
|`aquabsd.black.ui`|Provides AQUA apps with a way to create consistent and pretty user interfaces.|
|`aquabsd.black.vk`|Maybe?|
|`aquabsd.black.win`|Provides a way to create windows on aquaBSD. Only supports Wayland as of now, use `aquabsd.alps.win` for X11 support.|
|`aquabsd.black.wgpu`|Interface to the [WebGPU API](https://developer.mozilla.org/en-US/docs/Web/API/WebGPU_API). Also provides a way to create native surfaces from aquaBSD windows (`aquabsd.alps.win`).|
|`aquabsd.black.wm`|Creates Wayland compositors and provides mechanisms for managing windows.|
