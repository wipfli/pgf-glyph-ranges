# Positioned Glyph Font

The assumption of a one-to-one mapping between glyphs and Unicode codepoints that usually exists in MapLibre font files is given up in a positioned glyph font. Instead, codepoints are used as indices of positioned glyphs.

Read more about how traditional MapLibre text rendering works here: https://github.com/wipfli/about-text-rendering-in-maplibre

## Demo

A positioned glyph font demo for several languages written in the Devanagari script and based on the Protomaps basemap is available here:

* [Nepali](https://wipfli.github.io/positioned-glyph-font/examples/protomaps/name-ne.html)
* [Hindi](https://wipfli.github.io/positioned-glyph-font/examples/protomaps/name-hi.html)
* [Marathi](https://wipfli.github.io/positioned-glyph-font/examples/protomaps/name-mr.html)

This demo uses the positioned-glyph-font-devanagari [branch](https://github.com/wipfli/basemaps/tree/positioned-glyph-font-devanagari) which has some modification to the basemaps Planetiler profile.

If you find bugs in the demo, please open an [Issue](https://github.com/wipfli/positioned-glyph-font/issues).

## Steps

Download Devanagari corpus from https://github.com/wipfli/word-corpus to the `corpus` folder. You should get the following 3 files:

- `corpus/osm-devanagari-corpus.txt`
- `corpus/wikidata-devanagari-corpus.txt`
- `corpus/wikipedia-devanagari-corpus.txt`

There is a docker image with the dependencies for Raqm, Harfbuzz, and font-maker.

```bash
docker build -t positioned-glyph-font .
```

```bash
docker run --rm -it -v "$(pwd)":/root/ positioned-glyph-font
```

Build raqm with:

```bash
./compile_raqm.sh
```

You should now have the file `run_raqm.so`.

Build the parameter space files with:

```bash
python3 build_parameter_space.py
```

You should now have these 3 files:

- `parameter_space/osm-devanagari-parameter-space.csv`
- `parameter_space/wikidata-devanagari-parameter-space.csv`
- `parameter_space/wikipedia-devanagari-parameter-space.csv`

Analyze and downsample the parameter space files with:

```bash
python3 analyze_parameter_space.py
```

You should now have the 3 files:

- `parameter_space/osm-devanagari-downsampled-parameter-space.csv`
- `parameter_space/wikidata-devanagari-downsampled-parameter-space.csv`
- `parameter_space/wikipedia-devanagari-downsampled-parameter-space.csv`


Generate encoding with:

```bash
python3 generate_encoding.py
```

You should now have the 2 files:

- `encoding.csv`
- `font-maker/encoding.hpp`

Create font pbf files with:

```bash
cd font-maker
docker run --rm -it -v "$(pwd)":/root/ positioned-glyph-font
./run.sh
```

This command will generate the font `.pbf` files in `font-maker/output_dir/the-name`.

Run the encoding http server with:

```bash
python3 server.py
```

To test the encoding server, open a web browser and go to the URL `http://localhost:3002/काठमाण्डौँ` (Kathmandu in Nepali). You should get this response: `इऀ।ऄऀ॰फ੏` which is the encoded label.
