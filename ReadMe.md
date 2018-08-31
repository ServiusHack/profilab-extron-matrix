# Extron-Matrix ProfiLab DLL

A DLL for [ProfiLab](http://www.abacom-online.de/html/profilab.html) to interact with Extron Matrix switchers.

The [documentation](docs/Documentation.md) explains what the DLL does in more detail.

## Building

[vcpkg](https://github.com/Microsoft/vcpkg) is required to get the dependencies. When installed run these commands:

	vcpkg install boost-algorithm:x86-windows-static boost-format:x86-windows-static boost-asio:x86-windows-static boost-asio:x86-windows-static catch2:x86-windows-static
	cmake <source_dir> -DCMAKE_TOOLCHAIN_FILE=<vcpkg_dir>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows-static

### Tests

Some test cases require at least one serial port to be present on the machine. These are tagged with `[hardware-required]`.
