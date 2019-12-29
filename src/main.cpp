#include <iostream>
#include "gcode2svg.h"

using namespace std;

int main(int argc, char** argv) {
    double tool_angle = 30; // default
    if(argc == 2)
    {
        char *end;
        tool_angle = strtod(argv[1], &end);
        if(*end || !(0 < tool_angle && tool_angle < 180)) // failed
        {
            cerr << "Usage: rastercarve-preview [TOOL_ANGLE]" << endl;
            return 1;
        }
    }

    gcode2svg(cin, cout, tool_angle);
}
