#include "sender.h"

sem_t *SENDER_SEM;
sem_t *RECEIVER_SEM;

void send(message_t message, mailbox_t* mailbox_ptr){
    if (mailbox_ptr->flag == 1) {
        if (msgsnd(mailbox_ptr->storage.msqid, &message, sizeof(message_t), 0) == -1) {
            perror("msgsnd failed");
        }

    } else if (mailbox_ptr->flag == 2) {
        strcpy(mailbox_ptr->storage.shm_addr, message.data);
    }
}

int main(int argc, char* argv[]){
    sem_unlink("/sender");
    sem_unlink("/receiver");
    if (argc != 3) {
        printf("Usage: %s <1 for msg passing, 2 for shared memory> <input_file>\n", argv[0]);
        exit(1);
    }

    mailbox_t mailbox;
    mailbox.flag = atoi(argv[1]);
    char* input_file = argv[2];
    key_t key = 1234;
    // Initialize mailbox based on mechanism
    if (mailbox.flag == 1) {
        // Message Passing: create or get message queue
        printf("Message Passing\n");
        
        mailbox.storage.msqid = msgget(key, 0666 | IPC_CREAT);
    } else if (mailbox.flag == 2) {
        // Shared Memory: create or get shared memory
        printf("Shared Memory\n");
        int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
        mailbox.storage.shm_addr = (char*)shmat(shmid, NULL, 0);
    }

    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) {
        perror("Failed to open input file");
        exit(1);
    }

    message_t message;
    SENDER_SEM = sem_open("/sender", O_CREAT, 0666, 1);
    RECEIVER_SEM = sem_open("/receiver", O_CREAT, 0666, 0);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);  // Start timing // ssh name@127.0.0.1 -p 2222
    while (fgets(message.data, 1024, fp) != NULL) {
        message.size = strlen(message.data);
        sem_wait(SENDER_SEM);
        send(message, &mailbox);  // Call send function
        sem_post(RECEIVER_SEM);
        printf("Sending message: %s", message.data);
    }
    sem_wait(SENDER_SEM);
    strcpy(message.data, "exit");
    message.size = strlen(message.data);
    send(message, &mailbox);
    sem_post(RECEIVER_SEM);
    printf("\nEnd of input file! exit!\n");
  
    clock_gettime(CLOCK_MONOTONIC, &end);  // End timing
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Total time taken in sending msg: %f\n", time_spent);

    // Cleanup resources
    fclose(fp);
    if (mailbox.flag == 1) {
        msgctl(mailbox.storage.msqid, IPC_RMID, NULL);  // Remove message queue
    } else if (mailbox.flag == 2) {
        shmdt(mailbox.storage.shm_addr);  // Detach shared memory
        // You may also need to remove the shared memory segment and semaphore
    }
    sem_wait(SENDER_SEM);
    sem_close(SENDER_SEM);
    sem_close(RECEIVER_SEM);
    sem_unlink("/sender");
    return 0;
}


