#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rmq_interface.h"

extern RMQ_Structure build_naive(const int* A, int n);
extern RMQ_Structure build_full_preprocessing(const int* A, int n);
extern RMQ_Structure build_sparce_table(const int* A, int n);

int main() {
    int n = 2000;
    int *A = (int*)malloc(n * sizeof(int));
    
    // 1. Generación del arreglo con "trampas"
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        A[i] = (rand() % 20000) - 10000; // Números entre -10000 y 9999
    }
    
    // Inyectando casos extremos manuales
    A[0] = -99999;       // Extremo izquierdo
    A[n-1] = -99999;     // Extremo derecho (empate con el izquierdo)
    A[n/2] = -99999;     // Empate en el medio exacto
    A[n/2 + 1] = 50000;  // Pico gigante justo al lado del mínimo

    printf("=== CONSTRUYENDO ESTRUCTURAS (N = %d) ===\n", n);
    RMQ_Structure rmq_naive = build_naive(A, n);
    RMQ_Structure rmq_full = build_full_preprocessing(A, n);
    RMQ_Structure rmq_sparse = build_sparce_table(A, n);
    printf("¡Construccion exitosa!\n\n");

    printf("=== CASOS EXTREMOS EXPLÍCITOS ===\n");
    int casos_i[] = {0, n-1, 0, n/2, 0};
    int casos_j[] = {0, n-1, n-1, n/2, 1}; // {tamaño 1 izq, tamaño 1 der, todo el arreglo, tamaño 1 centro, tamaño 2}
    
    for (int c = 0; c < 5; c++) {
        int i = casos_i[c], j = casos_j[c];
        int ans1 = rmq_naive.query(rmq_naive.pointer_to_structure, A, i, j);
        int ans2 = rmq_full.query(rmq_full.pointer_to_structure, A, i, j);
        int ans3 = rmq_sparse.query(rmq_sparse.pointer_to_structure, A, i, j);
        
        printf("Rango [%d, %d]: Naive=%d | Full=%d | Sparse=%d\n", i, j, ans1, ans2, ans3);
        if (ans1 != ans2 || ans2 != ans3) {
            printf("[FATAL ERROR] Discrepancia en caso extremo.\n");
            return 1;
        }
    }
    printf("-> [OK] Casos extremos superados.\n\n");

    // 2. Stress Test Masivo (10,000 consultas aleatorias)
    printf("=== STRESS TEST MASIVO (10,000 Consultas) ===\n");
    int errores = 0;
    int num_queries = 10000;

    for (int q = 0; q < num_queries; q++) {
        int i = rand() % n;
        int j = rand() % n;
        
        // Garantizar que i <= j
        if (i > j) { int temp = i; i = j; j = temp; }

        int ans1 = rmq_naive.query(rmq_naive.pointer_to_structure, A, i, j);
        int ans2 = rmq_full.query(rmq_full.pointer_to_structure, A, i, j);
        int ans3 = rmq_sparse.query(rmq_sparse.pointer_to_structure, A, i, j);

        if (ans1 != ans2 || ans2 != ans3) {
            printf("Fallo en query [%d, %d]. N: %d, F: %d, S: %d\n", i, j, ans1, ans2, ans3);
            errores++;
            break; // Detener al primer error
        }
    }

    if (errores == 0) {
        printf("-> [PERFECTO] Las tres estructuras sobrevivieron al Stress Test sin un solo fallo.\n\n");
    }

    // 3. Reporte de Memoria
    printf("=== REPORTE DE CONSUMO DE MEMORIA (Bytes extra) ===\n");
    printf("Naive:                %zu bytes\n", rmq_naive.memory_bytes);
    printf("Full Preprocessing:   %zu bytes\n", rmq_full.memory_bytes);
    printf("Sparse Table:         %zu bytes\n\n", rmq_sparse.memory_bytes);

    // 4. Limpieza impecable
    rmq_naive.free_data(rmq_naive.pointer_to_structure, n);
    rmq_full.free_data(rmq_full.pointer_to_structure, n);
    rmq_sparse.free_data(rmq_sparse.pointer_to_structure, n);
    free(A);

    printf("Todo limpio. Terminando programa con exito.\n");
    return 0;
}