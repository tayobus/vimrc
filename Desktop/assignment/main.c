#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "button.h"

pthread_t *pt;
int count = 0;
void *onLongClick()
{
    printf("onLongClick\n");
    if (++count == 2)
    {
        count = 0;
        pthread_cancel(*pt);
    }
}

void *onPressDown()
{
    printf("onPressDown\n");
}

void *onPressUp()
{
    printf("onPressUp\n");
}

int main(int argc, char *argv[])
{
    while (1)
    {
        BUTTON *button = (BUTTON *)calloc(1, sizeof(BUTTON));
        button->pin = atoi(argv[1]);
        button->pout = atoi(argv[2]);
        button->polling_rate = atoi(argv[3]);
        button->onLongClick = onLongClick;
        button->onPressDown = onPressDown;
        button->onPressUp = onPressUp;
        if ((pt = initButton(*button)) == NULL)
        {
            printf("button init fail\n");
            return 1;
        }
        else
        {
            printf("Hello Button!\n");
            pthread_join(*pt, NULL);
            printf("button disposed. please enter to rerun\n");
            char c[1000];
            fgets(c, 1000, stdin);
        }
    }
    return 0;
}