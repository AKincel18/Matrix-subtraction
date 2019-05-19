#include<iostream>
#include <stdio.h>

void MyProc2(int startingPos, int numberOfPos, float *t1, float *t2, float *destMatrix)
{
	for (int i = startingPos; i < startingPos + numberOfPos; i++)
		destMatrix[i] = t1[i] - t2[i];
}