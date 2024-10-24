#include "receiver.h"

sem_t *SENDER_SEM;
sem_t *RECEIVER_SEM;

void receive(message_t* message, mailbox_t* mailbox) {
    if (mailbox->flag == 1) {
        
        if (msgrcv(mailbox->storage.msqid, message, sizeof(message_t), 0, 0) == -1) {
            perror("msgsnd failed");
        }
        
    } else if (mailbox->flag == 2) {
        strcpy(message->data, mailbox->storage.shm_addr);
        message->size = strlen(mailbox->storage.shm_addr);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <1 for msg passing, 2 for shared memory>\n", argv[0]);
        exit(1);
    }

    mailbox_t mailbox;
    mailbox.flag = atoi(argv[1]);
    
    if (mailbox.flag == 1) {
        printf("Message Passing\n");
        key_t key = 1234;
        mailbox.storage.msqid = msgget(key, 0666 | IPC_CREAT);
    } else if (mailbox.flag == 2) {
        printf("Shared Passing\n");
        int shmid = shmget(IPC_PRIVATE, 1024, 0666 | IPC_CREAT);
        mailbox.storage.shm_addr = (char*)shmat(shmid, NULL, 0);
    }

    message_t message;
    SENDER_SEM = sem_open("/sender", 0);
    RECEIVER_SEM = sem_open("/receiver", 0);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        sem_wait(RECEIVER_SEM);
        receive(&message, &mailbox);  // 接收訊息並通知發送者
        sem_post(SENDER_SEM);
        if (strcmp(message.data, "exit") == 0) {
            break;
        }
        printf("Received: %s", message.data);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("\nTotal time taken in receiving receiving msg: %f seconds\n", time_spent);
    sem_post(SENDER_SEM);
    sem_close(SENDER_SEM);
    sem_close(RECEIVER_SEM);
    sem_unlink("/receiver");
    return 0;
}

