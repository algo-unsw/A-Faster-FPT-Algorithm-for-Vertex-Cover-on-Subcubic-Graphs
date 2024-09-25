#define _CRT_SECURE_NO_DEPRECATE
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
#include <map>
#include <assert.h>
#include <thread>

#include "utils.hpp"
FILE *optfile;

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
    // params["--alpha"] = "0";
    params["--beta1"] = "0";
    params["--beta2"] = "0";
    params["--beta"] = "0";
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
    if (params.count("--alpha") == 0)
    {
        std::cerr << "Error: --alpha parameter is required\n";
        showHelp();
        exit(1);
    }
}
long getPeakRSS()
{
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    return rusage.ru_maxrss;
}
int main(int argc, char *argv[])
{
    // printf("%s\n%s\n", t2(31).toString().c_str(), t3(22).toString().c_str());
    std::map<std::string, std::string> params;
    fillDefaultParams(params);
    parseArgs(argc, argv, params);
    alpha = stod(params["--alpha"]);
    beta1 = stod(params["--beta1"]);
    beta2 = stod(params["--beta2"]);
    // beta3 = stod(params["--beta3"]);
    beta = stod(params["--beta"]);
    assert(MAXDEGREE == 3);
    if (beta1 == 0)
        beta1 = -alpha / 4;
    if (beta2 == 0)
        beta2 = -alpha / 2;
    if (params["-o"] != "")
        optfile = fopen(params["-o"].c_str(), "w");
    else
        optfile = fopen("/dev/null", "w");
    printf("Alpha=%lf Beta1=%lf Beta2=%lf Beta3=%lf Beta=%lf\n", alpha, beta1, beta2, beta3, beta);
    bool multiThread = params["-p"] == "true";
    std::vector<std::pair<int, int>> info;
    std::map<string, Graph> gmap;

    Graph g232 = Graph(Property::None);
    g232.addVertex(0, 2);
    g232.addVertex(1, 3);
    g232.addVertex(2, 2);
    g232.link(0, 1);
    g232.link(1, 2);
    gmap["g232"] = g232;
    // solve1({g232});

    Graph bc2333 = Graph(Property::NO_232);
    bc2333.addVertex(0, 2);
    bc2333.addVertex(1, 3);
    bc2333.addVertex(2, 3);
    bc2333.addVertex(3, 3);
    bc2333.link(0, 1);
    bc2333.link(1, 2);
    bc2333.link(2, 3);
    bc2333.link(3, 0);
    gmap["bc2333"] = bc2333;
    // solve1({bc2333});

    Graph bc23333 = Graph(Property::NO_232 | Property::NO_cycle2333);
    bc23333.addVertex(0, 2);
    bc23333.addVertex(1, 3);
    bc23333.addVertex(2, 3);
    bc23333.addVertex(3, 3);
    bc23333.addVertex(4, 3);
    bc23333.link(0, 1);
    bc23333.link(1, 2);
    bc23333.link(2, 3);
    bc23333.link(3, 4);
    bc23333.link(4, 0);
    gmap["bc23333"] = bc23333;
    // solve1({bc2333333});
    Graph bc233333 = Graph(Property::NO_232 | Property::NO_cycle2333 | Property::NO_cycle23333);
    bc233333.addVertex(0, 2);
    bc233333.addVertex(1, 3);
    bc233333.addVertex(2, 3);
    bc233333.addVertex(3, 3);
    bc233333.addVertex(4, 3);
    bc233333.addVertex(5, 3);
    bc233333.link(0, 1);
    bc233333.link(1, 2);
    bc233333.link(2, 3);
    bc233333.link(3, 4);
    bc233333.link(4, 5);
    bc233333.link(5, 0);
    gmap["bc233333"] = bc233333;

    Graph g2 = Graph(Property::NO_232 | Property::NO_cycle2333 | Property::NO_cycle23333 | Property::NO_cycle233333);
    g2.addVertex(0, 2);
    gmap["g2"] = g2;
    // solve1({g2});

    Graph c3 = Graph(Property::NO_D2);
    c3.addVertex(0, 3);
    c3.addVertex(1, 3);
    c3.addVertex(2, 3);
    c3.link(0, 1);
    c3.link(0, 2);
    c3.link(1, 2);
    gmap["c3"] = c3;
    // solve1({c3});

    Graph c4 = Graph(Property::NO_D2 | Property::NO_D3cycle3);
    c4.addVertex(0, 3);
    c4.addVertex(1, 3);
    c4.addVertex(2, 3);
    c4.addVertex(3, 3);
    c4.link(0, 1);
    c4.link(1, 2);
    c4.link(2, 3);
    c4.link(0, 3);
    gmap["c4"] = c4;
    // solve1({c4});

    Graph c551 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4);
    c551.addVertex(0, 3);
    c551.addVertex(1, 3);
    c551.addVertex(2, 3);
    c551.addVertex(3, 3);
    c551.addVertex(4, 3);
    c551.addVertex(5, 3);
    c551.addVertex(6, 3);
    c551.addVertex(7, 3);
    c551.link(0, 1);
    c551.link(1, 2);
    c551.link(2, 3);
    c551.link(3, 4);
    c551.link(0, 4);
    c551.link(1, 5);
    c551.link(5, 6);
    c551.link(6, 7);
    c551.link(0, 7);
    gmap["c551"] = c551;
    // solve1({c551});

    Graph c571 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle551);
    c571.addVertex(0, 3);
    c571.addVertex(1, 3);
    c571.addVertex(2, 3);
    c571.addVertex(3, 3);
    c571.addVertex(4, 3);
    c571.addVertex(5, 3);
    c571.addVertex(6, 3);
    c571.addVertex(7, 3);
    c571.addVertex(8, 3);
    c571.addVertex(9, 3);
    c571.link(0, 1);
    c571.link(1, 2);
    c571.link(2, 3);
    c571.link(3, 4);
    c571.link(0, 4);
    c571.link(1, 5);
    c571.link(5, 6);
    c571.link(6, 7);
    c571.link(7, 8);
    c571.link(8, 9);
    c571.link(0, 9);
    gmap["c571"] = c571;
    // solve1({c571});

    Graph c5 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle551 | Property::NO_D3cycle571);
    c5.addVertex(0, 3);
    c5.addVertex(1, 3);
    c5.addVertex(2, 3);
    c5.addVertex(3, 3);
    c5.addVertex(4, 3);
    c5.link(0, 1);
    c5.link(1, 2);
    c5.link(2, 3);
    c5.link(3, 4);
    c5.link(0, 4);
    gmap["c5"] = c5;
    // solve1({c5});

    Graph c661 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5);
    c661.addVertex(0, 3);
    c661.addVertex(1, 3);
    c661.addVertex(2, 3);
    c661.addVertex(3, 3);
    c661.addVertex(4, 3);
    c661.addVertex(5, 3);
    c661.addVertex(6, 3);
    c661.addVertex(7, 3);
    c661.addVertex(8, 3);
    c661.addVertex(9, 3);
    c661.link(0, 1);
    c661.link(1, 2);
    c661.link(2, 3);
    c661.link(3, 4);
    c661.link(4, 5);
    c661.link(0, 5);
    c661.link(1, 9);
    c661.link(9, 8);
    c661.link(8, 7);
    c661.link(7, 6);
    c661.link(0, 6);
    gmap["c661"] = c661;
    // solve1({c661});

    Graph c6 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5 | Property::NO_D3cycle661);
    c6.addVertex(0, 3);
    c6.addVertex(1, 3);
    c6.addVertex(2, 3);
    c6.addVertex(3, 3);
    c6.addVertex(4, 3);
    c6.addVertex(5, 3);
    c6.link(0, 1);
    c6.link(1, 2);
    c6.link(2, 3);
    c6.link(3, 4);
    c6.link(4, 5);
    c6.link(0, 5);
    gmap["c6"] = c6;
    // solve1({c6});

    Graph c773 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5 | Property::NO_D3cycle6);
    c773.addVertex(0, 3);
    c773.addVertex(1, 3);
    c773.addVertex(2, 3);
    c773.addVertex(3, 3);
    c773.addVertex(4, 3);
    c773.addVertex(5, 3);
    c773.addVertex(6, 3);
    c773.addVertex(7, 3);
    c773.addVertex(8, 3);
    c773.addVertex(9, 3);
    c773.link(0, 1);
    c773.link(1, 2);
    c773.link(2, 3);
    c773.link(3, 4);
    c773.link(4, 5);
    c773.link(5, 6);
    c773.link(0, 6);
    c773.link(0, 7);
    c773.link(7, 8);
    c773.link(8, 9);
    c773.link(3, 9);
    gmap["c773"] = c773;
    // solve1({c773});

    Graph c772 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5 | Property::NO_D3cycle6 | Property::NO_D3cycle773);
    c772.addVertex(0, 3);
    c772.addVertex(1, 3);
    c772.addVertex(2, 3);
    c772.addVertex(3, 3);
    c772.addVertex(4, 3);
    c772.addVertex(5, 3);
    c772.addVertex(6, 3);
    c772.addVertex(7, 3);
    c772.addVertex(8, 3);
    c772.addVertex(9, 3);
    c772.addVertex(10, 3);
    c772.link(0, 1);
    c772.link(1, 2);
    c772.link(2, 3);
    c772.link(3, 4);
    c772.link(4, 5);
    c772.link(5, 6);
    c772.link(0, 6);
    c772.link(0, 7);
    c772.link(7, 8);
    c772.link(8, 9);
    c772.link(9, 10);
    c772.link(2, 10);
    gmap["c772"] = c772;
    // solve1({c772});

    Graph c771 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5 | Property::NO_D3cycle6 | Property::NO_D3cycle772 | Property::NO_D3cycle773);
    c771.addVertex(0, 3);
    c771.addVertex(1, 3);
    c771.addVertex(2, 3);
    c771.addVertex(3, 3);
    c771.addVertex(4, 3);
    c771.addVertex(5, 3);
    c771.addVertex(6, 3);
    c771.addVertex(7, 3);
    c771.addVertex(8, 3);
    c771.addVertex(9, 3);
    c771.addVertex(10, 3);
    c771.addVertex(11, 3);
    c771.link(0, 1);
    c771.link(1, 2);
    c771.link(2, 3);
    c771.link(3, 4);
    c771.link(4, 5);
    c771.link(5, 6);
    c771.link(0, 6);
    c771.link(0, 7);
    c771.link(7, 8);
    c771.link(8, 9);
    c771.link(9, 10);
    c771.link(10, 11);
    c771.link(1, 11);
    gmap["c771"] = c771;

    Graph c7 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5 | Property::NO_D3cycle6 | Property::NO_D3cycle771 | Property::NO_D3cycle772 | Property::NO_D3cycle773);
    c7.addVertex(0, 3);
    c7.addVertex(1, 3);
    c7.addVertex(2, 3);
    c7.addVertex(3, 3);
    c7.addVertex(4, 3);
    c7.addVertex(5, 3);
    c7.addVertex(6, 3);
    c7.link(0, 1);
    c7.link(1, 2);
    c7.link(2, 3);
    c7.link(3, 4);
    c7.link(4, 5);
    c7.link(5, 6);
    c7.link(0, 6);
    gmap["c7"] = c7;
    // solve1({c7});

    Graph c8 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5 | Property::NO_D3cycle6 | Property::NO_D3cycle7);
    c8.addVertex(0, 3);
    c8.addVertex(1, 3);
    c8.addVertex(2, 3);
    c8.addVertex(3, 3);
    c8.addVertex(4, 3);
    c8.addVertex(5, 3);
    c8.addVertex(6, 3);
    c8.addVertex(7, 3);
    c8.link(0, 1);
    c8.link(1, 2);
    c8.link(2, 3);
    c8.link(3, 4);
    c8.link(4, 5);
    c8.link(5, 6);
    c8.link(6, 7);
    c8.link(0, 7);
    gmap["c8"] = c8;
    // solve1({c8});

    Graph g3 = Graph(Property::NO_D2 | Property::NO_D3cycle3 | Property::NO_D3cycle4 | Property::NO_D3cycle5 | Property::NO_D3cycle6 | Property::NO_D3cycle7 | Property::NO_D3cycle8);
    g3.addVertex(0, 3);
    gmap["g3"] = g3;

    vector<Graph> exe;
    if (params.count("--exe"))
    {
        exe = {gmap[params["--exe"]]};
    }
    else
    {
        for (auto g : gmap)
        {
            exe.push_back(g.second);
        }
    }
    if (multiThread)
    {
        solve3(exe);
    }
    else
        solve3(exe, 1);
    printf("Alpha=%lf Beta1=%lf Beta2=%lf done\n", alpha, beta1, beta2);
    printf("Isomorphism Detected=%d\n", isocnt.load());
    std::cout << "Peak memory usage: " << getPeakRSS() / 1000000.0 << " MB" << std::endl; // The unit may not correct depending on the system
    printf("#Vertex Cover: %lu\n", vertexCoverSize.size());
    return 0;
}