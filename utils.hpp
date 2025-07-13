#include "graph.hpp"
#include <thread>
#include <iomanip>
#include <map>

#define lowbit(x) ((x) & (-(x)))

std::unordered_set<string> solved;
std::mutex solvedMutex;
std::mutex coutMutex;
std::mutex queueMutex;
std::condition_variable cv;
std::atomic<int> isocnt(0);
std::atomic<int> cnt(0);
extern FILE *optfile;

double stoddefault(const std::string &str, double defaultValue);

void showHelp()
{
    std::cout << "Usage: ./main [options]\n";
    std::cout << "Options:\n";
    std::cout << "--exe=<sub-space name>    Solve specific sub-space\n";
    std::cout << "-o=<filename>             Save generated rules to file\n";
    std::cout << "--alpha=<value>           Set the alpha parameter\n";
    std::cout << "--beta1=<value>           Set the beta1 parameter (default: -alpha/4)\n";
    std::cout << "--beta2=<value>           Set the beta2 parameter (default: -alpha/2)\n";
    std::cout << "--beta=<value>            Set the beta parameter (default: 0)\n";
    std::cout << "-p=true/false             Enable or disable multi-threading (default: false)\n";
    std::cout << "-h, --help                Show this help message\n";
}
void fillDefaultParams(std::map<std::string, std::string> &params)
{
    params["-p"] = "false";
}
void parseArgs(int argc, char *argv[], std::map<std::string, std::string> &params)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            showHelp();
            exit(0);
        }
        size_t pos = arg.find('=');
        if (pos != std::string::npos)
        {
            std::string key = arg.substr(0, pos);
            params[key] = arg.substr(pos + 1);
            printf("%s=%s\n", key.c_str(), params[key].c_str());
        }
    }
}
std::map<std::string, std::string> init(int argc, char *argv[])
{
    std::map<std::string, std::string> params;
    fillDefaultParams(params);
    parseArgs(argc, argv, params);
    alpha = stoddefault(params["--alpha"], 0);
    beta[1] = stoddefault(params["--beta1"], 0);
    beta[2] = stoddefault(params["--beta2"], 0);
    beta[3] = stoddefault(params["--beta3"], 0);
    beta[4] = stoddefault(params["--beta4"], 0);
    beta[5] = stoddefault(params["--beta5"], 0);
    if (beta[1] == 0)
        beta[1] = -alpha / 4;
    if (beta[2] == 0)
        beta[2] = -alpha / 2;
    if (params["-o"] != "")
        optfile = fopen(params["-o"].c_str(), "w");
    else
        optfile = fopen("/dev/null", "w");
    printf("Alpha=%lf Beta1=%lf Beta2=%lf Beta3=%lf Beta4=%lf Beta5=%lf\n", alpha, beta[1], beta[2], beta[3], beta[4], beta[5]);
    return params;
}

double stoddefault(const std::string &str, double defaultValue)
{
    try
    {
        return std::stod(str);
    }
    catch (const std::invalid_argument &)
    {
        return defaultValue;
    }
    catch (const std::out_of_range &)
    {
        return defaultValue;
    }
}
string tod2(double x)
{
    char buf[15];
    sprintf(buf, "%.2lf", x);
    return buf;
}
Graph t3(int n)
{
    Graph g(Property::None);
    for (int i = 0; i < n; ++i)
    {
        g.addVertex(i, 3);
    }
    for (int i = 1; i < n; ++i)
    {
        g.link((i - 2) / 2, i);
    }
    return g;
}
Graph t2(int n)
{
    Graph g(Property::None);
    int gcnt = 0, ccnt = 3;
    int cnt1 = 1, remain = 1, ring = 1;
    for (int i = 0; i < n; ++i)
    {

        g.addVertex(i, 2 + ring);
        if (i)
        {
            if (ccnt == 0)
            {
                ccnt = 2 - ring;
                gcnt++;
            }
            g.link(gcnt, i);
            ccnt--;
        }
        remain--;
        if (remain == 0)
        {
            if (ring)
            {
                cnt1 *= cnt1 == 1 ? 3 : 2;
                ring = 0;
            }
            else
            {
                ring ^= 1;
            }
            remain = cnt1;
        }
    }
    return g;
}
vector<string> search(string boundary)
{
    vector<string> res;
    std::deque<string> q;
    q.push_back("");
    while (!q.empty())
    {
        string cur = q.front();
        q.pop_front();
        if (cur.size() == boundary.size())
        {
            res.push_back(cur);
            continue;
        }
        for (char i = '0'; i <= boundary[q.size()]; ++i)
        {
            q.push_back(cur + i);
        }
    }
    return res;
}
double computeCost(const tuple<Graph, vector<tuple<vector<bool>, Measure, SETWIDTH>>, vector<double>> &leaf)
{

    double sum = 0;
    for (int i = 0; i < std::get<1>(leaf).size(); ++i)
    {
        double w = alpha * get<1>(get<1>(leaf)[i]).k + beta[1] * get<1>(get<1>(leaf)[i]).n1 + beta[2] * get<1>(get<1>(leaf)[i]).n2 + beta[3] * get<1>(get<1>(leaf)[i]).n3;
        w = std::pow(2, w) * std::get<2>(leaf)[i];
        sum += w;
    }
    return sum;
}
int solveinner(vector<pair<Graph, vector<SETWIDTH>>> gs, vector<tuple<Graph, vector<tuple<vector<bool>, Measure, SETWIDTH>>, vector<double>>> &leaves, bool print = true, double eps = 0)
{
    leaves.clear();
    int currentCnt = gs.size(), lastdepth = 0, remain = gs.size();
    int expandcnt = 0;
    int sicnt = 0;
    int flcnt = 0;
    std::deque<tuple<int, Graph, vector<SETWIDTH>>> q;
    for (auto g : gs)
    {
        q.push_back({0, g.first, g.second});
    }
    while (!q.empty())
    {
        auto [depth, g, branches] = q.front();
        q.pop_front();
        if (g.n > MAXV)
        {
            perror("TOO LARGE");
            continue;
        }
        if (depth != lastdepth)
        {
            lastdepth = depth;
            remain = q.size() + 1;
            currentCnt = q.size() + 1;
            expandcnt = 0;
        }
        string isolabel = g.isolabel();
        SETWIDTH mask = 0;
        if (solved.count(isolabel))
        {
            isocnt++;
            --remain;
            continue;
        }
        solved.insert(isolabel);
        ++cnt;
        if (print)
        {
            printf("%d/%d/%d/%.2f n=%d %s ", depth, remain, currentCnt, (double)expandcnt / (currentCnt - remain), g.n, g.toString().c_str());
            remain--;
            for (int i = 0; i < g.n; ++i)
            {
                printf("%d", g.remain[i]);
            }
        }
        SimplifyInfo info;
        int res = g.simplify(info, true);
        if (res)
        {
            if (print)
                std::cout << " SIMP " << res << " " << cnt << "\n";
            continue;
        }
        vector<tuple<vector<bool>, Measure, SETWIDTH>> resstate;
        vector<double> resweight;
        double branchRes = g.tryBranching(eps, resstate, resweight, branches);
        if (branchRes <= 1 - eps)
        {
            if (print)
            {
                std::cout << " " << branchRes << " BRANCH " << cnt << "\n";

                fprintf(optfile, "LC %d:\n%s", cnt.load(), g.toReadableString().c_str());
                fprintf(optfile, "Branching rule:");
                int id = 0, cnt = 0;
                for (int i = 0; i < resstate.size(); ++i)
                {
                    if (resweight[i] < 1e-4)
                        continue;
                    cnt++;
                }
                fprintf(optfile, " %d\n", cnt);
                for (int i = 0; i < resstate.size(); ++i)
                {
                    if (resweight[i] < 1e-4)
                        continue;
                    fprintf(optfile, "#%d:", id++);
                    int cnt = countBit(get<2>(resstate[i]));
                    fprintf(optfile, " %d", cnt);
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
            leaves.push_back({g, resstate, resweight});
            continue;
        }
        if (print)
            std::cout << " " << branchRes << " EXPAND " << cnt << "\n";
        expandcnt++;
        vector<SETWIDTH> suggestBranches;
        for (auto [state, measure, mask] : resstate)
        {
            suggestBranches.push_back(mask);
        }
        auto gs = g.expand(suggestBranches);
        for (int i = 0; i < gs.size(); ++i)
        {
            q.push_back({depth + 1, gs[i].first, gs[i].second});
        }
    }
    return cnt;
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
                fprintf(optfile, "Branching rule:");
                int id = 0, cnt = 0;
                for (int i = 0; i < resstate.size(); ++i)
                {
                    if (resweight[i] < 1e-4)
                        continue;
                    cnt++;
                }
                fprintf(optfile, " %d\n", cnt);
                for (int i = 0; i < resstate.size(); ++i)
                {
                    if (resweight[i] < 1e-4)
                        continue;
                    fprintf(optfile, "#%d:", id++);
                    int cnt = countBit(get<2>(resstate[i]));
                    fprintf(optfile, " %d", cnt);
                    for (int j = 0; j < g.n; ++j)
                    {
                        if (get<2>(resstate[i]) & (1ll << j))
                        {
                            fprintf(optfile, " %d", j);
                        }
                    }
                    fprintf(optfile, " / %.6lf %.6lf\n", resweight[i], get<1>(resstate[i]).cost());
                }
                fprintf(optfile, "\n");
            }
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
int solve3(vector<Graph> gs, int ncpu = std::thread::hardware_concurrency() - 2, bool print = true)
{
    vector<tuple<Graph, vector<tuple<vector<bool>, Measure, SETWIDTH>>, vector<double>>> leaves;
    vector<pair<Graph, vector<SETWIDTH>>> gs1;
    for (auto g : gs)
    {
        gs1.push_back({g, {}});
    }
    return solveinnerParallel(gs1, leaves, ncpu, print);
}