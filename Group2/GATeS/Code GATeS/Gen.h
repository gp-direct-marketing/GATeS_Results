#pragma once
#include "definitions_CAN.h"


//22/03

typedef struct gene1
{
	int id, revenue, cost, npp, offerSlots;
	int offers[MAXPROD];
}gene;
typedef struct cromo1
{
	gene clients[MAXC];
	int fitness, revenue, cost;
	int budget[MAXPROD];
	bool offeredProducts[MAXPROD];
}cromo;

struct list3 {
	int inf;
	struct list* prox;
	struct list* ant;
	struct list* last;

};

struct list2 {
	int inf;
	int profit;
	struct list2* prox;
	struct list2* ant;
	struct list2* last;

};



void crossover(cromo *p1, cromo *p2, cromo *n, int prod, int cli, int* mcpo, int totalOffers, int** revenue, int** cost, int** profit, list **sorted_npp, int *maxOffer, int *oBudget, float hurdle, float penaltie, int *fcpo, list2 *client_order, list2 *prod_order);
//void Genloop(int prod, int cli, int* mcpo, int *prod_can, int *budgetpo, int *maxOffer, int** revenue, int** cost, int** profit, list** sorted_npp, float hurdle, float penaltie, int generations, int popSize, int *fcpo, float range, float hurdleTolerance, int **solc, int *solp, int *budget, float* sum_cost, float* sum_profit, int *active_clients, int *active_prod);
//int reproduction(cromo *pop1, cromo *pop2, int prod, int cli, int* mcpo, int totalOffers, int** revenue, int** cost, int** profit, list **sorted_npp, int *maxOffer, int *oBudget, float hurdle, float penaltie, int tam, float *probability, int *fcpo, cromo *elite, list2* client_order, list2 *prod_order);
void mutation(cromo *n, int cli, int prod, int** cost, int ** revenue, float penalty, float hurdle);
void mutation2(cromo *n, int cli, int prod, int** cost, int ** revenue, float penalty, float hurdle, list **npp);
void Genloop_CAN(cromo *p1, cromo *p2, int prod, int cli, int* mcpo, int *prod_can, int *budgetpo, int *maxOffer, int** revenue, int** cost, int** profit, list** sorted_npp, float hurdle, float penaltie, int generations, int popSize, int *fcpo, float range, float hurdleTolerance, int** solc, int *solp, int* budget, float* sum_cost, float* sum_profit, int *active_clients, int *active_prod, bool new_pop, int *client_po, bool diversification, bool alternative_const, bool initialized);

//bool mutation_matrix[MAXPROD][MAXC];