# Q3: Code Breakdown

## Overview
The college is opening for the new semester, and the campus is overrun with excited first years and their parents. The Music Club has decided to host an event on the first day to entertain all the newcomers.

## Run the program
1. Run `gcc q3.c -lpthread`
2. Run `./a.out`

## Input
First line will have k, a, e, c, t1, t2, t (0 <= k, a, e, c, t1, t2, t and t1 <= t2)
The following k lines will have info of each instrumentalist. Each line is of the form:
<name> <instrument character> <time in secs of arrival>

## Variables
`struct performer` - struct for a performer
It has the following variables:
`name[]` - Name of the performer
`instch` - Instrument character (character value corresponding to the instrument he/she plays)
`tPer` - Time for which the performer performs
`perfThreadId` - Performer thread
`acouThreadId` - Thread to check if any acoustic stage is free for the performer
`electThreadId` - Thread to check if any electric stage is free for the performer
`withsoloThreadId` - Thread for a singer to check if there is a solo musician performing so that he/she can join
`status` - stores status of the performer as follows:
0: Not yet arrived, 
1: Waiting to perform, 
2: Performing solo,
3: Performing with singer, 
4: Performing with musician,
5: Waiting for t-shirt, 
6: T-shirt being collected,
7: Exited

`performer perf[1001]` - array of performers

`k` - Number of performers
`a` - Number of acoustic stages
`e` - Number of eletric stages
`c` - Number of coordinators
`t1` - Minimum amount of time a performer performs
`t2` - Maximum amount of time a performer performs
`t` - Maximum amount of time a performer waits

Semaphores:-
`freeAcoustic` - Semaphore to ensure maximum a performers take up an acoustic stage at a time
`freeElectric` - Semaphore to ensure maximum e performers take up an acoustic stage at a time
`freeWithSolo` - Semaphore to ensure singers take up appropriate number of stages (max a+e) while joining a musician
`freeCoor` -  Semaphore to ensure maximum c coordinators are engaged at a time

## Functions

1. `int main()`
We take input in main and then create and send k threads of performers to srujana().

2. `void* srujana(void *args)`
In this function, performers arrive at their arrival time. 
`sleep(arg->tarr)`

Upon arrival, they search for acoustic or electric stages or chance to perform with a musician who is currently performing solo, as applicable. Separate threads are created for this purpose and appropriate semaphores are used to regulate them. 
```
if(arg->instch == 'p' || arg->instch == 'g') {
    pthread_create(&(arg->acouThreadId), NULL, check_acoustic , arg);
    pthread_create(&(arg->electThreadId), NULL, check_electric , arg);
}
else if(arg->instch == 'v') {
    pthread_create(&(arg->acouThreadId), NULL, check_acoustic , arg);
}
else if(arg->instch == 'b') {
    pthread_create(&(arg->electThreadId), NULL, check_electric , arg);
}
if(arg->instch == 's') {
    pthread_create(&(arg->acouThreadId), NULL, check_acoustic , arg);
    pthread_create(&(arg->electThreadId), NULL, check_electric , arg);
    pthread_create(&(arg->withsoloThreadId), NULL, check_solo , arg);
}
```

In the meanwhile, if the waiting time crosses t, the impatient performer exits srujana. 
```
clock_t time_taken = clock() - t;
double times = ((double)time_taken)/CLOCKS_PER_SEC;
if(times > ((double)t))
{
    printf("\n\033[1;31m%s %c left because of impatience\n", arg->name, arg->instch);
    break;
}
```

3. `void* check_acoustic(void *args)`
This function checks if there is any available acoustic stage for the performer to perform and appropriate status changes are made.
After the completion of the performance, if the performer is a musician, he/she goes to collect t-shirt in get_tshirt(). 


4. `void* check_electric(void *args)`
This function checks if there is any available electric stage for the performer to perform and appropriate status changes are made.
After the completion of the performance, if the performer is a musician, he/she goes to collect t-shirt in get_tshirt().


5. `void* check_solo(void *args)`
This function checks if there is any available musician performing solo at a stage for the singer to join and appropriate status changes are made. The performance time of the musician joined is increased by 2 seconds.


6. `void* get_tshirt(void *args)`
After the musician has performed, they collect their t-shirts from the coordinator and then exit.
