def get_next_available_codepoint(current_codepoint):
    i = current_codepoint + 1

    def is_allowed(codepoint):
        if 0x0900 <= codepoint <= 0x0DFF:
            return True
        if 0x0F00 <= codepoint <= 0x109F:
            return True
        if 0x1780 <= codepoint <= 0x17FF:
            return True
        return False
    
    while i < 2 ** 16 and not is_allowed(i):
        i += 1
    
    if i == 2 ** 16:
        print('Error: Did not find any free codepoint.')
        exit()
    
    return i

def generate_encoding(source):
    glyphs = [] # most frequent last, least frequent first

    downsampled_parameter_space_path = f'parameter_space/{source}-devanagari-downsampled-parameter-space.csv'

    with open(downsampled_parameter_space_path) as f:
        # skip csv header
        line = f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            line = line.strip()
            index, x_offset, y_offset, x_advance, y_advance, _ = [int(num) for num in line.split(',')]
            glyph = (index, x_offset, y_offset, x_advance, y_advance)
            glyphs.append(glyph)

    glyph_to_unicode_encoding = {}
    codepoint = -1
    for glyph in reversed(glyphs):
        codepoint = get_next_available_codepoint(codepoint)
        glyph_to_unicode_encoding[glyph] = codepoint

    return glyph_to_unicode_encoding

def write_encoding_csv(glyph_to_unicode_encoding):
    encoding_path = 'encoding.csv'
    print(f'writing {encoding_path}...')
    with open(encoding_path, 'w') as f:
        f.write(f'index,x_offset,y_offset,x_advance,y_advance,codepoint\n')
        for item in glyph_to_unicode_encoding.items():
            f.write(f'{item[0][0]},{item[0][1]},{item[0][2]},{item[0][3]},{item[0][4]},{item[1]}\n')

def write_encoding_hpp(font_path, glyph_to_unicode_encoding):
    unicode_to_glyph_encoding = {}
    for key in glyph_to_unicode_encoding:
        unicode_to_glyph_encoding[glyph_to_unicode_encoding[key]] = key
    
    str_font = 'std::map<int, std::string> encoding_unicode_to_font = {\n'
    str_index = 'std::map<int, int> encoding_unicode_to_index = {\n'
    str_x_offset = 'std::map<int, int> encoding_unicode_to_x_offset = {\n'
    str_y_offset =  'std::map<int, int> encoding_unicode_to_y_offset = {\n'
    str_x_advance = 'std::map<int, int> encoding_unicode_to_x_advance = {\n'

    for codepoint in unicode_to_glyph_encoding:
        index, x_offset, y_offset, x_advance, _ = unicode_to_glyph_encoding[codepoint]
        font = font_path
        str_font += f'    {{{codepoint}, "{font}"}},\n'
        str_index += f'    {{{codepoint}, {index}}},\n'
        str_x_offset += f'    {{{codepoint}, {x_offset}}},\n'
        str_y_offset += f'    {{{codepoint}, {y_offset}}},\n'
        str_x_advance += f'    {{{codepoint}, {x_advance}}},\n'
    str_font += "};\n"
    str_index += "};\n"
    str_x_offset += "};\n"
    str_y_offset += "};\n"
    str_x_advance += "};\n"
    
    encoding_hpp = '#include <map>\n#include <string>\n'
    encoding_hpp += '\n'
    encoding_hpp += str_font
    encoding_hpp += '\n'
    encoding_hpp += str_index
    encoding_hpp += '\n'
    encoding_hpp += str_x_offset
    encoding_hpp += '\n'
    encoding_hpp += str_y_offset
    encoding_hpp += '\n'
    encoding_hpp += str_x_advance

    output_path = 'font-maker/encoding.hpp'
    print(f'writing {output_path}...')
    with open(output_path, 'w') as f:
        f.write(encoding_hpp)


glyph_to_unicode_encoding = generate_encoding(source='wikipedia')

write_encoding_csv(glyph_to_unicode_encoding)

font_path = 'fonts/NotoSansDevanagari-Regular.ttf'
write_encoding_hpp(font_path, glyph_to_unicode_encoding)
