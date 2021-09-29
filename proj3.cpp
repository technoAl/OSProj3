//
// Created by alexander on 9/26/21.
//

#include <iostream>
using namespace std;
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct msg {
    int iFrom; /* who sent the message (0 .. number-of-threads) */
    int value; /* its value */
    int cnt; /* count of operations (not needed by all msgs) */
    int tot; /* total time (not needed by all msgs) */
};

msg *mailboxes;
pthread_t *threadIds;
sem_t *cSems;
sem_t *pSems;

/* g++ -o pcthreads pcthreads.C -lpthread */
#define REQUEST 1
#define REPLY 2
int MAXTHREAD;

void sendMsg(int iTo, struct msg &Msg) {
    sem_wait(&pSems[iTo]);

    mailboxes[iTo] = Msg;

    sem_post(&cSems[iTo]);
}

void recvMsg(int iRecv, struct msg &Msg) {
    sem_wait(&cSems[iRecv]);

    Msg.value = mailboxes[iRecv].value;
    mailboxes[iRecv] = Msg;

    sem_post(&pSems[iRecv]);
}

void* adder(void* arg){
    int id = (long) arg;
    int sum = 0;
    int numOps = 0;

    while(true){
        msg newMsg;
        recvMsg(id, newMsg);
        if(newMsg.value >= 0) {
            sum += newMsg.value;
            numOps++;
            cout << "sum: " << sum;
            sleep(1);
        } else {
            msg newMsg = malloc(sizeof msg);
            newMsg.iFrom = id;
            newMsg.cnt = numOps;
            newMsg.tot = sum;
            sendMsg(0, newMsg);
            exit(0);
        }
    }
}

void initMailboxes(){
    for(int i = 0; i < MAXTHREAD+1; i++){
        if (sem_init(&cSems[i], 0, 0) < 0) {
            perror("sem_init");
            exit(1);
        }
        if (sem_init(&pSems[i], 0, 1) < 0) {
            perror("sem_init");
            exit(1);
        }
    }

    for(int i = 1; i < MAXTHREAD + 1; i++){
        if (pthread_create(&threadIds[i], NULL, adder, (void *)i) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
}

void killAll(){
    (void) sem_destroy(&cSems[0]);
    (void) sem_destroy(&pSems[0]);
    for(int i = 1; i < MAXTHREAD+1; i++){
        (void) pthread_join(threadIds[i],NULL);
        (void) sem_destroy(&cSems[i]);
        (void) sem_destroy(&pSems[i]);
    }
}

int main(int argc, char* argv[]) {
    MAXTHREAD = atoi(argv[1]);
    mailboxes = new msg[MAXTHREAD+1];
    threadIds = new pthread_t[MAXTHREAD+1];
    pSems = new sem_t[MAXTHREAD+1];
    cSems = new sem_t[MAXTHREAD+1];

    initMailboxes();

    while(true){
        int value;
        int sendTo;
        cin >> value >> sendTo;
//        cout << value << " " << sendTo;
//        cout.flush();
        if(cin.fail()){
            killAll();
            exit(0);
        }
        msg newMsg;
        newMsg.value = value;
        newMsg.iFrom = 0;
        sendMsg(sendTo, newMsg);
        if(value < 0){
            recvMsg(0, newMsg);
            cout << "The result from thread " << newMsg.iFrom << " is " << newMsg.tot << " operations during " << time(NULL) << " secs.";
        }

    }
    return 0;
}


