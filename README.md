
# noodlec
![CMake Workflow](https://github.com/cadenmiller/noodlec/actions/workflows/cmake-multi-platform.yml/badge.svg)

![Noodle Logo](https://raw.githubusercontent.com/cadenmiller/noodlec/cc84de0fc0f7911c4fc080a8a9359c4168ba38c4/Images/noodle.svg)

Welcome to the Noodle C99 Parser project! This repository contains a C99 parser designed for the Noodle data language. Noodle is a lightweight language for data representation. This parser lets you seamlessly integrate Noodle data structures into C applications.

## Features

- Parse Noodle data files into C99 data structures.
- Intuitive API for navigating and manipulating parsed Noodle data.
- Lightweight and minimal dependencies.

## Getting Started

1. Clone this repository: `git clone https://github.com/cadenmiller/noodlec.git`
2. Navigate to the project directory: `cd noodlec`
3. Build the parser library using CMake.
4. Link with your C project.

## Usage

1. Include `"noodle.h"` in your code.
2. Initialize by creating a root `Noodle_t` using noodleParse().
4. Work with the parsed data using the functions noodleFrom() and noodleAt().
5. Free resources with `noodleFree()`.

## Example

```c
#include <stdio.h>
#include "noodle.h"

int main() {
    NoodleGroup_t* settings = noodleParseFromFile("settings.noodle");
    if (!parser) return EXIT_FAILURE;

    bool valid = false;
    float volume = noodleFloatFrom(settings, "volume", &valid);
    int vulkanDeviceId = noodleIntFrom(settings, "vulkanDeviceId", &valid);  

    if (!valid) return EXIT_FAILURE;

    // And it's just that easy, use the parsed data here!
    // ...
    
    noodleFree(settings);
    return 0;
}
```

## Contributing

Feel free to contribute and stick to continuing the current code styling.

## License

This project is licensed under the MIT License.
