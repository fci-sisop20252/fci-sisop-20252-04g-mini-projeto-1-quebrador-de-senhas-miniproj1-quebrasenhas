#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO TRABALHADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa verifica um subconjunto do espaço de senhas, usando a biblioteca
 * MD5 FORNECIDA para calcular hashes e comparar com o hash alvo.
 * 
 * Uso: ./worker <hash_alvo> <senha_inicial> <senha_final> <charset> <tamanho> <worker_id>
 * 
 * EXECUTADO AUTOMATICAMENTE pelo coordinator através de fork() + execl()
 * SEU TRABALHO: Implementar os TODOs de busca e comunicação
 */

#define RESULT_FILE "password_found.txt"
#define PROGRESS_INTERVAL 100000  // Reportar progresso a cada N senhas

/**
 * Incrementa uma senha para a próxima na ordem lexicográfica (aaa -> aab -> aac...)
 * 
 * @param password Senha atual (será modificada)
 * @param charset Conjunto de caracteres permitidos
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @return 1 se incrementou com sucesso, 0 se chegou ao limite (overflow)
 */
int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    // Incrementa a partir da ultima letra
    // Se a ultima letra fazer a "volta" toda no charset, incrementa na proxima, até o fim de *password
    for (int i = password_len - 1; i >= 0; i--) {

        // Acha a letra password[i] em charset[charset_index]
        int charset_index = 0;
        while (password[i] != charset[charset_index] && charset_index < charset_len) charset_index++;

        if (charset_index >= charset_len) return 0; // Retorna 0 se a letra em password não for encontrada em charset

        // Retorna 1 se foi possivel incrementar a letra, caso contrário reseta password[i] roda o loop de novo
        if (charset_index < charset_len - 1) {
            password[i] = charset[charset_index + 1];
            return 1;
        }
        else {
            password[i] = charset[0];
        }
    }

    return 0; // Retrna 0 quando todas as opções ja forem testadas
}

/**
 * Compara duas senhas lexicograficamente
 * 
 * @return -1 se a < b, 0 se a == b, 1 se a > b
 */
int password_compare(const char *a, const char *b) {
    return strcmp(a, b);
}

/**
 * Verifica se o arquivo de resultado já existe
 * Usado para parada antecipada se outro worker já encontrou a senha
 */
int check_result_exists() {
    return access(RESULT_FILE, F_OK) == 0;
}

/**
 * Salva a senha encontrada no arquivo de resultado
 * Usa O_CREAT | O_EXCL para garantir escrita atômica (apenas um worker escreve)
 */
void save_result(int worker_id, const char *password) {
    // Abre o arquivo no modo:
    //          O_CREAT              | O_EXCL              | O_WRONLY  , 0644
    // uo seja: Criar se não existir | Falha se ja existir | Só escrita, só eu posso ler e escrever e os outros só podem ler
    int file = open(RESULT_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);

    if (file >= 0) {
        char buffer[256];
        int len = snprintf(buffer, sizeof(buffer), "%d:%s", worker_id, password); // Altera o buffer para a string desejada
        write(file, buffer, len);                                                 // e retona o tamanho exato

        close(file);

        printf("Worker: [%d] encontrou o resultado!\n", worker_id);
        printf("Resultado salvo: %s\n", password);
    }
}


/**
 * FIXME: Apagar print_string()
 * 
 * Usado para printar strings durante os testes
 */
void print_string(char string[], int string_size) {
    for (int i = 0; i < string_size; i++) putchar(string[i]);
    putchar('\n');
}

/**
 * FIXME: Apagar esse main()
 * 
 * Usado apenas para testar o increment_password() e o save_result()
 */
int main2() { // Main2
    // Teste do increment_password():
    char teste[] = "aaaaa";
    char set[]   = "abc";

    int teste_len = sizeof(teste) - 1; // -1 para remover o \0
    int set_len   = sizeof(set)   - 1; // -1 para remover o \0

    print_string(teste, teste_len);
    while(increment_password(teste, set, set_len, teste_len)) print_string(teste, teste_len);

    // Teste do save_result():
    int worker = 99999;
    char senha[] = "bom dia rapaziada :)";

    save_result(worker, senha);

    return 0;
}

/**
 * Função principal do worker
 */
int main(int argc, char *argv[]) {
    // Validar argumentos
    if (argc != 7) {
        fprintf(stderr, "Uso interno: %s <hash> <start> <end> <charset> <len> <id>\n", argv[0]);
        return 1;
    }
    
    // Parse dos argumentos
    const char *target_hash = argv[1];
    char *start_password = argv[2];
    const char *end_password = argv[3];
    const char *charset = argv[4];
    int password_len = atoi(argv[5]);
    int worker_id = atoi(argv[6]);
    int charset_len = strlen(charset);
    
    printf("[Worker %d] Iniciado: %s até %s\n", worker_id, start_password, end_password);
    
    // Buffer para a senha atual
    char current_password[11];
    strcpy(current_password, start_password);
    
    // Buffer para o hash calculado
    char computed_hash[33];
    
    // Contadores para estatísticas
    long long passwords_checked = 0;
    time_t start_time = time(NULL);
    time_t last_progress_time = start_time;
    
    // Loop principal de verificação
    while (1) {
        // TODO 3: Verificar periodicamente se outro worker já encontrou a senha
        // DICA: A cada PROGRESS_INTERVAL senhas, verificar se arquivo resultado existe
        
        // TODO 4: Calcular o hash MD5 da senha atual
        // IMPORTANTE: Use a biblioteca MD5 FORNECIDA - md5_string(senha, hash_buffer)
        
        // TODO 5: Comparar com o hash alvo
        // Se encontrou: salvar resultado e terminar
        
        // TODO 6: Incrementar para a próxima senha
        // DICA: Use a função increment_password implementada acima
        
        // TODO: Verificar se chegou ao fim do intervalo
        // Se sim: terminar loop
        
        passwords_checked++;
    }
    
    // Estatísticas finais
    time_t end_time = time(NULL);
    double total_time = difftime(end_time, start_time);
    
    printf("[Worker %d] Finalizado. Total: %lld senhas em %.2f segundos", 
           worker_id, passwords_checked, total_time);
    if (total_time > 0) {
        printf(" (%.0f senhas/s)", passwords_checked / total_time);
    }
    printf("\n");
    
    return 0;
}