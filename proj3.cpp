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

struct msg {
    int iFrom; /* who sent the message (0 .. number-of-threads) */
    int value; /* its value */
    int cnt; /* count of operations (not needed by all msgs) */
    int tot; /* total time (not needed by all msgs) */
};

msg *mailboxes;
pthread_t *threadIds;
sem_t *sems;

/* g++ -o pcthreads pcthreads.C -lpthread */
#define REQUEST 1
#define REPLY 2
int MAXTHREAD;


void initMailboxes(){
    for(int i = 1; i < MAXTHREAD+1; i++){
        if (sem_init(&sems[i], 0, 0) < 0) {
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

void* adder(void* arg){
    int id = (int) arg;
    int sum = 0;
    int numOps = 0;
    sem_wait(&sems[id]); // wait till signalled
    while(true){
        msg newMsg;
        recvMsg(id, newMsg);
        if(newMsg.value >= 0) {
            sum += newMsg.value;
            numOps++;
            sleep(1);
        } else {
            cout << "The result from thread " << id << " is " << sum << " from " << numOps << " operations during " << time() << " secs.";
            exit(0);
        }
    }
}

void sendMsg(int iTo, struct msg &Msg){
    if(mailboxes[iTo] != NULL){
        sem_wait(sems[iTo]);
    } else {
        mailboxes[iTo] = Msg;
        sem_post(sems[iTo]);
    }
}

void recvMsg(int iRecv, struct msg &Msg){
    if(mailboxes[iRecv] == NULL){
        sem_wait(sems[iRecv]);
    }else {
        Msg.value = mailboxes[iRecv];
        mailboxes[iRecv] = NULL;
        sem_post(sems[iRecv]);
    }
}

int main(int argc, char* argv[]) {
    MAXTHREAD = atoi(argv[1]);
    mailboxes = new msg[MAXTHREAD+1];
    threadIds = new pthread_t[MAXTHREAD+1];
    sems = new sem_t[MAXTHREAD+1];

    initMailboxes();

    while(true){
        int value;
        int sendTo;
        cin << value << sendTo;
        if(cin.fail()){
            // End everything
            exit(0);
        }
        sendMsg(sendTo, value);
    }
    return 0;
}

