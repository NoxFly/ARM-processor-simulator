#include <stdio.h>
#define SIZE 10

int main() {
    int t[] = { 3, 6, 2, 0, 4, 1, 9, 8, 5, 7 };
    int size = SIZE;
    int i, j;
	int tmp;

    for (i = 0; i < size; i++) {
        j = i;
        while (j > 0) {
            if (t[j-1] > t[j]) {
               tmp = t[j-1];
               t[j-1] = t[j];
               t[j] = tmp;
            }
        }
    }

/*
    for (i = 0; i < SIZE; i++)
        printf("%d ", t[i]);
    printf("\n");
*/

	return 0;
}
