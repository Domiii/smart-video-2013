#ifndef OBJECTPROFILE_H
#define OBJECTPROFILE_H

#include <vector>

//#include "matcher.h"

namespace SmartVideo {

    const int maxreso = 100000;

    class ColorProfile {
    public:
        double r,g,b;
        ColorProfile(double r=0, double g=0, double b=0):r(r),g(g),b(b) {}
        ColorProfile& operator+=(const ColorProfile &y) {
            r+=y.r;
            g+=y.g;
            b+=y.b;
            return *this;
        }
        ColorProfile operator+(const ColorProfile &y) const {
            return (ColorProfile)*this+=y;
        }
        ColorProfile& operator/=(int d) {
            if(d==0) return *this;
            r/=d;
            g/=d;
            b/=d;
            return *this;
        }
        ColorProfile operator/(int d) const {
            return (ColorProfile)*this/=d;
        }

        // generator
        static ColorProfile randomProfile();
    };

    class ObjectProfile {
    public:
        std::vector<ColorProfile> colorProfile;
        int x, y, area;
        int x1, x2, y1, y2;
        ObjectProfile():x(0),y(0),area(0) {
            x1=y1=maxreso;
            x2=y2=-1;
        }
        void addPixel(int x,int y);
        void statistics();
        void adoptColor(ColorProfile cp);
        ColorProfile avgColor();
        //operator Matcher::ClusterInfo();
    };

    ColorProfile mixture(const std::vector<ColorProfile>& colors);

}

#endif // OBJECTPROFILE_H