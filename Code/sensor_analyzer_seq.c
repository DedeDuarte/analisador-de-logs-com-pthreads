#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TOKENS_QUANT 7 // Quantidades de tokens do .log
#define MAX_SENSORES 100 // Quantidade máxima de sensores aceita

typedef struct {
    int count;

    long double soma_temperatura;
    long double media_temperatura;
    long double M2;

    long double soma_umidade;
    long double soma_energia;
    long double soma_corrente;
    long double soma_pressao;
} SensorData;

/**
 * @brief Adiciona uma nova leitura de temperatura ao sensor e atualiza as estatísticas
 * @details Atualiza o contador, a média e o acumulador M2 utilizando o algoritmo iterativo
 *          para cálculo de desvio padrão (Welford). Também acumula a soma das temperaturas
 * 
 * @param sensor Ponteiro para a estrutura do sensor a ser atualizada
 * @param valor Valor da temperatura a ser adicionada
 */
void add_temperatura(SensorData* sensor, double valor) {
    sensor->count++;

    long double delta = valor - sensor->media_temperatura;
    sensor->media_temperatura += delta / sensor->count;

    long double delta2 = valor - sensor->media_temperatura;
    sensor->M2 += delta * delta2;

    sensor->soma_temperatura += valor;
}

/**
 * @brief Calcula o Desvio Padrão Populacional de um sensor de temperatura
 * @details Usando o método de Welford, para economizar memória
 * 
 * @param sensor Ponteiro para a estrutura do sensor a ser atualizada
 */
long double calcular_DP_temperatura(SensorData* sensor) {
    long double variancia = sensor->M2 / sensor->count;
    double desvio = sqrtl(variancia);

    return desvio;
}

/**
 * @brief Printa todos a Data de todos os sensores
 * 
 * @param sensores Lista de sensores
 * @param MAX_SENSORES Quantidade sensores
 */
void print_sensores(SensorData* sensores) {
    for (int i = 0; i < MAX_SENSORES; i++) {
        SensorData* sensor = &sensores[i];

        if (sensor->count == 0) continue;

        printf("Sensor %d:\n", i+1);
        printf("    count: %d\n", sensor->count);
        if (sensor->soma_temperatura != 0) {
            printf("    Soma temperatura:  %Lf\n", sensor->soma_temperatura);
            printf("    Media temperatura: %Lf\n", sensor->soma_temperatura / sensor->count);
            printf("    Media temperatura: %Lf\n", sensor->media_temperatura);
            printf("    DP da temperatura: %Lf\n", calcular_DP_temperatura(sensor));
        }
        // BUG:
        // Deu resultado diferente de eu fazendo a média como:
        //      sensor->soma_temperatura / sensor->count
        // ao invés de
        //      sensor->media_temp
        // que provavelmente está mais correto que o anterior

        /*
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE
            CONTINUE CONTINUE CONTINUE CONTINUE CONTINUE

            Acabei de terminar a implementação do calcular_DP_temperatura()
            ACHO que é so printar os dados direito agora. Ênfase no ---> __**ACHO**__ <---
        */
        if (sensor->soma_umidade != 0)     {
            printf("    Soma umidade:  %Lf\n", sensor->soma_umidade);
            printf("    Media umidade: %Lf\n", sensor->soma_umidade / sensor->count);
        }
        if (sensor->soma_energia != 0)     {
            printf("    Soma energia:  %Lf\n", sensor->soma_energia);
            printf("    Media energia: %Lf\n", sensor->soma_energia / sensor->count);
        }
        if (sensor->soma_corrente != 0)    {
            printf("    Soma corrente:  %Lf\n", sensor->soma_corrente);
            printf("    Media corrente: %Lf\n", sensor->soma_corrente / sensor->count);
        }
        if (sensor->soma_pressao != 0)     {
            printf("    Soma pressao:  %Lf\n", sensor->soma_pressao);
            printf("    Media pressao: %Lf\n", sensor->soma_pressao / sensor->count);
        }
        printf("\n");
    }
}

/**
 * @brief Printa todos a Data semi-tratada do .log
 * @details Usa a função @c print_sensores()
 * 
 * @param energiaTotal Total de enerdia
 * @param alertasTotais Total de alertas
 * @param criticosTotais Total de críticos
 * @param sensores Lista de sensores
 */
void print_data(long double energiaTotal, int okTotais, int alertasTotais, int criticosTotais, SensorData* sensores) {
    printf(
        "Energia total: %Lf\n"
        "OKs totais: %d\n"
        "Alertas totais: %d\n"
        "Crtiticos totais: %d\n\n",
        energiaTotal, okTotais, alertasTotais, criticosTotais
    );

    print_sensores(sensores);
}

int main(int argc, char* argv[]) {
    // Variaveis gerais
    SensorData sensores[MAX_SENSORES] = {0};
    long double energiaTotal = 0;
    int okTotais = 0;
    int alertasTotais = 0;
    int criticosTotais = 0;

    // Buscar nome do arquivo de logs
    char fileName[128];
    if (argc > 2) {
        printf(
            "Usos:\n"
            "Nome de arquivo padrão:        %s\n"
            "Nome de arquivo personalizado: %s <.log>\n",
            argv[0], argv[0]
        );

        exit(1);
    }
    else if (argc == 2) strcpy(fileName, argv[1]);
    else strcpy(fileName, "sensores.log");

    // Abrir arquivo
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        perror("fopen");
        exit(2);
    }

    // Ler cada linha do arquivo
    char linha[256];
    while (fgets(linha, sizeof(linha), file) != NULL) {
        linha[strcspn(linha, "\n")] = '\0'; // Remove o \n do final, e troca por \0

        char* token[TOKENS_QUANT];
        /* Formato dos tokens:

            IDENTIFICAÇÃO    |          MEDIDA         |       STATUS       |
        =====================|=========================|====================|
        token[0]: sensor_XXX | token[3]: tipo data     | token[5]: "status" |
        token[1]: AAAA-MM-DD | token[4]: <float> valor | token[6]: status   |
        token[2]: HH:MM:SS   |                         |                    |
        */

        token[0] = strtok(linha, " ");

        // Separando tokens da linha
        for (int i = 1; i < TOKENS_QUANT; i++) {
            token[i] = strtok(NULL, " "); // BUG Se uma linha não tiver exatamente TOKENS_QUANT tokens, vai dar merda aqui
        }

        int sensorID = atoi(token[0] + 7)-1;    // Formato: sensor_XXX -> sensor_ tem sete linhas,
                                                // então (sensor_ + 7) = Parte numérica.
                                                // -1 por ser index partindo de 0

        if (sensorID >= MAX_SENSORES) {
            printf("ERRO: Quantidade de sensores maior do que o permitido! (Qauntidade máxima: %d)\n", MAX_SENSORES);
            exit(3);
        }

        sensores[sensorID].count++;

        if (!strcmp(token[3], "energia")) {
            sensores[sensorID].soma_energia += atof(token[4]);
            energiaTotal += atof(token[4]);
        }
        // else if (!strcmp(token[3], "temperatura")) sensores[sensorID].soma_temperatura += atof(token[4]);
        else if (!strcmp(token[3], "temperatura")) add_temperatura(&sensores[sensorID], atof(token[4]));
        else if (!strcmp(token[3], "umidade"))     sensores[sensorID].soma_umidade     += atof(token[4]);
        else if (!strcmp(token[3], "corrente"))    sensores[sensorID].soma_corrente    += atof(token[4]);
        else if (!strcmp(token[3], "pressao"))     sensores[sensorID].soma_pressao     += atof(token[4]);
        else {
            printf("ERRO: Tipo de dado incorreto (%s)\n", token[3]);
            exit(4);
        }

        if      (!strcmp(token[6], "OK"))      okTotais++;
        else if (!strcmp(token[6], "ALERTA"))  alertasTotais++;
        else if (!strcmp(token[6], "CRITICO")) criticosTotais++;
        else {
            printf("ERRO: Status de dado incorreto (%s)\n", token[6]);
            exit(5);
        }
    }

    print_data(energiaTotal, okTotais, alertasTotais, criticosTotais, sensores);

    fclose(file);
    return 0;
}