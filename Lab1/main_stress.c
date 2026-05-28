#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rmq_interface.h"

// Declaración de las 9 estructuras
extern RMQ_Structure build_naive(const int* A, int n);
extern RMQ_Structure build_full_preprocessing(const int* A, int n);
extern RMQ_Structure build_block_decomposition(const int* A, int n);
extern RMQ_Structure build_sparce_table(const int* A, int n);
extern RMQ_Structure build_segment_tree(const int* A, int n);
extern RMQ_Structure build_hybrid_1(const int* A, int n);
extern RMQ_Structure build_hybrid_2(const int* A, int n);
extern RMQ_Structure build_hybrid_3(const int* A, int n);
extern RMQ_Structure build_fischer_heun(const int* A, int n);

typedef RMQ_Structure (*RMQ_Builder)(const int*, int);

const char* nombres_estructuras[9] = {
    "1. Naive", "2. Full Precalc", "3. Block Decomp", "4. Sparse Table",
    "5. Segment Tree", "6. Hybrid 1", "7. Hybrid 2", "8. Hybrid 3", "9. Fischer-Heun"
};

// Función para imprimir el arreglo cuando hay un error (limitado a 50 elementos para no inundar la pantalla)
void print_array_context(const int* A, int n, int i, int j) {
    printf("\nContexto del arreglo (Mostrando rango [%d, %d]):\n", i, j);
    int start = (i - 5 >= 0) ? i - 5 : 0;
    int end = (j + 5 < n) ? j + 5 : n - 1;
    
    for (int k = start; k <= end; k++) {
        if (k == i) printf(" [ ");
        printf("%d(%d) ", A[k], k); // Formato: Valor(Índice)
        if (k == j) printf(" ] ");
    }
    printf("\n\n");
}

void run_inquisitor(const char* nombre_topologia, int* A, int n, int num_queries, RMQ_Builder* builders) {
    printf("====================================================================\n");
    printf(" INICIANDO TOPOLOGÍA: %s (N=%d, Queries=%d)\n", nombre_topologia, n, num_queries);
    printf("====================================================================\n");

    // Construir todas las estructuras
    RMQ_Structure db[9];
    for (int s = 0; s < 9; s++) {
        db[s] = builders[s](A, n);
    }

    int errores = 0;

    // Ejecutar consultas de estrés
    for (int q = 0; q < num_queries; q++) {
        int i = rand() % n;
        int j = rand() % n;
        if (i > j) { int temp = i; i = j; j = temp; } // Asegurar i <= j

        // Tomamos a Naive como la fuente absoluta de la verdad
        int truth_idx = db[0].query(db[0].pointer_to_structure, A, i, j, n);
        
        for (int s = 1; s < 9; s++) {
            int test_idx = db[s].query(db[s].pointer_to_structure, A, i, j, n);
            
            if (truth_idx != test_idx) {
                printf("\n🔥 ¡ALERTA DE INCONSISTENCIA ENCONTRADA! 🔥\n");
                printf("Topologia: %s\n", nombre_topologia);
                printf("Rango de Consulta: [%d, %d]\n", i, j);
                print_array_context(A, n, i, j);
                
                printf("RESULTADOS:\n");
                printf("-> %-18s devolvio el indice: %d (Valor: %d) [VERDAD ABSOLUTA]\n", nombres_estructuras[0], truth_idx, A[truth_idx]);
                printf("-> %-18s devolvio el indice: %d (Valor: %d) [FALLO]\n\n", nombres_estructuras[s], test_idx, A[test_idx]);
                
                if (A[truth_idx] == A[test_idx]) {
                    printf("DIAGNÓSTICO: ¡Fallo de Leftmost! Ambos tienen el valor mínimo correcto, pero %s eligió un índice más a la derecha.\n", nombres_estructuras[s]);
                } else {
                    printf("DIAGNÓSTICO: ¡Fallo de Lógica Matemática! %s no encontró el valor mínimo real.\n", nombres_estructuras[s]);
                }
                
                errores++;
                break; // Romper el loop de estructuras
            }
        }
        if (errores > 0) break; // Romper el loop de queries si hubo un error crítico
    }

    // Liberar memoria
    for (int s = 0; s < 9; s++) {
        db[s].free_data(db[s].pointer_to_structure, n);
    }

    if (errores == 0) {
        printf("✅ SUPERADO: Las 9 estructuras devolvieron índices idénticos.\n\n");
    } else {
        printf("❌ ABORTANDO: Corrige el error en %s antes de continuar.\n", nombre_topologia);
        exit(1);
    }
}

int main() {
    srand(12345); // Semilla fija para que los errores sean 100% reproducibles

    RMQ_Builder builders[9] = {
        build_naive, build_full_preprocessing, build_block_decomposition,
        build_sparce_table, build_segment_tree, build_hybrid_1,
        build_hybrid_2, build_hybrid_3, build_fischer_heun
    };

    int N = 2000; // Suficientemente grande para crear bloques, lo bastante pequeño para Full Precalc
    int Q = 25000; // 25,000 consultas por topología
    int* A = (int*)malloc(N * sizeof(int));

    printf("Iniciando el Gran Inquisidor RMQ de Correctitud Absoluta...\n\n");

    // 1. TOPOLOGÍA PLANA (Flat): El mayor terror del "Leftmost". Todos los valores son iguales.
    // Esto obliga a las estructuras a siempre elegir el borde izquierdo absoluto de cualquier rango.
    for (int i = 0; i < N; i++) A[i] = 42;
    run_inquisitor("1. Llanura Infinita (Todos los valores iguales)", A, N, Q, builders);

    // 2. TOPOLOGÍA ASCENDENTE: Estresa el prefijo derecho (siempre gana el primer elemento).
    for (int i = 0; i < N; i++) A[i] = i;
    run_inquisitor("2. Escalera Ascendente Estricta", A, N, Q, builders);

    // 3. TOPOLOGÍA DESCENDENTE: Estresa el sufijo izquierdo (siempre gana el último elemento).
    for (int i = 0; i < N; i++) A[i] = N - i;
    run_inquisitor("3. Escalera Descendente Estricta", A, N, Q, builders);

    // 4. TOPOLOGÍA DIENTE DE SIERRA BINARIO: Estresa las divisiones de bloques iterativas.
    for (int i = 0; i < N; i++) A[i] = i % 2; // Arreglo: 0, 1, 0, 1, 0, 1...
    run_inquisitor("4. Diente de Sierra (0, 1, 0, 1...)", A, N, Q, builders);

    // 5. TOPOLOGÍA RANDOM CON ALTA COLISIÓN: Valores aleatorios del 0 al 5.
    // Mezcla mínimos reales con empates agresivos en todo el arreglo.
    for (int i = 0; i < N; i++) A[i] = rand() % 6;
    run_inquisitor("5. Caos de Alta Colisión (Valores Random 0-5)", A, N, Q, builders);

    // 6. TOPOLOGÍA RANDOM PURA: Comportamiento general estadístico sin estrés particular.
    for (int i = 0; i < N; i++) A[i] = rand() % 10000;
    run_inquisitor("6. Ruido Aleatorio Estándar", A, N, Q, builders);

    printf("====================================================================\n");
    printf(" 🏆 ¡VICTORIA ABSOLUTA! 🏆\n");
    printf(" Todas las estructuras sobrevivieron a las 6 topologías.\n");
    printf(" El código fuente es oficialmente de grado de producción.\n");
    printf("====================================================================\n");

    free(A);
    return 0;
}