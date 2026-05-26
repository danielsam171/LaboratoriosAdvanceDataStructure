#include "rmq_interface.h"
#include <stdlib.h>
#include <math.h>

static int min(int i, int j, const int* A){return (A[i] > A[j])? j : i;}

int techo_raiz_entera(int n) {
    if (n <= 0) return 0;
    int r = 1;
    while (r * r < n) {
        r++;
    }
    return r; 
}

int* reserve_space_bd(const int n){
    int *array = (int *)malloc(techo_raiz_entera(n) * sizeof(int));
    return array;
}

void* build_block_decomposition_structure(const int* A, const int n){
    int * array = reserve_space_bd(n);
    int b = techo_raiz_entera(n);
    int index_min = 0;
    int index_array_min = 0;
    

    for (int i = 1; i < n; i++){
        if (i % b == 0){
            array[index_array_min++] = index_min;
            index_min = i;
        }else if (A[i] < A[index_min]){
            index_min = i;
        }
        
    }
    
    array[index_array_min] = index_min;

    return (void*) array;
}

int query_block_decomposition_structure(const void* internal_state,const int* A, int i, int j,const int n){
    int * array = (int *)internal_state;
    int block_size = techo_raiz_entera(n);
    int num_bloque_i = (int)(i / block_size);
    int num_bloque_j = (int)(j / block_size);

    int index_min = i;
    
    if (num_bloque_i != num_bloque_j){

        for (int c = i+1; c < block_size * (num_bloque_i+1) ; c++){
            if (A[c] < A[index_min]){
                index_min = c;
            }
        }

        int min1 = index_min;

        int min2 = min1; // Por defecto, si no hay bloques medios, no afecta el resultado

        if((num_bloque_j - num_bloque_i) > 1){
            min2 = array[num_bloque_i + 1]; // Asumimos que el primer bloque medio es el ganador inicial
            for (int c = num_bloque_i + 2; c < num_bloque_j; c++){
                if (A[array[c]] < A[min2]){
                    min2 = array[c];
                }
            }
        }
        
        index_min = j;
        
        for (int c = j-1; c >= block_size * num_bloque_j && c >= i ; c--){
            if (A[c] < A[index_min]){
                index_min = c;
            }
        }

        int min3 = index_min; 
        
        index_min = min(min(min1,min2,A),min3,A);
    }
    else{
        for (int c = i+1; c <= j; c++){
            if (A[c] < A[index_min]){
                index_min = c;
            }
        }
    }
    return index_min;
}


void free_block_decomposition_structure(void* internal_state, const int n) {
    free(internal_state);
}

RMQ_Structure build_block_decomposition(const int* A, int n) {
    RMQ_Structure block_decomposition_structure;
    
    // Naive no tiene preprocesamiento ni memoria extra [cite: 14, 19]
    block_decomposition_structure.pointer_to_structure = build_block_decomposition_structure(A, n); 
    int b = techo_raiz_entera(n);
    block_decomposition_structure.memory_bytes = b * sizeof(int);
    
    // hacemos que ambos punteros apunten a ambas funciones (query y liberacion)
    block_decomposition_structure.query = query_block_decomposition_structure; 
    block_decomposition_structure.free_data = free_block_decomposition_structure;
    
    return block_decomposition_structure;
}