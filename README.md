# prime-finder
A multi-threaded C program that efficiently finds all primes up to a given maximum number.

## Summary

This program uses four auxilliary threads to find all prime numbers less than a given 
value *n*. Each thread checks a series of integers less than *n* of one of the following forms:
```
1) 12n + 1
2) 12n + 5
3) 12n + 7
4) 12n + 11
```
For each number in one of the above forms, divisibility checks are done 
against each discovered prime less than or equal to ``sqrt(n)``.

This program outputs all discovered primes less than *n* in plaintext to an indicated test file.
Note that some primes greater than *n* may be discovered, but all primes less than or equal to *n* are 
guaranteed to be discovered.

## Compile Instructions
If GNU Make and the gcc compiler are both installed, one simply needs to run the ``make`` command in
the project directory. This default target will compile with the gnu89 standard linked with the pthread and math library.

For other compilers or standards, there is no guarantee that the program will compile without slight modification.
However, if you do use a different compiler be sure to link the pthread and math libraries (``-lpthread -lm``).

## Execution

Usage is below:
```
prime-finder [-q] [-n number] output_file

    -q: runs in quiet mode; will not write primes to stdout also.

    -n: must be followed by an integer greater than 0.

    output_file: the output file.
```

For more information about the program, run it with the '-h' flag.