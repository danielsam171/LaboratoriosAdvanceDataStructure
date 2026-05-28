#include "rmq_interface.h"
#include <stdlib.h>
#include <math.h>

typedef struct {
    int** summary_st;    // La Sparse Table de arriba (del resumen de bloques)
    int*** blocks_st;    // ¡Un arreglo de Sparse Tables! Una matriz por cada bloque
    int block_size;      // Guardamos 'b' para no tener que recalcularlo
    int num_blocks;      // Cantidad total de bloques
} Hybrid2_State;

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

// Función de apoyo para obtener los bloques totales (techo entero de n / b)
static int get_num_blocks_h2(int n, int b) {
    return (n + b - 1) / b;
}

int* reserve_space_bd_h2(const int n, const int b){
    int num_bloques = get_num_blocks_h2(n, b);
    int *array = (int *)malloc(num_bloques * sizeof(int));
    return array;
}

int** reserve_space_st_h2(const int num_bloques){
    int filas = num_bloques;
    int columnas = (int)log2(num_bloques) + 1;
    
    int **matriz = (int **)malloc(filas * sizeof(int *));
    for (int i = 0; i < filas; i++) {
        matriz[i] = (int *)malloc(columnas * sizeof(int));
    }
    return matriz;
}

int* build_block_decomposition_structure_h2(const int* A, const int n, const int b){
    int * array = reserve_space_bd_h2(n, b);
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

void* build_sp_structure_summary_h2(const int* A, const int n, const int b){

    int* array_bd_h2 = build_block_decomposition_structure_h2(A, n, b);
    
    // 3. Reservamos la Sparse Table usando el techo exacto de bloques
    int filas = get_num_blocks_h2(n, b);
    int columnas = (int)log2(filas) + 1;
    int ** matriz = reserve_space_st_h2(filas);
    
    // 4. Llenamos la Sparse Table
    int exp = 0;
    for (int j = 0; j < columnas; j++){
        for (int i = 0; i <= filas - (1 << j); i++){
            if(j != 0){
                exp = 1 << (j - 1);
                matriz[i][j] = min(matriz[i][j - 1], matriz[i + exp][j - 1], A);
            }
            else{
                matriz[i][j] = array_bd_h2[i]; // Copiamos el indice original, el del A[] no el de array[]
            }
        }
    }
    
    // Se libera el array de summary, ya tengo la sparce table y no lo necesito mas
    free(array_bd_h2);
    
    return (void*) matriz;  
}

void* build_single_block_st(const int* A,const int n,const int b,const int m){

    int start_idx = m * b;
    int end_idx = start_idx + b;
    
    if (end_idx > n) {
        end_idx = n;
    }

    int filas = end_idx-start_idx;
    int columnas = (int)log2(filas)+ 1;
    if (filas <= 0) return NULL;

    int ** matriz = reserve_space_st_h2(filas);
    
    // 4. Llenamos la Sparse Table
    int exp = 0;
    for (int j = 0; j < columnas; j++){
        for (int i = 0; i <= filas - (1 << j); i++){
            if(j != 0){
                exp = 1 << (j - 1);
                matriz[i][j] = min(matriz[i][j - 1], matriz[i + exp][j - 1], A);
            }
            else{
                matriz[i][j] = i + start_idx; // Copiamos el indice original, el del A[] no el de array[]
            }
        }
    }

    
    return (void*) matriz;  
}
void* reserve_space_array_of_arrays(const int num_blocks){
    return (void*)malloc(num_blocks * sizeof(int**));
}
 
void* build_hybrid2_structure(const int* A, const int n){
    Hybrid2_State* estado = (Hybrid2_State*)malloc(sizeof(Hybrid2_State));
    // 1. Calculamos b
    int b = (int)log2(n);
    if (b < 1) b = 1; // Protección por si n es muy pequeño

    int num_blocks = get_num_blocks_h2(n,b);

    estado -> summary_st = build_sp_structure_summary_h2(A,n,b);
    estado -> num_blocks = num_blocks;
    estado -> block_size = b;
    estado -> blocks_st = (int***)reserve_space_array_of_arrays( num_blocks);

    for (int m = 0; m < num_blocks; m++) {
        estado->blocks_st[m] = build_single_block_st(A, n, b, m);
    }
    return (void*)estado;
}

int query_hybrid_2_structure(const void* internal_state, const int* A, int i, int j, const int n){
    Hybrid2_State* estado = (Hybrid2_State*)internal_state;
    int b = estado->block_size;
    
    int num_bloque_i = i / b;
    int num_bloque_j = j / b;

    // Caso 1: Ambos índices pertenecen al mismo bloque
    if (num_bloque_i == num_bloque_j){
        int local_i = i % b;
        int local_j = j % b;
        
        int k = find_k(local_i, local_j);
        int** matriz_bloque = estado->blocks_st[num_bloque_i];
        
        return min(matriz_bloque[local_i][k], matriz_bloque[local_j - (1 << k) + 1][k], A);
    }
    
    // Caso 2: Bloques distintos (Lógica de superposición de 3 partes)
    // Parte 1: Consulta sobre el bloque izquierdo (desde i local hasta el final del bloque)
    int local_i = i % b;
    int k_izq = find_k(local_i, b - 1);
    int** matriz_izq = estado->blocks_st[num_bloque_i];
    int min_izq = min(matriz_izq[local_i][k_izq], matriz_izq[(b - 1) - (1 << k_izq) + 1][k_izq], A);

    // Parte 2: Consulta sobre el bloque derecho (desde el inicio 0 hasta j local)
    int local_j = j % b;
    int k_der = find_k(0, local_j);
    int** matriz_der = estado->blocks_st[num_bloque_j];
    int min_der = min(matriz_der[0][k_der], matriz_der[local_j - (1 << k_der) + 1][k_der], A);

    int index_min = min(min_izq, min_der, A);

    // Parte 3: Consulta sobre los bloques intermedios completos usando la Sparse Table resumen
    if ((num_bloque_j - num_bloque_i) > 1){
        int k_mid = find_k(num_bloque_i + 1, num_bloque_j - 1);
        int min_medio = min(estado->summary_st[num_bloque_i + 1][k_mid], 
                            estado->summary_st[(num_bloque_j - 1) - (1 << k_mid) + 1][k_mid], A);
        index_min = min(index_min, min_medio, A);
    }

    return index_min;
}



void free_hybrid_2_structure(void* internal_state, const int n) {
    Hybrid2_State* estado = (Hybrid2_State*)internal_state;
    int b = estado->block_size;
    
    // 1. Liberar la Sparse Table del resumen
    int filas_summary = estado->num_blocks;
    for (int i = 0; i < filas_summary; i++) {
        free(estado->summary_st[i]); 
    }
    free(estado->summary_st);
    
    // 2. Liberar cada Sparse Table de los bloques individuales
    for (int m = 0; m < estado->num_blocks; m++) {
        int start_idx = m * b;
        int end_idx = start_idx + b;
        if (end_idx > n) end_idx = n;
        
        int filas_bloque = end_idx - start_idx;
        if (filas_bloque > 0 && estado->blocks_st[m] != NULL) {
            for (int i = 0; i < filas_bloque; i++) {
                free(estado->blocks_st[m][i]);
            }
            free(estado->blocks_st[m]);
        }
    }
    free(estado->blocks_st);
    
    // 3. Liberar el contenedor principal
    free(estado);
}

size_t memory_used(const int n){
    int b = (int)log2(n);
    if (b < 1) b = 1;
    int num_blocks = get_num_blocks_h2(n, b);

    size_t total_bytes = sizeof(Hybrid2_State);
    
    // Memoria de la Sparse Table resumen
    int columnas_summary = (int)log2(num_blocks) + 1;
    total_bytes += (num_blocks * sizeof(int*)) + (num_blocks * columnas_summary * sizeof(int));
    
    // Memoria del arreglo de punteros a bloques
    total_bytes += num_blocks * sizeof(int**);
    
    // Memoria de las tablas internas de cada bloque
    for (int m = 0; m < num_blocks; m++) {
        int start_idx = m * b;
        int end_idx = start_idx + b;
        if (end_idx > n) end_idx = n;
        
        int filas_bloque = end_idx - start_idx;
        int columnas_bloque = (int)log2(filas_bloque) + 1;
        total_bytes += (filas_bloque * sizeof(int*)) + (filas_bloque * columnas_bloque * sizeof(int));
    }
    return total_bytes;
}
RMQ_Structure build_hybrid_2(const int* A, int n) {
    RMQ_Structure hybrid_2;
    
    hybrid_2.pointer_to_structure = build_hybrid2_structure(A, n); 
    hybrid_2.memory_bytes = memory_used(n);    
    hybrid_2.query = query_hybrid_2_structure; 
    hybrid_2.free_data = free_hybrid_2_structure;
    
    return hybrid_2;
}