#include<pthread.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/wait.h>
#include <unistd.h>

long long n;
pthread_mutex_t mutex;
// struct for arguments for using threads
typedef struct args
{
    int l, r;
	int* arr;
} args;

// for getting shared memory
int * shareMem(size_t size)
{
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, 0);
}

// merge code of mergesort
void merge(int* arr, int start, int mid, int end)
{
	int left[n + 1], right[n + 1];
	int nl = mid - start + 1, nr = end - mid;
	for(int i = 0; i < nl; i++)
		left[i] = arr[start + i];
	for(int i = 0; i < nr; i++)
		right[i] = arr[mid + 1 + i];
	int i, j, k;
	i = j = k = 0;
	while(i < nl && j < nr)
	{
		if(left[i] < right[j])
		{
			arr[k + start] = left[i];
			i++, k++;
		}
		else
		{
			arr[k + start] = right[j];
			j++, k++;
		}
	}
	while(i < nl)
	{
		arr[k + start] = left[i];
		i++, k++;
	}
	while(j < nr)
	{
		arr[k + start] = right[j];
		j++, k++;
	}
}

// normal merge sort
void mergesort_normal(int* a, int start,int end)
{
	if(start >= end)
		return;
	int mid = start + (end - start) / 2;
	mergesort_normal(a, start, mid);
	mergesort_normal(a, mid + 1, end);
	merge(a, start, mid, end);
}

// concurrent merge sort
void mergesort_processes(int* arr, int l, int r)
{
	int len = r - l + 1;
	// selection sort for length < 5
	// edge case
	if(len < 5)
	{
		int mn, temp;
		for(int i = 0; i < len; i++)
		{
			mn = l + i;
			for(int j = i + 1; j < len; j++)
			{
				if(arr[l + j] < arr[mn])
					mn = l + j;
			}
			temp = arr[mn];
			arr[mn] = arr[l + i];
			arr[l + i] = temp;
		}
	}
	else
	{
		pid_t lpid, rpid;
		lpid = fork();
		if(lpid == 0)
		{
			// sorting left half
			mergesort_processes(arr, l, l + len / 2 - 1);
			_exit(1);
		}
		else
		{
			rpid = fork();
			if(rpid == 0)
			{
				// sorting right half
				mergesort_processes(arr, l + len / 2, r);
				_exit(1);
			}
			else
			{
				// wait for both halves to be sorted, then merge them
				int status;
				waitpid(lpid, &status, 0);
				waitpid(rpid, &status, 0);
				merge(arr, l, l + len / 2 - 1, r);
			}
		}
		return;
	}

}

void * mergesort_threads(void * temp)
{
    args *elem = (args*)temp;
    int l = elem->l, r = elem->r;
	int *arr = elem->arr;
	int len = r - l + 1;
    if(l >= r)
        pthread_exit(0);
    if(len < 5)
    {
		// selection sort for length < 5
		// using thread
        pthread_mutex_lock(&mutex);
        for(int i = l; i <= r; i++)
        {
            int min = i;
            for(int j = i + 1; j <= r; j++)
            {
                if(arr[j] < arr[min])
                    min = j;
            }
            int temp = arr[min];
            arr[min] = arr[i];
            arr[i] = temp;
        }
        pthread_mutex_unlock(&mutex);
        pthread_exit(0);
    }

    args llim, rlim;
    int mid = (l + r) / 2;
    pthread_t ltid, rtid;
    llim.l = l, llim.r = mid, llim.arr = arr;
    rlim.l = mid + 1, rlim.r = r, rlim.arr = arr;

	// threads for left and right halves
    pthread_create(&ltid, NULL, mergesort_threads, &llim);
    pthread_create(&rtid, NULL, mergesort_threads, &rlim);

	// joining the threads back after they are terminated
    pthread_join(ltid, NULL);
    pthread_join(rtid, NULL);

	// merge after the two halves are sorted 
    pthread_mutex_lock(&mutex);
    merge(arr, l, mid, r);
    pthread_mutex_unlock(&mutex);
    pthread_exit(0);
}

int main()
{
	struct timespec ts;
	int shmid;
	key_t key = IPC_PRIVATE;
	// get n
	scanf("%lld", &n);
	shmid = shmget(key, sizeof(int) * n, IPC_CREAT | 0666);
	int *arr = shareMem(sizeof(int)*(n + 1));
	int brr[n + 1], crr[n + 1];

	// get array to be sorted
	for(int i = 0; i < n; i++)
	{
		scanf("%d", &arr[i]);
		crr[i] = brr[i] = arr[i];
	}

	// normal mergesort
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec / (1e9) + ts.tv_sec;
	mergesort_normal(crr, 0, n - 1);
	printf("\n");
	for(int i = 0; i < n; i++)
		printf("%d ", crr[i]);
	printf("\n");
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec / (1e9) + ts.tv_sec;
    printf("Time taken by normal mergesort for n = %lld is %Lf s\n", n, en - st);

	// concurrent mergesort
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec / (1e9) + ts.tv_sec;
	mergesort_processes(arr, 0, n - 1);
	for(int i = 0; i < n; i++)
		printf("%d ", arr[i]);
	printf("\n");
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec / (1e9) + ts.tv_sec;
    printf("Time taken by process mergesort for n = %lld is %Lf s\n", n, en - st);
	
	// multithreaded mergesort
	args temp;
    temp.l = 0, temp.r = n - 1, temp.arr = brr;
    pthread_t root;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9) + ts.tv_sec;
    pthread_create(&root, NULL, mergesort_threads, &temp);
    pthread_join(root, NULL);
	for(int i = 0; i < n; i++)
		printf("%d ", temp.arr[i]);
	printf("\n");
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec / (1e9) + ts.tv_sec;
    printf("Time taken by threaded mergesort for n = %lld is %Lf s\n\n", n, en - st);

	return 0;
}
