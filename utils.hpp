#include "graph.hpp"
#include <thread>
#include <iomanip>

#define lowbit(x) ((x) & (-(x)))
extern FILE *optfile;

std::unordered_set<string> solved;
std::mutex solvedMutex;
std::mutex coutMutex;
std::mutex queueMutex;
std::condition_variable cv;
std::atomic<int> isocnt(0);
std::atomic<int> cnt(0);
string tod2(double x)
{
    char buf[15];
    sprintf(buf, "%.2lf", x);
    return buf;
}
void worker(std::deque<tuple<int, Graph, vector<SETWIDTH>, int, string>> &q, vector<tuple<Graph, vector<tuple<vector<bool>, Measure, SETWIDTH>>, vector<double>>> &leaves, bool print, double eps, std::atomic<int> &expandcnt, std::atomic<int> &currentCnt, std::atomic<int> &currentDepth, std::atomic<int> &remain, std::atomic<int> &busyThreads)
{
    while (1)
    {
        tuple<int, Graph, vector<SETWIDTH>, int, string> task;
        bool exist = false;
        int ith, r;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [&]
                    { 
                    if(q.size()&&get<0>(q.front())==currentDepth)
                    {
                        task = q.front();
                        q.pop_front();
                        ++busyThreads;
                        cv.notify_all();

                        r=remain--;
                        if (!(exist = solved.count(get<4>(task))))
                            solved.insert(get<4>(task)),
                            ith=++cnt;
                        
                        return true;
                    }
                    if(q.size()&&busyThreads==0)
                    {
                        currentDepth = get<0>(q.front());
                        currentCnt = q.size();
                        remain = q.size();
                        expandcnt=0;
                        // stable_sort(q.begin(),q.end(),[](const auto &a,const auto &b){return get<1>(a).n==get<1>(b).n?get<3>(a)<get<3>(b):get<1>(a).n>get<1>(b).n;});
                        stable_sort(q.begin(),q.end(),[](const auto &a,const auto &b){return get<3>(a)<get<3>(b);});


                        task = q.front();
                        q.pop_front();
                        ++busyThreads;
                        cv.notify_all();

                        r=remain--;
                        if (!(exist = solved.count(get<4>(task))))
                            solved.insert(get<4>(task)),
                            ith=++cnt;
                        return true;
                    }
                    return busyThreads==0; });
        }
        if (busyThreads == 0)
        {
            break;
        }
        if (exist)
        {
            isocnt++;
            busyThreads--;
            cv.notify_all();
            continue;
        }
        auto [depth, g, branches, from, isolabel] = task;
        string prefix = std::to_string(depth) + "/" + std::to_string(r) + "/" + std::to_string(currentCnt) + "/" + tod2((double)expandcnt / (currentCnt - r)) + " n=" + std::to_string(g.n) + " " + g.toString() + " ";

        for (int i = 0; i < g.n; ++i)
        {
            prefix += std::to_string(g.remain[i]);
        }

        SimplifyInfo info;
        int res = g.simplify(info, true);
        if (res)
        {
            if (print)
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << prefix << " SIMP " << res << " " << from << " --> " << ith << "\n";

                fprintf(optfile, "LC %d:\n%s", ith, g.toReadableString().c_str());
                fprintf(optfile, "Simplification rule applied\n\n");
            }
            busyThreads--;
            cv.notify_all();
            continue;
        }

        vector<tuple<vector<bool>, Measure, SETWIDTH>> resstate;
        vector<double> resweight;
        double branchRes = g.tryBranching(eps, resstate, resweight, branches);
        if (branchRes <= 1 - eps)
        {
            if (print)
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << prefix << " " << branchRes << " BRANCH " << from << " --> " << ith << "\n";
                fprintf(optfile, "LC %d:\n%s", ith, g.toReadableString().c_str());
                fprintf(optfile, "Branching rule:\n");
                int id = 0;
                for (int i = 0; i < resstate.size(); ++i)
                {
                    if (resweight[i] < 1e-4)
                        continue;
                    fprintf(optfile, "#%d:", id++);
                    for (int j = 0; j < g.n; ++j)
                    {
                        if (get<2>(resstate[i]) & (1ll << j))
                        {
                            fprintf(optfile, " %d", j);
                        }
                    }
                    fprintf(optfile, " / %.4lf\n", resweight[i]);
                }
                fprintf(optfile, "\n");
            }
            // leaves.push_back({g, resstate, resweight}); // We don't use automated measure optimization.
            busyThreads--;
            cv.notify_all();
            continue;
        }
        if (print)
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << prefix << " " << branchRes << " EXPAND " << from << " --> " << ith << "\n";
        }
        expandcnt++;

        vector<SETWIDTH> suggestBranches;
        for (auto [state, measure, mask] : resstate)
        {
            suggestBranches.push_back(mask);
        }
        auto gs = g.expand(suggestBranches);
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (const auto &newG : gs)
            {
                q.push_back({depth + 1, newG.first, newG.second, ith, newG.first.isolabel()});
            }
            busyThreads--;
            cv.notify_all();
        }
    }
}
int solveinnerParallel(vector<pair<Graph, vector<SETWIDTH>>> gs, vector<tuple<Graph, vector<tuple<vector<bool>, Measure, SETWIDTH>>, vector<double>>> &leaves, int ncpu, bool print = true, double eps = 0)
{
    // std::cout << std::fixed << std::setprecision(2);
    leaves.clear();
    std::atomic<int> currentCnt(gs.size()), lastdepth(0), remain(gs.size()), currentDepth(0);
    std::atomic<int> expandcnt(0);
    std::deque<tuple<int, Graph, vector<SETWIDTH>, int, string>> q;
    for (const auto &g : gs)
    {
        q.push_back({0, g.first, g.second, 0, g.first.isolabel()});
    }

    std::deque<std::thread> workers;
    const int maxThreads = ncpu;
    std::atomic<int> busyThreads(0);

    for (int i = 0; i < maxThreads; ++i)
    {
        workers.emplace_back(worker, std::ref(q), std::ref(leaves), print, eps, std::ref(expandcnt), std::ref(currentCnt), std::ref(remain), std::ref(currentDepth), std::ref(busyThreads));
    }
    for (auto &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    return cnt;
}
int solve3(vector<Graph> gs, int ncpu = std::thread::hardware_concurrency(), bool print = true)
{
    vector<tuple<Graph, vector<tuple<vector<bool>, Measure, SETWIDTH>>, vector<double>>> leaves;
    vector<pair<Graph, vector<SETWIDTH>>> gs1;
    for (auto g : gs)
    {
        gs1.push_back({g, {}});
    }
    return solveinnerParallel(gs1, leaves, ncpu, print);
}