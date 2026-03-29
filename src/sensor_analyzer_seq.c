#define _POSIX_C_SOURCE 199309L // Nescessário para usar CLOCK_MONOTONIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define TOKENS_QUANT 7 // Quantidades de tokens do .log
#define MAX_SENSORES 100 // Quantidade máxima de sensores aceita. Nescessário para alocar memória

/**
 * @brief Estrutura com informações gerais do arquivo lido
 * 
 * @param sensores Total de sensores lidos
 * 
 * @param okTotais Total de 'OK' lidos
 * @param alertasTotais Total de 'ALERTA' lidos
 * @param criticosTotais Total de 'CRITICO' lidos
 * @param energiaTotal Total de energia utilizada
 * 
 * @param inicio struct de @c timespec . Início do processo de analize do arquivo
 * @param fim struct de @c timespec . Fim do processo de analize do arquivo
 */
typedef struct {
    long long sensores;

    int okTotais;
    int alertasTotais;
    int criticosTotais;
    long double energiaTotal;

    struct timespec inicio;
    struct timespec fim;
} DataGeral;

/**
 * @brief Estrutura de um único sensor
 * @details @c sensor_... são variavis que indicam de qual tipo é aquele sensor
 * 
 * @param count Quantidade de vezes que aquele sensor foi lido
 * 
 * @param soma Soma total das leituras do sensor
 * @param media Media total do sensor, atualizado a cada nova leitura do mesmo
 * @param M2 Medida usada para calcular a DP do sensor no final.  Atualizado a cada nova leitura
 * 
 * @param sensor_temperatura Indica se o sensor é de temperatura
 * @param sensor_umidade Indica se o sensor é de umidade
 * @param sensor_energia Indica se o sensor é de energia
 * @param sensor_corrente Indica se o sensor é de corrente
 * @param sensor_pressao Indica se o sensor é de pressao
 */
typedef struct {
    int count;

    long double soma;
    long double media;
    long double M2;
    
    char sensor_temperatura;
    char sensor_umidade;
    char sensor_energia;
    char sensor_corrente;
    char sensor_pressao;
} SensorData;

/** NOTE Atualizar esse bloco de comentário
 * @brief Adiciona uma nova leitura de temperatura ao sensor e atualiza as estatísticas
 * @details Atualiza o contador, a média e o acumulador M2 utilizando o algoritmo iterativo
 *          para cálculo de desvio padrão (Welford). Também acumula a soma das temperaturas
 * 
 * @param sensor Ponteiro para a estrutura do sensor a ser atualizada
 * @param valor Valor da temperatura a ser adicionada
 */
void add_sensor(SensorData* sensor, double valor, char* tipo, char* status, DataGeral* data) {
    // Atribui qual o tipo do sensor
    if (sensor->count == 0) {
        data->sensores++;

        if      (!strcmp(tipo, "energia"))     { sensor->sensor_energia     = 1; }
        else if (!strcmp(tipo, "temperatura")) { sensor->sensor_temperatura = 1; }
        else if (!strcmp(tipo, "umidade"))     { sensor->sensor_umidade     = 1; }
        else if (!strcmp(tipo, "corrente"))    { sensor->sensor_corrente    = 1; }
        else if (!strcmp(tipo, "pressao"))     { sensor->sensor_pressao     = 1; }
        else {
            printf("ERRO: Tipo de dado incorreto (%s)\n", tipo);
            exit(4);
        }
    }

    // Atualizando soma, média e M2
    sensor->count++;

    long double delta = valor - sensor->media;
    sensor->media += delta / sensor->count;

    long double delta2 = valor - sensor->media;
    sensor->M2 += delta * delta2;

    sensor->soma += valor;
    if (!strcmp(tipo, "energia")) { data->energiaTotal += valor; }
    
    // Soma o status
    if      (!strcmp(status, "OK"))      data->okTotais++;
    else if (!strcmp(status, "ALERTA"))  data->alertasTotais++;
    else if (!strcmp(status, "CRITICO")) data->criticosTotais++;
    else {
        printf("ERRO: Status de dado incorreto (%s)\n", status);
        exit(5);
    }
}

/**
 * @brief Calcula o Desvio Padrão Populacional de um sensor de temperatura
 * @details Usando o método de Welford, para economizar memória
 * 
 * @param sensor Ponteiro para a estrutura do sensor a ser atualizada
 */
long double calcular_DP(SensorData* sensor) {
    long double variancia = sensor->M2 / sensor->count;
    double desvio = sqrtl(variancia);

    return desvio;
}

/**
 * @brief Printa a Data de um sensor
 * 
 * @param sensor Sensor a ser printado
 */
void print_sensor(SensorData* sensor, char* tipo) {
    printf(
        "    Count: %d\n"
        "    Soma  %s: %Lf\n"
        "    Media %s: %Lf\n"
        "    DP da %s: %Lf\n\n",
        sensor->count,
        tipo, sensor->soma,
        tipo, sensor->media, 
        tipo, calcular_DP(sensor)
    );
}

/**
 * @brief Printa todos a Data de todos os sensores
 * @details Usa @c print_sensor()
 * 
 * @param sensores Lista de sensores
 * @param MAX_SENSORES Quantidade sensores
 */
void print_sensores(SensorData* sensores) {
    for (int i = 0; i < MAX_SENSORES; i++) {
        SensorData* sensor = &sensores[i];

        if (sensor->count == 0) continue;

        printf("Sensor %d:\n", i+1);
        if      (sensor->sensor_temperatura != 0) { print_sensor(sensor, "temperatura"); }
        else if (sensor->sensor_umidade != 0)     { print_sensor(sensor, "umidade"); }
        else if (sensor->sensor_energia != 0)     { print_sensor(sensor, "energia"); }
        else if (sensor->sensor_corrente != 0)    { print_sensor(sensor, "corrente"); }
        else if (sensor->sensor_pressao != 0)     { print_sensor(sensor, "pressao"); }
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
void print_data(DataGeral* data, SensorData* sensores) {
    printf(
        "Energia total: %Lf\n"
        "OKs totais: %d\n"
        "Alertas totais: %d\n"
        "Crtiticos totais: %d\n\n",
        data->energiaTotal, data->okTotais, data->alertasTotais, data->criticosTotais
    );

    print_sensores(sensores);
}

/**
 * @brief Inicio do cronômetro
 * 
 * @param data Estrutura onde estão as struct @c timespec de inicio e fim
 */
void timer_inicio(DataGeral* data) {
    clock_gettime(CLOCK_MONOTONIC, &data->inicio);
}

/**
 * @brief Fim do cronômetro
 * 
 * @param data Estrutura onde estão as struct @c timespec de inicio e fim
 */
void timer_fim(DataGeral* data) {
    clock_gettime(CLOCK_MONOTONIC, &data->fim);
}

/**
 * @brief Retorna o tempo decorrido entre o inicio e fim do cronômetro
 * 
 * @param data Estrutura onde estão as struct @c timespec de inicio e fim
 */
double timer_resultado(DataGeral* data) {
    return (data->fim.tv_sec - data->inicio.tv_sec) +
           (data->fim.tv_nsec - data->inicio.tv_nsec) / 1e9;
}

/**
 * @brief Print final para o trabalho
 * @details Chama as funções: @c calcular_DP() e @c timer_resultado()
 * 
 * @param data Estrutura de dados das informações gerais do arquivo
 * @param sensores Lista de estruturas @c SensorData . Quantidade: @c MAX_SENSORES
 */
void print_final(DataGeral* data, SensorData* sensores) {
    char temp_printados = 0;
    printf("\nMedia dos 10 primeiros sensores de temperatura:\n");
    for (int i = 0; i < data->sensores; i++) {
        if (sensores[i].sensor_temperatura) {
            printf("    Media sensor %2d (temp): %3.2Lf\n", i+1, sensores[i].media);
            temp_printados++;

            if (temp_printados >= 10) break;
        }
    }

    // Calculo e print do sensor mais instavel
    int instavel_index = 0;
    float DP = calcular_DP(&sensores[instavel_index]);
    for (int i = 1; i < data->sensores; i++) {
        float DP_i = calcular_DP(&sensores[i]);

        if (DP_i > DP) {
            instavel_index = i;
            DP = DP_i;
        }
    }

    SensorData* instavel = &sensores[instavel_index];

    char tipo[12];

    if      (instavel->sensor_energia)     { strcpy(tipo, "energia"); }
    else if (instavel->sensor_temperatura) { strcpy(tipo, "temperatura"); }
    else if (instavel->sensor_umidade)     { strcpy(tipo, "umidade"); }
    else if (instavel->sensor_corrente)    { strcpy(tipo, "corrente"); }
    else if (instavel->sensor_pressao)     { strcpy(tipo, "pressao"); }

    printf("\n"
        "Sensor mais instavel:\n"
        "    Sensor %d (%s)\n"
        "    DP: %f\n",
        instavel_index+1, tipo, DP
    );

    // Total de alertas
    printf("\n"
        "Total de status:\n"
        "    Total:   %lld\n"
        "    OK:      %d\n"
        "    Alerta:  %d\n"
        "    Critico: %d\n"
        "\n"
        "    Energia: %Lf\n",
        (long long) data->okTotais + (long long) data->alertasTotais + (long long) data->criticosTotais,
        data->okTotais,  data->alertasTotais,  data->criticosTotais, data->energiaTotal
    );

    // Tempo de execução
    printf("\n"
        "Tempo de execução:\n"
        "    %.6fseg\n",
        timer_resultado(data)
    );
}

/**
 * @brief Abrir e analizar arquivo solicitado, preenchendo estsrutauras com seus dados
 * @details Chama a função @c add_sensor()
 * 
 * @param fileName Nome do arquivo desejado
 * @param data Estrutura de dados das informações gerais do arquivo
 * @param sensores Lista de estruturas @c SensorData . Quantidade: @c MAX_SENSORES
 */
void sensor_analyzer_seq(char* fileName, DataGeral* data, SensorData* sensores) {
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
        // Formato dos tokens:
        //
        //     IDENTIFICAÇÃO    |          MEDIDA         |       STATUS       |
        // =====================|=========================|====================|
        // token[0]: sensor_XXX | token[3]: tipo data     | token[5]: "status" |
        // token[1]: AAAA-MM-DD | token[4]: <float> valor | token[6]: status   |
        // token[2]: HH:MM:SS   |                         |                    |
        
        token[0] = strtok(linha, " ");

        // Separando tokens da linha
        for (int i = 1; i < TOKENS_QUANT; i++) {
            token[i] = strtok(NULL, " ");
        }

        int sensorID = atoi(token[0] + 7)-1; // Formato: sensor_XXX -> sensor_ tem sete linhas,
                                             // então (sensor_ + 7) = Parte numérica.
                                             // -1 por ser index partindo de 0

        if (sensorID >= MAX_SENSORES) {
            printf("ERRO: Quantidade de sensores maior do que o permitido! (Qauntidade máxima: %d)\n", MAX_SENSORES);
            exit(3);
        }

        add_sensor(&sensores[sensorID], atof(token[4]), token[3], token[6], data);
    }

    fclose(file);
}

/**
 * @brief Função principal
 * @details Chama as funções @c sensor_analyzer_seq() e @c time_...()
 * 
 * @param argc Quantidade de argumentos passados na execução
 * @param argv Lista de strings dos argumentos passados
 */
int main(int argc, char* argv[]) {
    // Variaveis gerais
    DataGeral data = {0};
    SensorData sensores[MAX_SENSORES] = {0};

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

    // Processo principal
    timer_inicio(&data);
    sensor_analyzer_seq(fileName, &data, sensores);
    timer_fim(&data);

    // print_data(&data, sensores);
    print_final(&data, sensores);

    return 0;
}