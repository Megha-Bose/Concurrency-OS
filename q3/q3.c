#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

typedef struct performer
{
    char name[30], instch;
    int tarr, status, tPer;
    // status
    // 0: Not yet arrived, 
    // 1: Waiting to perform, 
    // 2: Performing solo,
    // 3: Performing with singer, 
    // 4: Performing with musician,
    // 5: Waiting for t-shirt, 
    // 6: T-shirt being collected,
    // 7: Exited
    pthread_t perfThreadId, acouThreadId, electThreadId, withsoloThreadId;
} performer;

performer perf[1001];

int k, a, e, c, t1, t2, t;
sem_t freeAcoustic, freeElectric, freeWithSolo, freeCoor;

void* check_acoustic(void *args);
void* check_electric(void *args);
void* check_solo(void *args);
void* get_tshirt(void *args);


void* srujana(void *args)
{
	performer *arg = (performer*) args;
    sleep(arg->tarr);
    printf("\n\033[1;32m%s %c arrived\n", arg->name, arg->instch);

    clock_t t = clock();
    arg->status = 1;
    while(arg->status < 2)
    {
        clock_t time_taken = clock() - t;
        double times = ((double)time_taken)/CLOCKS_PER_SEC;
		if(times > ((double)t))
		{
			printf("\n\033[1;31m%s %c left because of impatience\n", arg->name, arg->instch);
			break;
		}
        if(arg->instch == 'p' || arg->instch == 'g')
        {
            pthread_create(&(arg->acouThreadId), NULL, check_acoustic , arg);
            pthread_create(&(arg->electThreadId), NULL, check_electric , arg);
        }
        else if(arg->instch == 'v')
        {
            pthread_create(&(arg->acouThreadId), NULL, check_acoustic , arg);
        }
        else if(arg->instch == 'b')
        {
            pthread_create(&(arg->electThreadId), NULL, check_electric , arg);
        }
        if(arg->instch == 's')
        {
            pthread_create(&(arg->acouThreadId), NULL, check_acoustic , arg);
            pthread_create(&(arg->electThreadId), NULL, check_electric , arg);
            pthread_create(&(arg->withsoloThreadId), NULL, check_solo , arg);
        }
    }
    
    if(arg->instch == 'p' || arg->instch == 'g')
    {
        pthread_join(arg->acouThreadId, NULL);
        pthread_join(arg->electThreadId, NULL);
    }
    else if(arg->instch == 'v')
    {
        pthread_join(arg->acouThreadId, NULL);
    }
    else if(arg->instch == 'b')
    {
        pthread_join(arg->electThreadId, NULL);
    }
    if(arg->instch == 's')
    {
        pthread_join(arg->acouThreadId, NULL);
        pthread_join(arg->electThreadId, NULL);
        pthread_join(arg->withsoloThreadId, NULL);
    }

    return NULL;
}


void* check_acoustic(void *args)
{
    performer *arg = (performer*) args;
    sem_wait(&freeAcoustic);
    if(arg->status == 1)
    {
        arg->status = 2;
        arg->tPer = (rand() % (t2-t1)) + t1;
        printf("\n\033[1;33m%s performing %c at acoustic stage for %d sec\n", arg->name, arg->instch, arg->tPer);
    
        //critical section 
        sleep(arg->tPer);
        if(arg->status == 3)
        {
            sleep(2);
            sem_post(&freeWithSolo);
        }
        
        printf("\n\033[1;33m%s performance at acoustic stage ended\n", arg->name);
    }
    else
    {
        sem_post(&freeAcoustic); 
        return NULL;
    } 
    sem_post(&freeAcoustic);

    if(arg->instch != 's')
        pthread_create(&(arg->perfThreadId), NULL, get_tshirt , arg);
    return NULL;
}


void* check_electric(void *args)
{
    performer *arg = (performer*) args;
    sem_wait(&freeElectric);
    if(arg->status == 1)
    {
        arg->status = 2;
        arg->tPer = (rand() % (t2-t1)) + t1;
        printf("\n\033[1;34m%s performing %c at electric stage for %d sec\n", arg->name, arg->instch, arg->tPer);
    
        //critical section 
        sleep(arg->tPer);
        if(arg->status == 3)
        {
            sleep(2);
            sem_post(&freeWithSolo);
        }
        
        printf("\n\033[1;34m%s performance at electric stage ended\n", arg->name);
    }
    else
    {
        sem_post(&freeElectric);
        return NULL;
    }
    sem_post(&freeElectric);
    if(arg->instch != 's')
        pthread_create(&(arg->perfThreadId), NULL, get_tshirt , arg);
    return NULL;
}


void* check_solo(void *args)
{
    performer *arg = (performer*) args;
    for(int i=0; i<a+e; i++)
    {
        if(perf[i].instch != 's' && perf[i].status == 2)
        {
            sem_wait(&freeWithSolo);
            if(arg->status == 1)
            {
                arg->status = 4;
                perf[i].status = 3;
                printf("\n\033[1;37m%s joined %s's performance, performance extended by 2 secs\n", arg->name, perf[i].name);
                break;
            }
            else
            {
                sem_post(&freeWithSolo);
                return NULL;
            }
        }
    }
    return NULL;
}


void* get_tshirt(void *args)
{
    performer *arg = (performer*) args;
    while(arg->status != 7)
    {
        arg->status = 5;
        sem_wait(&freeCoor);
        printf("\n\033[1;35m%s collecting t-shirt\n", arg->name);
        arg->status = 6;
        sleep(2);
        arg->status = 7;
        sem_post(&freeCoor);
    }
    return NULL;
}


int main()
{
    scanf("%d %d %d %d %d %d %d", &k, &a, &e, &c, &t1, &t2, &t);
    for(int i=0; i<k; i++)
    {
        char inst;
        int arr;
        scanf("%s %c %d", perf[i].name, &inst, &arr);
        perf[i].instch = inst;
        perf[i].tarr = arr;
        perf[i].status = 0;
    }
    int ind = 0;

    sem_init(&freeAcoustic, 0, a);
    sem_init(&freeElectric, 0, e);
    sem_init(&freeWithSolo, 0, a+e);
    sem_init(&freeCoor, 0, c);

    for(int i = 0; i < k; i++)
    {
		pthread_create(&(perf[i].perfThreadId), NULL, srujana , &perf[i]);
    }
    for(int i = 0; i < k; i++)
    {
		pthread_join(perf[i].perfThreadId, NULL);
    }

    sem_destroy(&freeAcoustic);
    sem_destroy(&freeElectric);
    sem_destroy(&freeWithSolo);
    sem_destroy(&freeCoor);
    printf("\n\033[1;31mFinished\n");
}
