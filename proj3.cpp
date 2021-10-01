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
#include <string.h>
#include <queue>

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
int MAXTHREAD;

int NBSendMsg(int iTo, struct msg &Msg){ // NB Sent Message
    //cout << sem_trywait(&pSems[iTo]) << " sem status\n";

    if(sem_trywait(&pSems[iTo]) == -1){ // PSEM at 0, can't use w/o blocking
        return -1;
    } else { // Sem Acquired
        mailboxes[iTo] = Msg;
        sem_post(&cSems[iTo]);
        return 0;
    }

}

void SendMsg(int iTo, struct msg &Msg) { // Send Message w/ blocking
    sem_wait(&pSems[iTo]);

    mailboxes[iTo] = Msg;

    sem_post(&cSems[iTo]);
}

void RecvMsg(int iRecv, struct msg &Msg) { // Receive Message
    sem_wait(&cSems[iRecv]);

    Msg = mailboxes[iRecv];

    sem_post(&pSems[iRecv]);
}

void* adder(void* arg){ // Adder thread
    int id = (long) arg;
    int sum = 0;
    int numOps = 0;
    int executionTime = 0;

    while(true){
        msg newMsg;
        RecvMsg(id, newMsg); // Receive a message, blocks if no message available
        int startTime = time(NULL); // Upon unblocking, start counting execution time
        if(newMsg.value >= 0) { // Normal Add
            sum += newMsg.value; // Add
            numOps++;
            sleep(1);
            executionTime += time(NULL) - startTime; // Calculate execution time for this one operation
        } else { // Terminate

            // Send message w/ info
            msg newMsg;
            newMsg.iFrom = id;
            newMsg.cnt = numOps;
            newMsg.value = sum;
            newMsg.tot = executionTime;

            SendMsg(0, newMsg);

            // Exit/Terminate thread
            pthread_exit(0);
        }
    }
}

void initMailboxes(){ // Init mailboxes & Sems
    for(int i = 0; i < MAXTHREAD+1; i++){ // thread 0 requires 2 sems as well
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

void killAll(){ // Destroy semaphores
    (void) sem_destroy(&cSems[0]);
    (void) sem_destroy(&pSems[0]);
    for(int i = 1; i < MAXTHREAD+1; i++){
        (void) sem_destroy(&cSems[i]);
        (void) sem_destroy(&pSems[i]);
    }
}

int main(int argc, char* argv[]) { // Start of Main
    MAXTHREAD = atoi(argv[1]);
    mailboxes = new msg[MAXTHREAD+1]; // Mailboxes
    threadIds = new pthread_t[MAXTHREAD+1]; // thread Ids
    pSems = new sem_t[MAXTHREAD+1]; // producer sems
    cSems = new sem_t[MAXTHREAD+1]; // consumer sems
    queue<msg> undeliveredRequests; // a queue of undelivered requests
    queue<int> undeliveredMailboxes; // a queue of the undelivered requests intended mailboxes (msg doesn't store intended destination)
    bool* threadTerminated = new bool[MAXTHREAD+1]; // stores which threads have been terminated
    int* queuedMessageCount = new int[MAXTHREAD+1]; // stores which threads still have messages queued for them & how many

    int finished = 0; // # of finished processes

    bool nb = false; // using nb?
    if(argc == 3 && strcmp(argv[2], "nb") == 0){
        nb = true;
    }

    initMailboxes();

    while(true){
        if(!nb) { // regular mode
            int value;
            int sendTo;
            cin >> value >> sendTo;

            if (cin.fail()) { // eof or unallowed input terminate all threads & program
                int expectedResponses = 0;
                for(int i = 1; i < MAXTHREAD+1; i++){ // send termination message to all threads
                    if(!threadTerminated[i]){
                        msg newMsg;
                        newMsg.value = -1;
                        newMsg.iFrom = 0;
                        SendMsg(i, newMsg);
                        expectedResponses++;
                    } else {
                        continue;
                    }
                }
                while(expectedResponses > 0){ // receive all termination messages
                    msg recMsg;
                    RecvMsg(0, recMsg);
                    cout << "The result from thread " << recMsg.iFrom << " is " << recMsg.value << " from " << recMsg.cnt
                         << " operations during " << recMsg.tot << " secs.\n";
                    cout.flush();
                    finished++;
                    expectedResponses--;
                }

                // destroy semaphores & quit program
                killAll();
                exit(0);
            }

            // send messages
            msg newMsg;
            newMsg.value = value;
            newMsg.iFrom = 0;
            SendMsg(sendTo, newMsg);

            // if sent a termination message, then expect a response w/ summary info from thr
            if (value < 0) {
                msg recMsg;
                RecvMsg(0, recMsg);
                cout << "The result from thread " << recMsg.iFrom << " is " << recMsg.value << " from " << recMsg.cnt
                     << " operations during " << recMsg.tot << " secs.\n";
                cout.flush();
                threadTerminated[sendTo] = true;
                finished++;
            }

            // if all threads finish naturally, then end the program
            if (finished == MAXTHREAD) {
                killAll();
                exit(0);
            }
        } else {
            int value;
            int sendTo;
            cin >> value >> sendTo;

            if (cin.fail()) { // terminate program when unexpected input
                int expectedResponses = 0;
                for(int i = 1; i < MAXTHREAD+1; i++){ // loop through & terminate all threads that aren't & that don't have messages queued
                    if(!threadTerminated[i] && queuedMessageCount[i] == 0){
                        msg newMsg;
                        newMsg.value = -1;
                        newMsg.iFrom = 0;
                        SendMsg(i, newMsg);
                        expectedResponses++;
                    } else {
                        continue;
                    }
                }
                while(expectedResponses > 0){ // receive summary messages from all terminating threads
                    msg recMsg;
                    RecvMsg(0, recMsg);
                    cout << "The result from thread " << recMsg.iFrom << " is " << recMsg.value << " from " << recMsg.cnt
                         << " operations during " << recMsg.tot << " secs.\n";
                    cout.flush();
                    finished++;
                    expectedResponses--;
                }

                //cout << undeliveredRequests.size() << "\n";
                while(!undeliveredRequests.empty()){ // stop when the queue is empty/ all messages have been sent

                    // Get first message & mailbox, remove it form the queues as well
                    msg cur = undeliveredRequests.front();
                    undeliveredRequests.pop();
                    int sendTo = undeliveredMailboxes.front();
                    undeliveredMailboxes.pop();

                    if(NBSendMsg(sendTo, cur) == -1){ // if it fails to send, then return to queue at the end
                        undeliveredRequests.push(cur);
                        undeliveredMailboxes.push(sendTo);

                    } else { // upon success mark down that threads queued messages
                        queuedMessageCount[sendTo]--;

                        if(queuedMessageCount[sendTo] == 0) { // if thread has run out of queued message terminate the thread
                            msg newMsg;
                            newMsg.value = -1;
                            newMsg.iFrom = 0;
                            while (NBSendMsg(sendTo, newMsg) == -1) {}
                            msg recMsg;
                            RecvMsg(0, recMsg);
                            cout << "The result from thread " << recMsg.iFrom << " is " << recMsg.value << " from " << recMsg.cnt
                                 << " operations during " << recMsg.tot << " secs.\n";
                            cout.flush();
                            threadTerminated[sendTo] = true;
                            finished++;
                        }
                    }
                }

                // end program
                killAll();
                exit(0);
            }
            // send message
            msg newMsg;
            newMsg.value = value;
            newMsg.iFrom = 0;
            if(NBSendMsg(sendTo, newMsg) == -1){ // failed to send message, so add it to the queue for later
                undeliveredRequests.push(newMsg);
                undeliveredMailboxes.push(sendTo);
                queuedMessageCount[sendTo]++;
                cout << "failed to send msg to " << sendTo << " putting it in queue\n";
                cout.flush();

            } else if(value < 0){ // successfully sent a termination message, termiate thread & expect summary response
                msg recMsg;
                RecvMsg(0, recMsg);
                cout << "The result from thread " << recMsg.iFrom << " is " << recMsg.value << " from " << recMsg.cnt
                     << " operations during " << recMsg.tot << " secs.\n";
                cout.flush();
                threadTerminated[sendTo] = true;
                finished++;
            }

            // if you finish naturally then end the program
            if (finished == MAXTHREAD) {
                killAll();
                exit(0);
            }

        }

    }
}


