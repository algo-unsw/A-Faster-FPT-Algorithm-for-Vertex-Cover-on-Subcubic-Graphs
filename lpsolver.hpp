#include "alglib-cpp/stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "alglib-cpp/optimization.h"
#include <math.h>
#include <vector>
#include "graph.hpp"

using namespace alglib;
inline double check(const vector<tuple<vector<bool>, Measure, SETWIDTH>> &f, vector<double> &result)
{
    // N*M
    minlpstate state;
    minlpreport rep;
    minlpcreate(f.size(), state); // N variables
    minlpsetalgoipm(state, 0.00001);

    real_1d_array c;
    c.setlength(f.size());
    for (int i = 0; i < f.size(); ++i)
    {
        c[i] = get<1>(f[i]).cost();
    }
    minlpsetcost(state, c); // Object function, minimize

    minlpsetbcall(state, 0, fp_posinf); // Box constraints

    real_2d_array a;                            // constraint matrix
    a.setlength(get<0>(f[0]).size(), f.size()); // M*N
    for (int i = 0; i < get<0>(f[0]).size(); ++i)
    {
        for (int j = 0; j < f.size(); ++j)
        {
            a[i][j] = get<0>(f[j])[i] ? 1 : 0;
        }
    }

    // Constraints for each formula
    real_1d_array al, au;
    al.setlength(get<0>(f[0]).size());
    au.setlength(get<0>(f[0]).size());
    for (int i = 0; i < get<0>(f[0]).size(); ++i)
    {
        al[i] = 1;
        au[i] = fp_posinf;
    }
    minlpsetlc2dense(state, a, al, au, get<0>(f[0]).size()); // General linear constraints

    // Scale
    real_1d_array s;
    s.setlength(f.size());
    for (int i = 0; i <= f.size(); ++i)
    {
        s[i] = 1;
    }

    minlpsetscale(state, s);

    // Solve
    minlpoptimize(state);

    real_1d_array x;
    minlpresults(state, x, rep);
    if (rep.terminationtype < 0)
    {
        return 1e9;
    }
    int cnt = 0;
    for (int i = 0; i < f.size(); ++i)
    {
        if (x[i] > 0.2)
            cnt++;
    }
    // if (cnt > 5)
    // {

    //     // printf("\n%s\n", x.tostring(5).c_str()); // EXPECTED: [0,1]
    //     for (int i = 0; i < f.size(); ++i)
    //     {
    //         for (auto j : get<1>(f[i]))
    //         {
    //             cout << j;
    //         }
    //         printf(" %.5f\n", x[i]);
    //     }
    //     printf("sum= %.5lf\n", x[f.size()]);
    // }
    // printf("%s\n", x.tostring(3).c_str()); // EXPECTED: [0,1]
    result.clear();
    for (int i = 0; i < f.size(); ++i)
    {
        result.push_back(x[i]);
    }
    double sum = 0;
    for (int i = 0; i < f.size(); ++i)
    {
        sum += x[i] * get<1>(f[i]).cost();
    }
    return sum;
}