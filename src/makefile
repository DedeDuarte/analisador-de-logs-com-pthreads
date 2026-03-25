CC = gcc
CFLAGS = -Wall -Wextra

LDFLAGS_SEQ = -lm
LDFLAGS_PAR = -lm -pthread
LDFLAGS_OPT = -lm -pthread

SRC_SEQ = sensor_analyzer_seq.c
SRC_PAR = sensor_analyzer_par.c
SRC_OPT = sensor_analyzer_optimized.c

TARGET_SEQ = a_seq.out
TARGET_PAR = a_par.out
TARGET_OPT = a_optimized.out

# Compilação
all:
	@echo "Compilando versão sequencial, paralela e otimizada"
	@$(CC) $(CFLAGS) -o $(TARGET_SEQ) $(SRC_SEQ) $(LDFLAGS_SEQ)
	@$(CC) $(CFLAGS) -o $(TARGET_PAR) $(SRC_PAR) $(LDFLAGS_PAR)
	@$(CC) $(CFLAGS) -o $(TARGET_OPT) $(SRC_OPT) $(LDFLAGS_OPT)

seq:
	@echo "Compilando versão sequencial"
	@$(CC) $(CFLAGS) -o $(TARGET_SEQ) $(SRC_SEQ) $(LDFLAGS_SEQ)

par:
	@echo "Compilando versão paralela"
	@$(CC) $(CFLAGS) -o $(TARGET_PAR) $(SRC_PAR) $(LDFLAGS_PAR)

opt:
	@echo "Compilando versão otimizada"
	@$(CC) $(CFLAGS) -o $(TARGET_OPT) $(SRC_OPT) $(LDFLAGS_OPT)

# Limpar
clean:
	@rm -f $(TARGET_SEQ) $(TARGET_PAR) $(TARGET_OPT)

# Help
help:
	@echo "Opções disponíveis:"
	@echo "    full  - Compila todas as versões"
	@echo "    seq   - Compila versão sequencial"
	@echo "    par   - Compila versão paralela"
	@echo "    opt   - Compila versão otimizada"
	@echo "    clean - Limpa arquivos"