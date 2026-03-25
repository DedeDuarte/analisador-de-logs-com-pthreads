CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS_SEQ = -lm
LDFLAGS_PAR = -lm -pthread

SRC_SEQ = sensor_analyzer_seq.c
TARGET_SEQ = a_seq.out

SRC_PAR = sensor_analyzer_par.c
TARGET_PAR = a_par.out

all:
	@echo "Compilando versão paralela"
	@$(CC) $(CFLAGS) -o $(TARGET_PAR) $(SRC_PAR) $(LDFLAGS_PAR)

# all:
# 	@echo "Opções disponíveis:"
# 	@echo "    seq   - Compila versão sequencial"
# 	@echo "    par   - Compila versão paralela"
# 	@echo "    clean - Limpa arquivos"

seq:
	@echo "Compilando versão sequencial"
	@$(CC) $(CFLAGS) -o $(TARGET_SEQ) $(SRC_SEQ) $(LDFLAGS_SEQ)

par:
	@echo "Compilando versão paralela"
	@$(CC) $(CFLAGS) -o $(TARGET_PAR) $(SRC_PAR) $(LDFLAGS_PAR)

clean:
	@rm -f $(TARGET_SEQ) $(TARGET_PAR)