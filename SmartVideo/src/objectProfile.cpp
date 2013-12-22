#include <vector>
#include <cstdlib>
#include <algorithm>

#include "objectProfile.h"

using namespace std;

namespace SmartVideo {

    double randNotTooDim() {
        return 0.25+0.75*(double)rand()/RAND_MAX;
    }
    ColorProfile ColorProfile::randomProfile() {
        // generate some not-too-dark color
        return ColorProfile(randNotTooDim(),randNotTooDim(),randNotTooDim());
    }

    void ObjectProfile::adoptColor(ColorProfile cp) {
        //colorProfile.insert(cp.begin(),cp.end(),colorProfile.end());
        colorProfile.push_back(cp);
    }

    /*ObjectProfile::operator Matcher::ClusterInfo() {
        return Matcher::ClusterInfo(x,y,area);
    }*/

    void ObjectProfile::addPixel(int x,int y) {
            this->x+=x;
            this->y+=y;
            x1=min(x1,x);
            x2=max(x2,x);
            y1=min(y1,y);
            y2=max(y2,y);
            area++;
        }
        void ObjectProfile::statistics() {
            if(!area) return;
            x/=area;
            y/=area;
        }

    ColorProfile ObjectProfile::avgColor() { return mixture(colorProfile); }

    ColorProfile mixture(const std::vector<ColorProfile>& colors) {
        ColorProfile sum;
        for(auto c: colors) {
            sum+=c;
        }
        return sum/colors.size();
    }

}