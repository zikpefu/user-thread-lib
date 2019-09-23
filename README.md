Zachary Ikpefua | Clemson ECE 3220
# Project 2 : User Mode Thread Library

KNOWN PROBLEMS:
Preemptive test 2: Cannot seem to get this test, I have assertion errors and segmentation faults
but I have no idea how to test in order to replicate the error. I understand fully the benefits to
creating your own tests, however with something as random as the preemtive_test it becomes almost
impossible to replicate an error or segfault in gdb.

DESIGN:
libmythreads - user mode threading library that generates threds to speed up the users processing.
General design is multiple functions down a line each completing the necessary functions in order
to create the threading library. All functions about what each function does is in the main c file.
I could have broken up the stunts to .h files but forgot how to. (sorry)
