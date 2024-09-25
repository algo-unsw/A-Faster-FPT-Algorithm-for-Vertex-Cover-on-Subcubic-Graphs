#ifndef GRAPH_HPP
#define GRAPH_HPP
#include <stdio.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <random>
#include <queue>
#include <bitset>
#include <sstream>
#include <set>
#include <stack>
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <condition_variable>
#include <sys/resource.h>
#include <atomic>
#include <shared_mutex>

#define MAXV 64
#define SETWIDTH uint64_t
#ifndef MAXDEGREE
#define MAXDEGREE 3
#endif
#define lowbit(x) ((x) & (-(x)))
#define countBit(x) __builtin_popcountll(x)
#define lastPos(x) __builtin_ctzll(x)
#define firstPos(x) (std::numeric_limits<SETWIDTH>::digits - __builtin_clzll(x) - 1)
using std::string, std::get, std::pair, std::tuple, std::vector, std::unordered_map, std::unordered_set, std::to_string;
extern std::unordered_map<string, int> vertexCoverSize;
extern FILE *outputfile;
extern double alpha, beta1, beta2, beta3, beta;
string bitsetToString(SETWIDTH x, int n = 0);
enum SimplifyType
{
    NotSimplified = 0,
    Isolated = 1 << 0,
    Degree1 = 1 << 1,
    Degree2Triangle = 1 << 2,
    Degree2Chain = 1 << 3,
    AlternatingCycle = 1 << 4
};
enum Property
{
    None = 0,
    NO_232 = 1,
    NO_D2 = 1 << 1,
    NO_D3 = 1 << 20,
    NO_D3cycle3 = 1 << 2,
    NO_D3cycle4 = 1 << 3,
    NO_D3cycle5 = 1 << 4,
    NO_D3cycle6 = 1 << 5,
    NO_D3cycle7 = 1 << 6,
    NO_D3cycle8 = 1 << 7,

    NO_D3cycle551 = 1 << 8,
    NO_D3cycle571 = 1 << 9,
    NO_D3cycle661 = 1 << 10,
    NO_D3cycle771 = 1 << 11,
    NO_D3cycle772 = 1 << 12,
    NO_D3cycle773 = 1 << 13,
    NO_D3cycle781 = 1 << 14,
    NO_D3cycle782 = 1 << 15,

    NO_cycle2333 = 1 << 16,
    NO_cycle23333 = 1 << 17,
    NO_cycle233333 = 1 << 18,
    NO_cycle2333333 = 1 << 19,
    NO_cycle233233 = 1 << 21,

    NO_D4cycle3 = 1 << 22,
};
using PropertySet = unsigned;
using SimplifyTypeSet = unsigned;
struct SimplifyInfo
{
    SETWIDTH deleted, selected;
    SimplifyTypeSet type;
    SimplifyInfo() : deleted(0), selected(0), type(NotSimplified) {}
};
extern SimplifyInfo ignoreInfo;
struct Measure
{
    int k, n, n3, n2, n1;
    Measure(int k, int n, int n3, int n2, int n1) : k(k), n(n), n3(n3), n2(n2), n1(n1) {}
    double cost() const
    {
        return pow(2, alpha * k + beta * n + beta3 * n3 + beta2 * n2 + beta1 * n1);
    }
};
class Graph
{
public:
    int n = 0, head[MAXV] = {0}, _edge = 1;
    int remain[MAXV] = {0};
    PropertySet state;
    struct Edge
    {
        int a, b, next;
    } edge[MAXV * MAXDEGREE + 2];
    // constructor
    Graph(PropertySet s = 0) : state(s), _edge(1), n(0) {}
    // core
    inline void addEdge(int a, int b);
    void addVertex(int x, int d);
    void link(int a, int b);
    // bool isConnected() const;
    int solveVertexCover(bool checkSimplify = true) const;
    // double mu() const;
    int degree(int x) const;
    inline int count_n(int d) const;

    // transformation
    inline void deleteVertex(SETWIDTH x);
    // bool canBeSimplified() const;
    int simplify(SimplifyInfo &info = ignoreInfo, bool check = false); // check=False: return how many vertices been chosen in vertex cover, check=True: return the id of first simp rule
    inline Graph deleteVertexCopy(SETWIDTH x) const;
    inline Graph simplifyCopy(SimplifyInfo &info = ignoreInfo) const;

    // debug
    std::string toReadableString() const;

    // isomorphic
    // void finish();
    inline std::string getId() const;
    std::string toString() const;
    string isolabel() const;

    // graph theory
    void bronKerbosch(SETWIDTH R, SETWIDTH P, SETWIDTH X, vector<SETWIDTH> &res) const;
    vector<SETWIDTH> getMaximalIS() const;
    vector<SETWIDTH> getIS() const;
    int minimumCircleLength() const;
    SETWIDTH findCycle(int length, int edge, SETWIDTH ban) const;
    unordered_map<int, std::set<vector<int>>> findCycles(int length) const;
    vector<Graph> split() const;
    // tools
    inline bool evaluateBranch(SETWIDTH branch, const vector<SETWIDTH> &boundary, unordered_map<SETWIDTH, int> &VCSizeAfterDelete, unordered_map<SETWIDTH, pair<vector<bool>, Measure>> &curres, SETWIDTH guide = 0) const;
    vector<pair<Graph, vector<SETWIDTH>>> expand(vector<SETWIDTH> suggestBranchesBase) const;
    double tryBranching(double eps, vector<tuple<vector<bool>, Measure, SETWIDTH>> &resstate, vector<double> &resweight, vector<SETWIDTH> suggestBranches) const;
    bool subgraphMatch(const unordered_set<std::string> &solved);
};
Measure applyBranch(const Graph &before, SETWIDTH branch);

#endif