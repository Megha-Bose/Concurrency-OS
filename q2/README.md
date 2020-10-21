# Q2: Code breakdown

## Overview
Risubrik wants to head back to college as soon as possible. He wants to suggest the college to get all the students vaccinated. There are `n` companies each producing coronavirus vaccines with a certain success probability. They supply vaccine batches to `m` vaccination zones. `o` students need to get vaccinated in total. Each student takes up the antibody test after vaccination and if tested positive for antibodies, can attend college else he/she re-enters the vaccination process. A student can go through maximum 3 vaccination phases and once those 3 phases are over, he/she is sent back home.

## Run the program
1. Run `gcc q2.c -lpthread`
2. Run `./a.out`

## Input
First line will have `n` pharmaceutical companies `m` vaccination zones, `o` students (0 <= n,m,o) Second Line will have `n` probabilities of the success rate of each vaccine from the `n` pharmaceutical companies.

## Variables
`n` - Number of pharmaceautical companies
`m` - Number of vaccination zones
`o` - Number of students 

`leftstud` - Number of students active in the vaccination process

```
typedef struct student
{
    int id, phase, status, zone; 
    pthread_t studThreadId;
} student;
student stud[1001];
```

`id` - student id
`phase` - vaccination phase he/she is in, can take values 0,1,2,3
`status` - 0 for not done and 1 after getting final verdict
`studThreadId` - Student threads


```
typedef struct vac_zone
{
    int id, vacleft, com, busy, fill, over;
    pthread_mutex_t vacz_mutex;
    pthread_t vaczThreadId;
} vac_zone;
vac_zone vacz[1001];
```

`id` - zone id
`vacleft` - vaccines left out of the vaccines in the batch last received 
`com` - company that has supplied the batch in use
`busy` - 1 when a vaccination phase is running, else 0
`fill` - number of slots filled till now
`over` - number of slots assigned till now for the phase
`vacz_mutex` - mutex for each zone 
`vaczThreadId` - Vaccination zone threads


```
typedef struct company
{
    int id, start, vacleft, distbatch, p;
    float prob;
    pthread_mutex_t comp_mutex;
    pthread_t compThreadId;
} company;
company comp[1001];
```

`id` - company id
`start` - bool for checking if it's the first time the company is making vaccines
`vacleft` - number of vaccines made by the company yet to be used
`distbatch` - number of batches yet to be distributed by the comapny at a time
`p` = storing number of batches made by the company at a time

## Functions

- `void* stud_func(void *args)` - student function
In this function first we assign a vaccination slot in a zone to a student by checking all zones and ensuring that the zone allotted is not busy currently and has vaccines available. After the slot is assigned, the student breaks out of the loop.

```
while(!zone_assgn && leftstud>0) {
    for(int i = 0; i < m; i++) {
        pthread_mutex_lock(&(vacz[i].vacz_mutex)); // locking so that other student threads don't interfere while one student is modifying the variables in the zone
        if(vacz[i].vacleft > 0 && vacz[i].busy == 0) {
            vacz[i].vacleft--;                     
            vacz[i].fill++;                      
            comp[vacz[i].com - 1].vacleft--;      
            zone_assgn = 1;                        
            arg->zone = i + 1;                    
            pthread_mutex_unlock(&(vacz[i].vacz_mutex));
            break;
        }
        pthread_mutex_unlock(&(vacz[i].vacz_mutex));
    }
}
```

Then the student is sent for antibody test whose results come according to the success probability of the vaccine given to him/her.
```
int r = rand() % 100 + 1; 
int success = 0;
if (r <= comp[vacz[arg->zone - 1].com - 1].prob) 
    success = 1;
```

If the result of the antibody test comes positive, then the student exits the process else he/she re-enters the process if phase 3 for the student is not over. If phase 3 is over, the student is sent back home.
```
if(success) {
    printf("\nYAY! Student %d has tested positive for antibodies.\n", arg->id);
    leftstud--;
    arg->status = 1;
}
else {
    printf("\nStudent %d has tested negative for antibodies.\n", arg->id);
    if(arg->phase == 3) {
        printf("\nALAS! Student %d has been sent back home.\n", arg->id);
        arg->status = 1; 
        leftstud--;                                
    }
    else arg->phase++;                               
}
```

- `void *vacz_func(void *args)`
When a vaccination zone runs out of vaccines, it searches for companies that have vaccine batches available and takes a batch from one of them. It locks the company it is checking so that other zones don't interfere with the comapny's variables in the meanwhile.
```
batch_assgn = 0;
while(!batch_assgn && leftstud>0) {
    for(int i = 0; i < n; i++) {
        pthread_mutex_lock(&(comp[i].comp_mutex));
        if(comp[i].distbatch > 0) {
            comp[i].distbatch--;
            batch_assgn = 1;
            arg->com = i + 1;
            printf("\nPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %.2f\n", comp[i].id, arg->id, comp[i].prob / 100); 
            sleep(0.5);
            printf("\nPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n", comp[i].id, arg->id);
            arg->vacleft = comp[i].p;
            pthread_mutex_unlock(&(comp[i].comp_mutex));
            break;
        }
        pthread_mutex_unlock(&(comp[i].comp_mutex));
    }
}
```
        
Then the zone makes available `k` slots. (`k` >= 1 and k <= min(8, number of vaccines left in the batch, number of waiting students)). When a zone has all the slots filled up at a particlular time, it makes itself busy until all the students of that vaccination phase are done with their vaccination slots.
```
int num = min(min(8, arg->vacleft), leftstud);
int vnum = rand() % num + 1;
while(leftstud>0 && arg->busy==0) {
    if(arg->fill == vnum) {
        arg->busy = 1;
        printf("\nVaccination Zone %d is ready to vaccinate with %d slot(s)\n​", arg->id, vnum);
        printf("\nVaccination Zone %d entering Vaccination Phase\n", arg->id);
        break;
    }
}
```
When all the students of the phase are done with their vaccination, the zone becomes available again.
```
if(arg->over == arg->fill)
    arg->busy = 0;
```

- `void *comp_func(void *args)`
When all the vaccines made by the company are used up, it manufactures a certain number of batches and initialises `vacleft` and `distbatch` appropriately. Each Pharmaceutical Company takes `w` seconds (random between 2-5) to prepare `r` batches (random between 1-5) of vaccine at any particular time. Each batch has a capacity to vaccinate `p` students (random between 10-20).
```
int batchnum = rand() % 5 + 1;
arg->p = rand() % 11 + 10;
arg->distbatch = batchnum;
arg->vacleft = arg->p * batchnum;
int w = rand() % 4 + 2;
printf("\nPharmaceutical Company %d is preparing %d batch(es) of vaccines which have success probability %.2f \n​", arg->id, batchnum, arg->prob / 100);
sleep(w);
while(leftstud>0) {
    if(arg->distbatch <= 0 || leftstud <= 0) {
        if(arg->distbatch <= 0)
            printf("\nAll the vaccines prepared by Pharmaceutical Company %d are distributed.\n", arg->id);
        break;
    }
}
```
