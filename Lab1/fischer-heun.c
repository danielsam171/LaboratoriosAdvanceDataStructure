#include "rmq_interface.h"
#include "fischer-heun.h"
#include <stdlib.h>
#include <math.h>

static int min(int i, int j, const int* A) {
    if (A[i] < A[j]) return i;
    if (A[j] < A[i]) return j;
    // Si los VALORES son exactamente iguales, desempatamos por el ÍNDICE menor (el de más a la izquierda)
    return (i < j) ? i : j; 
}

// CORRECCIÓN 1: Se añadió el parámetro 'b' a la firma
static int cartesian_number(const int *A, int start, int len, int b) {
    int stack[64], top = 0, num = 0;
    
    for (int i = 0; i < len; i++) {
        while (top > 0 && A[stack[top-1]] > A[start + i]) {
            top--;
            num = (num << 1) | 1; 
        }
        stack[top++] = start + i;
        num = (num << 1) | 0;
    }
    
    // Novedad A: Vaciamos la pila obligatoriamente. 
    // Esto garantiza exactamente 'len' ceros y 'len' unos en la firma.
    while (top > 0) {
        top--;
        num = (num << 1) | 1;
    }
    
    // Novedad B: Padding (Alineación)
    // Si el bloque es más pequeño que 'b' (el último), alineamos los bits a 
    // la izquierda para que su firma sea inconfundible en el espacio de 2b bits.
    num = num << (2 * (b - len));
    
    return num;
}

// CORRECCIÓN 2: Se añadió el parámetro 'n' a la firma para vigilar el límite
static unsigned char *build_block_table(const int *A, int start, int b, int n) {
    unsigned char *tbl = (unsigned char *)malloc(b * b * sizeof(unsigned char));
    
    for (int i = 0; i < b; i++) {
        tbl[i * b + i] = (unsigned char)i;
        for (int j = i + 1; j < b; j++) {
            int prev = tbl[i * b + (j-1)];
            
            // Novedad C: Evitamos el Out-Of-Bounds
            if (start + j < n) {
                tbl[i * b + j] = (A[start + prev] <= A[start + j])
                                 ? (unsigned char)prev : (unsigned char)j;
            } else {
                // Rellenamos con el mínimo actual propagado (dummy state) 
                // para que la consulta no se rompa ni afecte rangos válidos.
                tbl[i * b + j] = (unsigned char)prev;
            }
        }
    }
    return tbl;
}

static FH_State *reserve_space_fh(const int b, const int nb){
    int levels    = (nb > 1) ? (int)floor(log2((double)nb)) + 1 : 1;
    int num_types = 1 << (2 * b);
 
    FH_State *state   = (FH_State *)malloc(sizeof(FH_State));
    state->b          = b;
    state->nb         = nb;
    state->st_levels  = levels;
    state->num_types  = num_types;
 
    state->block_min  = (int *)malloc(nb * sizeof(int));
    state->cartoon    = (int *)malloc(nb * sizeof(int));
    state->in_block   = (unsigned char **)calloc(num_types, sizeof(unsigned char *));
 
    state->st = (int **)malloc(levels * sizeof(int *));
    for (int k = 0; k < levels; k++)
        state->st[k] = (int *)malloc(nb * sizeof(int));
 
    return state;
}

void *build_fischer_heun_structure(const int *A, const int n) {
    // Magia de Fischer-Heun: El bloque mide log(n) / 2
    int b  = (n > 1) ? (int)floor(log2((double)n) / 2.0) : 1;
    if (b < 1) b = 1;
    int nb = (n + b - 1) / b;
 
    FH_State *state = reserve_space_fh(b, nb);
    state->n = n;
 
    for (int bi = 0; bi < nb; bi++) {
        int start = bi * b;
        int len   = (start + b <= n) ? b : (n - start);
 
        int mi = start;
        for (int k = 1; k < len; k++)
            if (A[start + k] < A[mi]) mi = start + k;
        state->block_min[bi] = mi;
 
        // Novedad D: Pasamos 'b' a cartesian_number y 'n' a build_block_table
        int ct = cartesian_number(A, start, len, b);
        state->cartoon[bi] = ct;
 
        if (state->in_block[ct] == NULL)
            state->in_block[ct] = build_block_table(A, start, b, n);
    }
 
    for (int i = 0; i < nb; i++)
        state->st[0][i] = i;
 
    for (int k = 1; k < state->st_levels; k++) {
        int half = 1 << (k - 1);
        for (int i = 0; i + (1 << k) <= nb; i++) {
            int l = state->st[k-1][i];
            int r = state->st[k-1][i + half];
            state->st[k][i] = (A[state->block_min[l]] <= A[state->block_min[r]]) ? l : r;
        }
    }
 
    return (void *)state;
}

// CORRECCIÓN: Se añadió 'const int n' al final para respetar el contrato
int query_fischer_heun_structure(const void *internal_state, const int *A, int i, int j, const int n) {
    (void)n; // Evita warning de variable no usada
    FH_State *state = (FH_State *)internal_state;
    int b  = state->b;
    int bi = i / b, bj = j / b;
 
    if (bi == bj) {
        int start = bi * b;
        int off   = (int)state->in_block[state->cartoon[bi]][(i - start) * b + (j - start)];
        return start + off;
    }
 
    int start_i = bi * b;
    int off_i   = (int)state->in_block[state->cartoon[bi]][(i - start_i) * b + (b - 1)];
    int cand    = start_i + off_i;
 
    int start_j = bj * b;
    int off_j   = (int)state->in_block[state->cartoon[bj]][0 * b + (j - start_j)];
    cand = min(cand, start_j + off_j, A); // CORRECCIÓN: fh_min -> min
 
    if (bj - bi > 1) {
        int bl = bi + 1, br = bj - 1;
        int k  = 31 - __builtin_clz(br - bl + 1);
        int l  = state->st[k][bl];
        int r  = state->st[k][br - (1 << k) + 1];
        int mid = (A[state->block_min[l]] <= A[state->block_min[r]]) ? l : r;
        cand = min(cand, state->block_min[mid], A); // CORRECCIÓN: fh_min -> min
    }
 
    return cand;
}

void free_fischer_heun_structure(void *internal_state, const int n) {
    (void)n;
    FH_State *state = (FH_State *)internal_state;
 
    for (int k = 0; k < state->st_levels; k++)
        free(state->st[k]);
    free(state->st);
 
    for (int t = 0; t < state->num_types; t++)
        if (state->in_block[t]) free(state->in_block[t]);
    free(state->in_block);
 
    free(state->block_min);
    free(state->cartoon);
    free(state);
}

RMQ_Structure build_fischer_heun(const int *A, int n) {
    RMQ_Structure fischer_heun_structure;
 
    fischer_heun_structure.pointer_to_structure = build_fischer_heun_structure(A, n);
 
    FH_State *state = (FH_State *)fischer_heun_structure.pointer_to_structure;
    
    // CORRECCIÓN: Cálculo riguroso de memoria real instanciada
    size_t memory = sizeof(FH_State)
                  + state->nb * sizeof(int) * 2
                  + state->nb * state->st_levels * sizeof(int)
                  + state->st_levels * sizeof(int *)
                  + state->num_types * sizeof(unsigned char *);
                  
    // Solo sumamos los micro-bloques que realmente se crearon
    for (int t = 0; t < state->num_types; t++) {
        if (state->in_block[t] != NULL) {
            memory += (size_t)(state->b * state->b) * sizeof(unsigned char);
        }
    }
    
    fischer_heun_structure.memory_bytes = memory;
    fischer_heun_structure.query        = query_fischer_heun_structure;
    fischer_heun_structure.free_data    = free_fischer_heun_structure;
 
    return fischer_heun_structure;
}