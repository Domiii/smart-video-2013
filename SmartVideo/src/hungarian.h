#ifndef HUNGARIAN_H
#define HUNGARIAN_H

#include <cassert>

namespace Hungarian {

    const int MAXNUM = 300;

    const int nil = -1;
    const int inf = 100000000;

    class HungarianMethod {

        int xn,yn,matched;
        //int cost[MAXNUM][MAXNUM];
        int *cost[MAXNUM];
        bool sets[MAXNUM]; // whether x is in set S
        bool sett[MAXNUM]; // whether y is in set T
        int xlabel[MAXNUM],ylabel[MAXNUM];
        int xy[MAXNUM],yx[MAXNUM]; // matched with whom
        int slack[MAXNUM];  // given y: min{xlabel[x]+ylabel[y]-cost[x][y]} | x not in S
        int prev[MAXNUM]; // for augmenting matching

        void relabel();
        void add_sets(int x);
        void augment(int final);
        void phase();

    public:
        HungarianMethod(int xn,int yn):xn(xn),yn(yn) {
            assert(xn<=MAXNUM && yn<=MAXNUM);
            for(int i=0;i<xn;i++) {
                cost[i] = new int[yn];
                for(int j=0;j<yn;j++)
                    cost[i][j]=0;
            }
            memset(sets,0,sizeof(sets));
            memset(sett,0,sizeof(sett));
        }
        ~HungarianMethod() {
            for(int i=0;i<xn;i++)
                delete [] cost[i];
        }

        void setCost(int x,int y,int c);
        int getCost(int x,int y);
        int getMatchX(int x) { return xy[x]; }
        int getMatchY(int y) { return yx[y]; }

        int maximumMatching();
        
    };

}

#endif // HUNGARIAN_H