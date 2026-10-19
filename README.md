# The client application for [scheda-informationis](https://github.com/JRD-HTLWY/scheda-informationis)

Used technologies are:
- wxWidgets
- make
- JSON
- CMAKE
- jsoncons
- asio
- C++
- wxFormBuilder

## Build

Requirements:

- CMake 3.16 or newer
- A C++17 compiler and Make
- wxWidgets, Asio, and jsoncons installed with CMake package configuration files

Configure and build from the project root:

```sh
cmake -S . -B build
cmake --build build --parallel
```

Run the application:

```sh
./build/MeinProgramm
```

If the dependencies are installed outside CMake's default search paths, provide
their installation prefixes during configuration:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/dependencies"
```

The wxWidgets package must provide the `wx::core` and `wx::base` targets; Asio
and jsoncons must provide `asio::asio` and `jsoncons::jsoncons` respectively.
