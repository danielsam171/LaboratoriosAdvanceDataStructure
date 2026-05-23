#include "rmq_interface.h"
#include <stdlib.h>

int** reserve_space_fp(const int n){
    int filas = n, columnas = filas;
    //Puntero a punteros
    int **matriz = (int **)malloc(filas * sizeof(int *));
    
    // Luego, asignamos memoria para las columnas de cada fila
    for (int i = 0; i < filas; i++) {
        matriz[i] = (int *)malloc(columnas * sizeof(int));
    }
    return matriz;
}  

static int min(int i, int j, const int* A){return (A[i] > A[j])? j : i;}

void* build_full_preprocessing_structure(const int* A, const int n){
    
    int ** matriz = reserve_space_fp(n);
    int filas = n, columnas = filas;
    
    for (int i = 0; i < filas; i++){
        for (int j = i; j < columnas; j++){
            if (i ==j){matriz[i][j] = i;}
            else{ 
                matriz[i][j] = min(matriz[i][j-1],j,A);
            }
            
        }
        
    }

    return (void*) matriz;
    
}

int query_full_preprocessing_structure(const void* internal_state,const int* A, int i, int j){
    int ** matriz = (int **)internal_state;
    return matriz[i][j];
}

void free_full_preprocessing_structure(void* internal_state, const int n) {
    int ** matriz = (int **)internal_state;
    int filas = n; 
    for (int i = 0; i < filas; i++)
        free(matriz[i]);   // libera cada fila
    free(matriz);    
}

RMQ_Structure build_full_preprocessing(const int* A, int n) {
    RMQ_Structure full_preprocessing_structure;
    
    // Naive no tiene preprocesamiento ni memoria extra [cite: 14, 19]
    full_preprocessing_structure.pointer_to_structure = build_full_preprocessing_structure(A, n); 
    full_preprocessing_structure.memory_bytes = (n * sizeof(int*)) + (n * n * sizeof(int));     
    
    // hacemos que ambos punteros apunten a ambas funciones (query y liberacion)
    full_preprocessing_structure.query = query_full_preprocessing_structure; 
    full_preprocessing_structure.free_data = free_full_preprocessing_structure;
    
    return full_preprocessing_structure;
}