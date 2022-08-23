# Measurement Assignment Questions
You may add your answers to the assignment questions in the space provided
below.  This is a markdown file (.md), so you may use markdown formatting. If
you do not know about markdown (or know but don't care to format anything) then
you may just treat this as a plain text file.


# Part A
Answer these questions before you start coding.  You may discuss the **Part A
questions *only*** with your classmates, but you must write your own answers
in your own words.


## Question 1
Research the `gettimeofday()` function.  (`man gettimeofday`)  What does this
function do?  How would you use it to measure the amount of time required
("latency" or "delay") to perform an action (say, calling the `foo()` function)?

### Answer:
This function returns the current time. We can call it before foo() to store the
start time, and then once again after foo(), storing the end time. To measure 
the amount of time required, we can perform sutraction endtime - starttime.

## Question 2
Now research the `getrusage()` function (`man getrusage`), paying particular
attention to the `ru_utime` and `ru_stime` fields in the result.  (Assume it's
being called with the `RUSAGE_SELF` argument.)  How are the user time
(`ru_utime`), system time (`ru_stime`), and the time returned by
`gettimeofday()` all different from each other?

### Answer:
getrusage() deals with CPU time. System time is the amount of time spent
in kernal. User time is the amount of time spent out of the kernal.
gettimeofday() returns the actual difference in wall clock time.

## Question 3
Suppose you want to measure the time it take to do one fast thing; something
that takes less than a minute to do (e.g., fill one glass with water).  However,
the only tool you have for taking measurements is a digital clock (regular
clock, not a stopwatch) that does not show seconds, only shows hours and
minutes.  How would you get an accurate measure of how long it takes to do the
thing once?

### Answer:
You could repeat the task for T minutes. To find the time it takes to perform the task once, divide the number of times you performed the task by T.

## Question 4
Suppose you want to find the average amount of time required to run function
`foo()`.  What is the difference between the following two approaches?  Which
one is better, and why?  (You may assume `foo()` is very fast.)

```c
latency = 0
loop N times
    measure start time
    call foo()
    measure end time
    latency += end time - start time
print latency/N
```

```c
measure start time
loop N times
    call foo()
measure end time
print (end time - start time) / N
```

### Answer:
The first approach executes instructions to measure start and end times for each iteration of the loop. If foo() is very fast, this could result in inaccuracies in measuring foo() due to the fact that it takes time to actually measure these times. As a result, for each iteration of the loop, an overhead is introduced, making the measured time of foo() appear greater than it actually is. This overhead would not appear in the second approach, as time is only measured once and will not occur for each loop iteration.

So, the second approach is better. By removing the overhead in time measurements, we can obtain more accurate measures.



## Question 5
Consider the following code.  What work is this code doing, besides calling
`foo()`?

```c
int i;
for (i = 0; i < N; ++i) {
  foo();
}
```

### Answer:
This code calls foo(), updates the value of i (increments it), and branches back to the start of the for loop.

## Question 6
If `foo()` is very fast, and the time to perform `for (i = 0; i < N; ++i)` may
be significant (relative to the time needed to call `foo()`), how could you make
your final measurement value includes *only* the average time required to call
`foo()`?

### Answer:
We can simply measure the time it takes to run an empty for loop with N iterations. When we later calculate foo(), we can subtract this time out before taking the average.

# Part B
Now that you've run all your experiments, answer the questions in "Part B".
You should **complete these questions on your own**, without discussing the
answers with anyone (unless you have questions for the instructor or TAs, of
course). Each question should only require approximately *a couple sentences*
to answer properly, so don't write an entire that isn't needed.


## Question 7
What was your general strategy for getting good measurements for all the
items?  (i.e., things you did in common for all of them, not the one-off
adjustments you had to figure out just for specific ones)

### Answer:
To get good measurements, I made sure I had large sample sizes (many repetitions/ measurements). I made sure to report the average time of the command in question by subtracting out the time of an empty loop, and then averaging over iterations.

## Question 8
What measurement result did you get for each of the six measurements?  Based on
these results, which functions do you think are system calls (syscalls) and
which do you think are not?

### Answer:

* allocate one page of memory with `mmap()`
  * user time:       **[ 0.023239 uS ]**
  * system time:     **[ 0.293994 uS ]**

* lock a mutex with `pthread_mutex_lock()`
  * user time:       **[ 0.007785 uS ]**
  * system time:     **[ 0.000900 uS ]**

* writing 4096 Bytes directly (bypassing the disk page cache) to /tmp
  * wall-clock time: **[ 884.439270 uS ]**

* reading 4096 Bytes directly (bypassing the disk page cache) from /tmp
  * wall-clock time: **[ 399.001801 uS ]**

* writing 4096 Bytes to the disk page cache
  * wall-clock time: **[ 3.384500 uS ]**

* reading 4096 Bytes from the disk page cache
  * wall-clock time: **[ 0.291600 uS ]**

* Syscalls:     **[ Read/Write , mmap(), time measuring functions (getrusage, timeval) ]**
* Not syscalls: **[lock a mutex]**


## Question 9
What is the memory page size?  (i.e., that you used with `mmap`)

### Answer:
4096 bytes.

## Question 10
How did you deal with the problem of not being able to lock a mutex
more than once without unlocking it first?

### Answer:
I initialized an array with many mutexes, and measured the time it took
to lock all of them. This avoided the issue of having to lock and unlock
mutexes.

## Question 11
Was performance affected by whether a file access operation is a read or write?
If so, how?

### Answer:
Yes. It seemed that in all cases reading from a file was much shorter than
writing to a file. Around 20x shorter on average.

## Question 12
What affect did the disk page cache have on file access performance?  Did it
affect reads and write differently?

### Answer:
Reading and writing using cache were much faster than using /tmp. However, the
relation that reading was faster than writing did not change. Generally speaking,
using cache seems to be a few orders of magnitude faster (ROUGHLY 100x faster, 
according to my data).
