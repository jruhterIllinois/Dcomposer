
#define MAX_CHILD 256

typedef struct dcsn_tree_t
{
    int num_branches;
    int generation;
    char * child_mem[MAX_CHILD];

}DCSN_TREE_t;