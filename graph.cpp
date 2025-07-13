#include "graph.hpp"
#include "nauty/nauty.h"
namespace labelg
{
#include "labelg.h"
}
#include "helperalgos.hpp"
#include "lpsolver.hpp"
#include "branchSelector.hpp"

FILE *outputfile;
std::unordered_map<string, int> vertexCoverSize;
std::shared_mutex vertexCoverSizeMutex;
SimplifyInfo ignoreInfo;
double alpha = 1, beta[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

SETWIDTH arrayInsert(SETWIDTH x, SETWIDTH positions)
{
    while (positions)
    {
        int pos = lastPos(positions);
        positions ^= 1ll << pos;
        x = (x & ((1ll << pos) - 1)) | (1ll << pos) | ((x & ~((1ll << pos) - 1)) << 1);
    }
    return x;
}
string bitsetToString(SETWIDTH x, int n = 0)
{
    string ans;
    while (x || n > 0)
    {
        ans += (char)((x & 1) + '0');
        x >>= 1;
        n--;
    }
    if (ans == "")
        ans = "0";
    return ans;
}
vector<int> getSubset(SETWIDTH x)
{
    vector<int> ans;
    int n = 0;
    while (x)
    {
        if (x & 1)
            ans.push_back(n);
        x >>= 1;
        n++;
    }
    return ans;
}
SETWIDTH getSet(vector<int> x)
{
    SETWIDTH ans = 0;
    for (int i : x)
    {
        ans |= 1ll << i;
    }
    return ans;
}
inline void Graph::addEdge(int a, int b)
{
    assert(_edge & 1);
    edge[++_edge] = {a, b, head[a]};
    head[a] = _edge;
    edge[++_edge] = {b, a, head[b]};
    head[b] = _edge;
}
void Graph::link(int a, int b)
{
    remain[a]--;
    remain[b]--;
    addEdge(a, b);
    assert(remain[a] >= 0);
    assert(remain[b] >= 0);
}
void Graph::addVertex(int x, int d)
{
    assert(n == x);
    n++;
    head[x] = 0;
    remain[x] = d;
}
int Graph::degree(int x) const
{
    int ans = 0;
    for (int i = head[x]; i; i = edge[i].next)
    {
        ans++;
    }
    return ans + remain[x];
}
inline int Graph::count_n(int d) const
{
    int cnt = 0;
    for (int i = 0; i < n; ++i)
        cnt += degree(i) == d;
    return cnt;
}
inline string Graph::getId() const
{
    if (n == 0)
    {
        // id = "";
        return "";
    }
    int m = SETWORDSNEEDED(n);
    graph g[n * m];

    EMPTYGRAPH(g, m, n);
    for (int i = 2; i <= _edge; i += 2)
    {
        ADDONEEDGE(g, edge[i].a, edge[i].b, m);
    }

    string ans = "";
    {
        ans = labelg::labelg(g, m, n);
    }
    return ans;
}
string Graph::toString() const
{
    if (n == 0)
    {
        // id = "";
        return "";
    }
    int m = SETWORDSNEEDED(n);
    graph g[n * m];
    int lab[n], ptn[n], orbits[n];
    memset(lab, 0, sizeof(lab));
    memset(ptn, 0, sizeof(ptn));
    memset(orbits, 0, sizeof(orbits));
    statsblk stats;

    EMPTYGRAPH(g, m, n);
    for (int i = 2; i <= _edge; i += 2)
    {
        ADDONEEDGE(g, edge[i].a, edge[i].b, m);
    }

    size_t buf_size = 2048;
    char *buffer = new char[buf_size];
    memset(buffer, 0, buf_size);
    FILE *stream = fmemopen(buffer, buf_size, "w");
    {
        labelg::writeg6(stream, g, m, n);
    }
    fclose(stream);
    std::string ans = buffer;
    delete[] buffer;
    while (ans.back() == '\n')
        ans.pop_back();
    return ans;
}
inline string Graph::isolabel() const
{
    string id = getId();
    int m = SETWORDSNEEDED(n);
    graph g[n * m];
    int lab[n], ptn[n], orbits[n];
    memset(lab, 0, sizeof(lab));
    memset(ptn, 0, sizeof(ptn));
    memset(orbits, 0, sizeof(orbits));
    DEFAULTOPTIONS_GRAPH(options);
    statsblk stats;
    options.getcanon = TRUE;
    options.defaultptn = TRUE;
    options.tc_level = 0;
    options.digraph = FALSE;

    EMPTYGRAPH(g, m, n);
    for (int i = 2; i <= _edge; i += 2)
    {
        ADDONEEDGE(g, edge[i].a, edge[i].b, m);
    }

    graph canong[n * m];
    densenauty(g, lab, ptn, orbits, &options, &stats, m, n, canong);
    string s;
    int remain2[MAXV], oldtonew[MAXV];
    for (int i = 0; i < n; ++i)
    {
        oldtonew[lab[i]] = i;
    }
    vector<int> iso[MAXV];
    for (int i = 0; i < n; ++i)
    {
        iso[orbits[i]].push_back(oldtonew[i]);
    }
    for (int i = 0; i < n; ++i)
    {
        remain2[i] = remain[lab[i]]; // Only divide remain or not remain
    }
    for (int i = 0; i < n; ++i)
    {
        std::stable_sort(iso[i].begin(), iso[i].end());
        vector<int> v;
        for (int j = 0; j < iso[i].size(); ++j)
        {
            v.push_back(remain2[iso[i][j]]);
        }
        std::stable_sort(v.begin(), v.end(), std::greater<int>());
        for (int j = 0; j < iso[i].size(); ++j)
        {
            remain2[iso[i][j]] = v[j];
        }
    }
    for (int i = 0; i < n; ++i)
    {
        s += std::to_string(remain2[i]);
    }
    return id + s;
}
Measure applyBranch(const Graph &before, SETWIDTH branch)
{
    Graph after = before;
    after.deleteVertex(branch);
    SimplifyInfo info;
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
    int k = -countBit(branch) - after.simplify(info);
    info.deleted = arrayInsert(info.deleted, branch);
    for (SETWIDTH i = info.deleted; i; i -= lowbit(i))
    {
        if (before.degree(lastPos(i)) >= 3)
        {
            if (before.remain[lastPos(i)] == 1)
                d++;
            if (before.remain[lastPos(i)] == 2)
                e++;
        }
        if (before.degree(lastPos(i)) == 2)
        {
            f += before.remain[lastPos(i)];
        }
    }
    for (int i = 0; i < after.n; ++i)
    {
        if (after.degree(i) == 2 && after.remain[i] == 1)
            ++a;
        if (after.degree(i) == 2 && after.remain[i] == 2)
            ++b;
        if (after.degree(i) == 1 && after.remain[i] == 1)
            ++c;
    }
    int n4 = after.count_n(4) - before.count_n(4),
        n3 = after.count_n(3) - before.count_n(3),
        n2 = after.count_n(2) - before.count_n(2), n1 = after.count_n(1) - before.count_n(1), dn = after.n - before.n;
    // below code only work when beta1=0.5beta2
    int adj = d + 2 * e + std::min(f, a + 2 * b + c);
    if (before.state & Property::NO_232)
    {
        adj = d + e + std::min(e + f, a + 2 * b + c);
    }
    if (before.state & Property::NO_D2)
    {
        assert(f == 0);
        adj = std::min(d + 2 * e, a + 2 * b + c);
    }
    n1 -= adj;

    return Measure(k, 0, n4, n3, n2, n1);
}
inline bool Graph::evaluateBranch(
    SETWIDTH branch,
    const vector<SETWIDTH> &boundary,
    unordered_map<SETWIDTH, int> &VCSizeAfterDelete,
    unordered_map<SETWIDTH, pair<vector<bool>, Measure>> &record,
    SETWIDTH guide) const
{
    vector<bool> success;

    double cost = 0;
    auto measure = applyBranch(*this, branch);
    cost = measure.cost();
    int fail = 0;

    success.resize(boundary.size(), false);
    vector<bool> *guideSuccess = nullptr; // One optimization, we can pick guide based on which guide has minimum success
    if (guide)
    {
        guideSuccess = &record.at(guide).first;
    }
    for (int i = 0; i < boundary.size(); ++i)
    {
        if (guideSuccess && !(*guideSuccess)[i])
            continue;
        SETWIDTH b = boundary[i];
        if (VCSizeAfterDelete.count(b) == 0)
        {
            Graph g = deleteVertexCopy(b);
            VCSizeAfterDelete[b] = g.solveVertexCover();
        }
        int withBoundaryVertexCoverSize = VCSizeAfterDelete[b];
        SETWIDTH deleteSet = branch & ~b;
        if (deleteSet == 0)
        {
            success[i] = true;
            continue;
        }
        if (VCSizeAfterDelete.count(deleteSet | b) == 0)
        {
            Graph g = deleteVertexCopy(deleteSet | b);
            VCSizeAfterDelete[deleteSet | b] = g.solveVertexCover();
        }
        int currentVertexCoverSize = VCSizeAfterDelete[deleteSet | b] + countBit(deleteSet);
        if (currentVertexCoverSize == withBoundaryVertexCoverSize)
        {
            success[i] = true;
        }
        else if (guideSuccess)
        {
            fail = 1;
        }
    }
    record.emplace(branch, std::make_pair(success, measure));
    return fail == 0;
}
bool branchDfs(
    SETWIDTH branch,
    const SETWIDTH bottom,
    const Graph &g,
    unordered_set<SETWIDTH> &topNodes,
    unordered_set<SETWIDTH> &visitedBranch,
    unordered_set<SETWIDTH> leveledBranch[],
    const vector<SETWIDTH> &boundary,
    unordered_map<SETWIDTH, int> &VCSizeAfterDelete,
    unordered_map<SETWIDTH, pair<vector<bool>, Measure>> &currentResult)
{
    if (!leveledBranch[countBit(branch)].count(branch) || visitedBranch.count(branch))
        return false;
    visitedBranch.insert(branch);
    if (currentResult.count(branch) == 0)
    {
        assert(bitsetToString(branch).size() <= g.n);
        bool res = g.evaluateBranch(branch, boundary, VCSizeAfterDelete, currentResult, bottom == branch ? 0 : bottom);
        if (!res) // Not same connected component
            return false;
    }
    if (currentResult.at(branch).first != currentResult.at(bottom).first)
    {
        return false;
    }
    int visited = 0;
    for (SETWIDTH topNode : topNodes)
    {
        if ((topNode & branch) == branch && currentResult.at(topNode).first == currentResult.at(branch).first)
        {
            visited = 1;
            // branch is bottom node
            for (SETWIDTH i = topNode ^ branch; i; i = (i - 1) & (topNode ^ branch)) // can be optimized by bfs
            {
                SETWIDTH newBranch = branch | i;
                if (leveledBranch[countBit(newBranch)].count(newBranch))
                {
                    visitedBranch.insert(newBranch);
                }
            }
        }
    }
    if (visited)
    {
        assert(branch == bottom);
        return true;
    }
    for (SETWIDTH i = ((1ll << g.n) - 1) ^ branch, del = lowbit(i); i; i -= del, del = lowbit(i))
    {

        bool res = branchDfs(branch ^ del, bottom, g, topNodes, visitedBranch, leveledBranch, boundary, VCSizeAfterDelete, currentResult);
        if (res == true)
        {
            visited = 1;
        }
    }
    if (!visited)
    {
        topNodes.insert(branch);
        for (SETWIDTH i = branch ^ bottom; i; i = (i - 1) & (branch ^ bottom)) // can be optimized by bfs
        {
            SETWIDTH newBranch = bottom | i;
            if (leveledBranch[countBit(newBranch)].count(newBranch))
            {
                visitedBranch.insert(newBranch);
            }
        }
    }
    return true;
}
double Graph::tryBranching(double eps, vector<tuple<vector<bool>, Measure, SETWIDTH>> &resstate, vector<double> &resweight, vector<SETWIDTH> suggestBranches) const
{
    int start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    resstate.clear();
    resweight.clear();
    double thisCost = 1;
    unordered_map<SETWIDTH, pair<SETWIDTH, Measure>> delSet;
    unordered_map<SETWIDTH, pair<SETWIDTH, Measure>> selSet;
    unordered_map<SETWIDTH, int> VCSizeAfterDelete;
    unordered_map<SETWIDTH, pair<vector<bool>, Measure>> currentResult;
    //                           Success       Measure
    unordered_set<SETWIDTH> visited;
    vector<SETWIDTH> boundary;
    vector<double> result;
    double lpRes = 10;

    SETWIDTH mask = 0;
    for (int i = 0; i < n; ++i)
        if (remain[i] > 0)
            mask |= 1ll << i;

    // Boundary compression
    for (SETWIDTH i = mask;; i = (i - 1) & mask) // boundary
    {
        int VC = (VCSizeAfterDelete.count(i) ? VCSizeAfterDelete[i] : VCSizeAfterDelete[i] = deleteVertexCopy(i).solveVertexCover()) + countBit(i);
        for (SETWIDTH j = i, del = lowbit(j); j; j ^= del, del = lowbit(j))
        {
            int VC2 = (VCSizeAfterDelete.count(i ^ del) ? VCSizeAfterDelete[i ^ del] : VCSizeAfterDelete[i ^ del] = deleteVertexCopy(i ^ del).solveVertexCover()) + countBit(i ^ del);
            if (VC == VC2)
            {
                visited.insert(i ^ del);
            }
            else
            {
                assert(VC == VC2 + 1);
                visited.insert(i);
            }
        }
        if (!visited.count(i))
            boundary.push_back(i);
        if (i == 0)
        {
            break;
        }
    }
    visited.clear();
    // Fast check
    for (int i = 0; i < n; ++i)
    {
        evaluateBranch(1ll << i, boundary, VCSizeAfterDelete, currentResult);
        const auto &[result, measure] = currentResult.at(1ll << i);
        bool hasFalse = false;
        for (int j = 0; j < result.size(); ++j)
        {
            hasFalse |= result[j] == false;
        }
        if (!hasFalse) // all success
        {
            resstate.push_back({result, measure, 1ll << i});
            resweight.push_back(1);
            return 0;
        }
    }
    for (int i = 0; i < n; ++i)
    {
        SETWIDTH v = 0;
        for (int j = head[i]; j; j = edge[j].next)
        {
            v |= 1ll << edge[j].b;
        }
        if (v == 0)
            continue;
        evaluateBranch(v, boundary, VCSizeAfterDelete, currentResult);
        const auto &[result, measure] = currentResult.at(v);
        bool hasFalse = false;
        for (int j = 0; j < result.size(); ++j)
        {
            hasFalse |= result[j] == false;
        }
        if (!hasFalse) // all success
        {
            resstate.push_back({result, measure, v});
            resweight.push_back(1);
            return 0;
        }
        const auto &[result2, measure2] = currentResult.at(1ll << i);
        if (thisCost >= measure.cost() + measure2.cost())
        {
            resstate.push_back({result, measure, v});
            resweight.push_back(1);
            resstate.push_back({result2, measure2, 1ll << i});
            resweight.push_back(1);
            return 0;
        }
    }
    SETWIDTH T = (1ll << n) - 1;
    resstate.clear();
    resweight.clear();
    if (suggestBranches.size() == 0)
        suggestBranches = branchSelectorISPlus().selectBranch(*this);

    unordered_set<SETWIDTH> visitedBranch, topNodes;
    unordered_set<SETWIDTH> leveledBranch[n + 1];

    for (SETWIDTH branch : suggestBranches)
    {
        SimplifyInfo info;
        Graph g = deleteVertexCopy(branch);
        Measure mu = applyBranch(*this, branch);
        g.simplify(info);
        info.deleted = arrayInsert(info.deleted, branch);
        info.selected = arrayInsert(info.selected, branch);
        if (selSet.count(info.selected) == 0 || selSet.at(info.selected).second.cost() > mu.cost())
        {
            selSet.emplace(info.selected, std::make_pair(branch, mu));
        }
    }
    for (auto [key, value] : selSet)
    {
        SETWIDTH branch = value.first;
        leveledBranch[countBit(branch)].insert(branch);
    }
    for (int i = 1; i < n; ++i)
    {
        for (SETWIDTH branch : leveledBranch[i])
        {
            if (visitedBranch.count(branch))
                continue;
            branchDfs(branch, branch, *this, topNodes, visitedBranch, leveledBranch, boundary, VCSizeAfterDelete, currentResult);
        }
    }
    int mid2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    vector<std::tuple<vector<bool>, Measure, SETWIDTH>> filteredResult, filteredResult2;
    if (currentResult.size() == 0)
        return 10;
    vector<vector<bool>> filterIso, filterIso2;
    int cnt = 0;
    for (auto [branch, res] : currentResult)
    {
        filteredResult.push_back({res.first, res.second, branch});
    }
    std::vector<int> idx(filteredResult.size());
    for (int i = 0; i < idx.size(); ++i)
        idx[i] = i;
    std::stable_sort(idx.begin(), idx.end(), [&](int a, int b)
                     { 
                    if(fabs(get<1>(filteredResult[a]).cost() - get<1>(filteredResult[b]).cost()) < 1e-6)// cost same
                    {
                        if (get<0>(filteredResult[a]) == get<0>(filteredResult[b])) //requirement same
                        {
                            if(countBit(get<2>(filteredResult[a])) == countBit(get<2>(filteredResult[b])))
                            {
                                return get<2>(filteredResult[a]) < get<2>(filteredResult[b]);
                            }
                            return countBit(get<2>(filteredResult[a])) < countBit(get<2>(filteredResult[b]));
                        }
                        return get<0>(filteredResult[a]) > get<0>(filteredResult[b]);
                    }
                    return get<1>(filteredResult[a]).cost() < get<1>(filteredResult[b]).cost(); });
    std::set<vector<bool>> vis;
    for (int i = 0; i < filteredResult.size(); ++i)
    {
        int ith = idx[i];
        bool flag = true;
        vector<int> trues;
        for (int j = 0; j < get<0>(filteredResult[ith]).size(); ++j)
        {
            if (get<0>(filteredResult[ith])[j])
            {
                trues.push_back(j);
            }
        }
        for (auto &branch : filteredResult2)
        {
            bool flag2 = true;
            for (int k : trues)
            {
                if (get<0>(branch)[k] == 0)
                {
                    flag2 = false;
                    break;
                }
            }
            if (flag2)
            {
                flag = false;
                break;
            }
        }
        vis.insert(get<0>(filteredResult[ith]));

        if (flag)
        {
            filteredResult2.push_back(std::move(filteredResult[ith]));
        }
    }
    lpRes = check(filteredResult2, result);
    for (int i = 0; i < filteredResult2.size(); ++i)
    {
        resstate.push_back(filteredResult2[i]);
        resweight.push_back(lpRes <= 100 ? result[i] : 0);
    }

    int end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return lpRes;
}
inline Graph Graph::deleteVertexCopy(SETWIDTH x) const
{
    Graph g = *this;
    g.deleteVertex(x);
    return g;
}
inline void Graph::deleteVertex(SETWIDTH x)
{
    int cnt = 0;
    for (int i = 0; i < n; ++i)
    {
        remain[i - cnt] = remain[i];
        if (x & (1ll << i))
        {
            cnt++;
        }
        head[i] = 0;
    }
    for (int i = n - cnt; i < n; ++i)
    {
        remain[i] = 0;
    }
    for (SETWIDTH i = x; i; i -= lowbit(i))
    {
        assert(lastPos(i) < n);
    }
    n = n - cnt;
    int m = _edge;
    _edge = 1;
    for (int i = 2; i <= m; i += 2)
    {
        int a = edge[i].a, b = edge[i].b;
        if ((x & (1ll << a)) || (x & (1ll << b)))
            continue;

        for (SETWIDTH j = x, pos = firstPos(j); j; j ^= (1ll << pos), pos = firstPos(j))
        {
            if (pos < a)
                --a;
            if (pos < b)
                --b;
        }
        addEdge(a, b);
    }
}
inline string compressString(string s)
{
    string ans;
    int cnt = 0;
    int bit_count = 0;
    for (char c : s)
    {
        int value = c - 63;

        cnt = (cnt << 6) | value;
        bit_count += 6;

        while (bit_count >= 8)
        {
            bit_count -= 8;
            ans.push_back((cnt >> bit_count) & 0xFF);
        }
    }

    if (bit_count > 0)
    {
        ans.push_back((cnt << (8 - bit_count)) & 0xFF);
    }

    if (cnt)
    {
        ans.push_back((char)cnt);
    }
    return ans;
}
int Graph::solveVertexCover(bool checkSimplify) const
{
    if (n == 0 || _edge == 1)
        return 0;
    if (checkSimplify)
    {
        SimplifyInfo info;
        Graph g = *this;
        for (int i = 0; i < g.n; ++i)
            g.remain[i] = 0;
        g.simplify(info);
        if (info.deleted)
        {
            return g.solveVertexCover(false) + countBit(info.selected);
        }
    }
    string id = compressString(getId());
    {
        std::shared_lock<std::shared_mutex> lock(vertexCoverSizeMutex);
        if (vertexCoverSize.count(id))
        {
            return vertexCoverSize[id];
        }
    }
    int vc = 1 + std::min(deleteVertexCopy(1ll << edge[_edge].a).solveVertexCover(), deleteVertexCopy(1ll << edge[_edge].b).solveVertexCover());
    {
        std::unique_lock<std::shared_mutex> lock(vertexCoverSizeMutex);
        vertexCoverSize[id] = vc;
    }

    return vc;
}
string Graph::toReadableString() const
{
    std::stringstream ss;
    ss << n << " " << _edge / 2 << std::endl;
    for (int i = 0; i < n; ++i)
    {
        ss << remain[i];
    }
    ss << std::endl;
    for (int i = 2; i <= _edge; i += 2)
    {
        if (edge[i].a < edge[i].b)
            ss << edge[i].a << " " << edge[i].b << std::endl;
        else
            ss << edge[i].b << " " << edge[i].a << std::endl;
    }
    return ss.str();
}
Graph Graph::simplifyCopy(SimplifyInfo &info) const
{
    Graph g = *this;
    g.simplify(info);
    return g;
}
bool dfs(const Graph &g, int x, int dfn[], int from, int start)
{
    dfn[x] = from < 0 ? 1 : dfn[from] + 1;
    for (int i = g.head[x]; i; i = g.edge[i].next)
    {
        int b = g.edge[i].b;
        if ((g.degree(b) == 2) == (g.degree(x) == 2) || b < start || b == from) // b>start does not guarantee the cycle start from start
            continue;
        if (dfn[b])
        {
            for (int j = 0; j < g.n; ++j)
            {
                if (dfn[j] < dfn[b])
                    dfn[j] = 0;
            }
            return true;
        }
        if (dfs(g, b, dfn, x, start))
            return true;
    }
    dfn[x] = 0;
    return false;
}
int Graph::simplify(SimplifyInfo &info, bool check)
{
    info = SimplifyInfo();
    int deg[n];
    for (int i = 0; i < n; ++i)
    {
        deg[i] = degree(i);
        if (deg[i] == 0) // isolated vertex
        {
            if (check)
                return 1;
            deleteVertex(1ll << i);
            int res = simplify(info, check);
            info.deleted = arrayInsert(info.deleted, 1ll << i);
            info.type |= SimplifyType::Isolated;
            return res;
        }
    }
    for (int i = 0; i < n; ++i)
    {
        if (deg[i] == 1 && remain[i] == 0) // degree 1
        {
            if (check)
                return 2;
            int b = edge[head[i]].b;
            deleteVertex((1ll << i) | (1ll << b));
            int res = simplify(info, check) + 1;
            info.selected = arrayInsert(info.selected, (1ll << b));
            info.deleted = arrayInsert(info.deleted, (1ll << i) | (1ll << b));
            info.type |= SimplifyType::Degree1;
            return res;
        }
    }
    for (int i = 0; i < n; ++i)
    {
        if (deg[i] != 2 || remain[i])
            continue;
        // triangle
        int a = i, b = edge[head[i]].b;
        int c = edge[edge[head[i]].next].b;
        int flag = 0;
        for (int j = head[b]; j; j = edge[j].next)
        {
            if (edge[j].b == c)
                flag = 1;
        }
        if (flag)
        {
            if (check)
                return 3;
            deleteVertex((1ll << a) | (1ll << b) | (1ll << c));
            int res = simplify(info, check) + 2;
            info.selected = arrayInsert(info.selected, (1ll << b) | (1ll << c));
            info.deleted = arrayInsert(info.deleted, (1ll << a) | (1ll << b) | (1ll << c));

            info.type |= SimplifyType::Degree2Triangle;
            return res;
        }
    }
    for (int i = 0; i < n; ++i)
    {
        if (deg[i] != 2 || remain[i])
            continue;
        // chain
        int a = i, b = edge[head[a]].b, c = edge[edge[head[a]].next].b;
        if (deg[b] != 2 || remain[b])
            std::swap(b, c);
        if (deg[b] != 2 || remain[b])
            continue;
        if (check)
            return 5;
        int d = edge[head[b]].b == a ? edge[edge[head[b]].next].b : edge[head[b]].b;
        int hasEdge = 0;
        for (int j = head[d]; j; j = edge[j].next)
        {
            if (edge[j].b == c)
            {
                hasEdge = 1;
                break;
            }
        }
        if (!hasEdge)
        {
            addEdge(c, d);
        }
        deleteVertex((1ll << a) | (1ll << b)); // 2--2
        int res = simplify(info, check) + 1;
        info.selected = arrayInsert(info.selected, (1ll << a));
        info.deleted = arrayInsert(info.deleted, (1ll << a) | (1ll << b));
        info.type |= SimplifyType::Degree2Chain;
        return res;
    }
    for (int i = 0; i < n; ++i)
    {
        if (deg[i] == 2 && remain[i] == 0) // degree 2
        {
            int vis[n];
            memset(vis, 0, sizeof(vis));
            if (dfs(*this, i, vis, -1, i)) // alternating cycle
            {
                if (check)
                    return 6;
                SETWIDTH cycle = 0, select = 0;
                for (int j = 0; j < n; ++j)
                {
                    cycle |= (long long)(bool)vis[j] << j;
                    select |= (long long)(vis[j] && deg[j] != 2) << j;
                }
                deleteVertex(cycle);
                int res = simplify(info, check) + countBit(cycle) / 2;
                info.selected = arrayInsert(info.selected, select);
                info.deleted = arrayInsert(info.deleted, cycle);
                info.type |= SimplifyType::AlternatingCycle;
                return res;
            }
        }
    }
    return 0;
}
int find(int x, int *fa)
{
    return fa[x] == x ? x : fa[x] = find(fa[x], fa);
}
void merge(int x, int y, int *fa)
{
    x = find(x, fa);
    y = find(y, fa);
    fa[x] = y;
}
bool validate(const Graph &g)
{
    if (g.state & Property::NO_D2)
    {
        for (int i = 0; i < g.n; ++i)
        {
            if (g.degree(i) == 2)
                return false;
        }
    }
    if (g.state & Property::NO_D3)
    {
        for (int i = 0; i < g.n; ++i)
        {
            if (g.degree(i) == 3)
                return false;
        }
    }
    {
        for (int i = 0; i < g.n; ++i)
        {
            if (g.degree(i) == 2)
                for (int j = g.head[i]; j; j = g.edge[j].next)
                {
                    if (g.degree(g.edge[j].b) == 2)
                        return false;
                }
        }
    }
    if (g.state & Property::NO_232)
    {
        for (int i = 0; i < g.n; ++i)
        {
            if (g.degree(i) == 3)
            {
                int cnt = 0;
                for (int j = g.head[i]; j; j = g.edge[j].next)
                {
                    if (g.degree(g.edge[j].b) == 2)
                    {
                        cnt++;
                    }
                }
                if (cnt >= 2)
                    return false;
            }
        }
    }
    if (g.state & Property::NO_cycle2333 || g.state & Property::NO_cycle23333 || g.state & Property::NO_cycle233333 || g.state & Property::NO_cycle2333333)
    {
        unordered_map<int, std::set<vector<int>>> cycles;
        if (g.state & Property::NO_cycle2333333)
            cycles = g.findCycles(7);
        else if (g.state & Property::NO_cycle233333)
            cycles = g.findCycles(6);
        else if (g.state & Property::NO_cycle23333)
            cycles = g.findCycles(5);
        else
            cycles = g.findCycles(4);
        auto cycleSet = cycles[0];
        if (g.state & Property::NO_cycle2333)
            for (auto &cycle : cycles[4])
                cycleSet.insert(cycle);
        if (g.state & Property::NO_cycle23333)
            for (auto &cycle : cycles[5])
                cycleSet.insert(cycle);
        if (g.state & Property::NO_cycle233333)
            for (auto &cycle : cycles[6])
                cycleSet.insert(cycle);
        if (g.state & Property::NO_cycle2333333)
            for (auto &cycle : cycles[7])
                cycleSet.insert(cycle);
        for (auto &cycle : cycleSet)
        {
            int cnt2 = 0, cnt3 = 0;
            for (int i = 0; i < cycle.size(); ++i)
            {
                cnt2 += g.degree(g.edge[cycle[i]].a) == 2;
                cnt3 += g.degree(g.edge[cycle[i]].a) == 3;
            }
            if (cnt2 == 1 && cnt2 + cnt3 == cycle.size())
                return false;
        }
    }
    if (g.state & Property::NO_cycle233233)
    {
        auto cycles = g.findCycles(6);

        for (auto x : cycles[6])
        {
            int has4 = false;
            SETWIDTH d3 = 0;
            for (auto y : x)
            {
                if (g.degree(g.edge[y].a) > 3)
                {
                    has4 = true;
                    break;
                }
                d3 <<= 1;
                if (g.degree(g.edge[y].a) == 3)
                {
                    d3 |= 1;
                }
            }
            if (has4)
                continue;
            if (d3 == 0b110110 || d3 == 0b101101 || d3 == 0b011011)
            {
                return false;
            }
        }
    }
    if (g.state & Property::NO_D3cycle3)
    {
        int minc = g.minimumCircleLength();
        if (minc == 3)
            return false;
        if (g.state & Property::NO_D3cycle4 && minc == 4)
            return false;
        if (g.state & Property::NO_D3cycle5 && minc == 5)
            return false;
        if (g.state & Property::NO_D3cycle6 && minc == 6)
            return false;
        if (g.state & Property::NO_D3cycle7 && minc == 7)
            return false;
        if (g.state & Property::NO_D3cycle8 && minc == 8)
            return false;
        if (g.state & Property::NO_D3cycle551)
        {
            if (cycleMatch(g, 5, 5, 1, 1))
                return false;
        }
        if (g.state & Property::NO_D3cycle571)
        {
            if (cycleMatch(g, 5, 7, 1, 1))
                return false;
        }
        if (g.state & Property::NO_D3cycle661)
        {
            if (cycleMatch(g, 6, 6, 1, 1))
                return false;
        }
        if (g.state & Property::NO_D3cycle771)
        {
            if (cycleMatch(g, 7, 7, 1, 1))
                return false;
        }
        else if (g.state & Property::NO_D3cycle772)
        {
            if (cycleMatch(g, 7, 7, 2, 2))
                return false;
        }
        else if (g.state & Property::NO_D3cycle773)
        {
            if (cycleMatch(g, 7, 7, 3, 3))
                return false;
        }
        if (g.state & Property::NO_D3cycle781)
        {
            if (cycleMatch(g, 7, 8, 1, 1))
                return false;
        }
        else if (g.state & Property::NO_D3cycle782)
        {
            if (cycleMatch(g, 7, 8, 2, 2))
                return false;
        }
    }
    if (g.state & Property::NO_D4cycle3)
    {
        int minc = g.minimumCircleLength();
        if (minc == 3)
            return false;
    }
    return true;
}

int cntall = 0, cntcor = 0;
vector<pair<Graph, vector<SETWIDTH>>> Graph::expand(vector<SETWIDTH> suggestBranchesBase) const
{
    int a = -1;
    if (n == 1)
        a = 0;
    if (state & Property::NO_D3)
    {
        for (int r = 1; a == -1 && r < MAXDEGREE; ++r)
        {
            for (int d = 4; a == -1 && d <= MAXDEGREE; ++d)
            {
                for (int i = 0; i < n; ++i)
                {
                    if (degree(i) == d && remain[i] == r)
                    {
                        a = i;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        for (int r = 1; a == -1 && r < 2; ++r)
        {
            for (int d = r + 1; a == -1 && d <= MAXDEGREE; ++d)
            {
                for (int i = 0; i < n; ++i)
                {
                    if (remain[i] == r && degree(i) == d)
                    {
                        a = i;
                        break;
                    }
                }
            }
        }
        if (a == -1)
            for (int i = 0; i < n; ++i)
            {
                if (remain[i])
                {
                    a = i;
                    break;
                }
            }
    }
    if (a == -1)
        return {};
    vector<pair<Graph, vector<SETWIDTH>>> ans;
    vector<Graph> q;
    int index = 0;
    for (int i = n - 1; i >= index; --i)
    {
        if (remain[i] == 0 || i == a)
            continue;
        int flag1 = 0;
        for (int j = head[a]; j; j = edge[j].next)
        {
            if (i == edge[j].b)
                flag1 = 1;
        }
        if (flag1)
            continue;

        Graph g = *this;
        g.link(a, i);
        q.push_back(g);
    }
    for (int newd = 2; newd <= MAXDEGREE; ++newd)
    {
        Graph g = *this;
        g.addVertex(n, newd);
        g.link(a, n);
        q.push_back(g);
    }
    for (auto &g : q)
    {
        vector<std::pair<vector<bool>, Measure>> resstate;
        vector<double> resweight;
        if (validate(g))
        {
            vector<SETWIDTH> suggestBranches;
            SETWIDTH a = 0, b = 1ll << g.edge[g._edge].a, c = 1ll << g.edge[g._edge].b, d = 1ll << g.edge[g._edge].a | 1ll << g.edge[g._edge].b, nei1 = 0, nei2 = 0;
            for (int i = g.head[g.edge[g._edge].a]; i; i = g.edge[i].next)
            {
                nei1 |= 1ll << g.edge[i].b;
            }
            for (int i = g.head[g.edge[g._edge].b]; i; i = g.edge[i].next)
            {
                nei2 |= 1ll << g.edge[i].b;
            }
            for (auto x : suggestBranchesBase)
            {
                suggestBranches.push_back(x | a);
                if ((x & nei1) == 0)
                    suggestBranches.push_back(x | b);
                if ((x & nei2) == 0)
                    suggestBranches.push_back(x | c);
                // suggestBranches.push_back(x | d);
            }
            ans.push_back({g, suggestBranches});
        }
    }
    return ans;
}
void Graph::bronKerbosch(SETWIDTH R, SETWIDTH P, SETWIDTH X, std::vector<SETWIDTH> &res) const
{
    if (P == 0 && X == 0)
    {
        res.push_back(R);
        return;
    }
    SETWIDTH T = P | X;
    SETWIDTH u = 0;
    for (int i = 0; i < n; ++i)
    {
        if (T & (1ll << i))
        {
            u = i;
            break;
        }
    }
    vector<int> vset;
    vset.push_back(u);
    for (int i = head[u]; i; i = edge[i].next)
    {
        vset.push_back(edge[i].b);
    }
    for (int i = 0; i < vset.size(); ++i)
        if (P & (1ll << vset[i]))
        {
            P ^= 1ll << vset[i];
            SETWIDTH newP = P, newX = X;
            for (int j = head[vset[i]]; j; j = edge[j].next)
            {
                newP &= ~(1ll << edge[j].b);
                newX &= ~(1ll << edge[j].b);
            }
            bronKerbosch(R | (1ll << vset[i]), newP, newX, res);
            X |= 1ll << vset[i];
            assert((P & X) == 0);
        }
}
vector<SETWIDTH> Graph::getMaximalIS() const
{
    int start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    vector<SETWIDTH> ans;
    SETWIDTH R = 0, P = (1ll << n) - 1, X = 0;
    bronKerbosch(R, P, X, ans);
    int end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return ans;
}
vector<SETWIDTH> Graph::getIS() const
{
    auto maximalIS = getMaximalIS();
    unordered_set<SETWIDTH> ans[MAXV];
    for (SETWIDTH i : maximalIS)
    {
        ans[countBit(i)].insert(i);
    }
    for (int i = n - 1; i; --i)
    {
        for (auto x : ans[i])
        {
            for (SETWIDTH j = x, del = lowbit(x); j; j ^= del, del = lowbit(j))
            {
                ans[i - 1].insert(x ^ del);
            }
        }
    }
    vector<SETWIDTH> res;
    for (int i = 1; i < n; ++i)
    {
        for (auto x : ans[i])
        {
            res.push_back(x);
        }
    }
    return res;
}
bool Graph::subgraphMatch(const unordered_set<std::string> &solved)
{
    for (int i = 1; i < n; ++i)
    {
        // https://programmingforinsomniacs.blogspot.com/2018/03/gospers-hack-explained.html
        SETWIDTH T = 1ll << i;
        SETWIDTH s = (1ll << i) - 1;
        while (s < T)
        {
            Graph g(state);
            auto vertices = getSubset(s);
            int oldtonew[n];
            memset(oldtonew, -1, sizeof(oldtonew));
            int _n = 0;
            for (auto i : vertices)
            {
                oldtonew[i] = _n++;
                g.addVertex(i, degree(i));
            }
            for (auto a : vertices)
            {
                for (int j = head[a]; j; j = edge[j].next)
                {
                    int b = edge[j].b;
                    if (a < b && oldtonew[b] >= 0)
                    {
                        g.link(oldtonew[a], oldtonew[b]);
                    }
                }
            }
            string ss = g.isolabel();
            if (solved.count(ss))
                return true;
            int c = s & -s;
            int r = s + c;
            s = (((r ^ s) >> 2) / c) | r;
        }
    }
    return false;
}

int Graph::minimumCircleLength() const
{
    int **e = new int *[n], **dis = new int *[n];
    for (int i = 0; i < n; ++i)
    {
        e[i] = new int[n];
        dis[i] = new int[n];
        memset(e[i], 0x3f, n * sizeof(int));
        memset(dis[i], 0x3f, n * sizeof(int));
    }
    for (int i = 2; i <= _edge; ++i)
        e[edge[i].a][edge[i].b] = dis[edge[i].a][edge[i].b] = 1;

    int ans = 0x3f3f3f;
    for (int k = 0; k < n; k++)
    {
        for (int i = 0; i < k; i++)
        {
            for (int j = i + 1; j < k; j++)
            {
                ans = std::min(ans, e[i][k] + e[k][j] + dis[i][j]);
            }
        }
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                dis[i][j] = std::min(dis[i][j], dis[i][k] + dis[k][j]);
            }
        }
    }
    for (int i = 0; i < n; ++i)
    {
        delete[] e[i];
        delete[] dis[i];
    }
    delete[] e;
    delete[] dis;
    return ans;
}

SETWIDTH Graph::findCycle(int length, int e, SETWIDTH ban) const
{
    std::deque<int> q;
    int x = edge[e].a, y = edge[e].b;
    SETWIDTH ans = 1ll << x;
    q.clear();
    q.push_back(x);
    int vis[MAXV] = {0}, from[MAXV];
    for (int i = 0; i < n; ++i)
        if ((1ll << i) & ban)
            vis[i] = 1e9;
    vis[x] = 1;
    while (!q.empty())
    {
        int a = q.front();
        q.pop_front();
        if (vis[a] > length)
            continue;
        if (a == y)
        {
            while (a != x)
            {
                ans |= 1ll << a;
                a = from[a];
            }
            return ans;
        }
        for (int j = head[a]; j; j = edge[j].next)
        {
            int b = edge[j].b;
            if (vis[b] || j >> 1 == e >> 1)
                continue;
            vis[b] = vis[a] + 1;
            from[b] = edge[j].a;
            q.push_back(b);
        }
    }
    return false;
}
std::unordered_map<int, std::set<std::vector<int>>> Graph::findCycles(int length) const
{
    std::unordered_map<int, std::set<std::vector<int>>> ans;
    std::deque<std::tuple<int, int, std::vector<int>>> q; // now start path
    for (int i = 0; i < n; ++i)
    {
        q.push_back({i, i, {}});
    }
    while (q.size())
    {
        auto [now, start, path] = q.front();
        q.pop_front();
        if (now == start && path.size())
        {
            ans[path.size()].insert(path);
            continue;
        }
        if (path.size() >= length)
            continue;
        for (int i = head[now]; i; i = edge[i].next)
        {
            int b = edge[i].b;
            if (b < start)
                continue;
            int flag = 0;
            for (int j = 0; j < path.size(); ++j)
            {
                if (path[j] >> 1 == i >> 1)
                {
                    flag = 1;
                    break;
                }
            }
            if (flag)
                continue;
            vector<int> newpath = path;
            newpath.push_back(i);
            q.push_back({b, start, newpath});
        }
    }
    unordered_map<int, std::set<vector<int>>> ans2;
    for (auto [k, v] : ans)
    {
        for (auto cycle : v)
        {

            if (cycle.front() > cycle.back())
            {
                std::reverse(cycle.begin(), cycle.end());
                for (auto &j : cycle)
                {
                    j ^= 1;
                }
            }
            ans2[k].insert(cycle);
        }
    }
    return ans2;
}