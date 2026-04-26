#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <windows.h>
typedef HANDLE thread_t;
typedef CRITICAL_SECTION mutex_t;
#define THREAD_RETURN DWORD WINAPI
#define THREAD_EXIT() return 0
static void mutex_init(mutex_t *m) { InitializeCriticalSection(m); }
static void mutex_lock(mutex_t *m) { EnterCriticalSection(m); }
static void mutex_unlock(mutex_t *m) { LeaveCriticalSection(m); }
static void mutex_destroy(mutex_t *m) { DeleteCriticalSection(m); }
static void sleep_ms(unsigned int ms) { Sleep(ms); }
static int thread_create(thread_t *thread, THREAD_RETURN (*start_routine)(void *), void *arg) {
    *thread = CreateThread(NULL, 0, start_routine, arg, 0, NULL);
    return (*thread == NULL) ? -1 : 0;
}
static int thread_join(thread_t thread) {
    DWORD result = WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return (result == WAIT_OBJECT_0) ? 0 : -1;
}

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3
#define ITERATIONS_PER_CUSTOMER 8

int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

mutex_t mutex;


void print_vector(const char *label, int v[]) {
    printf("%s [", label);
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        printf("%d", v[i]);
        if (i < NUMBER_OF_RESOURCES - 1) printf(" ");
    }
    printf("]\n");
}

void print_state() {
    printf("\n================ ESTADO ATUAL ================\n");

    print_vector("Available =", available);

    printf("\nMaximum:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("C%d: [", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d", maximum[i][j]);
            if (j < NUMBER_OF_RESOURCES - 1) printf(" ");
        }
        printf("]\n");
    }

    printf("\nAllocation:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("C%d: [", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d", allocation[i][j]);
            if (j < NUMBER_OF_RESOURCES - 1) printf(" ");
        }
        printf("]\n");
    }

    printf("\nNeed:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("C%d: [", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d", need[i][j]);
            if (j < NUMBER_OF_RESOURCES - 1) printf(" ");
        }
        printf("]\n");
    }

    printf("==============================================\n\n");
}

/* ---------- Algoritmo de segurança ---------- */

int is_safe() {
    int work[NUMBER_OF_RESOURCES];
    int finish[NUMBER_OF_CUSTOMERS];

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        work[j] = available[j];
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        finish[i] = 0;
    }

    int found;
    do {
        found = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int can_finish = 1;
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        can_finish = 0;
                        break;
                    }
                }

                if (can_finish) {
                    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = 1;
                    found = 1;
                }
            }
        }
    } while (found);

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        if (!finish[i]) {
            return 0; // inseguro
        }
    }

    return 1; // seguro
}

/* ---------- Solicitação e liberação ---------- */

int request_resources(int customer_num, int request[]) {
    mutex_lock(&mutex);

    printf("Cliente %d solicitou: [", customer_num);
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        printf("%d", request[j]);
        if (j < NUMBER_OF_RESOURCES - 1) printf(" ");
    }
    printf("]\n");

    /* Passo 1: request <= need */
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        if (request[j] > need[customer_num][j]) {
            printf("-> Pedido NEGADO: excede necessidade do cliente.\n\n");
            mutex_unlock(&mutex);
            return -1;
        }
    }

    /* Passo 2: request <= available */
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        if (request[j] > available[j]) {
            printf("-> Pedido NEGADO: recursos indisponíveis no momento.\n\n");
            mutex_unlock(&mutex);
            return -1;
        }
    }

    /* Passo 3: simular alocação */
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        available[j] -= request[j];
        allocation[customer_num][j] += request[j];
        need[customer_num][j] -= request[j];
    }

    /* Verificar segurança */
    if (is_safe()) {
        printf("-> Pedido APROVADO: estado seguro.\n");
        print_state();
        mutex_unlock(&mutex);
        return 0;
    } else {
        /* rollback */
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            available[j] += request[j];
            allocation[customer_num][j] -= request[j];
            need[customer_num][j] += request[j];
        }
        printf("-> Pedido NEGADO: deixaria o sistema em estado inseguro.\n\n");
        mutex_unlock(&mutex);
        return -1;
    }
}

int release_resources(int customer_num, int release[]) {
    mutex_lock(&mutex);

    printf("Cliente %d liberou: [", customer_num);
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        printf("%d", release[j]);
        if (j < NUMBER_OF_RESOURCES - 1) printf(" ");
    }
    printf("]\n");

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        if (release[j] > allocation[customer_num][j]) {
            mutex_unlock(&mutex);
            printf("-> Liberação inválida: cliente tentou liberar mais do que possui.\n\n");
            return -1;
        }
    }

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        available[j] += release[j];
        allocation[customer_num][j] -= release[j];
        need[customer_num][j] += release[j];
    }

    print_state();
    mutex_unlock(&mutex);
    return 0;
}

/* ---------- Thread do cliente ---------- */

THREAD_RETURN customer_thread(void *param) {
    int customer_num = *((int *)param);

    for (int iter = 0; iter < ITERATIONS_PER_CUSTOMER; iter++) {
        int request[NUMBER_OF_RESOURCES];
        int has_request = 0;

        /* Gera pedido aleatório limitado por need */
        mutex_lock(&mutex);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            if (need[customer_num][j] > 0) {
                request[j] = rand() % (need[customer_num][j] + 1);
            } else {
                request[j] = 0;
            }
            if (request[j] > 0) has_request = 1;
        }
        mutex_unlock(&mutex);

        if (has_request) {
            request_resources(customer_num, request);
        }

        sleep_ms(100 + rand() % 400);

        int release[NUMBER_OF_RESOURCES];
        int has_release = 0;

        /* Gera liberação aleatória limitada por allocation */
        mutex_lock(&mutex);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            if (allocation[customer_num][j] > 0) {
                release[j] = rand() % (allocation[customer_num][j] + 1);
            } else {
                release[j] = 0;
            }
            if (release[j] > 0) has_release = 1;
        }
        mutex_unlock(&mutex);

        if (has_release) {
            release_resources(customer_num, release);
        }

        sleep_ms(100 + rand() % 400);
    }

    /* Ao final, libera tudo que ainda tiver */
    int final_release[NUMBER_OF_RESOURCES];

    mutex_lock(&mutex);
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        final_release[j] = allocation[customer_num][j];
    }
    mutex_unlock(&mutex);

    int has_final = 0;
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        if (final_release[j] > 0) {
            has_final = 1;
            break;
        }
    }

    if (has_final) {
        release_resources(customer_num, final_release);
    }

    printf("Cliente %d terminou.\n", customer_num);
    THREAD_EXIT();
}

/* ---------- Inicialização ---------- */

void initialize_matrices() {
    /* máximo aleatório, limitado pelos recursos totais disponíveis no sistema */
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != NUMBER_OF_RESOURCES + 1) {
        fprintf(stderr, "Uso: %s <R1> <R2> <R3>\n", argv[0]);
        return 1;
    }

    srand((unsigned int)time(NULL));

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
        if (available[i] < 0) {
            fprintf(stderr, "Erro: recursos devem ser >= 0.\n");
            return 1;
        }
    }

    mutex_init(&mutex);

    initialize_matrices();

    printf("Estado inicial:\n");
    print_state();

    thread_t customers[NUMBER_OF_CUSTOMERS];
    int ids[NUMBER_OF_CUSTOMERS];

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        ids[i] = i;
        if (thread_create(&customers[i], customer_thread, &ids[i]) != 0) {
            perror("Erro ao criar thread");
            return 1;
        }
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        thread_join(customers[i]);
    }

    printf("Execução finalizada.\n");
    print_state();

    mutex_destroy(&mutex);
    return 0;
}