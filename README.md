# pgf-glyph-ranges

Positioned Glyph Font Glyph Ranges for MapLibre GL JS and MapLibre Native

## Folders

- `/`: contains code to generate pgf glyph ranges
- `/font/` contains MapLibre glyphs ranges
- `/vendor/` contains submodules to compile font-maker

## Steps

Build a docker image for compiling and running font-maker:

```bash
docker build -t positioned-glyph-font .
```

Get the git submodules:

```bash
git submodule update --init --recursive
```

Configure the font and version you want to generate glyph ranges for at the beginning of `main.cpp`.

```cpp
vector<InputFont> input_fonts = {
    {"NotoSansDevanagari-Regular", "1"}
};

string output_name = "NotoSansDevanagari-Regular";
```

Note that only font names and versions that are contained the `vendor/pgf-encoding` submodule are supported. This submodule points to [wipfli/pgf-encoding](https://github.com/wipfli/pgf-encoding).


Create the font PBF files:

```bash
docker run --rm -it -v "$(pwd)":/root/ positioned-glyph-font /bin/bash /root/run.sh
```

You should now have some `.pbf` glyph ranges in the `font/NotoSansDevanagari-Regular-v1` folder.

## License

- The code in this repo is based on [maplibre/font-maker](https://github.com/maplibre/font-maker) and is published under the [BSD-3-Clause License](./LICENSE.txt).
- The glyph ranges in the `font/` folder are published under the [SIL Open Font License](./font/OFL.txt).
