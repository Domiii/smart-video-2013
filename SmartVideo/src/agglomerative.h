#ifndef AGGLOMERATIVE_H
#define AGGLOMERATIVE_H

#include <vector>
//#include "kdtree++/kdtree.hpp"

using namespace std;

namespace Agglomerative
{
    typedef pair<double,double> Point;

    struct Point2D {
        int x,y;
        Point2D(int x,int y):x(x),y(y) {}
    };

    struct Result {
        Point2D pt;
        int id;
        Result(Point2D pt,int id):pt(pt),id(id) {}
    };

    class AgglomerativeClustering
    {
        int n;
        vector<Point2D> pts;

    public:
        AgglomerativeClustering(const vector<Point2D> &pts):pts(pts) {
            n = pts.size();
        }
        vector<Result> cluster(double dthreshold, int cthreshold);
        // break cluster with L2-distance $dthreshold$, and clean up noise with cluster size below $cthreshold$
    };

}

#endif // AGGLOMERATIVE_H