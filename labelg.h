/* labelg.c version 2.0; B D McKay, Jun 2015 */

#define USAGE "labelg [-q] [-sgz | -C#W#] [-fxxx] [-S|-t] \n\
                          [-i# -I#:# -K#] [infile [outfile]]"

#define HELPTEXT \
    " Canonically label a file of graphs or digraphs.\n\
\n\
    -s  force output to sparse6 format\n\
    -g  force output to graph6 format\n\
    -z  force output to digraph6 format\n\
        If neither -s, -g or -z are given, the output format is\n\
        determined by the header or, if there is none, by the\n\
        format of the first input graph. As an exception, digraphs\n\
        are always written in digraph6 format.\n\
    -S  Use sparse representation internally.\n\
         Note that this changes the canonical labelling.\n\
         Multiple edges are not supported.  One loop per vertex is ok.\n\
    -t  Use Traces.\n\
         Note that this changes the canonical labelling.\n\
         Multiple edges and loops are not supported, nor invariants.\n\
\n\
    -C# Make an invariant in 0..#-1 and output the number of graphs\n\
        with each value of the invariant.  Don't write graphs unless\n\
        -W too.\n\
    -W# (requires -C) Output the graphs with this invariant value,\n\
        in their original labelling.  Don't write the table.\n\
\n\
    The output file will have a header if and only if the input file does.\n\
\n\
    -fxxx  Specify a partition of the point set.  xxx is any\n\
        string of ASCII characters except nul.  This string is\n\
        considered extended to infinity on the right with the\n\
        character 'z'.  One character is associated with each point,\n\
        in the order given.  The labelling used obeys these rules:\n\
         (1) the new order of the points is such that the associated\n\
        characters are in ASCII ascending order\n\
         (2) if two graphs are labelled using the same string xxx,\n\
        the output graphs are identical iff there is an\n\
        associated-character-preserving isomorphism between them.\n\
        No option can be concatenated to the right of -f.\n\
\n\
    -i#  select an invariant (1 = twopaths, 2 = adjtriang(K), 3 = triples,\n\
        4 = quadruples, 5 = celltrips, 6 = cellquads, 7 = cellquins,\n\
        8 = distances(K), 9 = indsets(K), 10 = cliques(K), 11 = cellcliq(K),\n\
       12 = cellind(K), 13 = adjacencies, 14 = cellfano, 15 = cellfano2,\n\
       16 = refinvar(K))\n\
    -I#:#  select mininvarlevel and maxinvarlevel (default 1:1)\n\
    -K#   select invararg (default 3)\n\
\n\
    -q  suppress auxiliary information\n"

/*************************************************************************/

#include "nauty/gtools.h"
#include "nauty/nautinv.h"
#include "nauty/gutils.h"
#include "nauty/traces.h"
#include <assert.h>

graph * /* read graph into nauty format */
readggg(char *s, graph *g, int reqm, int *pm, int *pn, boolean *digraph)
/* graph6, digraph6 and sparse6 formats are supported
   f = an open file
   g = place to put the answer (NULL for dynamic allocation)
   reqm = the requested value of m (0 => compute from n)
   *pm = the actual value of m
   *pn = the value of n
   *digraph = whether the input is a digraph
*/
{
    char *p;
    int m, n;
    int readg_code;
    // printf("s=%s\n", s);
    if (s[0] == ';')
    {
        gt_abort(
            ">E readgg: sorry but showg doesn't understand incremental format\n");
    }
    else if (s[0] == ':')
    {
        *digraph = FALSE;
        p = s + 1;
    }
    else if (s[0] == '&')
    {
        readg_code = DIGRAPH6;
        *digraph = TRUE;
        p = s + 1;
    }
    else
    {
        readg_code = GRAPH6;
        *digraph = FALSE;
        p = s;
    }

    while (*p >= BIAS6 && *p <= MAXBYTE)
        ++p;
    if (*p == '\0')
        gt_abort("labelg.h: >E readgg: missing newline\n");
    else if (*p != '\n')
        gt_abort(">E readgg: illegal character\n");

    n = graphsize(s);
    if (readg_code == GRAPH6 && p - s != G6LEN(n))
        gt_abort(">E readgg: truncated graph6 line\n");
    if (readg_code == DIGRAPH6 && p - s != D6LEN(n))
        gt_abort(">E readgg: truncated digraph6 line\n");

    if (reqm > 0 && TIMESWORDSIZE(reqm) < n)
        gt_abort(">E readgg: reqm too small\n");
    else if (reqm > 0)
        m = reqm;
    else
        m = (n + WORDSIZE - 1) / WORDSIZE;

    if (g == NULL)
    {
        if ((g = (graph *)ALLOCS(n, m * sizeof(graph))) == NULL)
            gt_abort(">E readgg: malloc failed\n");
    }

    *pn = n;
    *pm = m;

    stringtograph(s, g, m);
    return g;
}
static struct invarrec
{
    void (*entrypoint)(graph *, int *, int *, int, int, int, int *,
                       int, boolean, int, int);
    void (*entrypoint_sg)(graph *, int *, int *, int, int, int, int *,
                          int, boolean, int, int);
    char *name;
} invarproc[] = {{NULL, NULL, "none"},
                 {twopaths, NULL, "twopaths"},
                 {adjtriang, NULL, "adjtriang"},
                 {triples, NULL, "triples"},
                 {quadruples, NULL, "quadruples"},
                 {celltrips, NULL, "celltrips"},
                 {cellquads, NULL, "cellquads"},
                 {cellquins, NULL, "cellquins"},
                 {distances, distances_sg, "distances"},
                 {indsets, NULL, "indsets"},
                 {cliques, NULL, "cliques"},
                 {cellcliq, NULL, "cellcliq"},
                 {cellind, NULL, "cellind"},
                 {adjacencies, adjacencies_sg, "adjacencies"},
                 {cellfano, NULL, "cellfano"},
                 {cellfano2, NULL, "cellfano2"},
                 {refinvar, NULL, "refinvar"}};

#define NUMINVARS ((int)(sizeof(invarproc) / sizeof(struct invarrec)))

static nauty_counter orbtotal;
static double unorbtotal;
extern int gt_numorbits;

/**************************************************************************/
#define WORDSIZE 32
char *labelg(graph *g, int m, int n)
{
    // graph *g;
    sparsegraph sg, sh;
    int codetype;
    int argnum, j, outcode;
    char *arg, sw, *fmt;
    boolean badargs;
    boolean dooutput;
    int tabsize, outinvar;
    int inv = 0, mininvarlevel, maxinvarlevel, invararg;
    long minil, maxil;
    double t;
    char *infilename, *outfilename;
    FILE *infile = stdin, *outfile = stdout;
    nauty_counter nin, nout;
    int ii, secret = 1, loops;
    DEFAULTOPTIONS_TRACES(traces_opts);
    TracesStats traces_stats;
    DYNALLSTAT(graph, h, h_sz);
    DYNALLSTAT(int, lab, lab_sz);
    DYNALLSTAT(int, ptn, ptn_sz);
    DYNALLSTAT(int, orbits, orbits_sz);
    DYNALLSTAT(nauty_counter, tab, tab_sz);
    nauty_check(WORDSIZE, 1, 1, NAUTYVERSIONID);

    outcode = GRAPH6;
    dooutput = (outcode == GRAPH6 || outcode == DIGRAPH6 || outcode == SPARSE6);

    fmt = NULL;

    if ((codetype & HAS_HEADER))
    {
        if (outcode == SPARSE6)
            writeline(outfile, SPARSE6_HEADER);
        else if (outcode == DIGRAPH6)
            writeline(outfile, DIGRAPH6_HEADER);
        else
            writeline(outfile, GRAPH6_HEADER);
    }
    orbtotal = 0;
    unorbtotal = 0.0;
    t = CPUTIME;
    // g = readggg(in, NULL, 0, &m, &n, &digraph);
    // printf("digraph=%d\n", digraph);
#if MAXN
    if (n > MAXN)
        gt_abort(">E shortg: graph larger than MAXN read\n");
#endif
    DYNALLOC2(graph, h, h_sz, n, m, "labelg");
    loops = loopcount(g, m, n);
    for (ii = 0; ii < secret; ++ii)
        fcanonise_inv(g, m, n, h, fmt, invarproc[inv].entrypoint,
                      mininvarlevel, maxinvarlevel, invararg, loops > 0);

    char *s = ntog6(h, m, n);
    // FREES(g);
    s[strlen(s) - 1] = '\0';
    return s;
}
