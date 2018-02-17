# AMX Crosspoint ProfiLab DLL

A DLL for ProfiLab to interact with a crosspoint from AMX.

The [documentation](docs/Documentation.md) explains what the DLL does in more detail.

## Building

[vcpkg](https://github.com/Microsoft/vcpkg) is required to get the dependencies. When installed run these commands:

	vcpkg install boost:x86-windows-static catch2:x86-windows-static
	cmake <source_dir> -DCMAKE_TOOLCHAIN_FILE=<vcpkg_dir>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows-static

### Tests

To successfully run the tests the following requirements must be satisfied:

* At least one serial port exists on the machine.
