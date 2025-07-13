## A Faster Algorithm for Vertex Cover: A Randomized Automated Approach

### Environment

- Apple clang version 15.0.0 (clang-1500.3.9.4)
- CPU: Apple M3 Max
- Memory: 48GB

### Compile

```shell
make
```

### Run

The following command is an example to generate all rules for $\alpha=0.3$, which gives an algorithm runs in $O(1.2312^k)$ time.

```shell
./main --alpha=0.3 -p=true -o=rules.txt
```

#### Explanation of the arguments:

- `--alpha=0.3`: The value of alpha, which is the desired running time of the algorithm.
- `-p=true`: Enable the parallel version of the algorithm. (Which may output the results in a different order)
- `-o=rules.txt`: The output file to store the rules. (This file will be VERY large if alpha is small)
- `--exe=c3`: Only generate rules for the specified subspace.

#### Explanation of terminal output

The terminal output is in the following format:

```
[DFS depth]/[#local configurations left in this depth]/[#local configurations in this depth]/[expansion rate] n=[number of vertices] [nauty style encoded graph] [bitwise representation of #incomplete edges] [Optional: LP objective value] [result: EXPAND/BRANCH/SIMP #rule_id] [id of the parent local configuration] --> [id of this local configuration]
```

Example:

```
20/268/5184/0.11 n=21 TkH?__GA?G?@@???_O??@?_??@GAA?A????@ 000000002202000201002 0.941753 BRANCH 51293 --> 55543

Depth of expansion tree: 20
Number of local configurations left in this depth: 268
Number of local configurations in this depth: 5184 (Note that not all 5184 local configurations are shown in the output, some of them are skipped because they are isomorphic to the previous ones)
There are 11% solved local configurations triggered expansion in this depth
This graph has 21 vertices
This graph is encoded as TkH?__GA?G?@@???_O??@?_??@GAA?A????@ (see https://pallini.di.uniroma1.it/)
Vertex 8 has 2 incomplete edges, vertex 9 has 2 incomplete edges, vertex 11 has 2 incomplete edge...
The LP objective value is 0.941753, which is less than 1, so there is a branching rule for this local configuration
This local configuration has ID 55543, and its parent local configuration has ID 51293
```

#### Explanation of generated rules

Each rule and its corresponding local configuration is presented in the following format:

```
LC [id]:
n=[number of vertices] m=[number of edges]
[a number, the i-th digit is the number of incomplete edges of vertex i]
[adjacency list]
...
Branching rule: [number of branches]
#0: [number of vertices] [vertex1] [vertex2] ... / [weight]
#1: [number of vertices] [vertex1] [vertex2] ... / [weight]
...
```

Note that the id of the local configuration is not continuous because not all local configurations can be solved by the branching rules.

#### Abbreviations and corresponding subspaces

The following abbreviations are used in the program to specify the subspace to generate rules for: e.g. `--exe=c3`

1. Null: Contains a degree $0$ or $1$ vertex. (This subspace is not explicitly defined, as it is trivial to solve)
2. g232: Contains a degree $3$ vertex adjacent to at least two degree $2$ vertices, excluding instances from $P_1$.
3. bc2333: Contains a 4-cycle with three degree $3$ vertices and one degree $2$ vertex, excluding instances from previous subspaces.
4. bc23333: Contains a 5-cycle with four degree $3$ vertices and one degree $2$ vertex, excluding instances from previous subspaces.
5. bc233333: Contains a 6-cycle with five degree $3$ vertices and one degree $2$ vertex, excluding previous subspaces.
6. g2: Contains a degree $2$ vertex, excluding instances from previous subspaces.
7. c3: Contains a 3-cycle, excluding previous subspaces.
8. c4: Contains a 4-cycle, excluding previous subspaces.
9. c551: Contains two 5-cycles sharing an edge, excluding instances from previous subspaces.
10. c571: Contains a 5-cycle and a 7-cycle sharing an edge, excluding previous subspaces.
11. c5: Contains a 5-cycle, excluding previous subspaces.
12. c661: Contains two 6-cycles sharing an edge, excluding previous subspaces.
13. c6: Contains a 6-cycle, excluding previous subspaces.
14. c773: Contains two 7-cycles sharing three edges, excluding previous subspaces.
15. c772: Contains two 7-cycles sharing two edges, excluding previous subspaces.
16. c771: Contains two 7-cycles sharing one edge, excluding previous subspaces.
17. c7: Contains a 7-cycle, excluding all previous subspaces.
18. c8: Contains an 8-cycle, excluding all previous subspaces.
19. g3: Contains no structures defined by the previous subspaces.
