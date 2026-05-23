#include "rmq_interface.h"

void* build_naive_structure(const int* A, const int n){
    return NULL;
}

int query_naive_structure(const void* internal_state,const int* A, int i, int j){
    int index_min = i;
    for (int c = i+1; c <= j;c++){
        if (A[c] < A[index_min]){
            index_min = c;
        }
    }
    return index_min;
}
void free_naive_structure(void* internal_state, const int n) {}

RMQ_Structure build_naive(const int* A, int n) {
    RMQ_Structure naive_structure;
    
    // Naive no tiene preprocesamiento ni memoria extra [cite: 14, 19]
    naive_structure.pointer_to_structure = build_naive_structure(A, n); 
    naive_structure.memory_bytes = 0;      
    
    // hacemos que ambos punteros apunten a ambas funciones (query y liberacion)
    naive_structure.query = query_naive_structure; 
    naive_structure.free_data = free_naive_structure;
    
    return naive_structure;
}