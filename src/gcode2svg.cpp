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
#include "fml/vec2.h"

using namespace gpr;
using namespace fml;

enum { DOTS, TRAPEZOIDS, FULL } render_mode = TRAPEZOIDS;

const bool pretty = false;

void svg_header(std::ostream &out, double w, double h) {
    out << "<?xml version=\"1.0\"?>";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" height=\"" << h << "\" width=\"" << w << "\">";
    if(pretty)
        out << "\n";
    //out << "<svg xmlns=\"http://www.w3.org/2000/svg\">\n";
}

void svg_footer(std::ostream &out) {
    out << "</svg>" << std::endl;
}

void svg_circle(std::ostream &out, double cx, double cy, double r) {
    out << "<circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << r << "\"/>";
    if(pretty)
        out << "\n";
}

void svg_path_arc(std::ostream &out,
                  double rx, double ry,
                  double rot, bool large, bool sweep,
                  double x, double y, bool relative = false)
{
    out << (relative ? "a" : "A");
    out << rx << " " << ry << " "
        << rot << " "
        << (int) large << " "
        << (int) sweep << " "
        << x << " " << y;
}

typedef struct stroke_pt {
    vec2 pos;
    double r; // thickness
} stroke_pt;

bool operator==(stroke_pt &a, stroke_pt &b) {
    return a.pos[0] == b.pos[0] && a.pos[1] == b.pos[1] && a.r == b.r;
}

void render_dots(std::ostream &out, std::vector<stroke_pt> &stroke) {
    for(auto pt : stroke) {
        svg_circle(out, pt.pos[0], pt.pos[1], pt.r); // half for radius
    }
}

void render_traps(std::ostream &out, std::vector<stroke_pt> &stroke, bool round_ends = true) {
    if(stroke.size() == 1) // single point -- use a circle insted
    {
        render_dots(out, stroke);
        return;
    }

    const double ninety = M_PI / 2;

    // store vertices of both sides of the path
    std::vector<vec2> pos_side, neg_side;

    // handle first point specially
    vec2 d = (stroke[1].pos - stroke[0].pos).rotateby(ninety).normalize();
    pos_side.push_back(stroke[0].pos + d * stroke[0].r);
    neg_side.push_back(stroke[0].pos - d * stroke[0].r);

    // generate vertices
    for(unsigned int i = 1; i < stroke.size(); i++) {
        vec2 d = (stroke[i].pos - stroke[i - 1].pos).rotateby(ninety).normalize();
        pos_side.push_back(stroke[i].pos + d * stroke[i].r);
        neg_side.push_back(stroke[i].pos - d * stroke[i].r);
    }

    // now dump as svg
    out << "<path d=\"";

    for(unsigned int i = 0; i < pos_side.size(); i++)
        out << (i ? "L" : "M") << pos_side[i][0] << " " << pos_side[i][1] << (pretty ? "\n" : "");

    vec2 cap;
    double r;

    if(round_ends) {
        cap = neg_side[neg_side.size() - 1] - pos_side[pos_side.size() - 1];
        r = cap.magnitude() / 2;

        svg_path_arc(out,
                     r, r, 0, false, false,
                     cap[0], cap[1], true);
    }

    for(unsigned int i = neg_side.size() - 1; i > 0; i--)
        out << "L" << neg_side[i - 1][0] << " " << neg_side[i - 1][1] << (pretty ? "\n" : "");

    if(round_ends) {
        cap = pos_side[0] - neg_side[0];
        r = cap.magnitude() / 2;

        svg_path_arc(out,
                     r, r, 0, false, false,
                     cap[0], cap[1], true);
    }

    out << "\"/>" << (pretty ? "\n" : "");
}

// not implemented
void render_full(std::ostream &out, std::vector<stroke_pt> &stroke) {
    for(auto pt : stroke) {
    }
}

double map_getdefault(std::map<char, double> m, char key, double def) {
    std::map<char, double>::iterator it = m.find(key);
    return (it == m.end()) ? def : it->second;
}

void gcode2svg(std::istream &in, std::ostream &out, double tool_angle) {
    const int ppi = 100;

    const double deg2rad = M_PI / 180;
    const double depth2width = 2 * tan(tool_angle / 2 * deg2rad);

    std::string s_in(std::istreambuf_iterator<char>(in), {});

    gcode_program prog = parse_gcode(s_in);

    // get vector of 3-vectors describing path
    std::vector<vec3> path;

    // also get dimensions (max x and min y) for svg header while we're at it
    double w = 0, h = 0;

    for(auto block : prog) {
        std::map<char, double> addrs; // map of [XYZF]->double addresses

        int g_addr = -1;

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

        if(g_addr >= 0)
        {
            // we have a move command - process it and render svg as necessary
            static double x = 0, y = 0, z = 0; // remember last position

            x = map_getdefault(addrs, 'X', x);
            y = map_getdefault(addrs, 'Y', y);
            z = map_getdefault(addrs, 'Z', z);

            path.push_back(vec3(x,y,z));
            //std::cerr << x << " " << y << " " << z << std::endl;

            // get max/min
            if(x > w)
                w = x;
            if(-y > h)
                h = -y;
        }
    }

    out << std::fixed;
    out.precision(1);
    svg_header(out, w * ppi, h * ppi);

    for(vec3 pt : path) {
        double x = pt[0], y = pt[1], z = pt[2];

        static double last_z = 1; // assume we start out of the material
        static std::vector<stroke_pt> stroke;

        if(z < 0)
        {
            //if(last_z > 0) // begin stroke
            //    stroke = std::vector<stroke_pt>();
            double r_px = -z * ppi * depth2width / 2;

            double x_px = x * ppi, y_px = -y * ppi; // negate y

            stroke_pt pt = { vec2(x_px, y_px), r_px };

            if(stroke.size() == 0 || !(pt == stroke.back())) // avoid duplicates
                stroke.push_back(pt);
        }
        else if(z >= 0 and last_z < 0) // end stroke
        {
            switch(render_mode) {
            case DOTS:
                render_dots(out, stroke);
                break;
            case TRAPEZOIDS:
                render_traps(out, stroke);
                break;
            case FULL:
                render_full(out, stroke);
                break;
            }
            stroke = std::vector<stroke_pt>();
        }

        last_z = z;
    }

    svg_footer(out);
}
