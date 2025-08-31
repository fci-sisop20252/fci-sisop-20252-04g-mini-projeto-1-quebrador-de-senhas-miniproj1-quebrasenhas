#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * 
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * 
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * 
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 * 
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 * 
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 * @return 0 em caso de sucesso, 1 em erro de uso, 2 caso senha não encontrada
 */
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso interno: %s <hash> <pass_len> <charset> <num_workers> <charset_len>\n", argv[0]);
        return 1;
    }

    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);

    // Validações dos parâmetros
    // password_len deve estar entre 1 e 10
    if (password_len < 1 || password_len > 10) {
        fprintf(stderr, "Tamanho de senha invalido [%d]. Valor deve estar entre 1 e 10\n", password_len);
        return 1;
    }

    // num_workers deve estar entre 1 e MAX_WORKERS
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Quantidade de workers invalida [%d]. Quantidade deve ser entre 1 e %d\n", num_workers, MAX_WORKERS);
        return 1;
    }

    // charset não pode ser vazio
    if (strlen(charset) == 0) {
        fprintf(stderr, "Charset nao pode estar vazio!\n");
        return 1;
    }

    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);

    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);

    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);

    // Registrar tempo de início
    time_t start_time = time(NULL);

    // Dividindo quantidade de senhas testadas entre os workers
    long long total_passwords = 1;
    for (int i = 0; i < password_len; i++) total_passwords *= charset_len;

    long long passwords_per_worker = total_passwords / num_workers;
    long long remaining = total_passwords % num_workers;

    // Arrays para armazenar PIDs dos workers
    pid_t workers[MAX_WORKERS];

    printf("Iniciando workers...\n");

    // Criando processos filhos com fork(), cada um com uma parte das senhas a serem testadas
    for (int i = 0; i < num_workers; i++) {
        long long start_index = i * passwords_per_worker;
        long long end_index   = start_index + passwords_per_worker;

        char start_pass [password_len];
        char end_pass [password_len];

        // OPTIMIZE: Tranformar start/end_index para start/end_pass
        // Acho que tem um jeito melhor de fazer isso, só n consegui pensar ainda

        // Gerando start_pass a partir do start_index
        long long tmp = start_index;
        for (int i = password_len - 1; i >= 0; i--) {
            start_pass[i] = charset[tmp % charset_len];
            tmp /= charset_len;
        }

        // Gerando end_pass a partir do end_index
        tmp = end_index;
        for (int i = password_len - 1; i >= 0; i--) {
            end_pass[i] = charset[tmp % charset_len];
            tmp /= charset_len;
        }

        // TODO 4: Usar fork() para criar processo filho
        pid_t pid = fork();

        if (pid < 0) {
            perror("frok");
            exit(1);
        }

        else if (pid == 0) { // Processo filho
            char password_len_str [16];
            snprintf(password_len_str, sizeof(password_len_str), "%d", password_len);
            
            char worker_id_str[16];
            snprintf(worker_id_str, sizeof(worker_id_str), "%d", i);
            
            // Uso do worker: ./worker <hash> <start> <end> <charset> <len> <id>\n"
            execl("./worker", "worker", target_hash, start_pass, end_pass, charset, password_len_str, worker_id_str, (char*)NULL);

            perror("execl");
            exit(1);
        }

        else { // Processo pai
            printf("Iniciando worker %d\n", i);

            workers[i] = pid;
        }
    }
    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");

    int finished_workers = 0;
    for (int i = 0; i < num_workers; i++) {
        int status;

        pid_t pid = wait(&status);

        if (pid == -1) {
            perror("wait");
            continue;
        }

        int worker = -1;
        for (int j = 0; j < MAX_WORKERS; j++) {
            if (workers[j] = pid) {
                worker = j;
                break;
            }
        }

        finished_workers++;

        if (worker == -1) {
            fprintf(stderr, "Worker %d desconhecido", pid);
            continue;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("Worker %d [PID %d] terminou corretamente com codigo: %d\n", worker, pid, exit_code);
        }
        else if (WIFSIGNALED(status)) {
            int exit_signal = WTERMSIG(status);
            printf("Worker %d [PID %d] terminou com um sinal: %d\n", worker, pid, exit_signal);
        }
        else {
            printf("Worker %d [PID %d] terminou de forma inesperada\n", worker, pid);
        }
    }

    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);

    printf("\n=== Resultado ===\n");

    int file = open(RESULT_FILE, O_RDONLY);

    if (file < 0) {
        printf("Senha não encontrada!");
        return 2;
    }

    char buffer[1024];
    ssize_t bytes = read(file, buffer, sizeof(buffer) - 1);
    if (bytes <= 0 ) {
        perror("read");
        close(file);
        return -1;
    }
    close(file);

    buffer[bytes] = '\0';

    char *infos = strchr(buffer, ':');
    if (!infos) {
        fprintf(stderr, "Formato incorreno no arquivo\n");
        return -1;
    }

    *infos = '\0';
    char *worker_id = buffer;
    char *password = infos + 1;


    char hash_found[33];
    md5_string(password, hash_found);

    if (strcmp(hash_found, target_hash))
        printf("Senha encontrada com sucesso!\nHash: %s\nSenha: %s", hash_found, password);
    else
        printf("Senha encontrada incorreta (tenho a impressão de que vou ver muito essa msgm durante o debug)");

    // Estatísticas finais (opcional)
    // TODO: Calcular e exibir estatísticas de performance

    printf("Tempo de execucao: %fsegundos", elapsed_time);

    return 0;
}