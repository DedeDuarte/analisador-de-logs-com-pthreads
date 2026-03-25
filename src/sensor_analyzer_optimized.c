#define _POSIX_C_SOURCE 199309L // Nescessário para usar CLOCK_MONOTONIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

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
    pthread_mutex_t mutex;

    long long sensores;

    int okTotais;
    int alertasTotais;
    int criticosTotais;
    long double energiaTotal;

    struct timespec inicio;
    struct timespec fim;
} DataGeral;

typedef struct {
    long long sensores;

    int okTotais_local;
    int alertasTotais_local;
    int criticosTotais_local;
    long double energiaTotal_local;
} DataLocal;

/**
 * @brief Estrutura de um único sensor. Nescessária a inicialização do mutex
 * @details @c sensor_... são variavis que indicam de qual tipo é aquele sensor
 * 
 * @param mutex Mutex para evitar que múltiplas threads editem um sensor ao mesmo tempo
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

/**
 * @brief Estrutura de args e ponteiros para cada thread
 * 
 * @param thread Thread_t dela mesma
 * 
 * @param id ID da thread (valor do @c i )
 * 
 * @param inicio De que linha a thread deve começar
 * @param fim Linha a qual a thread n deve mais acessar
 * @param fileName Nome do arquivo a ser lido
 * 
 * @param data Data geral do arquivo lido
 * @param sensores Array de sensores com seus mutex
 */
typedef struct {
    pthread_t thread;

    int id;

    long long inicio;
    long long fim;
    char* fileName;

    DataGeral* data;
    SensorData* sensores;
} Thread;

/**
 * @brief Adiciona uma nova leitura de temperatura ao sensor e atualiza as estatísticas
 * @details Atualiza o contador, a média e o acumulador M2 utilizando o algoritmo iterativo
 *          para cálculo de desvio padrão (Welford). Também acumula a soma das temperaturas
 * 
 * @param sensor Ponteiro para a estrutura do sensor a ser atualizada
 * @param valor Valor da temperatura a ser adicionada
 */
void add_sensor(SensorData* sensor_local, double valor, char* tipo, char* status, DataLocal* data_local, char* linha) {
    // Atribui qual o tipo do sensor
    if (sensor_local->count == 0) {
        data_local->sensores++;

        if      (!strcmp(tipo, "energia"))     { sensor_local->sensor_energia     = 1; }
        else if (!strcmp(tipo, "temperatura")) { sensor_local->sensor_temperatura = 1; }
        else if (!strcmp(tipo, "umidade"))     { sensor_local->sensor_umidade     = 1; }
        else if (!strcmp(tipo, "corrente"))    { sensor_local->sensor_corrente    = 1; }
        else if (!strcmp(tipo, "pressao"))     { sensor_local->sensor_pressao     = 1; }
        else {
            printf("ERRO: Tipo de dado incorreto (%s)\n", tipo);
            exit(4);
        }
    }

    // Atualizando soma, média e M2
    sensor_local->count++;

    long double delta = valor - sensor_local->media;
    sensor_local->media += delta / sensor_local->count;

    long double delta2 = valor - sensor_local->media;
    sensor_local->M2 += delta * delta2;

    sensor_local->soma += valor;
    
    // Soma o status
    if (!strcmp(tipo, "energia")) { data_local->energiaTotal_local += valor; }
    
    if      (!strcmp(status, "OK"))      { data_local->okTotais_local++; }
    else if (!strcmp(status, "ALERTA"))  { data_local->alertasTotais_local++; }
    else if (!strcmp(status, "CRITICO")) { data_local->criticosTotais_local++; }
    else {
        printf("ERRO: Status de dado incorreto (%s):\n%s\n", status, linha);
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
    printf(
        "Ps.:\n"
        "    Estou printando duas medias:\n"
        "    - A media de todos os sensores\n"
        "    - A media apenas dos sensores de temperatura\n"
        "    Nao entendi qual dos dois que era para ser printado.\n"
        "    Printando os 2 nao tem erro hahaha\n\n"
    );
    printf("Media dos 10 primeiros sensores:\n");
    for (int i = 0; i < 10 && i < data->sensores; i++) {
        printf("    Media sensor %2d: %6.2Lf\n", i+1, sensores[i].media);
    }

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
    int instavel_index = -1;
    long double DP = 0.0L;
    for (int i = 0; i < MAX_SENSORES; i++) {
        if (sensores[i].count == 0) continue;

        long double dp_i = calcular_DP(&sensores[i]);

        if (instavel_index < 0 || dp_i > DP) {
            instavel_index = i;
            DP = dp_i;
        }
    }

    if (instavel_index < 0) {
        printf("\nNenhum sensor para calcular instabilidade\n");
        return;
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
        "    DP: %Lf\n",
        instavel_index+1, tipo, DP
    );

    // Total de alertas
    printf("\n"
        "Total de status:\n"
        "    Total:   %lld\n"
        "    OK:      %d\n"
        "    Alerta:  %d\n"
        "    Critico: %d\n",
        (long long) data->okTotais + (long long) data->alertasTotais + (long long) data->criticosTotais,
        data->okTotais,  data->alertasTotais,  data->criticosTotais
    );

    // Tempo de execução
    printf("\n"
        "Tempo de execução:\n"
        "    %.6fseg\n",
        timer_resultado(data)
    );
}

void* func(void* args) {
    DataLocal data_local = {0};
    SensorData sensores_local[MAX_SENSORES] = {0};

    Thread* thread = (Thread*) args;

    printf(
        "Thread %d: %11lld -> %11lld\n",
        thread->id, thread->inicio, thread-> fim
    );

    // Abrir arquivo
    FILE* file = fopen(thread->fileName, "r");
    if (file == NULL) {
        perror("fopen");
        exit(2);
    }

    fseek(file, thread->inicio, SEEK_SET);
    
    // Se não for a primeira thread, vai pra próxima linha
    if (thread->inicio != 0) {
        int c;
        while ((c = fgetc(file)) != '\n' && c != EOF);
    }

    // Lê o arquivo até o fim do bloco dessa thread
    char linha[256];
    while (ftell(file) < thread->fim && fgets(linha, sizeof(linha), file) != NULL) {
        linha[strcspn(linha, "\n")] = '\0'; // Remove o \n do final, e troca por \0

        char linha_save[256];
        strcpy(linha_save, linha);

        char* token[TOKENS_QUANT];
        // Formato dos tokens:
        //
        //     IDENTIFICAÇÃO    |          MEDIDA         |       STATUS       |
        // =====================|=========================|====================|
        // token[0]: sensor_XXX | token[3]: tipo data     | token[5]: "status" |
        // token[1]: AAAA-MM-DD | token[4]: <float> valor | token[6]: status   |
        // token[2]: HH:MM:SS   |                         |                    |
        
        char* saveptr; // Necessário para strtok_r (thread-safe)
        token[0] = strtok_r(linha, " ", &saveptr);
        
        // Separando tokens da linha
        for (int i = 1; i < TOKENS_QUANT; i++) {
            token[i] = strtok_r(NULL, " ", &saveptr);
        }

        if (!token[0] || !token[3] || !token[4] || !token[6]) {
            continue; // pula linha inválida
        }
        
        int sensorID = atoi(token[0] + 7)-1; // Formato: sensor_XXX -> sensor_ tem sete linhas,
                                             // então (sensor_ + 7) = Parte numérica.
                                             // -1 por ser index partindo de 0

        if (sensorID >= MAX_SENSORES) {
            printf("ERRO: Quantidade de sensores maior do que o permitido! (Qauntidade máxima: %d)\n", MAX_SENSORES);
            exit(3);
        }

        // Computando dados. O mutex atua dentro da função
        add_sensor(&sensores_local[sensorID], atof(token[4]), token[3], token[6], &data_local, linha_save);
    }

    fclose(file);

    // SensorData sensores_local[MAX_SENSORES] = {0};

    pthread_mutex_lock(&thread->data->mutex);
    thread->data->sensores       += data_local.sensores;
    thread->data->okTotais       += data_local.okTotais_local;
    thread->data->alertasTotais  += data_local.alertasTotais_local;
    thread->data->criticosTotais += data_local.criticosTotais_local;
    thread->data->energiaTotal   += data_local.energiaTotal_local;

    for (int i = 0; i < MAX_SENSORES && i < data_local.sensores; i++) {
        SensorData *s_global = &thread->sensores[i];
        SensorData *s_local = &sensores_local[i];

        if (s_local->count == 0) continue;

        // copia direta se não havia dados globais ainda
        if (s_global->count == 0) {
            *s_global = *s_local;
            continue;
        }

        long long n_a = s_global->count;
        long long n_b = s_local->count;
        long long n_ab = n_a + n_b;

        long double delta = s_local->media - s_global->media;
        long double media_ab = (n_a * s_global->media + n_b * s_local->media) / n_ab;
        long double M2_ab = s_global->M2 + s_local->M2 + delta * delta * ((long double)n_a * n_b) / n_ab;

        s_global->count = n_ab;
        s_global->soma += s_local->soma;
        s_global->media = media_ab;
        s_global->M2 = M2_ab;

        s_global->sensor_temperatura |= s_local->sensor_temperatura;
        s_global->sensor_umidade     |= s_local->sensor_umidade;
        s_global->sensor_energia     |= s_local->sensor_energia;
        s_global->sensor_corrente    |= s_local->sensor_corrente;
        s_global->sensor_pressao     |= s_local->sensor_pressao;
    }
    pthread_mutex_unlock(&thread->data->mutex);

    printf(
        "Thread %d: Finalizando!\n",
        thread->id
    );

    return NULL;
}

void sensor_analyzer_par(char* fileName, DataGeral* data, SensorData* sensores, const int quantThreads) {
    // Descobrindo tamanho do arquivo
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        perror("fopen");
        exit(2);
    }
    
    fseek(file, 0, SEEK_END);
    long long tamanhoBytes = ftell(file); // Tamanho do arquivo em bytes
    rewind(file);

    fclose(file);

    // Inicializando threads
    Thread* threads = malloc(quantThreads * sizeof(Thread));

    long long chunk = tamanhoBytes / quantThreads;

    for (int i = 0; i < quantThreads; i++) {
        Thread* thread = &threads[i];

        thread->id = i;
        thread->inicio = i * chunk;
        thread->fim = (i == quantThreads-1) ? tamanhoBytes : (i+1) * chunk;
        thread->fileName = fileName;
        thread->data = data;
        thread->sensores = sensores;

        pthread_create(&thread->thread, NULL, func, thread);
    }

    for (int i = 0; i < quantThreads; i++) { pthread_join(threads[i].thread, NULL); }
    putchar('\n');

    free(threads);
}

int main(int argc, char* argv[]) {
    // Buscar nome do arquivo de logs
    char fileName[128];
    if (argc < 2 || argc > 3) {
        printf(
            "Usos:\n"
            "Nome de arquivo padrão:        %s <num threads>\n"
            "Nome de arquivo personalizado: %s <num threads> <.log>\n",
            argv[0], argv[0]
        );
        
        exit(1);
    }
    int quantThreads = atoi(argv[1]);
    if (argc == 3) strcpy(fileName, argv[2]);
    else strcpy(fileName, "sensores.log");
    
    // Variaveis gerais
    DataGeral data = {0};
    pthread_mutex_init(&data.mutex, NULL);

    SensorData sensores[MAX_SENSORES] = {0};

    // Processo principal
    timer_inicio(&data);
    sensor_analyzer_par(fileName, &data, sensores, quantThreads);
    timer_fim(&data);

    // print_data(&data, sensores);
    print_final(&data, sensores);

    return 0;
}