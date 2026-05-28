#include "rmq_interface.h"
#include <stdlib.h>

static int seg_min(int i, int j, const int *A){
    if (A[i] < A[j]) return i;
    if (A[j] < A[i]) return j;
    // Si los VALORES son exactamente iguales, desempatamos por el ÍNDICE menor (el de más a la izquierda)
    return (i < j) ? i : j; 
}

static int *reserve_space_seg(const int n) {
    return (int *)malloc((4 * n + 1) * sizeof(int));
}

static void build_seg(int *tree, const int *A, int node, int start, int end) {
    if (start == end) {
        tree[node] = start;
    } else {
        int mid = (start + end) / 2;
        build_seg(tree, A, 2 * node,     start,   mid);
        build_seg(tree, A, 2 * node + 1, mid + 1, end);
        tree[node] = seg_min(tree[2 * node], tree[2 * node + 1], A);
    }
}

void *build_segment_tree_structure(const int *A, const int n) {
    int *tree = reserve_space_seg(n);
    tree[0]   = n;                      
    build_seg(tree, A, 1, 0, n - 1);
    return (void *)tree;
}

static int query_seg(const int *tree, const int *A, int node, int start, int end, int i, int j) {
    if (i > end || j < start) return -1;
    if (i <= start && end <= j) return tree[node];
    int mid   = (start + end) / 2;
    int left  = query_seg(tree, A, 2 * node,     start,   mid, i, j);
    int right = query_seg(tree, A, 2 * node + 1, mid + 1, end, i, j);
    if (left  == -1) return right;
    if (right == -1) return left;
    return seg_min(left, right, A);
}

// CORRECCIÓN: Se añadió 'const int n' al final para respetar el contrato
int query_segment_tree_structure(const void *internal_state, const int *A, int i, int j, const int n) {
    (void)n; // Evita warning de compilador
    const int *tree = (const int *)internal_state;
    int root_n = tree[0];
    return query_seg(tree, A, 1, 0, root_n - 1, i, j);
}

void free_segment_tree_structure(void *internal_state, const int n) {
    (void)n;
    free(internal_state);
}

RMQ_Structure build_segment_tree(const int *A, int n) {
    RMQ_Structure segment_tree_structure;

    segment_tree_structure.pointer_to_structure = build_segment_tree_structure(A, n);
    segment_tree_structure.memory_bytes         = (4 * n + 1) * sizeof(int);

    segment_tree_structure.query     = query_segment_tree_structure;
    segment_tree_structure.free_data = free_segment_tree_structure;

    return segment_tree_structure;
}