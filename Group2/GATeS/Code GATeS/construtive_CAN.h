#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "definitions_CAN.h"
#include "Gen.h"

//22/03
/* Tipo Exportado */
struct list {
	int inf;
	struct list* prox;
	struct list* ant;
	struct list* last;

};

typedef struct list List;


void build_initial_solution(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can, int *prod_order);
void build_initial_solution2(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can, int iter);

bool mutate_solution(int client, int prod, float hurdle, int blocked_prod,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can);

void compute_solution_value(int client, int prod, float hurdle,
                            int **cost, int **revenue, int* maxofer,
                            int* mcpo, int* budgetpo, int* fcostpo,
                            int **solc, int* solp, float* solution_cost, int* prod_can, bool mode);

bool build_population(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	cromo *pop,
	List** sorted_npp, int* prod_can, int popSize, float range, float hurdleTolerance, float penalty, float *average_npp);

void alternative_constructive_algorithm(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can);

void build_population_random(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	cromo *pop,
	List** sorted_npp, int* prod_can, int popSize, float range, float hurdleTolerance, float penalty, float *average_npp, int initial_position);

int verify_solution(cromo *sol, int *obudget, int prod, int cli, int	**cost, int **revenue, int *mcpo, int *maxoffer, int *prod_can);

bool check_budget(int **sol, int *oBuget, int *budget, int **cost, int prod, int cli);

bool check_budget(cromo *sol, int *oBuget, int **cost, int prod, int cli);

bool check_values(cromo *sol,int* fcpo, int** cost, int **revenue, int sum_cost, int sum_revenue, int total_cost, int prod, int cli);

int* create_variance_vector(int **cost, int **revenue, int client, int prod, int *solp);

int** create_cvp_npp_matrix(int** cost, int** revenue, int prod, int client);

void build_population_random(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	cromo *pop,
	List** sorted_npp, int* prod_can, int popSize, float range, float hurdleTolerance, float penalty, float *average_npp, int initial_position);
void alternative_constructive_algorithm_GEN(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	cromo *pop,
	List** sorted_npp,
	int* prod_can, int popSize, int initial_pos);
void build_initial_solution1(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can);
void build_initial_solution21(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can, int iter);
void build_initial_solution21(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can, int iter);

void build_initial_solution_max_offers(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* fcostpo,
	int **solc, int* solp,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can, int *prod_order);

//extern bool newpop;