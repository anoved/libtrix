# libtrix

A rudimentary C library for generating [STL files](https://en.wikipedia.org/wiki/STL_%28file_format%29) from triangle lists and vice versa.

## Limitations

`libtrix` enforces no topological rules. No checks are implemented to prohibit degenerate triangles, non-watertight meshes, or inconsistent surface orientation. It is considered the application's responsibility to generate or check for valid geometry.

## API

The `libtrix.h` header includes comments documenting the functions and data structures which comprise the `libtrix` interface.

## License

Freely distributed under an MIT License. See the `LICENSE` file for details.
