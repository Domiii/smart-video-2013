#ifndef MATCHER_H
#define MATCHER_H

#include <vector>
#include <algorithm>
#include "objectProfile.h"

using namespace std;

/*
 * COSTS:
 * + kill a node (disappearance/creation) = +20*sz
 * + linking two nodes into one (overlap) = +250
 * + displacement from estimation (error) = 5*dx
 * + cluster size (feature distance) = 1*da
 */

/*
 * NODES:
 * + main node                  = n
 * + (maxOverlap*) overlap node = n*maxOverlap
 * + trash node:                = n*maxOver
 */

// TODO: employ a more accurate kalman filter

namespace Matcher
{
    const int maxOverlap = 4;
    const int inf = 1000000; // FIXME: handle this more carefully

    struct ClusterInfo {
        int x,y;
        int sz;
        ClusterInfo(int x,int y,int sz):x(x),y(y),sz(sz) {}
        //ClusterInfo(SmartVideo::ObjectProfile obj):x(obj.x),y(obj.y),sz(obj.area) {}
    };

    class ClusterMatcher {

        int ln,rn;
        vector<ClusterInfo> lvInfo,rvInfo;
        int costOverlap;
        int rateAbandon;
        int rateDisplacement;
        int rateSizeChange;

        int lv(int x,int th);
        int ltrash(int x);
        int rv(int x,int th);
        int rtrash(int x);
        int nativeCost(int a,int b);

    public:
        ClusterMatcher(vector<ClusterInfo> lvInfo,
                       vector<ClusterInfo> rvInfo,
                       int costOverlap = 2000,
                       int rateAbandon = 20,
                       int rateDisplacement = 5,
                       int rateSizeChange = 1):
                       lvInfo(lvInfo), rvInfo(rvInfo), costOverlap(costOverlap), rateAbandon(rateAbandon),
                       rateDisplacement(rateDisplacement), rateSizeChange(rateSizeChange) {
            //
            ln = lvInfo.size();
            rn = rvInfo.size();
        }
        int solve(vector<pair<int,int>> &);
    };

    vector<ClusterInfo> obj2cinfo(const vector<SmartVideo::ObjectProfile>& objs);

}

#endif // MATCHER_H