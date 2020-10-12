# Concurrent Merge Sort

On running the three implementations of mergesort (normal, using processes, using threads) for the same input (of size 104), we see that the time taken by them is as follows:

1. Time taken by normal mergesort for n = 10000 is 0.000807 s
2. Time taken by threaded mergesort for n = 10000 is 0.320200 s
3. Time taken by process mergesort for n = 10000 is 0.394465 s

Hence, we see that standard mergesort is the fastest, followed by multithreaded mergesort and then processes mergesort.

In standard mergesort, no new thread or process is created. There is no overhead of the creation of processes and threads, and hence it is the fastest. Thread mergesort is faster than process mergesort as threads can be run parallelly in the same process, and the overhead of creating threads is lesser than process creation.
