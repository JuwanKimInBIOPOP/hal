#ifndef __ANCESTORSML_H_
#define __ANCESTORSML_H_
#include "halAlignment.h"
#include "halDefs.h"
#include "halGenome.h"
#include "sonLibTree.h"
#include <map>
#undef __cplusplus
extern "C" {
#include "tree_model.h"
}
#define __cplusplus
// PHAST code defines min, max macros which conflict with the reserved C++ names.
#undef min
#undef max
typedef struct {
    const hal::Genome *rootGenome;
    hal_index_t pos;
    // Reversed with respect to reference?
    bool reversed;
} rootInfo;

typedef struct {
    // Position of this site in the genome
    hal_index_t pos;
    // Probability of leaves under this node given each nucleotide.
    double pLeaves[4];
    // This is a terrible and incorrect name
    // TODO change it without breaking everything
    double pOtherLeaves[4];
    // phast ID from the model.
    int phastId;
    // Posterior probability of this call (in case we need it later)
    double post;
    // should only be set on the leaves at first.
    char dna;
    // Reversed with respect to reference?
    bool reversed;
    // Whether this node has already been calculated.
    bool done;
} felsensteinData;

using namespace hal;

void doFelsenstein(stTree *node, TreeModel *mod);

void reEstimate(TreeModel *mod, const Alignment *alignment, const Genome *genome, hal_index_t startPos, hal_index_t endPos,
                std::map<std::string, int> &nameToId, double threshold, bool printWrites, bool outputPosts);

#endif
// Local Variables:
// mode: c++
// End:
