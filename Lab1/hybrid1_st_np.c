#include "rmq_interface.h"
#include <stdlib.h>
#include <math.h>

static int min(int i, int j, const int* A){ return (A[i] > A[j]) ? j : i; }

static int find_k(int i, int j) {
    int len = j - i + 1;
    return 31 - __builtin_clz(len);
}

// Función de apoyo para obtener los bloques totales (techo entero de n / b)
static int get_num_blocks_h1(int n, int b) {
    return (n + b - 1) / b;
}

int* reserve_space_bd_h1(const int n, const int b){
    int num_bloques = get_num_blocks_h1(n, b);
    int *array = (int *)malloc(num_bloques * sizeof(int));
    return array;
}

int** reserve_space_st_h1(const int num_bloques){
    int filas = num_bloques;
    int columnas = (int)log2(num_bloques) + 1;
    
    int **matriz = (int **)malloc(filas * sizeof(int *));
    for (int i = 0; i < filas; i++) {
        matriz[i] = (int *)malloc(columnas * sizeof(int));
    }
    return matriz;
}

int* build_block_decomposition_structure_h1(const int* A, const int n, const int b){
    int * array = reserve_space_bd_h1(n, b);
    int index_min = 0;
    int index_array_min = 0;

    for (int i = 1; i < n; i++){
        if (i % b == 0){
            array[index_array_min++] = index_min;
            index_min = i;
        } else if (A[i] < A[index_min]){
            index_min = i;
        }
    }
    array[index_array_min] = index_min;

    return array;
}

void* build_sp_structure_summary(const int* A, const int n){
    // 1. Calculamos b
    int b = (int)log2(n);
    if (b < 1) b = 1; // Protección por si n es muy pequeño
    
    // 2. Construimos el arreglo de bloques temporales
    int* array_bd_h1 = build_block_decomposition_structure_h1(A, n, b);
    
    // 3. Reservamos la Sparse Table usando el techo exacto de bloques
    int filas = get_num_blocks_h1(n, b);
    int columnas = (int)log2(filas) + 1;
    int ** matriz = reserve_space_st_h1(filas);
    
    // 4. Llenamos la Sparse Table
    int exp = 0;
    for (int j = 0; j < columnas; j++){
        for (int i = 0; i <= filas - (1 << j); i++){
            if(j != 0){
                exp = 1 << (j - 1);
                matriz[i][j] = min(matriz[i][j - 1], matriz[i + exp][j - 1], A);
            }
            else{
                matriz[i][j] = array_bd_h1[i]; // ¡Copia genial!
            }
        }
    }
    
    // 5. ¡Limpiamos la fuga de memoria!
    free(array_bd_h1);
    
    return (void*) matriz;  
}

int query_hybrid_1_structure(const void* internal_state,const int* A, int i, int j,const int n){
    int ** matriz = (int **)internal_state;
    int block_size = (int)log2(n);
    if (block_size < 1) block_size = 1;
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
            int k = find_k(num_bloque_i+1,num_bloque_j-1);
            min2 = min(matriz[num_bloque_i+1][k],matriz[(num_bloque_j-1)-(1<<k)+1][k],A);
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


void free_hybrid_1_structure(void* internal_state, const int n) {
    int ** matriz = (int **)internal_state;
    int b = (int)log2(n);
    if (b < 1) b = 1;
    
    int filas = get_num_blocks_h1(n, b);
    for (int i = 0; i < filas; i++) {
        free(matriz[i]); 
    }
    free(matriz);
}

RMQ_Structure build_hybrid_1(const int* A, int n) {
    RMQ_Structure hybrid_1;
    
    // Naive no tiene preprocesamiento ni memoria extra [cite: 14, 19]
    hybrid_1.pointer_to_structure = build_sp_structure_summary(A, n); 

    int b = (int)log2(n);
    if (b < 1) b = 1;
    int filas = get_num_blocks_h1(n, b);
    int columnas = (int)log2(filas) + 1;

    hybrid_1.memory_bytes = (filas * sizeof(int*)) + (filas * columnas * sizeof(int));    
    
    // hacemos que ambos punteros apunten a ambas funciones (query y liberacion)
    hybrid_1.query = query_hybrid_1_structure; 
    hybrid_1.free_data = free_hybrid_1_structure;
    
    return hybrid_1;
}