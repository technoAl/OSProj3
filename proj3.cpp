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
sem_t *sems;

/* g++ -o pcthreads pcthreads.C -lpthread */
#define REQUEST 1
#define REPLY 2
int MAXTHREAD;

void sendMsg(int iTo, struct msg &Msg){
    if(mailboxes[iTo].cnt == 0){  // New message so block
        sem_wait(&sems[iTo]);
    } else { // Old Message so replace
        cout << iTo;
        Msg.cnt = 0;
        mailboxes[iTo] = Msg;
        sem_post(&sems[iTo]);
    }
}

void recvMsg(int iRecv, struct msg &Msg){
    if(mailboxes[iRecv].cnt != 0){ // Old message so empty mailbox
        sem_wait(&sems[iRecv]);
    }else { // New message so receive and mark as old
        Msg.value = mailboxes[iRecv].value;
        mailboxes[iRecv] = Msg;
        mailboxes[iRecv].cnt = 1;
        //sem_post(&sems[iRecv]);
    }
}

void* adder(void* arg){
    int id = (long) arg;
    int sum = 0;
    int numOps = 0;
//    cout << "init";
//    cout.flush();
    sem_wait(&sems[id]); // wait till signalled
    while(true){
        cout << "posted";
        cout.flush();
        msg newMsg;
        recvMsg(id, newMsg);
        cout << "received";
        cout.flush();
        cout.flush();
        if(newMsg.value >= 0) {
            sum += newMsg.value;
            numOps++;
            cout << "sum: " << sum;
            sleep(1);
        } else {
            cout << "The result from thread " << id << " is " << sum << " from " << numOps << " operations during " << time(NULL) << " secs.";
            exit(0);
        }
    }
}

void initMailboxes(){
    for(int i = 1; i < MAXTHREAD+1; i++){
        mailboxes[i].cnt = 1;
    }

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

void killAll(){
    for(int i = 1; i < MAXTHREAD+1; i++){
        (void) pthread_join(threadIds[i],NULL);
        (void) sem_destroy(&sems[i]);
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
    }
    return 0;
}


