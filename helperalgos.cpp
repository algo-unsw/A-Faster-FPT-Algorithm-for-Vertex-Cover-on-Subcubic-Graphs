#include "helperalgos.hpp"

int countSameEdges(const std::vector<int> &a, const std::vector<int> &b)
{
    std::set<int> v;
    for (auto x : a)
    {
        v.insert(x >> 1);
    }
    int cnt = 0;
    for (auto x : b)
    {
        if (v.count(x >> 1))
        {
            cnt++;
        }
    }
    return cnt;
}
bool cycleMatch(const Graph &g, int l1, int l2, int min, int max) // return true if there are two cycles with length l1 and l2 has at least match number of same edges
{
    if (l1 > l2)
        std::swap(l1, l2);
    std::set<int> edges;
    auto res = g.findCycles(l2);
    // for (auto [k, v] : res)
    // {
    //     printf("%d %d\n", k, v.size());
    //     for (auto x : v)
    //     {
    //         for (auto y : x)
    //         {
    //             printf("%d ", g.edge[y].a);
    //         }
    //         printf("\n");
    //     }
    // }
    for (int i = 0; i < l1; ++i)
    {
        assert(res.count(i) == 0);
    }
    if (res.count(l1) == 0)
    {
        return false;
    }
    if (res.count(l2) == 0)
    {
        return false;
    }
    if (min == 0 && max == 0)
        return true;
    for (auto a = res[l1].begin(); a != res[l1].end(); ++a)
    {
        for (auto b = res[l2].begin(); b != res[l2].end(); ++b)
        {
            int cnt = countSameEdges(*a, *b);
            if (min <= cnt && cnt <= max)
            {
                return true;
            }
        }
    }
    return false;
}