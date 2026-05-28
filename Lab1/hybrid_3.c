#include "rmq_interface.h"
#include <stdlib.h>
#include <math.h>

// ¡Punto clave! Importamos el constructor de tu archivo anterior.
extern RMQ_Structure build_hybrid_1(const int* A, int n);

// Estado interno: Contiene la ST de resumen y un arreglo de tus estructuras Híbridas 1
typedef struct {
    int** summary_st;
    RMQ_Structure* blocks_h1; 
    int block_size;
    int num_blocks;
} Hybrid3_State;

static int min(int i, int j, const int* A) {
    if (A[i] < A[j]) return i;
    if (A[j] < A[i]) return j;
    // Si los VALORES son exactamente iguales, desempatamos por el ÍNDICE menor (el de más a la izquierda)
    return (i < j) ? i : j; 
}

static int find_k(int i, int j) {
    int len = j - i + 1;
    return 31 - __builtin_clz(len);
}

static int get_num_blocks_h3(int n, int b) {
    return (n + b - 1) / b;
}

static int* reserve_space_bd_h3(const int n, const int b){
    int num_bloques = get_num_blocks_h3(n, b);
    return (int *)malloc(num_bloques * sizeof(int));
}

static int** reserve_space_st_h3(const int num_bloques){
    int filas = num_bloques;
    int columnas = (int)log2(num_bloques) + 1;
    
    int **matriz = (int **)malloc(filas * sizeof(int *));
    for (int i = 0; i < filas; i++) matriz[i] = (int *)malloc(columnas * sizeof(int));
    return matriz;
}

static int* build_block_decomposition_structure_h3(const int* A, const int n, const int b){
    int * array = reserve_space_bd_h3(n, b);
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
    array[index_array_min] = index_min; // Guarda índices GLOBALES
    return array;
}

static int** build_sp_structure_summary_h3(const int* A, const int n, const int b){
    int* array_bd = build_block_decomposition_structure_h3(A, n, b);
    int filas = get_num_blocks_h3(n, b);
    int columnas = (int)log2(filas) + 1;
    int ** matriz = reserve_space_st_h3(filas);
    
    int exp = 0;
    for (int j = 0; j < columnas; j++){
        for (int i = 0; i <= filas - (1 << j); i++){
            if(j != 0){
                exp = 1 << (j - 1);
                matriz[i][j] = min(matriz[i][j - 1], matriz[i + exp][j - 1], A);
            } else {
                matriz[i][j] = array_bd[i]; 
            }
        }
    }
    free(array_bd);
    return matriz;  
}

void* build_hybrid3_structure(const int* A, const int n){
    Hybrid3_State* estado = (Hybrid3_State*)malloc(sizeof(Hybrid3_State));
    
    int b = (int)log2(n);
    if (b < 1) b = 1; 

    int num_blocks = get_num_blocks_h3(n, b);

    estado->summary_st = build_sp_structure_summary_h3(A, n, b);
    estado->num_blocks = num_blocks;
    estado->block_size = b;
    
    // Reservamos espacio para el arreglo de Híbridas 1
    estado->blocks_h1 = (RMQ_Structure*)malloc(num_blocks * sizeof(RMQ_Structure));

    for (int m = 0; m < num_blocks; m++) {
        int start_idx = m * b;
        int end_idx = start_idx + b;
        if (end_idx > n) end_idx = n;
        int filas_bloque = end_idx - start_idx;
        
        // ¡LA COMPOSICIÓN! Mandamos la Híbrida 1 desplazando el puntero
        estado->blocks_h1[m] = build_hybrid_1(A + start_idx, filas_bloque);
    }
    return (void*)estado;
}

int query_hybrid_3_structure(const void* internal_state, const int* A, int i, int j, const int n){
    Hybrid3_State* estado = (Hybrid3_State*)internal_state;
    int b = estado->block_size;
    
    int num_bloque_i = i / b;
    int num_bloque_j = j / b;

    // Caso 1: Mismo bloque (Resuelto enteramente por la Híbrida 1 local)
    if (num_bloque_i == num_bloque_j){
        int local_i = i % b;
        int local_j = j % b;
        int start_idx = num_bloque_i * b;
        
        int filas_bloque = ((start_idx + b) > n) ? (n - start_idx) : b;
        
        RMQ_Structure mi_h1 = estado->blocks_h1[num_bloque_i];
        
        // Consultamos la Híbrida 1 (retorna índice local)
        int local_ans = mi_h1.query(mi_h1.pointer_to_structure, A + start_idx, local_i, local_j, filas_bloque);
        
        // ¡Transformación a índice global!
        return start_idx + local_ans; 
    }
    
    // Caso 2: Bloques distintos
    // Parte 1: Bloque Izquierdo
    int local_i = i % b;
    int start_i = num_bloque_i * b;
    int filas_bloque_i = ((start_i + b) > n) ? (n - start_i) : b;
    
    RMQ_Structure h1_izq = estado->blocks_h1[num_bloque_i];
    int min_izq_local = h1_izq.query(h1_izq.pointer_to_structure, A + start_i, local_i, filas_bloque_i - 1, filas_bloque_i);
    int min_izq_global = start_i + min_izq_local;

    // Parte 2: Bloque Derecho
    int local_j = j % b;
    int start_j = num_bloque_j * b;
    int filas_bloque_j = ((start_j + b) > n) ? (n - start_j) : b;
    
    RMQ_Structure h1_der = estado->blocks_h1[num_bloque_j];
    int min_der_local = h1_der.query(h1_der.pointer_to_structure, A + start_j, 0, local_j, filas_bloque_j);
    int min_der_global = start_j + min_der_local;

    int index_min = min(min_izq_global, min_der_global, A);

    // Parte 3: El Summary ST global (tal cual Hybrid 2)
    if ((num_bloque_j - num_bloque_i) > 1){
        int k_mid = find_k(num_bloque_i + 1, num_bloque_j - 1);
        int min_medio = min(estado->summary_st[num_bloque_i + 1][k_mid], 
                            estado->summary_st[(num_bloque_j - 1) - (1 << k_mid) + 1][k_mid], A);
        index_min = min(index_min, min_medio, A);
    }

    return index_min;
}

void free_hybrid_3_structure(void* internal_state, const int n) {
    Hybrid3_State* estado = (Hybrid3_State*)internal_state;
    int b = estado->block_size;
    
    int filas_summary = estado->num_blocks;
    for (int i = 0; i < filas_summary; i++) {
        free(estado->summary_st[i]); 
    }
    free(estado->summary_st);
    
    // Delegamos la limpieza a la interfaz de la Híbrida 1
    for (int m = 0; m < estado->num_blocks; m++) {
        int start_idx = m * b;
        int filas_bloque = ((start_idx + b) > n) ? (n - start_idx) : b;
        
        RMQ_Structure mi_h1 = estado->blocks_h1[m];
        if(mi_h1.free_data) {
            mi_h1.free_data(mi_h1.pointer_to_structure, filas_bloque);
        }
    }
    free(estado->blocks_h1);
    free(estado);
}

size_t memory_used_h3(const int n,RMQ_Structure hybrid_3){
    int b = (int)log2(n);
    if (b < 1) b = 1;
    int num_blocks = get_num_blocks_h3(n, b);

    size_t total_bytes = sizeof(Hybrid3_State);

    int columnas_summary = (int)log2(num_blocks) + 1;
    total_bytes += (num_blocks * sizeof(int*)) + (num_blocks * columnas_summary * sizeof(int));
    total_bytes += num_blocks * sizeof(RMQ_Structure);
    
    // Acumulamos la memoria reportada dinámicamente por CADA Híbrida 1 inferior
    Hybrid3_State* estado = (Hybrid3_State*)hybrid_3.pointer_to_structure;
    for (int m = 0; m < num_blocks; m++) {
        total_bytes += estado->blocks_h1[m].memory_bytes;
    }
    return total_bytes;
}

RMQ_Structure build_hybrid_3(const int* A, int n) {
    RMQ_Structure hybrid_3;
    
    hybrid_3.pointer_to_structure = build_hybrid3_structure(A, n); 
    hybrid_3.memory_bytes = memory_used_h3(n,hybrid_3);    
    hybrid_3.query = query_hybrid_3_structure; 
    hybrid_3.free_data = free_hybrid_3_structure;
    
    return hybrid_3;
}