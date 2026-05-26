#ifndef RMQ_INTERFACE_H
#define RMQ_INTERFACE_H

#include <stddef.h> 

 //Todas las implementaciones devolverán esta misma estructura.
typedef struct {
    void* pointer_to_structure;  // Puntero a la estructura
    size_t memory_bytes;
    
    // Función para buscar el mínimo (recibe la estructura, el arreglo y los índices)
    int (*query)(const void* pointer_to_structure, const int* A, int i, int j,const int n); 
    
    // Función para destruir y liberar la memoria de la estructura
    void (*free_data)(void* pointer_to_structure, const int n); 
} RMQ_Structure;

//Puntero a las funciones de creacion de las estructuras
typedef RMQ_Structure (*RMQ_Builder)(const int* A, int n);

#endif 