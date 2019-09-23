#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mythreads.h"

#define WHICH_LOCK 6
#define WHICH_CV 2
#define TCOUNT 200
int x=0;

void *consumer(void *arg)
{
        threadYield();
        threadYield();
        threadLock(WHICH_LOCK);
        threadWait(WHICH_LOCK,WHICH_CV);
        int x2 = x;
        //do some randomish number of yields to make it less likely to succeed by accident
        for (int i=0; i < (x2 % 7)+1; i++)
                threadYield();
        //check here to make sure that x is not negative
        assert(x >= 0);
        x = x2-5;
        threadUnlock(WHICH_LOCK);
        //check here to make sure that x is not negative
        assert(x >= 0);
        assert(interruptsAreDisabled == 0);
        return &x;
}

int main(int argc, char **argv)
{
        //int *targ = malloc(sizeof(int));
        //*targ = START_VAL;
        threadInit();

        int cons[TCOUNT];
        for (int i=0; i < TCOUNT; i++)
        {
                cons[i] = threadCreate(consumer,NULL);
                //check to make sure x isn't negative
                assert(x >= 0);
                threadYield();
        }

        for (int i=0; i < TCOUNT; i++)
        {
                threadLock(WHICH_LOCK);
                int x2=x;
                for (int i=0; i < (x2 % 3); i++)
                        threadYield();
                //check to make sure x isn't negative
                assert(x >= 0);
                x = x2+5;
                threadSignal(WHICH_LOCK,WHICH_CV);
                threadUnlock(WHICH_LOCK);
                threadYield();
                //check to make sure x isn't negative
                assert(x >= 0);
        }

        //int result_fail = 0;
        for (int i=0; i < TCOUNT; i++)
        {
                int *result;
                threadJoin(cons[i], (void**)&result);
                assert(result == &x);
        }

        //check to see if x is zero
        assert(x == 0);
        printf("done with testing\n");
}
