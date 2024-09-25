#include "graph.hpp"
class branchSelector
{
    // abstract class
public:
    virtual vector<SETWIDTH> selectBranch(const Graph &g) = 0;
    virtual ~branchSelector() {}
};
class branchSelectorIS : public branchSelector
{
public:
    vector<SETWIDTH> selectBranch(const Graph &g) override
    {
        return g.getIS();
    }
};
class branchSelectorMinimal : public branchSelector // This is guaranteed to be the best
{
public:
    vector<SETWIDTH> selectBranch(const Graph &g) override
    {
        auto ls = g.getMaximalIS();
        for (auto &x : ls)
        {
            x ^= (1ll << g.n) - 1;
        }
        return ls;
        unordered_set<SETWIDTH> st[g.n + 1];
        for (auto &x : ls)
        {
            st[countBit(x)].insert(x);
        }
        for (int i = g.n; i > 1; --i)
        {
            for (auto x : st[i])
            {
                for (int j = 0; j < g.n; ++j)
                {
                    if (x & (1ll << j))
                    {
                        st[i - 1].insert(x ^ (1ll << j));
                    }
                }
            }
        }
        vector<SETWIDTH> res;
        SimplifyInfo info;
        for (int i = 0; i < g.n; ++i)
        {
            for (auto x : st[i])
            {
                res.push_back(x);
            }
        }
        return res;
    }
};
void floyd(const Graph &g, int dis[MAXV][MAXV])
{
    for (int i = 0; i < g.n; ++i)
    {
        for (int j = 0; j < g.n; ++j)
        {
            dis[i][j] = 1e9;
        }
    }
    for (int i = 0; i < g.n; ++i)
    {
        dis[i][i] = 0;
    }
    for (int i = 2; i <= g._edge; i += 2)
    {
        dis[g.edge[i].a][g.edge[i].b] = dis[g.edge[i].b][g.edge[i].a] = 1;
    }
    for (int k = 0; k < g.n; ++k)
    {
        for (int i = 0; i < g.n; ++i)
        {
            for (int j = 0; j < g.n; ++j)
            {
                dis[i][j] = std::min(dis[i][j], dis[i][k] + dis[k][j]);
            }
        }
    }
}
class branchSelectorISPlus : public branchSelector
{
public:
    vector<SETWIDTH> selectBranch(const Graph &g)
    {
        vector<SETWIDTH> ls = g.getIS(), res;
        for (auto &branch : ls)
        {
            if (g.state & Property::NO_D3cycle5 || g.state == Property::None)
            {
                bool disconnected = 0;
                for (SETWIDTH j = branch, a = lastPos(j); j; j ^= 1ll << a, a = lastPos(j))
                {
                    SETWIDTH s1 = 0, s2 = 0;

                    for (int l = g.head[a]; l; l = g.edge[l].next)
                    {
                        s1 |= 1ll << g.edge[l].b;
                        // if (g.degree(g.edge[l].b) == 2)
                        //     s1 |= (1ll << g.n) - 1;
                    }
                    for (SETWIDTH k = branch, b = lastPos(k); k; k ^= 1ll << b, b = lastPos(k))
                    {
                        if (!(branch & (1ll << b)) || b == a)
                            continue;
                        for (int l = g.head[b]; l; l = g.edge[l].next)
                        {
                            s2 |= 1ll << g.edge[l].b;
                        }
                    }
                    if ((s1 & s2) == 0)
                    {
                        disconnected = 1;
                        break;
                    }
                }
                if (disconnected)
                    continue;
            }
            int flag = 0;
            for (int j = 0; j < g.n; ++j)
            {
                if ((branch & (1ll << j)) && g.degree(j) == 2)
                    flag = 1;
            }
            if (flag && g.state)
                continue;
            SimplifyInfo info;
            if (g.deleteVertexCopy(branch).simplify(info) != 0 && (info.type & SimplifyType::AlternatingCycle) == 0)
                res.push_back(branch);
            // res.push_back(branch);
        }
        return res;
    }
};
int f[MAXV];
int find(int x) { return f[x] == x ? x : f[x] = find(f[x]); }
void merge(int x, int y) { f[find(x)] = find(y); }
class branchSelectorC5 : public branchSelector
{
public:
    vector<SETWIDTH> selectBranch(const Graph &g)
    {
        vector<SETWIDTH> ls = g.getIS(), res;
        int dis[MAXV][MAXV];
        floyd(g, dis);
        for (auto &branch : ls)
        {
            for (int i = 0; i < g.n; ++i)
                f[i] = i;
            for (SETWIDTH j = branch, a = lastPos(j); j; j ^= 1ll << a, a = lastPos(j))
            {
                for (SETWIDTH k = branch, b = lastPos(k); k; k ^= 1ll << b, b = lastPos(k))
                {
                    if (b == a)
                        continue;
                    if (dis[a][b] <= 3)
                    {
                        merge(a, b);
                    }
                }
            }
            int fail = 0;
            for (SETWIDTH j = branch, a = lastPos(j); j; j ^= 1ll << a, a = lastPos(j))
            {
                if (find(a) != find(lastPos(branch)))
                    fail = 1;
            }
            if (fail)
                continue;
            SimplifyInfo info;
            if (g.deleteVertexCopy(branch).simplify(info) != 0 && (info.type & SimplifyType::AlternatingCycle) == 0)
                res.push_back(branch);
            // res.push_back(branch);
        }
        return res;
    }
};