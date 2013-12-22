#include "agglomerative.h"

namespace Agglomerative {

    // DisjointSet {{{
    class DisjointSet {
    public:
        int n;
        int *rep,*rank,*sz;
        DisjointSet(int _n):n(_n) {
            rep = new int[n];
            rank = new int[n];
            sz = new int[n];
            for(int i=0;i<n;i++) {
                rep[i] = i;
                sz[i] = 1;
            }
        }
        ~DisjointSet() {
            delete [] rep;
            delete [] rank;
            delete [] sz;
        }
        int getrep(int v) {
            if(rep[v]!=v) rep[v] = getrep(rep[v]);
            return rep[v];
        }
        int getsize(int v) {
            return sz[getrep(v)];
        }
        bool merge(int v,int u) {
            v = getrep(v);
            u = getrep(u);
            if(v==u) return 0;
            if(rank[v] < rank[u]) {
                rep[v] = u;
                sz[u] += sz[v];
            } else {
                rep[u] = v;
                sz[v] += sz[u];
                if(rank[v] == rank[u]) rank[v]++;
            }
            return 1;
        }
    };
    // }}}

    double dist2(Point2D a,Point2D b) {
        return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y);
    }

    vector<Result> AgglomerativeClustering::cluster(double dthreshold, int cthreshold) {
        // TODO: faster version (i.e. planar MST / real agglomerative)
        DisjointSet djs(n);
        vector<int> idmap(n,-1);
        int idc = 0;
        double thr2 = dthreshold * dthreshold;
        for(int i=0; i<n; i++) {
            for(int j=0; j<n; j++) {
                if(dist2(pts[i],pts[j])<thr2) {
                    djs.merge(i,j);
                }
            }
        }
        vector<Result> results;
        for(int i=0; i<n; i++) {
            if(djs.getsize(i) < cthreshold) continue;
            int h = djs.getrep(i);
            if(idmap[h] < 0) idmap[h] = idc++;
            results.push_back(Result(pts[i],idmap[h]));
        }
        return results;
    }

}