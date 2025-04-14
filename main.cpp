#define BOOST_NO_CXX98_FUNCTION_BASE
// workaround unary_function in boost::geometry
#include <cstdint>
#include "mapbox/glyph_foundry.hpp"
#include "mapbox/glyph_foundry_impl.hpp"
#include <protozero/pbf_writer.hpp>

#include "ghc/filesystem.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>

using namespace std;

struct InputFont {
    string name;
    string version;
};

vector<InputFont> input_fonts = {
    {"NotoSansBengali-Regular", "1.multiscript"},
    {"NotoSansDevanagari-Regular", "1.multiscript"},
    {"NotoSansGujarati-Regular", "1.multiscript"},
    {"NotoSansGurmukhi-Regular", "1.multiscript"},
    {"NotoSansKannada-Regular", "1.multiscript"},
    {"NotoSansKhmer-Regular", "1.multiscript"},
    {"NotoSansMalayalam-Regular", "1.multiscript"},
    {"NotoSansMyanmar-Regular", "1.multiscript"},
    {"NotoSansOriya-Regular", "1.multiscript"},
    {"NotoSansTamil-Regular", "1.multiscript"},
    {"NotoSansTelugu-Regular", "1.multiscript"}
};

string output_name = "NotoSansMultiscript-Regular-v1";

struct PositionedGlyph {
    int index;
    int x_offset;
    int y_offset;
    int x_advance;
    int y_advance;
};

struct FontContainer
{
    FT_Library library;
    FT_Face face;
    char *data;
    string name;
    map<int, PositionedGlyph> encoding;
};

struct GlyphBuffer
{
    char *data;
    uint32_t size;
};

void load_encoding(const string& filename, FontContainer& f) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Error opening file: " + filename);
    }

    string line;
    // Skip the header line
    getline(file, line);

    while (getline(file, line)) {
        stringstream lineStream(line);
        string cell;
        PositionedGlyph positioned_glyph;

        getline(lineStream, cell, ',');
        positioned_glyph.index = stoi(cell);
        
        getline(lineStream, cell, ',');
        positioned_glyph.x_offset = stoi(cell);
        
        getline(lineStream, cell, ',');
        positioned_glyph.y_offset = stoi(cell);
        
        getline(lineStream, cell, ',');
        positioned_glyph.x_advance = stoi(cell);
        
        getline(lineStream, cell, ',');
        positioned_glyph.y_advance = stoi(cell);
        
        getline(lineStream, cell, ',');
        int codepoint = stoi(cell);

        f.encoding[codepoint] = positioned_glyph;
    }

    file.close();
}

void do_codepoint(FontContainer& f, protozero::pbf_writer &inner_range, FT_ULong char_code, bool &has_content)
{
    if (f.encoding.count(char_code) == 0)
    {
        // codepoint not present in encoding
        return;
    }
    PositionedGlyph positioned_glyph = f.encoding[char_code];
    
    sdf_glyph_foundry::glyph_info glyph;
    glyph.glyph_index = (FT_UInt)positioned_glyph.index;
    sdf_glyph_foundry::RenderSDF(glyph, 24, 3, 0.25, f.face);

    string glyph_data;
    protozero::pbf_writer glyph_message{glyph_data};

    glyph.top += positioned_glyph.y_offset;
    glyph.left += positioned_glyph.x_offset;
    // glyph.advance = encoding_unicode_to_x_advance[char_code];

    glyph_message.add_uint32(3, glyph.width);
    glyph_message.add_uint32(4, glyph.height);
    glyph_message.add_sint32(5, glyph.left);


    if (char_code > numeric_limits<uint32_t>::max())
    {
        throw runtime_error("Invalid value for char_code: too large");
    }
    else
    {
        glyph_message.add_uint32(1, static_cast<uint32_t>(char_code));
    }

    // node-fontnik uses glyph.top - glyph.ascender, assuming that the baseline
    // will be based on the ascender. However, Mapbox/MapLibre shaping assumes
    // a baseline calibrated on DIN Pro w/ ascender of ~25 at 24pt
    int32_t top = glyph.top - 25;
    if (top < numeric_limits<int32_t>::min() || top > numeric_limits<int32_t>::max())
    {
        throw runtime_error("Invalid value for glyph.top-25");
    }
    else
    {
        glyph_message.add_sint32(6, top);
    }

    if (glyph.advance < numeric_limits<uint32_t>::min() || glyph.advance > numeric_limits<uint32_t>::max())
    {
        throw runtime_error("Invalid value for glyph.top-glyph.ascender");
    }
    else
    {
        glyph_message.add_uint32(7, static_cast<uint32_t>(glyph.advance));
    }

    if (glyph.width > 0)
    {
        glyph_message.add_bytes(2, glyph.bitmap);
    }
    inner_range.add_message(3, glyph_data);
    has_content |= true;

}

string do_range(vector<FontContainer>& font_containers, unsigned start, unsigned end, bool &has_content)
{
    string inner_range_data;
    {
        protozero::pbf_writer inner_range{inner_range_data};
        inner_range.add_string(1, output_name);
        inner_range.add_string(2, to_string(start) + "-" + to_string(end));

        for (unsigned x = start; x <= end; x++)
        {
            FT_ULong char_code = x;
            for (int i = 0; i < font_containers.size(); ++i) {
                do_codepoint(font_containers[i], inner_range, x, has_content);
            }
        }
    }

    string range_data;
    {
        protozero::pbf_writer range{range_data};
        range.add_message(1, inner_range_data);
    }
    return range_data;
}

void init_font_container(FontContainer& f, string name)
{
    f.face = 0;
    f.data = nullptr;
    f.name = name.c_str();

    FT_Library library = nullptr;
    FT_Error error = FT_Init_FreeType(&library);

    f.library = library;
}

void load_face(FontContainer& f, string font_path)
{
    ifstream file(font_path, ios::binary | ios::ate);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    char *buffer = (char *)malloc(size);
    f.data = buffer;
    file.read(buffer, size);

    FT_Face face = 0;
    FT_Error face_error = FT_New_Memory_Face(f.library, (FT_Byte *)buffer, size, 0, &face);
    if (face_error)
    {
        throw runtime_error("Could not open font face");
    }
    if (face->num_faces > 1)
    {
        throw runtime_error("file has multiple faces; cowardly exiting");
    }
    if (!face->family_name)
    {
        throw runtime_error("face does not have family name");
    }
    double text_size = 24;
    FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(text_size * (1 << 6)), 0, 0);
    f.face = face;
}

void deinit_font_container(FontContainer& f)
{
    FT_Done_Face(f.face);
    free(f.data);
    FT_Done_FreeType(f.library);
}

GlyphBuffer generate_glyph_buffer(vector<FontContainer>& font_containers, uint32_t start_codepoint, bool &has_content)
{
    string result = do_range(font_containers, start_codepoint, start_codepoint + 255, has_content);

    GlyphBuffer g;
    char *result_ptr = (char *)malloc(result.size());
    result.copy(result_ptr, result.size());
    g.data = result_ptr;
    g.size = result.size();
    return g;
}

int main(int argc, char *argv[])
{
    string output_dir = "font/" + output_name;

    if (ghc::filesystem::exists(output_dir)) {
        ghc::filesystem::remove_all(output_dir);
    }

    ghc::filesystem::create_directory(output_dir);


    vector<FontContainer> font_containers = {};

    for (int input_font_index = 0; input_font_index < input_fonts.size(); ++input_font_index) {
        string name = input_fonts[input_font_index].name + "-v" + input_fonts[input_font_index].version;

        FontContainer f;
        init_font_container(f, name);

        string font_path = "vendor/pgf-encoding/fonts/" + input_fonts[input_font_index].name + ".ttf";
        load_face(f, font_path);

        string encoding_path = "vendor/pgf-encoding/encoding/" + name + ".csv";
        load_encoding(encoding_path, f);

        font_containers.push_back(f);
    }
    
    for (int i = 0; i < 65536; i += 256)
    {
        bool has_content = false;
        GlyphBuffer g = generate_glyph_buffer(font_containers, i, has_content);
 
        if (has_content) {
            ofstream output;
            string outname = output_dir + "/" + to_string(i) + "-" + to_string(i + 255) + ".pbf";
            output.open(outname);
            output.write(g.data, g.size);
            output.close();
        }

        free(g.data);
    }

    for (int input_font_index = 0; input_font_index < input_fonts.size(); ++input_font_index) {
        deinit_font_container(font_containers[input_font_index]);
    }

    return 0;
}
