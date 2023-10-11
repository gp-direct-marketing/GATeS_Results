
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include "matriz.h"

int ** generate_2D_matrix_int(int n, int m) {
	int **matrix;

	matrix = new int*[n];
	for (int i = 0; i < n; i++) {
		matrix[i] = new int[m];
	}
	//initialize the 2-d array
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			matrix[i][j] = 0.0;
		}
	}
	return matrix;
}

float ** generate_2D_matrix_float(int n, int m) {
	float **matrix;

	matrix = new float*[n];
	for (int i = 0; i < n; i++) {
		matrix[i] = new float[m];
	}
	//initialize the 2-d array
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			matrix[i][j] = 0.0;
		}
	}
	return matrix;
}

short int **generate_2D_matrix_short(int n, int m)
{
	short int **matrix;

	matrix = new short int*[n];
	for (int i = 0; i < n; i++) {
		matrix[i] = new short int[m];
	}
	//initialize the 2-d array
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			matrix[i][j] = 0.0;
		}
	}
	return matrix;
}

bool **generate_2D_matrix_bool(int n, int m)
{
	bool **matrix;

	matrix = new bool*[n];
	for (int i = 0; i < n; i++) {
		matrix[i] = new bool[m];
	}
	//initialize the 2-d array
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			matrix[i][j] = false;
		}
	}
	return matrix;
}