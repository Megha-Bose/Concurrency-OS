#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

int min(int a, int b)
{
    return a < b ? a : b;
}

int n, m, o;
int leftstud;

// struct of arguments for using threads

typedef struct student
{
    int id, phase, status, zone;
    // status = 0 -> waiting
    // status = 1 -> done
    pthread_t studThreadId;
} student;

typedef struct vac_zone
{
    int id, vacleft, com, busy, fill, over;
    pthread_mutex_t vacz_mutex;
    pthread_t vaczThreadId;
} vac_zone;

typedef struct company
{
    int id, start, vacleft, distbatch, p;
    float prob;
    pthread_mutex_t comp_mutex;
    pthread_t compThreadId;
} company;


student stud[1001];
vac_zone vacz[1001];
company comp[1001];


void* stud_func(void *args)
{
	student *arg = (student*) args;
    
    // loop while not done
    while(arg->status==0 && leftstud>0)
    {
        printf("\nStudent %d has arrived for his/her round %d of Vaccination\n", arg->id, arg->phase);
        int zone_assgn = 0;
        
        // loop while zone not assigned
        while(!zone_assgn && leftstud>0)
        {
            // check all m zones for availability
            for(int i = 0; i < m; i++)
            {
                // checking inside zone lock so that other student threads don't interfere
                pthread_mutex_lock(&(vacz[i].vacz_mutex));
                if(vacz[i].vacleft > 0 && vacz[i].busy == 0)
                {
                    vacz[i].vacleft--;                      // reducing vaccines left
                    vacz[i].fill++;                         // increasing fill amt
                    comp[vacz[i].com - 1].vacleft--;        // decreasing vaccine count of corresponding company as well
                    zone_assgn = 1;                         // zone assigned
                    arg->zone = i + 1;                      // store zone id
                    pthread_mutex_unlock(&(vacz[i].vacz_mutex));
                    break;
                }
                pthread_mutex_unlock(&(vacz[i].vacz_mutex));// unlock after getting a zone
            }
        }


        printf("\nStudent %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n",arg->id, arg->zone);
        printf("\nStudent %d on Vaccination Zone %d has been vaccinated which has success probability %.2f\n",arg->id, arg->zone, comp[vacz[arg->zone - 1].com - 1].prob / 100);
        vacz[arg->zone - 1].over++;

        printf("\nStudent %d is having antibody test\n",arg->id);
        // antibody test results in accordance with the success probability 
        int r = rand() % 100 + 1; 
        int success = 0;
        if (r <= comp[vacz[arg->zone - 1].com - 1].prob) 
            success = 1; 
        if(success)
        {
            printf("\nYAY! Student %d has tested positive for antibodies.\n", arg->id);
            leftstud--;                                     // decreasing students left to be vaccinated
            arg->status = 1;                                // updating status as done
        }
        else
        {
            printf("\nStudent %d has tested negative for antibodies.\n", arg->id);
            if(arg->phase == 3)
            {
                printf("\nALAS! Student %d has been sent back home.\n", arg->id);
                arg->status = 1;                            // phase 3 over
                leftstud--;                                 // decreasing students left to be vaccinated
            }
            else
                arg->phase++;                               // re-entering the thread as antibodies weren't formed
        }
    }
    return NULL;
}


void *vacz_func(void *args)
{
    vac_zone *arg = (vac_zone*) args;
    while(leftstud > 0)
    {
        printf("\nVaccine zone %d is waiting for vaccine batch to be delivered to them\n",arg->id);
        int batch_assgn = 0;
        
        // checking all companies to get vaccine batch
        while(!batch_assgn && leftstud>0)
        {
            for(int i = 0; i < n; i++)
            {
                // checking availability inside company lock so that other zones don't interfere
                // while taking batch from company
                pthread_mutex_lock(&(comp[i].comp_mutex));
                if(comp[i].distbatch > 0)
                {
                    comp[i].distbatch--;
                    batch_assgn = 1; // batch assigned
                    arg->com = i + 1; // store id of company
                    printf("\nPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %.2f\n", comp[i].id, arg->id, comp[i].prob / 100); 
                    sleep(0.5);
                    printf("\nPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n", comp[i].id, arg->id);
                    arg->vacleft = comp[i].p; // received one batch of p vaccines
                    pthread_mutex_unlock(&(comp[i].comp_mutex));
                    break;
                }
                pthread_mutex_unlock(&(comp[i].comp_mutex));
            }
        }


        arg->busy = 0;
        while(arg->vacleft>0 && leftstud>0)
        {
            //pthread_mutex_lock(&(arg->vacz_mutex));
            //printf("%d %d\n",arg->vacleft,leftstud);
            // creating slots in a zone
            if(arg->over == arg->fill)
            {
                arg->busy = 0;
            }
            int num = min(min(8, arg->vacleft), leftstud);
            int vnum = rand() % num + 1;
            if(arg->busy == 0)
            {
                arg->over = 0; // when not busy, initialise number of students over at a time with 0
            }
            // fill till all slots are full
            while(leftstud>0 && arg->busy==0)
            {
                if(arg->fill == vnum)
                {
                    // when full, mark zone as busy
                    arg->busy = 1;
                    printf("\nVaccination Zone %d is ready to vaccinate with %d slot(s)\n​", arg->id, vnum);
                    printf("\nVaccination Zone %d entering Vaccination Phase\n", arg->id);
                    break;
                }
            }
            // when all students who were assigned slots are done with vaccination, free the zone
            if(arg->over == arg->fill)
            {
                arg->busy = 0;
            }
            // if no vaccine left, get them from company
            if(arg->vacleft <= 0)
            {
                printf("\nVaccination Zone %d has run out of vaccines\n", arg->id);
            }
            //pthread_mutex_unlock(&(arg->vacz_mutex));
        }
    }
    return NULL;
}


void *comp_func(void *args)
{
    company *arg = (company*) args;
    while(leftstud>0)
    {
        if(arg->vacleft == 0)
        {
            // print message before starting production
            if(arg->start == 0)
                arg->start = 1;
            else
                printf("\nAll the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n", arg->id);

            //pthread_mutex_lock(&(arg->comp_mutex));
            
            // generate batchnum number of vaccine batches
            int batchnum = rand() % 5 + 1;
            arg->p = rand() % 11 + 10;
            arg->distbatch = batchnum;
            arg->vacleft = arg->p * batchnum;
            int w = rand() % 4 + 2;
            printf("\nPharmaceutical Company %d is preparing %d batch(es) of vaccines which have success probability %.2f \n​", arg->id, batchnum, arg->prob / 100);
            sleep(w);
            // loop until all batches are distributed
            while(leftstud>0)
            {
                // printf("%d %d\n",arg->id,arg->distbatch);
                if(arg->distbatch <= 0 || leftstud <= 0)
                {
                    if(arg->distbatch <= 0)
                        printf("\nAll the vaccines prepared by Pharmaceutical Company %d are distributed.\n", arg->id);
                    break;
                }
            }
            // pthread_mutex_unlock(&(arg->comp_mutex));
        }
    }
    return NULL;
}


int main()
{
	scanf("%d %d %d", &n, &m, &o);
    leftstud = o;
    for(int i = 0; i < n; i++)
    {
        float x;
        scanf("%f", &x);
        comp[i].prob = 100 * x;
    }
    // initialisation
    for(int i = 0; i < n; i++)
    {
        comp[i].id = i + 1;
        comp[i].start = 0;
        comp[i].vacleft = 0;
        comp[i].distbatch = 0;
    }
    for(int i = 0; i < m; i++)
    {
        vacz[i].id = i + 1;
        vacz[i].vacleft = 0;
        vacz[i].com = 0;
        vacz[i].busy = 0;
        vacz[i].fill = 0;
        vacz[i].over = 0;
    }
    for(int i = 0; i < o; i++)
    {
        stud[i].id = i + 1;
        stud[i].phase = 1;
        stud[i].status = 0;
        stud[i].zone = 0;
    }

    // mutex initialisation
    for(int i=0; i < n; i++)
        pthread_mutex_init(&(comp[i].comp_mutex), NULL);    
    for(int i=0; i < m; i++)
        pthread_mutex_init(&(vacz[i].vacz_mutex), NULL);

    // initiate simulation
    printf("\nSimulation Initiated\n\n");
    // thread creation
    for(int i = 0; i < n; i++)
		pthread_create(&(comp[i].compThreadId), NULL, comp_func , &comp[i]);
    
    for(int i = 0; i < m; i++)
		pthread_create(&(vacz[i].vaczThreadId), NULL, vacz_func, &vacz[i]);
    
    for(int i = 0; i < o; i++)
		pthread_create(&(stud[i].studThreadId), NULL, stud_func , &stud[i]);
    // all student threads terminate
    for(int i=0; i < o; i++)
		pthread_join(stud[i].studThreadId, 0);  

    // destroy mutexes
    for(int i=0; i< n; i++)
		pthread_mutex_destroy(&(comp[i].comp_mutex));
    
    for(int i=0; i < m; i++)
		pthread_mutex_destroy(&(vacz[i].vacz_mutex));

    printf("\nSimulation over.\n");
	return 0;
}
