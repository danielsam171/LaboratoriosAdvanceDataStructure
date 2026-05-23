#include "rmq_interface.h"
#include <stdlib.h>
#include <math.h>

static int min(int i, int j, const int* A){return (A[i] > A[j])? j : i;}

int find_k(int i, int j) {
    int len = j - i + 1;
    return 31 - __builtin_clz(len);
}

int** reserve_space_st(const int n){
    int filas = n, columnas = (int)log2(n) + 1;
    //Puntero a punteros
    int **matriz = (int **)malloc(filas * sizeof(int *));
    
    // Luego, asignamos memoria para las columnas de cada fila
    for (int i = 0; i < filas; i++) {
        matriz[i] = (int *)malloc(columnas * sizeof(int));
    }
    return matriz;
}


void* build_sparce_table_structure(const int* A, const int n){
    
    int ** matriz = reserve_space_st(n);
    int filas = n, columnas = (int)log2(n) + 1;
    int exp = 0;
    for (int j = 0; j < columnas; j++){
        for (int i = 0; i <= filas - (1<<j); i++){
            if(j!=0){
                exp = 1<<(j-1);
                matriz[i][j]=min(matriz[i][j-1],matriz[i+exp][j-1],A);
            }
            else{
                matriz[i][j] = i;
            }
        }
        
    }
    return (void*) matriz;
    
}

int query_sparce_table_structure(const void* internal_state,const int* A, int i, int j){
    int ** matriz = (int **)internal_state;
    int k = find_k(i, j);
    int index = min(matriz[i][k],matriz[j-(1<<k)+1][k],A);
    return index;
}


void free_sparce_table_structure(void* internal_state, const int n) {
    int ** matriz = (int **)internal_state;
    int filas = n; 
    for (int i = 0; i < filas; i++)
        free(matriz[i]);   // libera cada fila
    free(matriz);    
}

RMQ_Structure build_sparce_table(const int* A, int n) {
    RMQ_Structure sparce_table_structure;
    
    // Naive no tiene preprocesamiento ni memoria extra [cite: 14, 19]
    sparce_table_structure.pointer_to_structure = build_sparce_table_structure(A, n); 
    sparce_table_structure.memory_bytes = (n * sizeof(int*)) + (n * (log2(n)+1) * sizeof(int));     
    
    // hacemos que ambos punteros apunten a ambas funciones (query y liberacion)
    sparce_table_structure.query = query_sparce_table_structure; 
    sparce_table_structure.free_data = free_sparce_table_structure;
    
    return sparce_table_structure;
}