# 1. VARIABLES
CC = gcc
CFLAGS = -Wall -Wextra -O3
LIBS = -lm
TARGET = experimento_rmq

# 2. ARCHIVOS
SRCS = main.c naive.c full_preprocessing.c block_decomposition.c sparse_table.c segment_tree.c hybrid_1.c hybrid_2.c hybrid_3.c fischer-heun.c
OBJS = $(SRCS:.c=.o)

# 3. REGLAS DE COMPILACIÓN
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 4. LIMPIEZA (¡YA NO BORRA EL CSV!)
clean:
	rm -f $(OBJS) $(TARGET)