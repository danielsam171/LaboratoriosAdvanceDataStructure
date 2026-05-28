#include <stdio.h>
#include <stdlib.h>
#include "rmq_interface.h"

// Declaración de las estructuras que quieres probar
extern RMQ_Structure build_naive(const int* A, int n);
extern RMQ_Structure build_block_decomposition(const int* A, int n);
extern RMQ_Structure build_sparce_table(const int* A, int n);
extern RMQ_Structure build_segment_tree(const int* A, int n);
extern RMQ_Structure build_hybrid_1(const int* A, int n);
extern RMQ_Structure build_fischer_heun(const int* A, int n);

// Estructura para definir nuestros casos de prueba manuales
typedef struct {
    int i;
    int j;
    int exp_idx; // Índice esperado (el de más a la izquierda)
    char* motivo;
} TestCase;

int main() {
    // Arreglo de prueba con empates estratégicos de mínimos
    // Índices:  0   1   2   3   4   5   6   7   8   9
    int A[] = {42, 10,  5, 99,  5,  5, 20, 10,  5, 30};
    int n = 10;

    printf("=================================================================\n");
    printf("         LABORATORIO DE PRUEBAS RMQ: VALOR ESPERADO VS DEVUELTO  \n");
    printf("=================================================================\n");
    printf("Arreglo de prueba:\n");
    for(int k = 0; k < n; k++) printf("[%d]:%-3d ", k, A[k]);
    printf("\n-----------------------------------------------------------------\n\n");

    // Definimos los escenarios "juguete" para ver si respetan el Leftmost
    TestCase pruebas[] = {
        {1, 7, 2, "El minimo es 5. Se repite en pos 2, 4, 5. Esperado: pos 2"},
        {4, 8, 4, "El minimo es 5. Se repite en pos 4, 5, 8. Esperado: pos 4"},
        {6, 7, 7, "Rango [20, 10]. El minimo es 10 en la posicion 7"},
        {0, 9, 2, "Todo el arreglo. El minimo global es 5. Esperado: pos 2"}
    };
    int num_pruebas = sizeof(pruebas) / sizeof(pruebas[0]);

    // Construimos las estructuras principales
    RMQ_Structure db_naive = build_naive(A, n);
    RMQ_Structure db_block = build_block_decomposition(A, n);
    RMQ_Structure db_st    = build_sparce_table(A, n);
    RMQ_Structure db_seg   = build_segment_tree(A, n);
    RMQ_Structure db_h1    = build_hybrid_1(A, n);
    RMQ_Structure db_fh    = build_fischer_heun(A, n);

    // Corremos los experimentos
    for (int t = 0; t < num_pruebas; t++) {
        TestCase c = pruebas[t];
        printf("🧪 TEST %d: Rango [%d, %d]\n", t + 1, c.i, c.j);
        printf("   Motivo: %s\n", c.motivo);
        printf("   -> VALOR ESPERADO (Leftmost Index): %d\n\n", c.exp_idx);
        
        // Ejecución de queries
        int res_naive = db_naive.query(db_naive.pointer_to_structure, A, c.i, c.j, n);
        int res_block = db_block.query(db_block.pointer_to_structure, A, c.i, c.j, n);
        int res_st    = db_st.query(db_st.pointer_to_structure, A, c.i, c.j, n);
        int res_seg   = db_seg.query(db_seg.pointer_to_structure, A, c.i, c.j, n);
        int res_h1    = db_h1.query(db_h1.pointer_to_structure, A, c.i, c.j, n);
        int res_fh    = db_fh.query(db_fh.pointer_to_structure, A, c.i, c.j, n);

        // Tabla de resultados para este caso
        printf("   %-22s | %-15s | %-10s\n", "Estructura", "Indice Devuelto", "Resultado");
        printf("   -----------------------|-----------------|-----------\n");
        printf("   %-22s | %-15d | %s\n", "1. Naive", res_naive, (res_naive == c.exp_idx) ? "✅ OK" : "❌ FALLO");
        printf("   %-22s | %-15d | %s\n", "3. Block Decomposition", res_block, (res_block == c.exp_idx) ? "✅ OK" : "❌ FALLO");
        printf("   %-22s | %-15d | %s\n", "4. Sparse Table", res_st, (res_st == c.exp_idx) ? "✅ OK" : "❌ FALLO");
        printf("   %-22s | %-15d | %s\n", "5. Segment Tree", res_seg, (res_seg == c.exp_idx) ? "✅ OK" : "❌ FALLO");
        printf("   %-22s | %-15d | %s\n", "6. Hybrid 1", res_h1, (res_h1 == c.exp_idx) ? "✅ OK" : "❌ FALLO");
        printf("   %-22s | %-15d | %s\n", "9. Fischer-Heun", res_fh, (res_fh == c.exp_idx) ? "✅ OK" : "❌ FALLO");
        printf("\n=================================================================\n\n");
    }

    // Liberación de memoria
    db_naive.free_data(db_naive.pointer_to_structure, n);
    db_block.free_data(db_block.pointer_to_structure, n);
    db_st.free_data(db_st.pointer_to_structure, n);
    db_seg.free_data(db_seg.pointer_to_structure, n);
    db_h1.free_data(db_h1.pointer_to_structure, n);
    db_fh.free_data(db_fh.pointer_to_structure, n);

    return 0;
}