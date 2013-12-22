#include "matcher.h"
#include "hungarian.h"

namespace Matcher {

    int ClusterMatcher::lv(int x,int th) {
        return x*(maxOverlap+1)+th;
    }
    int ClusterMatcher::ltrash(int x) {
        return ln*(maxOverlap+1)+x;
    }
    int ClusterMatcher::rv(int x,int th) {
        return x*(maxOverlap+1)+th;
    }
    int ClusterMatcher::rtrash(int x) {
        return rn*(maxOverlap+1)+x;
    }

    int ClusterMatcher::nativeCost(int a,int b) {
        int dsz = lvInfo[a].sz-rvInfo[b].sz;
        int dis = (int)sqrt((lvInfo[a].x-rvInfo[b].x)*(lvInfo[a].x-rvInfo[b].x)+(lvInfo[a].y-rvInfo[b].y)*(lvInfo[a].y-rvInfo[b].y));
        return rateSizeChange*dsz+rateDisplacement*dis;
    }
    vector<pair<int,int>> ClusterMatcher::solve() {
        // construct
        int lvn = ln*(maxOverlap+1);
        int rvn = rn*(maxOverlap+1);
        int nn = lvn + rvn;
        Hungarian::HungarianMethod hgm(nn,nn);
        for(int x=0; x<nn; x++) {
            int xi = x/(maxOverlap+1);
            bool xIsMain = x<lvn && x%(maxOverlap+1)==0;
            bool xIsOverlap = x<lvn && x%(maxOverlap+1);
            bool xIsTrash = x>=lvn;
            for(int y=0; y<nn; y++) {
                int yi = y/(maxOverlap+1);
                bool yIsMain = y<rvn && y%(maxOverlap+1)==0;
                bool yIsOverlap = y<rvn && y%(maxOverlap+1);
                bool yIsTrash = y>=rvn;
                int cost;
                if(!xIsTrash&&!yIsTrash) {
                    cost = nativeCost(xi,yi);
                    if(xIsOverlap && yIsOverlap) cost = inf;
                    else cost += costOverlap;
                } else if(xIsTrash&&yIsTrash) {
                    cost = 0;
                } else {
                    if(xIsOverlap || yIsOverlap) cost = 0;
                    else if(xIsMain) cost = rateAbandon*lvInfo[xi].sz;
                    else if(yIsMain) cost = rateAbandon*rvInfo[yi].sz;
                    else assert(false); // should never happen
                }
                assert(cost<=inf);
                hgm.setCost(x,y,cost);
            }
        }
        // invert the cost since we want minimum matching
        for(int x=0; x<nn; x++) {
            for(int y=0; y<nn; y++) {
                hgm.setCost(x,y,inf-hgm.getCost(x,y));
            }
        }
        // hungarian
        int mincost = hgm.maximumMatching();
        // return pair of relations
        vector<pair<int,int>> ret;
        for(int j=0; j<rn; j++) {
            for(int yi=0; yi<maxOverlap+1; yi++) {
                int y = j*(maxOverlap+1)+yi;
                int x = hgm.getMatchY(y);
                if( x==Hungarian::nil || x>=lvn ) continue;
                int i = x/(maxOverlap+1);
                ret.push_back(make_pair(i,j));
            }
        }
        return ret;
    }

    vector<ClusterInfo> obj2cinfo(const vector<SmartVideo::ObjectProfile>& objs) {
        vector<ClusterInfo> cinfo;
        cinfo.reserve(objs.size());
        for(auto obj: objs) {
            //cinfo.push_back(obj);
            cinfo.push_back(ClusterInfo(obj.x,obj.y,obj.area));
        }
        return cinfo;
    }
}
