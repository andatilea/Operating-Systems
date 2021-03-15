#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>

sem_t *sem5_1 = NULL;
sem_t *sem5_2 = NULL;
sem_t *sem2 = NULL;

sem_t *sem5_4 = NULL;
sem_t *sem6_3 = NULL;

pthread_t th5[4];
pthread_t th2[47];
pthread_t th6[5];

//Function for threads created by process 5
void *th5Func(void *arg)
{
    int index = *((int *)arg);
    if (index == 0)
    {
        //wait for thread 2 to start
        sem_wait(sem5_1);
        info(BEGIN, 5, 1);
        info(END, 5, 1);
        //thread 1 finished so thread 2 can finish
        sem_post(sem5_2);
        return 0;
    }
    if (index == 1)
    {
        info(BEGIN, 5, 2);
        //thread 2 started so thread 1 can begin
        sem_post(sem5_1);
        //wait for thread 1 to finish
        sem_wait(sem5_2);
        info(END, 5, 2);
        return 0;
    }
    if(index == 3)
    {
        //wait for thread 2 from process 6 to finish
        sem_wait(sem5_4);
        info(BEGIN, 5, 4);
        info(END, 5, 4);
        //thread 4 finished so thread 3 from process 6 can start
        sem_post(sem6_3);
        return 0;
    }
    info(BEGIN, 5, index + 1);
    info(END, 5, index + 1);
    return 0;
}

//Function for threads created by process 2
void *th2Func(void *arg)
{
    int index = *((int *)arg);
    //wait for a thread to finish if there are 4 threads in process
    sem_wait(sem2);
    info(BEGIN, 2, index + 1);
    info(END, 2, index + 1);
    sem_post(sem2);
    return 0;
}

//Function for threads created by process 6
void *th6Func(void *arg){
    int index = *((int *)arg);
    if(index == 2)
    {
        //thread 3 waits for thread 4 from process 5 to finish
        sem_wait(sem6_3);
    }
    info(BEGIN, 6, index + 1);
    info(END, 6, index + 1);
    if(index == 1)
    {
        //thread 2 from process 6 finished so thread 4 from process 5 can start 
        sem_post(sem5_4);
    }
    return 0;
}

int main()
{
    init();
    
    //We create those semaphores here so we can use them in any process
    sem_unlink("sem5_1");
    sem_unlink("sem5_2");
    sem_unlink("sem5_4");
    sem_unlink("sem6_3");
    sem5_1 = sem_open("sem5_1", O_CREAT, 0644, 0);
    sem5_2 = sem_open("sem5_2", O_CREAT, 0644, 0);
    sem5_4 = sem_open("sem5_4", O_CREAT, 0644, 0);
    sem6_3 = sem_open("sem6_3", O_CREAT, 0644, 0);
    
    info(BEGIN, 1, 0);
    pid_t pid[7];
    pid[2] = fork();
    if (pid[2] == 0)
    {
        info(BEGIN, 2, 0);
        for (int i = 3; i <= 6; i++)
        {
            pid[i] = fork();
            if (pid[i] == 0)
            {
                info(BEGIN, i, 0);
                if (i == 5)
                {
                    //Create threads for process 5
                    int thIndex[4];
                    for (int thi = 0; thi < 4; thi++)
                    {
                        thIndex[thi] = thi;
                        pthread_create(&th5[thi], NULL, th5Func, &thIndex[thi]);
                    }
                    for (int thi = 0; thi < 4; thi++)
                    {
                        pthread_join(th5[thi], NULL);
                    }
                }
                if(i == 6)
                {
                    //Create threads for process 6
                    int thIndex[5];
                    for (int thi = 0; thi < 5; thi++)
                    {
                        thIndex[thi] = thi;
                        pthread_create(&th6[thi], NULL, th6Func, &thIndex[thi]);
                    }
                    for (int thi = 0; thi < 5; thi++)
                    {
                        pthread_join(th6[thi], NULL);
                    }
                }

                info(END, i, 0);
                exit(0);
            }
        }
        //Semaphore which alows 4 threads at a time
        sem_unlink("sem2");
        sem2 = sem_open("sem2", O_CREAT, 0644, 4);
        //Create threads for process 2
        int thIndex[47];
        for (int thi = 0; thi < 47; thi++)
        {
            thIndex[thi] = thi;
            pthread_create(&th2[thi], NULL, th2Func, &thIndex[thi]);
        }
        for (int thi = 0; thi < 47; thi++)
        {
            pthread_join(th2[thi], NULL);
        }

        for (int i = 3; i <= 6; i++)
        {
            waitpid(pid[i], NULL, 0);
        }

        info(END, 2, 0);
        exit(0);
    }
    else
    {
        pid[7] = fork();
        if (pid[7] == 0)
        {
            info(BEGIN, 7, 0);
            info(END, 7, 0);
            exit(0);
        }
        waitpid(pid[2], NULL, 0);
        waitpid(pid[7], NULL, 0);

        info(END, 1, 0);
        exit(0);
        
    }

    return 0;
}
