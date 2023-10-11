#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "construtive_CAN.h"
#include "definitions_CAN.h"
#include "tabu_CAN.h"
#include "matriz.h"


bool newpop = true;
int *prod_order=NULL;
float global_best_time = 0;
cromo *pop1 = (cromo*)malloc(sizeof(cromo)*POPSIZE);
cromo *pop2 = (cromo*)malloc(sizeof(cromo)*POPSIZE);
bool can2 = true;
int global_best = 0;
bool initialized = false;
int compare(const void * a, const void * b)
{
	return (prod_order[*(int*)b] - prod_order[*(int*)a]);
}


int main(void)
{
	int i, j,h,jj;
	int client, prod;
	int cont;
	float gap, gap_sum;
	float hurdle;
	int **cost;
	cost = generate_2D_matrix_int(MAXC, MAXPROD);
	int **revenue;
	revenue = generate_2D_matrix_int(MAXC, MAXPROD);
	int **profit = generate_2D_matrix_int(MAXC, MAXPROD);
	int maxofer[MAXC];
	int mcpo[MAXPROD];       /* minimum number of clients per offer */
	int budgetpo[MAXPROD];   /* budget per offer */
	int budgetpo2[MAXPROD];   /* budget per offer */
	int fcostpo[MAXPROD];    /* fixed cost per offer */
	int **solc; /* solution considering 1 if the product is allocated to the client, 0 otherwise */
	solc = generate_2D_matrix_int(MAXC, MAXPROD);
	int **best_solc; /* best solution found between random resets */
	best_solc = generate_2D_matrix_int(MAXC, MAXPROD);
	int solp[MAXPROD];       /* solution considering 1 if the product is used, 0 otherwise */
	int best_solp[MAXPROD];
	float solution_cost;     /* variable to store the current solution value */
	float sum_profit;
	float sum_cost_fix;
	int prod_can[MAXPROD];
	int l, k, i2;
	int active_clients[MAXC];
	int active_products[MAXPROD];
	List* sorted_npp[MAXPROD];
	List* aux_print;
	FILE* data;
	FILE* results;
	FILE* results_tabu;
	char file_name[121];
	char file_name_out[121];
	List *aux_list;
	List *aux_list2;
	FILE *log = fopen("Log_results.txt", "a+");
	fprintf(log, "--------------- Inicio da run ------------\n");
	fclose(log);
	float best_solution_cost = 0;
	int resets = 8;
	
	bool initialized = false;
	bool newpop = true;
	

	srand(1);

	char search[40];
	char search_aux[40];
	char c;
	char gapFile[40];
	int *npp_ordered_product_list = NULL;
	int max_cc = 0;
	int max_pc = 0;

	char sufix1[40];
	char sufix2[40];

	strcpy(sufix1, "-l-DISSIMILARITY.txt");
	strcpy(sufix2, "-s-DISSIMILARITY.txt");

	char number1[3];
	char number2[3];
	char number3[3];

	strcpy(number1, "-1");
	strcpy(number2, "-2");
	strcpy(number3, "-3");

	char prefix1[40];
	char prefix2[40];
	char prefix3[40];
	//Uncomment to adjust instance names automatically for the original instances
	//printf("Digite o nome do arquivo de instancia desejada: (Ex: L - 15 - 5 - 2 - l - CAN.txt): \n");
	////**Parametros para mudar entre baterias de teste**
	//scanf("%39s",prefix1);
	////strcpy(search, file_name);
	////strcat(file_name, "-CAN.txt");
	//strcpy(prefix2, prefix1);
	//strcpy(prefix3, prefix1);
	//strcat(prefix1, "-5");
	//strcat(prefix2, "-10");
	//strcat(prefix3, "-15");
	//strcpy(gapFile, "GAP_S3.txt");

	char prod_number1[40];
	char prod_number2[40];
	char prod_number3[40];

	/*strcpy(prod_number1, "-5");
	strcpy(prod_number2, "-10");
	strcpy(prod_number3, "-15");*/
	


	clock_t begin_alg, end_constr, end_tabu;
	float time_constructive;
	float time_tabu=0;
	float time_total;

	for (i = 0; i < POPSIZE; i++)
	{
		for (i2 = 0; i2 < MAXPROD; i2++)
			pop1[i].budget[i2] = budgetpo[i2];
		pop1[i].cost = 0;
		pop1[i].revenue = 0;
		pop1[i].fitness = 0;
		for (j = 0; j < MAXPROD; j++)
			pop1[i].offeredProducts[j] = false;
		for (j = 0; j < MAXC; j++)
		{
			for (i2 = 0; i2 < MAXPROD; i2++)
				pop1[i].clients[j].offers[i2] = 0;
			pop1[i].clients[j].cost = 0;
			pop1[i].clients[j].id = j;
			pop1[i].clients[j].npp = 0;
			pop1[i].clients[j].offerSlots = mcpo[j];
			pop1[i].clients[j].revenue = 0;
		}
	}

	for (i = 0; i < POPSIZE; i++)
	{
		for (i2 = 0; i2 < MAXPROD; i2++)
			pop2[i].budget[i2] = budgetpo[i2];
		pop2[i].cost = 0;
		pop2[i].revenue = 0;
		pop2[i].fitness = 0;
		for (j = 0; j < MAXPROD; j++)
			pop2[i].offeredProducts[j] = false;

		for (j = 0; j < MAXC; j++)
		{
			for (i2 = 0; i2 < MAXPROD; i2++)
				pop2[i].clients[j].offers[i2] = 0;
			pop2[i].clients[j].cost = 0;
			pop2[i].clients[j].id = j;
			pop2[i].clients[j].npp = 0;
			pop2[i].clients[j].offerSlots = mcpo[j];
			pop2[i].clients[j].revenue = 0;
		}
	}
	prod_order = (int*)malloc(sizeof(int)*MAXPROD);
	npp_ordered_product_list = (int*)malloc(sizeof(int)*MAXPROD);

	for (j = 0; j < MAXPROD; j++)
	{
		sorted_npp[j] = (List*)malloc(sizeof(List));
	}
	//Adjust the indexes in case of multiple 
	for (k = 0, cont = 0, gap = 0, gap_sum = 0;k < 1;k++)
	{
		for (h = 0;h < 1;h++)
		{
			for (l = 0;l < 1;l++)
			{
				for (int i1 = 0;i1 < 1;i1++)
				{
					cont++;
					for (j = 0; j < MAXPROD; j++)
						prod_can[j] = -100;
					//Instance that will be processed. Uncomment lines 102 to 106 and replace "Est-15K-10-10-10-8-10-15-40-35-CAN.txt" for file_name in order to be able to select instances through input. All instances must end in -CAN.txt
					strcpy(file_name, "Est-15K-10-10-10-8-10-15-40-35-CAN.txt");
					data = fopen(file_name, "r");
					if (data == NULL) {
						printf("\n FILE OPEN ERROR \n");
						exit(1);
					}
					fscanf(data, "%d %d %f\n", &client, &prod, &hurdle);

					for (i = 0; i < client; i++)
					{
						for (j = 0; j < prod; j++)
							fscanf(data, "%d", &cost[i][j]);
						for (j = 0; j < prod; j++)
							fscanf(data, "%d", &revenue[i][j]);
						fscanf(data, "%d\n", &maxofer[i]);
					}
					for (j = 0; j < prod; j++)
						fscanf(data, "%d", &mcpo[j]);
					fscanf(data, "\n");
					for (j = 0; j < prod; j++)
						fscanf(data, "%d", &budgetpo[j]);
					fscanf(data, "\n");
					for (j = 0; j < prod; j++)
						fscanf(data, "%d", &fcostpo[j]);
					fscanf(data, "\n");
					for (j = 1; j <= ((int)prod / 5); j++)
					{
						fscanf(data, "%d", &i);
						fscanf(data, "%d", &jj);
						if (i < 0 || jj < 0) break;
						prod_can[i] = jj;
						
						prod_can[jj] = i;
					}
					fscanf(data, "\n");
					fclose(data);
					
					for (j = 0;j < prod;j++)
					{
						best_solp[j] = 0;
						solp[j] = 0;
						for (i = 0;i < client;i++)
						{
							solc[i][j] = 0;
							best_solc[i][j] = 0;
						}
					}
					
					/* Initializing structure to store solution */
					
					for (j = 0; j < MAXPROD; j++)
					{
						active_products[j] = 1;
						npp_ordered_product_list[j] = j;
						for (i = 0; i < MAXC; i++)
						{
							active_clients[i] = maxofer[i];
							solc[i][j] = 0;
						}
						solp[j] = 0;
						sorted_npp[j]->inf = 0;
						sorted_npp[j]->prox = NULL;

					}
						
					
					if (max_cc < client)
						max_cc = client;
					if (max_pc < prod)
						max_pc = prod;
					begin_alg = clock();
					initialized = false;
					for (i2 = 0, best_solution_cost=0, global_best=0;i2 < resets;i2++)
					{
						for (int c = 0;c < prod;c++)
						{
							budgetpo2[c] = budgetpo[c];
							solp[c] = 0;
						}
						for (j = 0; j < prod; j++)
						{
							active_products[j] = 1;
							for (i = 0; i < client; i++)
							{
								active_clients[i] = maxofer[i];
								solc[i][j] = 0;
							}
							solp[j] = 0;

						}
						
						//In the first run, build the solution with the default constructive algorithm
						if (i2==0)
						{
							build_initial_solution(client, prod, hurdle,
								cost, revenue, maxofer,
								mcpo, budgetpo, fcostpo,
								solc, solp, sorted_npp, active_clients, active_products,
								&sum_cost_fix, &sum_profit, prod_can,prod_order);

 							compute_solution_value(client, prod, hurdle,
								cost, revenue, maxofer,
								mcpo, budgetpo, fcostpo,
								solc, solp, &solution_cost, prod_can,false);
						}
						else
						{	//If it is the second run, builds the solution with the alternative constructive algorithm
							if (i2 == 1 )
							{
								alternative_constructive_algorithm(client, prod, hurdle,
									cost, revenue, maxofer,
									mcpo, budgetpo, fcostpo,
									solc, solp, sorted_npp, active_clients, active_products,
									&sum_cost_fix, &sum_profit, prod_can); 
								compute_solution_value(client, prod, hurdle,
									cost, revenue, maxofer,
									mcpo, budgetpo, fcostpo,
									solc, solp, &solution_cost, prod_can,false);
							}//From the third run and over, utilizes the greedy randomized constructive algorithm
							if (i2 == 2 )
							{
								build_initial_solution_max_offers(client, prod, hurdle,
									cost, revenue, maxofer,
									mcpo, budgetpo, fcostpo,
									solc, solp, sorted_npp, active_clients, active_products,
									&sum_cost_fix, &sum_profit, prod_can, prod_order); //Enviar ponteiros de vetores ordenados
								compute_solution_value(client, prod, hurdle,
									cost, revenue, maxofer,
									mcpo, budgetpo, fcostpo,
									solc, solp, &solution_cost, prod_can, false);
							}

							if (i2 > 2 || solution_cost == 1)
							{
								int counter = 0;
								solution_cost = 0;
								while (solution_cost <=1 && counter < 20)
								{
									build_initial_solution2(client, prod, hurdle,
										cost, revenue, maxofer,
										mcpo, budgetpo, fcostpo,
										solc, solp, sorted_npp, active_clients, active_products,
										&sum_cost_fix, &sum_profit, prod_can, i2);//Enviar ponteiros de vetores ordenados
									compute_solution_value(client, prod, hurdle,
										cost, revenue, maxofer,
										mcpo, budgetpo, fcostpo,
										solc, solp, &solution_cost, prod_can,false);
									counter++;
								}
							}
						}

						end_constr = clock();
						if (global_best < solution_cost)
						{
							global_best = solution_cost;
							global_best_time = (end_constr - begin_alg) / CLOCKS_PER_SEC;
						}
						
						if (solution_cost > 1)
						{				
														for (i = 0; i < client; i++)
							{
								for (j = 0; j < prod; j++)
									profit[i][j] = revenue[i][j] - cost[i][j];
							}
							
							//fail safe in case the solution is infactible.
							if (solution_cost > 1)
							{
								tabu_search2(client, prod, hurdle,
									cost, revenue, maxofer,
									mcpo, budgetpo, budgetpo2, fcostpo,
									solc, solp, &solution_cost,
									sorted_npp, active_clients, active_products,
									&sum_cost_fix, &sum_profit, prod_can, profit, pop1, pop2, npp_ordered_product_list, newpop, &begin_alg, &global_best, &global_best_time, initialized);
								initialized = true;
							}

							compute_solution_value(client, prod, hurdle,
								cost, revenue, maxofer,
								mcpo, budgetpo, fcostpo,
								solc, solp, &solution_cost, prod_can,true);
							end_tabu = clock();

							
							
							if (solution_cost > best_solution_cost)
							{
								for (int j1 = 0; j1 < prod; j1++)
								{
									best_solp[j1] = solp[j1];
									for (int i1 = 0; i1 < client; i1++)
									{
										best_solc[i1][j1] = solc[i1][j1];
									}
								}
								best_solution_cost = solution_cost;
							}
							
						}
					}

					time_total = (clock() - begin_alg) / CLOCKS_PER_SEC;
					
					strcpy(file_name_out, "Div_Tabu_");
					strcat(file_name_out, file_name);
					results_tabu = fopen(file_name_out, "w");
					if (results_tabu == NULL)
					{
						printf("\n FILE OPEN ERROR \n");
						exit(1);
					}
 					fprintf(results_tabu, "Sol_value_tabu = %f\n", best_solution_cost);

					for (j = 0; j < prod; j++)
					{
						fprintf(results_tabu, "\n");
						fprintf(results_tabu, "prod[%d] = %d\n", j, best_solp[j]);
						for (i = 0; i < client; i++)
							fprintf(results_tabu, "client[%d][%d] = %d\n", i, j, best_solc[i][j]);
						fprintf(results_tabu, "\n");
					}
					fclose(results_tabu);

					log = fopen("Log_results.txt", "a+");
					fprintf(log, file_name);
					fprintf(results_tabu, ": Sol_value = %f - ", best_solution_cost);					
					fprintf(log, " Time: %f", time_total);
					fprintf(log, " - Time_best_solution: %f\n", global_best_time);
					fclose(log);

				}
			}
		}
	}



	return 0;
}