/*
 * Rastercarve
 *
 * Copyright (C) 2019 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 */

/*
 * Generate an SVG preview from a G-code sequence, assuming a flat
 * piece of material at Z=0 and a V-bit of a known angle.
 *
 * This is useful for visualizing rastercarve (or photovcarve (TM))
 * toolpaths.
 */

#include <cmath>
#include <iostream>
#include <map>

#include "gcode_program.h"
#include "parser.h"

using namespace gpr;

void svg_header(std::ostream &out, int w, int h) {
    out << "<?xml version=\"1.0\"?>";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" height=\"" << h << "\" width=\"" << w << "\">\n";
}

void svg_footer(std::ostream &out) {
    out << "</svg>" << std::endl;
}

void svg_circle(std::ostream &out, double cx, double cy, double r) {
    out << "<circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << r << "\"/>\n";
}

void gcode2svg(std::istream &in, std::ostream &out) {
    const int tool_angle = 30;
    const int ppi = 100;
    const int w_in = 4, h_in = 4; // update later

    const int w = ppi * w_in, h = ppi * h_in;

    const double deg2rad = M_PI / 180;
    const double depth2width = 2 * tan(tool_angle / 2 * deg2rad);

    svg_header(out, w, h);

    std::string s_in(std::istreambuf_iterator<char>(in), {});

    gcode_program prog = parse_gcode(s_in);

    for(auto block : prog) {
        int g_addr = -1;
        std::map<char, double> addrs; // map of [XYZF]->double addresses

        for(auto chunk : block) {
            if(chunk.tp() == CHUNK_TYPE_WORD_ADDRESS) {
                char word = chunk.get_word();
                switch(word) {
                case 'G':
                {
                    int addr = chunk.get_address().int_value();
                    if(addr == 0 || addr == 1) {
                        g_addr = addr;
                    }
                    break;
                }
                case 'F':
                case 'X':
                case 'Y':
                case 'Z':
                    addrs[word] = chunk.get_address().double_value();
                    break;
                }
            }
        }

        double x, y, z;
        x = addrs['X'];
        y = addrs['Y'];
        z = addrs['Z'];

        if(z < 0)
            svg_circle(out, x * ppi, -y * ppi, -z * ppi * depth2width);
    }

    svg_footer(out);
}
