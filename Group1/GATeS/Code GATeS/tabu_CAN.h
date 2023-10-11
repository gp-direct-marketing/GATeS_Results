#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "definitions_CAN.h"
#include "construtive_CAN.h"

/* Tipo Exportado */
//22/03
typedef struct list List;

void tabu_search(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int *maxBudgetpo, int* fcostpo,
	int **solc, int* solp, float* solution_cost,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can);
void tabu_search2(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* maxBudgetpo, int* fcostpo,
	int **solc, int* solp, float* solution_cost,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can, int **profit, cromo *p1, cromo *p2, int* npp_sorted_prod_list, bool newpop, clock_t *initial_time, int *global_best, float *global_best_time, bool initialized);

//extern bool newpop;