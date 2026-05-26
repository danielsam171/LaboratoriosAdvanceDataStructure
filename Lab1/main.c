#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "rmq_interface.h"

// Declaración de constructores externos
extern RMQ_Structure build_naive(const int* A, int n);
extern RMQ_Structure build_full_preprocessing(const int* A, int n);
extern RMQ_Structure build_block_decomposition(const int* A, int n);
extern RMQ_Structure build_sparce_table(const int* A, int n);
extern RMQ_Structure build_hybrid_1(const int* A, int n);

// Tipos auxiliares para automatizar pruebas
typedef RMQ_Structure (*RMQ_Builder)(const int*, int);

// Generadores de patrones críticos de arreglos
void llenar_aleatorio(int* A, int n) {
    for (int i = 0; i < n; i++) A[i] = rand() % 100000;
}
void llenar_ordenado(int* A, int n) {
    for (int i = 0; i < n; i++) A[i] = i;
}
void llenar_inverso(int* A, int n) {
    for (int i = 0; i < n; i++) A[i] = n - i;
}
void llenar_duplicados_masivos(int* A, int n) {
    // Genera solo 0s y 1s para forzar infinitos mínimos duplicados repetidos
    for (int i = 0; i < n; i++) A[i] = rand() % 2;
}

// Ejecuta una batería de queries exhaustivas y densas
long comprobar_correctitud(RMQ_Structure* estructuras, int num_estructuras, const char** nombres, 
                          const int* A, int n, long num_queries) {
    long errores = 0;

    // 1. Forzar casos extremos manuales primero (Edge Cases)
    // Caso A: Consultas de tamaño 1 (i == j) para cada elemento
    for (int i = 0; i < n; i++) {
        int v_esperado = A[i];
        for (int s = 0; s < num_estructuras; s++) {
            int idx = estructuras[s].query(estructuras[s].pointer_to_structure, A, i, i, n);
            if (A[idx] != v_esperado) errores++;
        }
    }
    // Caso B: Consulta de todo el arreglo
    for (int s = 0; s < num_estructuras; s++) {
        int idx = estructuras[s].query(estructuras[s].pointer_to_structure, A, 0, n - 1, n);
        // Validamos contra una búsqueda lineal rápida
        int min_real = A[0];
        for(int k=1; k<n; k++) if(A[k] < min_real) min_real = A[k];
        if (A[idx] != min_real) errores++;
    }

    // 2. Consultas masivas aleatorias
    for (long q = 0; q < num_queries; q++) {
        int i = rand() % n;
        int j = rand() % n;
        if (i > j) { int tmp = i; i = j; j = tmp; }

        // Usamos Naive (índice 0) o una consulta de referencia para extraer el valor mínimo real esperado
        int idx_ref = estructuras[0].query(estructuras[0].pointer_to_structure, A, i, j, n);
        int valor_esperado = A[idx_ref];

        for (int s = 1; s < num_estructuras; s++) {
            int idx_res = estructuras[s].query(estructuras[s].pointer_to_structure, A, i, j, n);
            
            // ¡CRÍTICO! Validamos que el VALOR en el arreglo sea idéntico. 
            // Si hay duplicados, los índices pueden cambiar pero el valor DEBE ser el mínimo.
            if (A[idx_res] != valor_esperado) {
                printf("[FALLO CRÍTICO] Rango [%d, %d] en Estructura %s\n", i, j, nombres[s]);
                printf("  Esperado (valor): %d | Obtenido: %d (en índice %d)\n", valor_esperado, A[idx_res], idx_res);
                errores++;
                if (errores > 10) return errores; // Abortar rápido si está roto
            }
        }
    }
    return errores;
}

int main() {
    srand(166); // Fijamos semilla para reproducibilidad matemática
    printf("====================================================================\n");
    printf("         BATERÍA DE PRUEBAS DE ESTRÉS RIGUROSO - RMQ\n");
    printf("====================================================================\n\n");

    // --------------------------------------------------------------------
    // FASE 1: VALIDACIÓN CRUZADA TOTAL (N=2,000 | 1,000,000 Queries)
    // --------------------------------------------------------------------
    int N1 = 2000;
    long Q1 = 1000000;
    printf("--- FASE 1: Las 5 estructuras compitiendo (N = %d, Queries = %ld) ---\n", N1, Q1);
    
    int* A1 = (int*)malloc(N1 * sizeof(int));
    llenar_aleatorio(A1, N1);

    RMQ_Builder constructores1[5] = { build_naive, build_full_preprocessing, build_block_decomposition, build_sparce_table, build_hybrid_1 };
    const char* nombres1[5] = { "Naive", "Full Precalc", "Block Decomp", "Sparse Table", "Hybrid 1" };
    RMQ_Structure est1[5];

    for(int i=0; i<5; i++) {
        est1[i] = constructores1[i](A1, N1);
        printf("  [Construida] %-15s -> Memoria: %10lu bytes\n", nombres1[i], est1[i].memory_bytes);
    }

    printf("  Ejecutando consultas densas...");
    long err1 = comprobar_correctitud(est1, 5, nombres1, A1, N1, Q1);
    if(err1 == 0) printf(" ¡ÉXITO! Cero discrepancias.\n");
    else printf(" ¡FALLO! Se encontraron %ld errores estructurales.\n", err1);

    // Liberar fase 1
    for(int i=0; i<5; i++) est1[i].free_data(est1[i].pointer_to_structure, N1);
    free(A1);


    // --------------------------------------------------------------------
    // FASE 2: ESCALA MASIVA Y PATRONES CRÍTICOS (N=500,000 | 5,000,000 Queries)
    // --------------------------------------------------------------------
    int N2 = 500000;
    long Q2 = 5000000;
    printf("\n--- FASE 2: Escala Masiva (Excluyendo Full Precalc) (N = %d, Queries = %ld) ---\n", N2, Q2);

    int* A2 = (int*)malloc(N2 * sizeof(int));
    
    // Probaremos las 4 estructuras restantes
    RMQ_Builder constructores2[4] = { build_naive, build_block_decomposition, build_sparce_table, build_hybrid_1 };
    const char* nombres2[4] = { "Naive", "Block Decomp", "Sparse Table", "Hybrid 1" };
    RMQ_Structure est2[4];

    // Evaluamos con 3 patrones diferentes de datos (Aleatorio, Ordenado, Duplicados Masivos)
    const char* patrones[3] = { "ALEATORIO", "ESTRICTAMENTE ORDENADO", "DUPLICADOS MASIVOS (0s y 1s)" };
    void (*llenadores[3])(int*, int) = { llenar_aleatorio, llenar_ordenado, llenar_duplicados_masivos };

    for(int p = 0; p < 3; p++) {
        printf("\n  >> Probando con Patrón: %s\n", patrones[p]);
        llenadores[p](A2, N2);

        for(int i=0; i<4; i++) {
            est2[i] = constructores2[i](A2, N2);
            if (p == 0) { // Imprimir memoria solo la primera vez
                printf("     %-15s -> Memoria: %10lu bytes (%.2f MB)\n", 
                       nombres2[i], est2[i].memory_bytes, (double)est2[i].memory_bytes / (1024*1024));
            }
        }

        printf("     Ejecutando consultas de alta densidad...");
        clock_t start = clock();
        long err2 = comprobar_correctitud(est2, 4, nombres2, A2, N2, Q2 / 3); // Dividido entre los 3 patrones
        clock_t end = clock();
        
        double tiempo = (double)(end - start) / CLOCKS_PER_SEC;

        if(err2 == 0) printf(" ¡ÉXITO! Cero discrepancias (Tiempo: %.3f seg).\n", tiempo);
        else printf(" ¡FALLO! Se detectaron errores.\n");

        for(int i=0; i<4; i++) est2[i].free_data(est2[i].pointer_to_structure, N2);
    }

    free(A2);
    printf("\n====================================================================\n");
    printf("                 BATERÍA DE PRUEBAS COMPLETADA\n");
    printf("====================================================================\n");
    return 0;
}