#include "graph.hpp"

int countSameEdges(const std::vector<int> &a, const std::vector<int> &b);
bool cycleMatch(const Graph &g, int l1, int l2, int min, int max); // return true if there are two cycles with length l has at least match number of same edges