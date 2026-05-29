#include <stdio.h>
#include <stdlib.h>
#include <time.h> 

#define NUM_SIZES 6
#define NUM_TRIALS 30

#include "rmq_interface.h" // Incluye tu archivo de firmas

// Declaraciones de tus 9 constructores externos
extern RMQ_Structure build_naive(const int* A, int n);
extern RMQ_Structure build_full_preprocessing(const int* A, int n);
extern RMQ_Structure build_block_decomposition(const int* A, int n);
extern RMQ_Structure build_sparce_table(const int* A, int n);
extern RMQ_Structure build_segment_tree(const int* A, int n);
extern RMQ_Structure build_hybrid_1(const int* A, int n);
extern RMQ_Structure build_hybrid_2(const int* A, int n);
extern RMQ_Structure build_hybrid_3(const int* A, int n);
extern RMQ_Structure build_fischer_heun(const int* A, int n);

// Tipo de dato para poder hacer un arreglo de funciones constructoras
typedef RMQ_Structure (*RMQ_Builder)(const int*, int);

int generar_entero_aleatorio() {
    // Pegamos dos llamados de rand() usando operaciones de bits para obtener 30 bits
    long long grande = ((long long)rand() << 15) | rand();
    // Ajustamos al rango estricto [0, 10^9] pedido por el laboratorio
    return (int)(grande % 1000000001); 
}

int main() {

    // 0. ABRIR EL ARCHIVO DE REPORTE CSV
    FILE *csv_file = fopen("resultados_rmq.csv", "w");
    if (csv_file == NULL) {
        printf("Error al crear el archivo CSV.\n");
        return 1;
    }
    // Cabecera del CSV con las metricas solicitadas
    fprintf(csv_file, "Estructura,N,Trial,Preprocesamiento_ns,TiempoBatch_s,Throughput,Memoria_Bytes\n");

    RMQ_Builder constructores[9] = {
        build_naive, build_full_preprocessing, build_block_decomposition,
        build_sparce_table, build_segment_tree, build_hybrid_1, 
        build_hybrid_2, build_hybrid_3, build_fischer_heun
    };

    const char* nombres[9] = {
        "Naive", "Full_Precalc", "Block_Decomp", "Sparse_Table",
        "Segment_Tree", "Hybrid_1", "Hybrid_2", "Hybrid_3", "Fischer_Heun"
    };

    int tamanos[NUM_SIZES];
    
    for (int i = 0; i < NUM_SIZES; i++) {
        tamanos[i] = 1 << (10 + 2 * i); 
    }

    // BUCLE PRINCIPAL: Cambia el tamaño del problema en cada vuelta
    for (int s = 0; s < NUM_SIZES; s++) {
        int N = tamanos[s]; // 'N' es el tamaño actual del arreglo
        
        printf("\n==================================================\n");
        printf(">>> INICIANDO EXPERIMENTACION PARA N = %d elementos <<<\n", N);
        printf("==================================================\n");

        // Aquí adentro meteremos el bucle de los 30 ensayos...
        // SEGUNDO BUCLE: Corre 30 pruebas independientes para el tamaño N actual
        for (int trial = 1; trial <= NUM_TRIALS; trial++) {
            printf("  -> Procesando Ensayo (Trial) %d de 30...\n", trial);

            // 1. Fijar semilla reproducible para este ensayo especifico
            srand(123456789 + trial);

            // 2. Asignar memoria para el arreglo principal de tamaño N
            int *A = (int *)malloc(N * sizeof(int));
            if (A == NULL) {
                printf("[ERROR] No hay memoria RAM suficiente para N = %d\n", N);
                return 1;
            }

            // 3. Rellenar el arreglo con los enteros aleatorios grandes
            for (int i = 0; i < N; i++) {
                A[i] = generar_entero_aleatorio();
            }
            // 4. Determinar dinamicamente el tamaño del lote de consultas (Q)
            int Q = (N <= 16384) ? 5000000 : 1000000;

            // 5. Reservar memoria para almacenar las Q consultas de este ensayo
            int *queries_left = (int *)malloc(Q * sizeof(int));
            int *queries_right = (int *)malloc(Q * sizeof(int));
            
            if (queries_left == NULL || queries_right == NULL) {
                printf("[ERROR] No se pudo asignar memoria para el lote de %d consultas.\n", Q);
                return 1;
            }

            // 6. Generar las consultas de forma aleatoria uniforme: i = min(x,y), j = max(x,y)
            for (int q = 0; q < Q; q++) {
                int x = rand() % N;
                int y = rand() % N;
                
                queries_left[q]  = (x < y) ? x : y; // i = min(x, y)
                queries_right[q] = (x > y) ? x : y; // j = max(x, y)
            }

            // 7. Generar las 10,000 consultas fijas para VALIDACION (Sin medir tiempo)
            int num_val = 10000;
            int *val_left = (int *)malloc(num_val * sizeof(int));
            int *val_right = (int *)malloc(num_val * sizeof(int));
            for (int q = 0; q < num_val; q++) {
                int x = rand() % N;
                int y = rand() % N;
                val_left[q]  = (x < y) ? x : y;
                val_right[q] = (x > y) ? x : y;
            }

            int *truth_answers = (int *)malloc(num_val * sizeof(int));

            // =============================================================
            // BUCLE 3: EVALUAR LAS 9 ESTRUCTURAS CON ESTOS DATOS FIJOS
            // =============================================================
            for (int est = 0; est < 9; est++) {
                
                // REGLA CRITICA: La estructura Cuadrática (índice 1) solo compite hasta N=16384
                if (est == 1 && N > 16384) continue; 

                printf("      [+] Evaluando %-15s ", nombres[est]);
                fflush(stdout);

                // --- A) MEDIR PREPROCESAMIENTO ---b
                struct timespec start_prep, end_prep;
                clock_gettime(CLOCK_MONOTONIC, &start_prep); // Inicia reloj
                
                RMQ_Structure db = constructores[est](A, N);
                
                clock_gettime(CLOCK_MONOTONIC, &end_prep);   // Detiene reloj
                long long t_prep_ns = (end_prep.tv_sec - start_prep.tv_sec) * 1000000000LL + 
                                      (end_prep.tv_nsec - start_prep.tv_nsec);

                // --- B) VALIDACION DE CORRECTITUD (10,000 queries) ---
                for (int q = 0; q < num_val; q++) {
                    int ans = db.query(db.pointer_to_structure, A, val_left[q], val_right[q], N);
                    (void)ans;
                    
                    if (est == 0) { // Si somos Naive, guardamos la respuesta oficial
                        truth_answers[q] = ans;
                    } else {        // Si somos otra estructura, comprobamos que el ÍNDICE coincida
                        int esperado = truth_answers[q];
                        
                        // CORRECCIÓN CRÍTICA: Comparamos índices, no valores
                        if (ans != esperado) { 
                            printf("\n[ERROR FATAL] %s fallo en consulta %d [%d, %d].\n", 
                                   nombres[est], q, val_left[q], val_right[q]);
                            printf("-> Esperaba Indice %d (Valor: %d) por regla Leftmost.\n", 
                                   esperado, A[esperado]);
                            printf("-> Obtuvo Indice   %d (Valor: %d).\n", 
                                   ans, A[ans]);
                            exit(1); // Abortar ejecucion segun requerimiento
                        }
                    }
                }   
                struct timespec start_q, end_q;
                clock_gettime(CLOCK_MONOTONIC, &start_q);
                
                for (int q = 0; q < Q; q++) {
                    // "volatile" obliga al compilador a no ignorar la funcion
                    volatile int ans = db.query(db.pointer_to_structure, A, queries_left[q], queries_right[q], N);
                }
                
                clock_gettime(CLOCK_MONOTONIC, &end_q);
                double t_batch_segundos = (end_q.tv_sec - start_q.tv_sec) + 
                                          (end_q.tv_nsec - start_q.tv_nsec) / 1000000000.0;
                
                // Calcular Throughput (Consultas por segundo: Q / T)
                double throughput = (double)Q / t_batch_segundos;

                // --- D) REGISTRAR EN EL CSV ---
                fprintf(csv_file, "%s,%d,%d,%lld,%f,%f,%zu\n", 
                        nombres[est], N, trial, t_prep_ns, t_batch_segundos, throughput, db.memory_bytes);

                // --- E) LIBERAR MEMORIA DE ESTA ESTRUCTURA ---
                db.free_data(db.pointer_to_structure, N);
                printf("[OK]\n");
            }
            free(queries_left);
            free(queries_right);
            free(val_left);      // <--- Faltaba liberar los arreglos de validacion
            free(val_right);     // <--- Faltaba liberar los arreglos de validacion
            free(truth_answers); // <--- Faltaba liberar la "hoja de respuestas" de Naive
            free(A);
        } // Fin del bucle de Trials
    }

    fclose(csv_file); // <--- MUY IMPORTANTE para que se guarden los datos en el disco duro
    
    printf("\n==================================================\n");
    printf("   EXPERIMENTO FINALIZADO CON EXITO\n");
    printf("   Revisa el archivo 'resultados_rmq.csv'\n");
    printf("==================================================\n");

    return 0;
}