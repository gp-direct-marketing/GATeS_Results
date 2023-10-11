#include <time.h>
#include "tabu_CAN.h"
#include "definitions_CAN.h"
#include "matriz.h"
#include "Gen.h"
#include "construtive_CAN.h"
#include <stdlib.h>
#include <stdio.h>
//22/03
bool mutation_vec[MAXPROD / 2];
bool mutation_matrix[MAXC][MAXPROD];
int offer_profile[MAXPROD][MAXC];
//Turns on and off the cannibalism treatment for the genetic algorithm
bool flag_can = true;

float deviation = 0.03;
int gen;

cromo* elite_solution;
int prod_order_aux[MAXPROD];
int client_order_aux[MAXC];
int prod_order[MAXPROD];
int client_order[MAXC];

int compare2g(const void * a, const void * b)
{
	return prod_order_aux[*(int*)b] - prod_order_aux[*(int*)a]; //(conflicts[*(int*)a]- (conflicts[*(int*)b]));

}
int compare1g(const void * a, const void * b)
{
	return client_order_aux[*(int*)b] - client_order_aux[*(int*)a]; //(conflicts[*(int*)a]- (conflicts[*(int*)b]));

}

/*Modifies the mutation_matrix utilising the offer profile of the population. The mutation matrix prevent offers to be inserted in solutions during the crossover.
If the pair (i,j) is true in the mutation matrix, the crossover won't allow the product j to be offered to the client i.*/
void mutate_matrix2(int prod, int client, bool *offered_products)
{
	int i, j, random, cont;

	for (i = 0; i < client; i++)
		for (j = 0; j < prod; j++)
		{
			mutation_matrix[i][j] = false;
		}
	for (i = 0, j = 0; i < (int)(client*0.10); i++)
	{
		random = rand() % (int)(client*0.20);
		if (j == prod)
			j = 0;
		for (cont = 0; !offered_products[j] && cont < prod; j++, cont++)
		{
			if (j == prod)
				j = 0;
		}
		if (cont == prod)
			break;
		if (offer_profile[j][random] >= 0 && offer_profile[j][random] < client)
			mutation_matrix[offer_profile[j][random]][j] = true;
	}
}

/*Builds a offer profile for the population. It starts identifying offers that happens on at least 35% (similarity_rate) of the population. If the quantity of offers profiled was less than 20% of the of the clients available, similiarity_rate will be decreased by 5% until the quota is met.  */
void offer_profiling(int prod, int client, int pop_size, cromo *pop, list** sorted_npp, int** revenue, int** cost, float* average_positive_deviation, float* average_npp)
{
	int i, j, c;
	list* aux_list;
	int candidate_genes[MAXC];
	float similarity_rate = 0.55;
	float dev = 0;
	float interval = 0.20;

	for (i = 0; i < prod; i++)
	{
		for (j = 0; j < client; j++)
		{
			offer_profile[i][j] = -1;
		}
	}

	for (j = 0; j < prod; j++)
	{
		for (aux_list = sorted_npp[j]->last; aux_list->ant; aux_list = aux_list->ant)
		{
			for (c = 0; c < pop_size; c++)
			{
				if (pop[c].clients[aux_list->inf].offers[j])
					offer_profile[j][aux_list->inf]++;
			}
		}
	}
	for (j = 0; j < prod; j++)
	{
		for (aux_list = sorted_npp[j]->last, i = 0; aux_list->ant && i < (int)(client*0.20); aux_list = aux_list->ant)
		{
			if (!cost[aux_list->inf][j])
				continue;
			dev = (revenue[aux_list->inf][j] / cost[aux_list->inf][j]) / average_npp[j];
			if (dev < 0)
				dev = dev * (-1);
			if ((offer_profile[j][aux_list->inf]) / pop_size > similarity_rate &&  dev <= average_positive_deviation[j])
			{
				candidate_genes[i] = aux_list->inf;
				i++;
			}

		}
		if (i == 0)
			break;
		if (i < (int)(client*interval))
		{
			similarity_rate -= 0.05;
			interval -= 0.025;
			j--;
		}
		else
		{
			for (i = 0; i < (int)(client*interval); i++)
			{
				offer_profile[j][i] = candidate_genes[i];
			}
			similarity_rate = 0.35;
		}
	}

}

/*Applies the local search(hill climbing) operator.*/
void local_search(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int *maxBudgetpo, int* fcostpo,
	cromo *solc, bool* solp, int* solution_cost,
	List** sorted_npp,
	int* sum_cost_fix, int* sum_profit, int* prod_can)
{
	float incumbent_solution = *solution_cost; /* Store the Best Iteration so far */
	float tabu_cost = *solution_cost;

	float *sum_revenue;
	sum_revenue = new float[prod];
	float *sum_cost;
	sum_cost = new float[prod];
	int sum_cost_i1, sum_cost_i2, sum_revenue_i1, sum_revenue_i2;
	int aspiration_criteria = 0;
	int *used_product;
	used_product = new int[prod];
	int i, j, i1, i2, j1, j2;
	int iter = 1;                              /* Iteration counter */
	int iter_no_improvement = 1;               /* Iterations without improvement */
	int iter_no_diversification = 1;          /* Iterations without diversification */
	int diver_not_found = 1;

	int cont;
	int tabu_time = 20;
	int *residual_budget;
	residual_budget = new int[prod];
	int *client_po;
	client_po = new int[prod];
	int flag_move1 = 0;
	int flag_move2 = 0;
	int flag_move3 = 0;
	int flag_move4 = 0;
	int flag_can = 0;
	int flag_diver = 0;
	int flag_double = 0;
	int cont_double = 0;
	int best_cont_double = 0;
	float previous_solution = tabu_cost;
	clock_t begin_phase, end_phase;
	float time_elapsed;
	int conti1 = 0;
	int conti2 = 0;

	/*Mutation parameters*/
	int **sol_aux; /* solution considering 1 if the product is allocated to the client, 0 otherwise */
	sol_aux = generate_2D_matrix_int(client, prod);
	int solp_aux[MAXPROD];       /* solution considering 1 if the product is used, 0 otherwise */
	float solution_cost_aux;     /* variable to store the current solution value */
	float sum_profit_aux;
	float sum_cost_fix_aux;
	int active_clients_aux[MAXC];
	int active_products_aux[MAXPROD];
	int blocked_prod = 0;
	int prod_cic = 1;
	bool div2_flag = false;
	int aux_budget[MAXPROD];




	int best_j1, best_j2, best_i1, best_i2, best_solution, best_move;

	for (j = 0; j < prod; j++)
	{
		residual_budget[j] = budgetpo[j];
		client_po[j] = 0;
		used_product[j] = 0;
	}

	best_i1 = -1;
	best_i2 = -1;
	best_j1 = -1;
	best_j2 = -1;
	best_solution = -10000;
	begin_phase = clock();
	time_elapsed = 0.0;

	while (iter_no_improvement < 20 && time_elapsed <= 3600.0) /* loop for the tabu search until a stopping criteria be found */
	{
		for (j1 = 0; (j1 < prod && !flag_move1); j1++) /* loop for the neighborhood of swap,  i.e., if prod[j] is active and customer[i1][j]=1 and customer[i2][j]=0 */
													   /* do customer[i1][j]=0 and customer[i2][j]=1 that improves the solution value */
		{
			//Se o produto j1 esta aberto
			if (solc->offeredProducts[j1] != 0)
			{
				for (i1 = 0; (i1 < client - 1 && !flag_move1); i1++)
				{
					//Se o produto esta sendo ofertado para o cliente i1
					if (solc->clients[i1].offers[j1] == 1)
					{
						for (i2 = 0; (i2 < client && !flag_move1); i2++)
						{
							//Se o produto j1 nao foi ofertado para o client i2 e se a diferenca de custos entre oferecer o produto j1 para o client i1 e i2 esta dentro do orcamento
							//e i2 ainda nao foi mexido pelo tabu e o lucro provido pela oferta esta dentro do hurdle rate
							if (((cost[i2][j1] - cost[i1][j1]) < residual_budget[j1]) &&
								(solc->clients[i2].revenue > 0) &&
								(((*sum_profit) - revenue[i1][j1] + revenue[i2][j1]) >=
								((1 + hurdle)*((*sum_cost_fix) + cost[i2][j1] - cost[i1][j1]))))
							{
								//Se o lucro da melhor solucao corrente e menor que o lucro provido pelo movimento e o movimento nao e tabu, considerar esse movimento como candidato para melhor movimento
								if ((best_solution < ((revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]))))
								{
									aspiration_criteria = 1;
								}
								//Se foi encontrado um candidato a melhor solucao
								if (aspiration_criteria == 1)
								{
									//Salva movimento (troca a oferta de um produto entre dois clientes), movimento do tipo 1
									best_i1 = i1;
									best_i2 = i2;
									best_j1 = j1;
									best_move = 1;
									//guarda valor (lucro) do melhor movimento atual
									best_solution = (revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]);
									//Se o movimento resultou em um lucro maior e o resultado total da solucao atual (tabu_cost+best_solution) e melhor que a melhor solucao encontrada ate o momento (incumbent_solution)
									if ((best_solution > 0 && (tabu_cost + best_solution) > incumbent_solution))
									{
										//Assinala que o movimento foi incorporado a solucao
										flag_move1 = 1;
										//Realiza movimento
										//Poe o client i1 na lista tabu e retira o cliente i2 (removido eh considerado tabu)
										//Reajusta orcamento residual
										residual_budget[j1] -= (cost[i2][j1] - cost[i1][j1]);
										//Calcula novo custo da solucao atual
										tabu_cost += best_solution;
										//Marca a solucao atual como melhor solucao encontrada
										incumbent_solution = tabu_cost;
										//Transfere a solucao tabu para solucao definitiva
										solc->clients[i1].offers[j1] = 0;
										solc->clients[i1].offerSlots += 1;
										solc->clients[i1].cost -= cost[i1][j1];
										solc->clients[i1].revenue -= revenue[i1][j1];
										if (solc->clients[i1].revenue > 0)
											solc->clients[i1].npp = solc->clients[i1].revenue / solc->clients[i1].cost;
										else
											solc->clients[i1].npp = 0;
										solc->clients[i2].offers[j1] = 1;
										solc->clients[i2].offerSlots -= 1;
										solc->clients[i2].cost += cost[i2][j1];
										solc->clients[i2].revenue += revenue[i2][j1];
										if (solc->clients[i2].revenue > 0)
											solc->clients[i2].npp = solc->clients[i2].revenue / solc->clients[i2].cost;
										else
											solc->clients[i2].npp = 0;

										solc->budget[j1] = solc->budget[j1] + cost[i1][j1] - cost[i2][j1];
										solc->cost += cost[i2][j1] - cost[i1][j1];
										solc->revenue += revenue[i2][j1] - revenue[i1][j1];
										solc->fitness = solc->revenue - solc->cost;
										//Ajusta parametros da solucao definitiva e bota o movimento na lista tabu com um timer (iteracao atual + quantas iteracoes o movimento ficara como tabu)
										*solution_cost = tabu_cost;
										*sum_profit = *sum_profit - revenue[i1][j1] + revenue[i2][j1];
										*sum_cost_fix = *sum_cost_fix - cost[i1][j1] + cost[i2][j1];
										//Reseta o loop de pois foi achada uma melhora
										/*printf("sol. Incumbente MOVE 1 = %f\n",incumbent_solution);*/
									}
								}
							}
						}
					}
				}
			}
		}


		for (j1 = 0; (j1 < prod - 1 && !flag_move1 && !flag_move2); j1++) /* loop for the neighborhood of swap,  i.e., if prod[j1] and prod[j2] is active and */
																		  /*    customer[i1][j1]=1 and customer[i2][j1]=0  and customer[i1][j2]=0 and customer[i2][j2]=1*/
																		  /* do customer[i1][j1]=0 and customer[i2][j1]=1  and customer[i1][j2]=1 and customer[i2][j2]=0 that improves the solution value */
		{
			//Se o produto j1 esta aberto
			if (solc->offeredProducts[j1] != 0)
			{
				for (j2 = 0; (j2 < prod && !flag_move2); j2++)
				{
					//Se o produto j2 esta aberto
					if ((solc->offeredProducts[j2] != 0) && (j1 != j2))
					{
						for (i1 = 0; (i1 < client - 1 && !flag_move2); i1++)
						{
							for (i2 = 0; (i2 < client && !flag_move2); i2++)
							{
								//Se o produto j1 esta sendo ofertado para o cliente i1 e nao esta sendo ofertado para o cliente i2
								//E o produto j2 nao esta sendo ofertado para o cliente i1 e esta sendo ofertado para o cliente i2
								if ((solc->clients[i1].offers[j1] == 1) && (solc->clients[i2].offers[j1] == 0) &&
									(solc->clients[i1].offers[j2] == 0) && (solc->clients[i2].offers[j2] == 1))
								{
									//Se a troca das duas ofertas esta dentro do orcamento e estas trocas respeitam o hurdle rate
									if (((cost[i2][j1] - cost[i1][j1]) < residual_budget[j1]) &&
										((cost[i1][j2] - cost[i2][j2]) < residual_budget[j2]) &&
										(((*sum_profit) - revenue[i1][j1] + revenue[i2][j1] + revenue[i1][j2] - revenue[i2][j2]) >=
										((1 + hurdle)*((*sum_cost_fix) - cost[i1][j1] + cost[i2][j1] + cost[i1][j2] - cost[i2][j2]))))
									{
										//Se o lucro gerado pelo movimento e maior que o lucro gerado pelo melhor movimento encontrado ate agora
										//Marca que um melhor movimento foi achado
										if ((best_solution < (((revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]))
											+ ((revenue[i1][j2] - cost[i1][j2]) - (revenue[i2][j2] - cost[i2][j2])))))
										{
											aspiration_criteria = 1;
										}
										else
										{
											//Caso o movimento esteja marcado como tabu mas provenha uma solucao melhor que a melhor solucao achada
											if ((tabu_cost + (((revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]))
												+ ((revenue[i1][j2] - cost[i1][j2]) - (revenue[i2][j2] - cost[i2][j2])))) > incumbent_solution)
												aspiration_criteria = 1;
											else aspiration_criteria = 0;
										}
										//Se um movimento melhor foi encontrado
										if (aspiration_criteria == 1)
										{
											//Marca o movimento e o tipo do movimento (movimento do tipo 2)
											best_i1 = i1;
											best_i2 = i2;
											best_j1 = j1;
											best_j2 = j2;
											best_move = 2;
											//Ajusta o custo do melhor movimento
											best_solution = ((revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]))
												+ ((revenue[i1][j2] - cost[i1][j2]) - (revenue[i2][j2] - cost[i2][j2]));
											//Se o movimento gera uma solucao melhor que a melhor solucao ja encontrada
											if ((best_solution > 0 && (tabu_cost + best_solution) > incumbent_solution))
											{
												//Marca que foi realizado um movimento do tipo 2 na solucao atual
												flag_move2 = 1;
												//Realiza o movimento na solucao atual
												/*tabu_solc[i1][j1] = 0;
												tabu_solc[i2][j1] = 1;
												tabu_solc[i1][j2] = 1;
												tabu_solc[i2][j2] = 0;*/
												//Ajusta o orcamento
												residual_budget[j1] -= (cost[i2][j1] - cost[i1][j1]);
												residual_budget[j2] -= (cost[i1][j2] - cost[i2][j2]);
												//Ajusta o lucro e marca o novo custo da melhor solucao jah encontrada
												tabu_cost += best_solution;
												printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
												incumbent_solution = tabu_cost;
												//Copia a solucao para solucao definitiva
												  /*    customer[i1][j1]=1 and customer[i2][j1]=0  and customer[i1][j2]=0 and customer[i2][j2]=1*/
												 /* do customer[i1][j1]=0 and customer[i2][j1]=1  and customer[i1][j2]=1 and customer[i2][j2]=0 that improves the solution value */
												solc->clients[i1].offers[j1] = 0;
												solc->clients[i1].offerSlots += 1;
												solc->clients[i1].cost -= cost[i1][j1];
												solc->clients[i1].revenue -= revenue[i1][j1];
												if (solc->clients[i1].revenue)
													solc->clients[i1].npp = solc->clients[i1].revenue / solc->clients[i1].cost;
												else
													solc->clients[i1].npp = 0;
												solc->clients[i2].offers[j2] = 0;
												solc->clients[i2].offerSlots += 1;
												solc->clients[i2].cost -= cost[i2][j2];
												solc->clients[i2].revenue -= revenue[i1][j2];
												if (solc->clients[i2].revenue)
													solc->clients[i2].npp = solc->clients[i2].revenue / solc->clients[i2].cost;
												else
													solc->clients[i2].npp = 0;

												solc->clients[i2].offers[j1] = 1;
												solc->clients[i2].offerSlots -= 1;
												solc->clients[i2].cost += cost[i2][j1];
												solc->clients[i2].revenue += revenue[i2][j1];
												if (solc->clients[i2].revenue)
													solc->clients[i2].npp = solc->clients[i2].revenue / solc->clients[i2].cost;
												else
													solc->clients[i2].npp = 0;
												solc->clients[i1].offers[j2] = 1;
												solc->clients[i1].offerSlots -= 1;
												solc->clients[i1].cost += cost[i1][j2];
												solc->clients[i1].revenue += revenue[i1][j2];
												if (solc->clients[i1].revenue)
													solc->clients[i1].npp = solc->clients[i1].revenue / solc->clients[i1].cost;
												else
													solc->clients[i1].npp = 0;

												solc->budget[j1] += cost[i1][j1] - cost[i2][j1];
												solc->budget[j2] += cost[i2][j2] - cost[i1][j2];


												*solution_cost = tabu_cost;
												//Insere o movimento na lista tabu
												*sum_profit = (*sum_profit) - revenue[i1][j1] + revenue[i2][j1] + revenue[i1][j2] - revenue[i2][j2];
												*sum_cost_fix = (*sum_cost_fix) - cost[i1][j1] + cost[i2][j1] + cost[i1][j2] - cost[i2][j2];
												//Reseta o loop porque uma melhor foi encontrada
												best_solution = -10000;
												iter_no_improvement = 1;
												/* printf("sol. Incumbente MOVE 2 = %f\n",incumbent_solution);*/
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		for (j1 = 0; (j1 < prod - 1 && (!flag_move1 && !flag_move2 && !flag_move3)); j1++) /* loop for the neighborhood of swap,  i.e., if prod[j] is active and customer[i1][j]=1 or customer[i2][j]=1 */
																						   /* do customer[i1][j]=customer[i2][j] and customer[i2][j]=customer[i1][j] that improves the solution value */
		{
			//Se o produto j1 esta aberto
			if (solc->offeredProducts[j1] != 0)
			{
				for (j2 = j1; (j2 < prod && !flag_move3); j2++)
				{
					//Se o produto j2 esta aberto e j1 e j2 sao diferentes
					if ((solc->offeredProducts[j2] != 0) && (j1 != j2))
					{
						for (i1 = 0; (i1 < client && !flag_move3); i1++)
						{
							for (i2 = 0; (i2 < client && !flag_move3); i2++)
							{
								//Se o produto i1 esta sendo ofertado para o cliente i1 e nao esta sendo ofertado para o cliente i2
								//E se o numero de ofertas  realizadas para o produto j1 menos uma ainda atendo o numero de ofertas minimas para o produto j1
								//E se o cliente i2 esta ativo (ainda pode receber ofertas)
								//E se o movimento de 
								if ((solc->clients[i1].offers[j1] == 1) && (solc->clients[i2].offers[j2] == 0) && ((client_po[j1] - 1) >= mcpo[j1]) &&
									(solc->clients[i2].revenue > 0) &&
									(((*sum_profit) - revenue[i1][j1] + revenue[i2][j2]) >=//Este calculo parece estar errado pois se a ideia eh igualar as oferas entao, ou as duas ofertas estao sendo removidas ou estao sendo adicionadas, nunca uma removida e outra adicionada
									((1 + hurdle)*((*sum_cost_fix) - cost[i1][j1] + cost[i2][j2]))))
								{
									if (cost[i2][j2] <= residual_budget[j2])
									{
										if ((best_solution < ((revenue[i2][j2] - cost[i2][j2]) - (revenue[i1][j1] - cost[i1][j1]))))
										{
											aspiration_criteria = 1;

										}
										else
										{
											if ((tabu_cost + ((revenue[i2][j2] - cost[i2][j2]) - (revenue[i1][j1] - cost[i1][j1])))
										> incumbent_solution)
												aspiration_criteria = 1;
											else aspiration_criteria = 0;
										}

										if (aspiration_criteria == 1)
										{
											best_i1 = i1;
											best_i2 = i2;
											best_j1 = j1;
											best_j2 = j2;
											best_move = 3;
											best_solution = ((revenue[i2][j2] - cost[i2][j2]) - (revenue[i1][j1] - cost[i1][j1]));
											if ((best_solution > 0 && (tabu_cost + best_solution) > incumbent_solution))
											{
												flag_move3 = 1;
												client_po[j1]--;
												client_po[j2] ++;
												residual_budget[j1] += cost[i1][j1];
												residual_budget[j2] -= cost[i2][j2];
												tabu_cost += best_solution;
												printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
												incumbent_solution = tabu_cost;

												solc->clients[i1].offers[j1] = 0;
												solc->clients[i1].offerSlots += 1;
												solc->clients[i1].cost -= cost[i1][j1];
												solc->clients[i1].revenue -= revenue[i1][j1];
												if (solc->clients[i1].revenue)
													solc->clients[i1].npp = solc->clients[i1].revenue / solc->clients[i1].cost;
												else
													solc->clients[i1].npp = 0;


												solc->clients[i2].offers[j2] = 1;
												solc->clients[i2].offerSlots -= 1;
												solc->clients[i2].cost += cost[i2][j2];
												solc->clients[i2].revenue += revenue[i2][j2];
												if (solc->clients[i2].revenue)
													solc->clients[i2].npp = solc->clients[i2].revenue / solc->clients[i2].cost;
												else
													solc->clients[i2].npp = 0;

												solc->budget[j1] += cost[i1][j1];
												solc->budget[j2] -= cost[i2][j2];

												*solution_cost = tabu_cost;
												iter_no_improvement = 1;
												best_solution = -10000;
												*sum_profit = (*sum_profit) - revenue[i1][j1] + revenue[i2][j2];
												*sum_cost_fix = (*sum_cost_fix) - cost[i1][j1] + cost[i2][j2];
												/*printf("sol. Incumbente MOVE3a = %f\n",incumbent_solution);*/
											}
										}
									}
								}
								else
								{
									if ((solc->clients[i1].offers[j1] == 0) && (solc->clients[i2].offers[j2] == 1) && ((client_po[j2] - 1) >= mcpo[j2]) &&
										(solc->clients[i1].revenue > 0) &&
										(((*sum_profit) + revenue[i1][j1] - revenue[i2][j2]) >=
										((1 + hurdle)*((*sum_cost_fix) + cost[i1][j1] - cost[i2][j2]))))
									{
										if (cost[i1][j1] <= residual_budget[j1])
										{
											if ((best_solution < ((revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j2] - cost[i2][j2]))))
											{
												aspiration_criteria = 1;
											}
											else
											{
												if ((tabu_cost + ((revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j2] - cost[i2][j2])))
											> incumbent_solution)
													aspiration_criteria = 1;
												else aspiration_criteria = 0;
											}

											if (aspiration_criteria == 1)
											{
												best_i1 = i1;
												best_i2 = i2;
												best_j1 = j1;
												best_j2 = j2;
												best_move = 4;
												best_solution = ((revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j2] - cost[i2][j2]));
												if ((best_solution > 0 && (tabu_cost + best_solution) > incumbent_solution))
												{
													flag_move3 = 1;
													/*tabu_solc[i1][j1] = 1;
													tabu_solc[i2][j2] = 0;*/
													client_po[j1]++;
													client_po[j2]--;
													residual_budget[j1] -= cost[i1][j1];
													residual_budget[j2] += cost[i2][j2];
													tabu_cost += best_solution;

													solc->clients[i2].offers[j2] = 0;
													solc->clients[i2].offerSlots += 1;
													solc->clients[i2].cost -= cost[i2][j2];
													solc->clients[i2].revenue -= revenue[i2][j2];
													if (solc->clients[i2].revenue)
														solc->clients[i2].npp = solc->clients[i2].revenue / solc->clients[i2].cost;
													else
														solc->clients[i2].npp = 0;


													solc->clients[i1].offers[j1] = 1;
													solc->clients[i1].offerSlots -= 1;
													solc->clients[i1].cost += cost[i1][j1];
													solc->clients[i1].revenue += revenue[i1][j1];
													if (solc->clients[i1].revenue)
														solc->clients[i1].npp = solc->clients[i1].revenue / solc->clients[i1].cost;
													else
														solc->clients[i1].npp = 0;

													solc->budget[j1] -= cost[i1][j1];
													solc->budget[j2] += cost[i2][j2];


													printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
													incumbent_solution = tabu_cost;
													*solution_cost = tabu_cost;
													iter_no_improvement = 1;
													best_solution = -10000;
													*sum_profit = (*sum_profit) + revenue[i1][j1] - revenue[i2][j2];
													*sum_cost_fix = (*sum_cost_fix) + cost[i1][j1] - cost[i2][j2];
													/*printf("sol. Incumbente MOVE3b = %f\n",incumbent_solution);*/
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}





		
		if (previous_solution < tabu_cost)
		{
			previous_solution = tabu_cost;
			iter_no_diversification = 1;
		}
		else
		{
			previous_solution = tabu_cost;
			iter_no_diversification++;
		}



		iter++;
		iter_no_improvement++;
		flag_move1 = 0;
		flag_move2 = 0;
		flag_move3 = 0;
		flag_move4 = 0;
		end_phase = clock();
		time_elapsed += (end_phase - begin_phase) / CLOCKS_PER_SEC;
		//printf("Time_elapsed=%f\n",time_elapsed);
		begin_phase = clock();

	}  /* while (iter_no_improvement < 100) */

	for (int i = 0; i < client; i++) {
		delete sol_aux[i];
	}
	delete sol_aux;
	delete sum_revenue;
	delete sum_cost;
	delete used_product;
	delete residual_budget;
	delete client_po;
	/*    printf("sol. Incumbente = %f\n",incumbent_solution);
	for (j=0;j<prod;j++) printf("client po [%d] = %d\n",j,client_po[j]);*/
}

/*Applies crossover operator with canibalism taken into consideration if the cannibalism flag is true. If the mutation flag is enabled it will prevent offers contained in the mutation_matrix from being included on the offspring.
The product exploration order on the genes is governed by the prod_order vector, which has the product indexes ordered by profitability. The order in which the genes are explored are governed by the client_order vector that follows the same logic as the prod_order vector.*/
int crossover_CAN(cromo *p1, cromo *p2, cromo *n, int prod, int cli, int* mcpo, int totalOffers, int** revenue, int** cost, int** profit, list **sorted_npp, int *maxOffer, int *oBudget, float hurdle, float penaltie, int *fcpo, int *client_order, int* prod_order, int* prod_can, bool mutation)
{
	bool flag = true;
	bool moffer = false;
	bool can = false;
	bool ignored_prod[MAXPROD];
	//int can_prods[MAXPROD];
	int cont = 0, i, j, k;
	int cont2;
	int offerCount;
	int minimum;
	int maximum;
	int index;
	float hurdle1;
	int profit1, profit2;
	int offers[MAXPROD], offers2[MAXPROD];
	int offercounter2[MAXPROD];
	int costcounter;
	int can_qt = 0;
	//int *selectProd = new int[prod];
	//int *bestGenes = new int[cli];
	//int *notSelected = new int[prod];
	bool bestGenes[MAXC];
	int cli_rev, cli_cost, sol_rev, sol_cost;
	list *aux, *aux2;
	list2 *aux_list;
	int random;
	int offercounter;
	int prod_rev[MAXPROD], prod_rev2[MAXPROD];
	int b1, b2;
	int can_list[MAXPROD];



	n->revenue = 0;
	n->cost = 0;
	n->fitness = 0;
	for (i = 0; i < cli; i++)
	{
		n->clients[i].npp = 0;
		n->clients[i].revenue = 0;
		n->clients[i].cost = 0;
		n->clients[i].offerSlots = maxOffer[i];
		for (j = 0; j < prod; j++)
			n->clients[i].offers[j] = 0;
	}
	for (j = 0; j < prod; j++)
	{
		n->budget[j] = oBudget[j];
		offers[j] = 0;
		prod_rev[j] = 0;
		can_list[j] = -100;
	}


	for (i = 0, j = 0; i < prod; i++)
	{
		if (p1->fitness > p2->fitness)
		{
			if (p1->offeredProducts[i])
			{
				if ((prod_can[i] >= 0) && (p2->offeredProducts[prod_can[i]]))
				{
					can = true;
					flag_can = true;
					/*can_prods[j] = i;
					can_prods[j + 1] = prod_can[i];*/
					if (mutation_vec[j])
						offers[prod_can[i]] = -1;
					else

						j += 1;
				}
			}
		}
		else
		{
			if (p2->offeredProducts[i])
			{
				if ((prod_can[i] >= 0) && (p1->offeredProducts[prod_can[i]]))
				{
					can = true;
					flag_can = true;
					/*can_prods[j] = i;
					can_prods[j + 1] = prod_can[i];*/
					if (mutation_vec[j])
						offers[prod_can[i]] = -1;
					else
						offers[i] = -1;
					j += 1;
				}
			}
		}
	}

	can_qt = j;

	for (j = 0, i = 0; j < prod; j++)
	{
		if (p1->offeredProducts[j] && (prod_can[j] >= 0) && (p2->offeredProducts[prod_can[j]]))
		{
			can = true;
			can_list[i] = j;
			can_list[i + 1] = prod_can[j];
			i += 2;
		}

	}

	offerCount = totalOffers;
	random = rand() % cli;
	cont = 0;
	//Inconsistencia nos custos
	cont2 = 0;
	int cont3 = 0;
	//aux_list = client_order;
	flag = true;

	k = 0;
	/*Phase 1: Add offers contained on the most profitable genes.*/
	//for each client (gene)
	while (k < cli)
	{
		cont = client_order[k];
		//checks each clients offer set and compares the profit of the feasible offers
		for (i = 0, profit1 = 0, profit2 = 0, cont2 = 0, cont3 = 0; i < prod; i++)
		{

			if (p1->clients[cont].offers[i] && offers[i] + offerCount >= mcpo[i] && n->budget[i] >= cost[cont][i] && offers[i] >= 0 && (!mutation_matrix[cont][i] || !mutation))
			{
				if (!can)
					profit1 += profit[cont][i];
				else
				{
					if (offers[i] >= 0)
						profit1 += profit[cont][i];
				}
				cont2++;
			}
			if (p2->clients[cont].offers[i] && offers[i] + offerCount >= mcpo[i] && n->budget[i] >= cost[cont][i] && offers[i] >= 0 && (!mutation_matrix[cont][i] || !mutation))
			{
				if (!can)
					profit2 += profit[cont][i];
				else
				{
					if (offers[i] >= 0)
						profit2 += profit[cont][i];
				}
				cont3++;
			}
		}
		if (cont2 > maxOffer[cont] || cont3 > maxOffer[cont])
		{
			printf("Produto %d teve limite estourado %d/%d \n", cont, cont2, cont3);
		}
		n->clients[cont].npp = 0;
		n->clients[cont].revenue = 0;
		n->clients[cont].cost = 0;
		n->clients[cont].offerSlots = maxOffer[cont];
		for (j = 0; j < prod; j++)
			n->clients[cont].offers[j] = 0;
		//Adds the feasible offers from the winning gene to the new chromosome 
		if (profit1 > profit2) //Adds offers from gene 1
		{

			for (i = 0, cont2 = 0, moffer = false; i < prod; i++)
			{
				//If the product was offered
				if (offers[i] >= 0)
				{
					//If it is still possible for the product to meet its offer quota and if there is enough budget for the offer. Inser the offer.
					if (p1->clients[cont].offers[i] > 0 && offers[i] + offerCount >= mcpo[i] && n->budget[i] >= cost[cont][i] && (!mutation_matrix[cont][i] || !mutation))
					{
						offerCount--;
						offers[i]++;
						n->clients[cont].offers[i] = 1;
						n->budget[i] -= cost[cont][i];
						n->revenue += revenue[cont][i];
						n->cost += cost[cont][i];
						n->clients[cont].revenue += revenue[cont][i];
						n->clients[cont].cost += cost[cont][i];
						n->clients[cont].offerSlots--;

						cont2++;
					}
					else//Otherwise, if the product can no longer meet its quota. Remove all offers of the product from the solution and mark this product as unfeasible.
					{

						if (p1->clients[cont].offers[i])
						{
							n->clients[cont].offers[i] = 0;

							if (offers[i] + offerCount < mcpo[i])
							{
								for (j = 0, index = -1, maximum = 0; j < prod; j++)//Varre a solucao e remove as ofertas previamente feitas
								{
									if (offers[j] > 0)
									{
										if (mcpo[j] - offers[j] > maximum && offers[j] > 0)
										{
											maximum = mcpo[j] - offers[j];
											index = j;
										}
									}
								}

								if (index > -1)
								{
									offerCount += offers[index];
									offers[index] = -1;
								}
							}

						}

					}
				}
				else
				{
					n->clients[cont].offers[i] = 0;
				}
			}

			bestGenes[cont] = false;
		}
		else//adds offers from gene 2
		{

			for (i = 0, moffer = false; i < prod; i++)
			{
				if (offers[i] >= 0)
				{
					//If it is still possible for the product to meet its offer quota and if there is enough budget for the offer. Inser the offer.
					if (p2->clients[cont].offers[i] && offers[i] + offerCount >= mcpo[i] && n->budget[i] >= cost[cont][i] && (!mutation_matrix[cont][i] || !mutation))
					{
						offerCount--;
						offers[i]++;
						n->clients[cont].offers[i] = 1;
						n->budget[i] -= cost[cont][i];
						n->revenue += revenue[cont][i];
						n->cost += cost[cont][i];
						n->clients[cont].revenue += revenue[cont][i];
						n->clients[cont].cost += cost[cont][i];
						n->clients[cont].offerSlots--;

					}
					else//Otherwise, if the product can no longer meet its quota. Remove all offers of the product from the solution and mark this product as unfeasible.
					{
						if (p2->clients[cont].offers[i])
						{
							n->clients[cont].offers[i] = 0;

							if (offers[i] + offerCount < mcpo[i])
							{

								for (j = 0, index = -1, maximum = 0; j < prod; j++)
								{
									if (offers[j] > 0)
									{
										if (mcpo[j] - offers[j] > maximum && offers[j] > 0)
										{
											maximum = mcpo[j] - offers[j];
											index = j;
										}
									}
								}

								if (index > -1)
								{
									offerCount += offers[index];
									offers[index] = -1;
								}
							}

						}

					}
				}
				else
				{
					n->clients[cont].offers[i] = 0;
				}
			}

			bestGenes[cont] = true;
		}
		if (!flag)
			flag = true;
		k++;
	}


	if (n->revenue <= 0)
		printf("Lucro inconsistente para solucao.\n");

	i = 0;

	for (i = 0; i < prod; i++)
	{
		if (offers[i] > 0)
			n->offeredProducts[i] = true;
		else
			n->offeredProducts[i] = false;
	}
	for (j = 0, cli_cost = 0, cli_rev = 0; j < prod; j++)
	{
		offers2[j] = 0;
		offercounter2[j] = 0;
	}
	

	/*Phase 2: Repairs the soultion resulting from phase 1.
	-Verifies if every product offered meet its quota.
	-Attempts to add offers for products that did not meet their quota. If it fails, remove the product from the solution.*/
	
	//Repairs each product offer set into feasibility. If it is not possible, removes the product from the solution.
	for (i = 0; i < prod; i++)
	{
		if (offers[i] <= 0)
		{
			n->budget[i] = oBudget[i];
			n->offeredProducts[i] = false;
			for (j = 0; j < cli; j++)
			{
				if (n->clients[j].offers[i] > 0)
				{
					n->clients[j].offers[i] = 0;
					n->clients[j].cost -= cost[j][i];
					n->cost -= cost[j][i];
					n->revenue -= revenue[j][i];
					n->clients[j].revenue -= revenue[j][i];
					n->clients[j].offerSlots += 1;
				}
			}

		}
	}
	for (i = 0; i < prod; i++)
	{
		if (offers[i] < mcpo[i] && offers[i] > 0)
		{
			for (aux = sorted_npp[i]->prox; aux && offers[i] < mcpo[i]; aux = aux->prox)
			{
				if (n->clients[aux->inf].offerSlots > 0 && n->budget[i] >= cost[aux->inf][i] && n->clients[aux->inf].offers[i] == 0 && (!mutation_matrix[aux->inf][i] || !mutation))
				{
					n->clients[aux->inf].offerSlots -= 1;
					n->budget[i] -= cost[aux->inf][i];
					n->cost += cost[aux->inf][i];
					n->revenue += revenue[aux->inf][i];
					n->clients[aux->inf].cost += cost[aux->inf][i];
					n->clients[aux->inf].revenue += revenue[aux->inf][i];
					n->clients[aux->inf].offers[i] = 1;
					offers[i]++;
				}
			}
			if (offers[i] < mcpo[i])
			{
				offers[i] = -1;
				n->budget[i] = oBudget[i];
				n->offeredProducts[i] = false;
				if (offers[i] < 0)
				{
					for (j = 0; j < cli; j++)
					{
						if (n->clients[j].offers[i] > 0)
						{
							n->clients[j].cost -= cost[j][i];
							n->cost -= cost[j][i];
							n->revenue -= revenue[j][i];
							n->clients[j].revenue -= revenue[j][i];
							n->clients[j].offerSlots += 1;
						}
						n->clients[j].offers[i] = 0;
					}

				}
			}
		}
	}

	/*Solution consistency and feasibility verification*/
	for (i = 0; i < prod; i++)
	{
		offers2[i] = 0;
	}
	for (i = 0, sol_cost = 0, sol_rev = 0; i < cli; i++)
	{
		sol_cost += n->clients[i].cost;
		sol_rev += n->clients[i].revenue;
		if (n->clients[i].offerSlots < 0)
			printf("Estourou offerslots\n");
		for (j = 0, offercounter = 0, cli_cost = 0, cli_rev = 0; j < prod; j++)
		{
			if (n->clients[i].offers[j] > 0)
			{
				cli_cost += cost[i][j];
				cli_rev += revenue[i][j];
				offers2[j] += cost[i][j];
				if (offers2[j] < 0)
					printf("Ofertado produto fora da solucao.\n");
				//offers[j] += cost[i][j];
				offercounter++;
				//printf("Prod: %d  Revenue: %d  Cost: %d\n", j, revenue[i][j], cost[i][j]);
			}
		}
		if (offercounter > maxOffer[i])
			printf("Inconsistenciaa na contagem de ofertas.\n");
		if (cli_cost != n->clients[i].cost)
			printf("Custo inconsistente para cliente.\n");
		if (cli_rev != n->clients[i].revenue)
			printf("Lucro inconsistente para cliente.\n");
	}
	if (sol_rev != n->revenue)
		printf("Lucro inconsistente para solucao.\n");
	for (j = 0, cli_cost = 0, cli_rev = 0; j < prod; j++)
	{
		if (offers2[j] > oBudget[j])
		{
			printf("Estourou orcamento. Budget do produto %d./n", n->budget[j]);
		}
		/*if (offercounter2[j] < mcpo[j] && offercounter2[j]>0)
		{
			printf("Nao atingiu min de ofertas.\n");
		}*/
	}

	/*Phase 3: Attempts to insert additional offers for viable products on the solution generated by phase 2.
	-Attempts to insert offers from the gene that lost the competition on phase 1.
	-Attempts to insert offers on order of profitability.*/
	
	//Adds offers from the losing gene on phase 1.
	for (i = 0; i < cli; i++)
	{
		if (!bestGenes[client_order[i]])
		{
			for (j = 0; j < prod; j++)
			{
				if (n->offeredProducts[j])
				{
					if (p2->clients[client_order[i]].offers[j] && n->clients[client_order[i]].offers[j] == 0 && cost[client_order[i]][j] <= n->budget[j] && n->clients[client_order[i]].offerSlots > 0 && (!mutation_matrix[client_order[i]][j] || !mutation))
					{
						n->clients[client_order[i]].offers[j] = 1;
						n->clients[client_order[i]].offerSlots -= 1;
						n->clients[client_order[i]].cost += cost[client_order[i]][j];
						n->cost += cost[client_order[i]][j];
						n->budget[j] -= cost[client_order[i]][j];
						n->clients[client_order[i]].revenue += revenue[client_order[i]][j];
						n->revenue += revenue[client_order[i]][j];
						offers[j]++;
					}
				}
			}
		}
		else
		{
			for (j = 0; j < prod; j++)
			{
				if (n->offeredProducts[j])
				{
					if (p1->clients[client_order[i]].offers[j] && n->clients[client_order[i]].offers[j] == 0 && cost[client_order[i]][j] <= n->budget[j] && n->clients[client_order[i]].offerSlots > 0 && (!mutation_matrix[client_order[i]][j] || !mutation))
					{
						n->clients[client_order[i]].offers[j] = 1;
						n->clients[client_order[i]].offerSlots -= 1;
						n->clients[client_order[i]].cost += cost[client_order[i]][j];
						n->cost += cost[client_order[i]][j];
						n->budget[j] -= cost[client_order[i]][j];
						n->clients[client_order[i]].revenue += revenue[client_order[i]][j];
						n->revenue += revenue[client_order[i]][j];
						offers[j]++;
					}
				}
			}
		}
	}
	//Adds any other lucrative feasible offer
	for (i = 0; i < prod; i++)//desalocar prod order entre instancias com numero diferentes de produtos produtos.
	{
		j = prod_order[i];
		if (j >= prod || j < 0)
			printf("Erro.\n");
		if (n->offeredProducts[j])
		{
			for (aux = sorted_npp[j]->prox; aux; aux = aux->prox)
			{
				if ((!mutation_matrix[aux->inf][j] || !mutation) && (revenue[aux->inf][j] > cost[aux->inf][j]) && (cost[aux->inf][j] <= n->budget[j]) &&
					(n->clients[aux->inf].offerSlots > 0) && (n->clients[aux->inf].offers[j] == 0) &&
					((n->revenue + revenue[aux->inf][j]) >= ((1 + hurdle)*(n->cost + cost[aux->inf][j]))))
				{
					n->budget[j] -= cost[aux->inf][j];
					n->cost += cost[aux->inf][j];
					n->clients[aux->inf].cost += cost[aux->inf][j];
					n->clients[aux->inf].offerSlots -= 1;
					n->clients[aux->inf].offers[j] = 1;
					n->revenue += revenue[aux->inf][j];
					n->clients[aux->inf].revenue += revenue[aux->inf][j];
					offers[j]++;
				}
				else
				{
					if ((cost[aux->inf][j] > n->budget[j]))
					{
						break;
					}
				}
			}

		}
	}
	
	//Calculates fitness and checks correctness of the solution.
	for (i = 0; i < cli; i++)
	{
		if (n->clients[i].revenue)
			n->clients[i].npp = n->clients[i].revenue / n->clients[i].cost;
		else
			n->clients[i].npp = 0;
	}


	for (i = 0; i < prod; i++)
	{
		if (n->offeredProducts[i] > 0)
		{
			n->cost += fcpo[i];
		}
	}
	n->fitness = n->revenue - n->cost;

	if (n->fitness == 0)
	{
		
		return can_qt;
	}

	hurdle1 = n->revenue / n->cost;
	
	for (j = 0, cli_cost = 0, cli_rev = 0; j < prod; j++)
	{
		offers[j] = 0;
		offercounter2[j] = 0;
		offers2[j] = 0;
	}
	for (i = 0, sol_cost = 0, sol_rev = 0; i < cli; i++)
	{

		if (n->clients[i].offerSlots < 0)
			printf("Estourou offerslots\n");
		for (j = 0, offercounter = 0, cli_cost = 0, cli_rev = 0; j < prod; j++)
		{
			if (n->clients[i].offers[j])
			{
				cli_cost += cost[i][j];
				cli_rev += revenue[i][j];
				offers[j] += cost[i][j];
				offers2[j] += cost[i][j];
				offercounter2[j]++;
				if (!(n->offeredProducts[j]))
					printf("Ofertado produto fora da solucao.\n");
				offercounter++;
				//printf("Prod: %d  Revenue: %d  Cost: %d\n", j, revenue[i][j], cost[i][j]);
			}
		}
		if (offercounter > maxOffer[i])
			printf("Inconsistenciaa na contagem de ofertas.\n");
		if (cli_cost != n->clients[i].cost)
			printf("Custo inconsistente para cliente.\n");
		if (cli_rev != n->clients[i].revenue)
			printf("Lucro inconsistente para cliente.\n");
		sol_cost += cli_cost;
		sol_rev += cli_rev;
	}
	if (sol_rev != n->revenue)
		printf("Lucro inconsistente para solucao.\n");
	for (j = 0, cli_cost = 0, cli_rev = 0; j < prod; j++)
	{
		if (offers[j] > oBudget[j])
		{
			printf("Estourou orcamento.\n");
		}
		if (offercounter2[j] < mcpo[j] && offercounter2[j]>0)
		{
			printf("Nao atingiu min de ofertas.\n");
		}
	}
	//offers sendo corrompido.
	for (j = 0, costcounter = 0; j < prod; j++)
	{
		if (offers[j] > 0)
		{
			costcounter += offers[j] + fcpo[j];
			sol_cost += fcpo[j];
		}
	}
	if (n->fitness != sol_rev - sol_cost)
		printf("Sol Nula\n");
	if (costcounter != n->cost)
		printf("Descrepancia no custo.\n");

	i = 0;

	check_values(n, fcpo, cost, revenue, n->cost, n->revenue, n->fitness, prod, cli);
	return can_qt;
}
//Creates a new population (generation) through sucessive crossovers. Utilizes an weighted roulette to choose both parents.
int reproduction_CAN(cromo *pop1, cromo *pop2, int prod, int cli, int* mcpo, int totalOffers, int** revenue, int** cost, int** profit, list **sorted_npp, int *maxOffer, int *oBudget, float hurdle, float penaltie, int tam, float *probability, int *fcpo, cromo *elite, int *client_order, int *prod_order, int* prod_can)
{
	int random;
	double probSum, prob;
	cromo *p1, *p2, *f;
	int best_sol = 0;
	int best_sol_index = 0;
	p1 = NULL;
	p2 = NULL;
	f = NULL;
	int can_qt = 0;
	int counter;
	bool flag = true;

	//Reproduction by weighted roulette
	for (int j = 0; j < tam; j++)
	{
		f = &pop2[j];
		p1 = NULL;
		random = rand() % 1000000;
		prob = (float)(random / 1000);
		for (int i = 0, probSum = 0; i < tam; i++)
		{


			probSum = probSum + probability[i] * 1000;
			if (probSum >= prob)
			{
				if (pop1[i].fitness > 0)
				{
					p1 = &pop1[i];
					break;
				}
				else
				{
					if (i == POPSIZE - 1)
					{
						i = i - POPSIZE / 2;
					}
				}
			}

		}
		if (!p1)
			p1 = elite;
		p2 = NULL;
		random = rand() % 1000000;
		prob = (float)(random / 1000);
		for (int i = 0, flag = true, probSum = 0; i < tam; i++)
		{


			probSum += probability[i] * 1000;
			if (probSum >= prob && (pop1[i].fitness != p1->fitness && flag))
			{
				if (pop1[i].fitness > 0)
				{
					p2 = &pop1[i];
					break;
				}
				else
				{
					if (i == POPSIZE - 1)
					{
						flag = false;
						i = 0;
					}
				}
			}

		}
		if (!p2)
			p2 = elite;
		for (int ii = 0; ii < prod / 5; ii++)
			mutation_vec[ii] = true;
		counter = 0;

		can_qt = crossover_CAN(p1, p2, f, prod, cli, mcpo, totalOffers, revenue, cost, profit, sorted_npp, maxOffer, oBudget, hurdle, penaltie, fcpo, client_order, prod_order, prod_can, false);


		random = rand() % 100 + 1;
		//If the offsping is has lower fitness than its parents. It has an 25% chance to trigger an mutation crossover to replace the current offspring.
		if (random <= 25 && gen > 1 && (f->fitness < elite->fitness || (f->fitness == elite->fitness && (float)(f->revenue) / f->cost) < ((float)(elite->revenue) / elite->cost)))
		{
			//mutation by offer blocking
			mutate_matrix2(prod, cli, f->offeredProducts);
			crossover_CAN(p1, p2, f, prod, cli, mcpo, totalOffers, revenue, cost, profit, sorted_npp, maxOffer, oBudget, hurdle, penaltie, fcpo, client_order, prod_order, prod_can, true);
		}

		if (f->fitness > best_sol || (f->fitness == best_sol && ((float)f->revenue / (float)f->cost) > ((float)pop2[best_sol_index].revenue / (float)pop2[best_sol_index].cost)))
		{
			best_sol = f->fitness;
			best_sol_index = j;
		}
		else
		{
			
			if ((f->fitness < p1->fitness && f->fitness < p2->fitness) || (f->fitness == p1->fitness && f->fitness == p2->fitness) && (f->revenue / f->cost > p1->revenue / p1->cost && f->revenue / f->cost > p1->revenue / p1->cost))
			{
				best_sol = f->fitness;
				best_sol_index = j;
			}
			else
			{
				if (flag_can)
				{
					random = rand() % 100;
					if (random <= 40)
					{	//Canibalism related mutation, switch with parent will have dominant products.
						if (can_qt == 1)
							mutation_vec[0] = !mutation_vec[0];
						else
						{
							if (can_qt == 2)
							{
								random = rand() % 1;
								mutation_vec[random] = !mutation_vec[random];
							}
							else
							{
								if (can_qt == 3)
								{
									random = rand() % 2;
									mutation_vec[random] = !mutation_vec[random];
								}
							}
						}
						can_qt = crossover_CAN(p1, p2, f, prod, cli, mcpo, totalOffers, revenue, cost, profit, sorted_npp, maxOffer, oBudget, hurdle, penaltie, fcpo, client_order, prod_order, prod_can, false);
						verify_solution(f, oBudget, prod, cli, cost, revenue, mcpo, maxOffer, prod_can);
						if ((f->fitness + 0.10*((float)f->revenue / (float)f->cost) > best_sol + 0.10*((float)pop2[best_sol_index].revenue / (float)pop2[best_sol_index].cost)))
						{
							best_sol = f->fitness;
							best_sol_index = j;
						}
						else
						{
							if (f->fitness == best_sol && (float)((float)(f->revenue) / (float)(f->cost)) > (float)((float)pop1[best_sol_index].revenue) / (float)(pop1[best_sol_index].cost))
							{
								best_sol = f->fitness;
								best_sol_index = j;
							}
						}
					}
				}
			}
		}

	}

	return best_sol_index;
}

//Loop that controlls the entire genetic algorithm process, receiving an solution and its parameters and reproducing until the desired number of generations
void Genloop_CAN(cromo *p1, cromo *p2, int prod, int cli, int* mcpo, int *prod_can, int *budgetpo, int *maxOffer, int** revenue, int** cost, int** profit, list** sorted_npp, float hurdle, float penaltie, int generations, int popSize, int *fcpo, float range, float hurdleTolerance, int** solc, int *solp, int* budget, float* sum_cost, float* sum_profit, int *active_clients, int *active_prod, bool new_pop, int *client_po, bool diversification, bool alternative_const, bool initialized)
{
	int cont, i, j, i2, i1;
	bool popflag;
	cromo *elite, *last_elite;
	int elitei, last_elitei;
	int min, mini;
	int totalOffer;
	int prod_profit[MAXPROD];

	list *aux_list2 = NULL;
	float *average_npp = (float*)malloc(sizeof(float)*prod);
	float average_positive_deviation[MAXPROD];
	int sumGap;
	int max, maxi;
	float *probabilities = (float*)malloc(sizeof(float)*popSize);
	int cost_counter, rev_counter;
	cromo *pop1, *pop2;
	pop1 = p1;
	pop2 = p2;
	
	for (i = 0, totalOffer = 0; i < cli; i++)
		totalOffer += maxOffer[i];
	for (j = 0; j < prod; j++)
	{
		prod_profit[j] = 0;
		prod_order[j] = j;
	}

	//If the genetic algorithm was not initialized, creates an initial population.
	if (!initialized)
	{

		deviation = 0.05;
		list2 *aux_list, *prox;
		float profit_potential;
		for (i = 0; i < cli; i++)
		{

			for (j = 0, profit_potential = 0; j < prod; j++)
			{
				if (revenue[i][j] > cost[i][j])
				{
					profit_potential += revenue[i][j] - cost[i][j];
					prod_profit[j] += revenue[i][j] - cost[i][j];
				}
			}
			client_order_aux[i] = profit_potential;
			client_order[i] = i;
		}


		for (i = 0; i < prod; i++)
		{
			prod_order_aux[i] = prod_profit[i];
		}
		//Builds the vectors that govern the order of exploration for products and clients based on their profitability
		qsort(client_order, cli, sizeof(int), compare1g);
		qsort(prod_order, prod, sizeof(int), compare2g);


		build_population(cli, prod, hurdle,
			cost, revenue, maxOffer,
			mcpo, budgetpo, fcpo,
			pop1,
			sorted_npp, prod_can, popSize, range, hurdleTolerance, penaltie, average_npp);


		float np = 0;
		for (j = 0; j < prod; j++)
		{
			average_positive_deviation[j] = 0;
			for (i = 0, aux_list2 = sorted_npp[j]->prox; i < prod; i++, aux_list2 = aux_list2->prox)
			{
				if (cost[aux_list2->inf][j])
					np = (float)(revenue[aux_list2->inf][j] / cost[aux_list2->inf][j]);
				if (np < average_npp[j])
					break;
				average_positive_deviation[j] += (float)(np - average_npp[j]);
			}
			average_positive_deviation[j] = average_positive_deviation[j] / i;
		}

		for (i = 0, max = 0, maxi = -1, min = INT_MAX; i < popSize; i++)
		{
			if (pop1[i].fitness > max)
			{
				max = pop1[i].fitness;
				maxi = i;
				if (pop1[i].fitness == 0)
					printf("Sol Nula\n");
			}
			else if (pop1[i].fitness < min)
			{
				min = pop1[i].fitness;
				mini = i;
			}
		}

		elitei = maxi;
		/*Builds the offer profile*/
		offer_profiling(prod, cli, popSize, pop1, sorted_npp, revenue, cost, average_positive_deviation, average_npp);
		if (maxi < 0)
			elitei = 0;

		elite = &pop1[elitei];
		elite_solution = elite;

		for (i = 0; i < prod; i++)
		{
			if (solp[i] > 0)
				pop1[mini].offeredProducts[i] = true;
			else
				pop1[mini].offeredProducts[i] = false;
			pop1[mini].budget[i] = budgetpo[i];
		}
		for (i = 0, cost_counter = 0, rev_counter = 0; i < cli; i++)
		{
			pop1[mini].clients[i].cost = 0;
			pop1[mini].clients[i].npp = 0;
			pop1[mini].clients[i].revenue = 0;
			pop1[mini].clients[i].offerSlots = maxOffer[i];
			for (j = 0; j < prod; j++)
			{
				pop1[mini].clients[i].offers[j] = solc[i][j];
				if (solc[i][j])
				{
					pop1[mini].clients[i].cost += cost[i][j];
					pop1[mini].clients[i].revenue += revenue[i][j];
					pop1[mini].clients[i].offerSlots--;
					pop1[mini].budget[j] -= cost[i][j];
				}
			}
			if (pop1[mini].clients[i].revenue > 0)
			{
				pop1[mini].clients[i].npp = pop1[mini].clients[i].revenue / pop1[mini].clients[i].cost;
			}
			else
			{
				pop1[mini].clients[i].npp = 0;
			}
			cost_counter += pop1[mini].clients[i].cost;
			rev_counter += pop1[mini].clients[i].revenue;
		}
		pop1[mini].cost = *sum_cost;
		pop1[mini].revenue = *sum_profit;
		pop1[mini].fitness = pop1[mini].revenue - pop1[mini].cost;
		elite_solution = &pop1[mini];


		if (pop1[mini].fitness > max)
		{
			elitei = mini;
			elite = &pop1[mini];
		}

	}
	else //If the GA was already initialized, checks the flag for a new population creation. If it is true, generates a new population.
	{

		if (new_pop)
		{
			deviation = 0.05;
			if (!alternative_const)
				build_population(cli, prod, hurdle,
					cost, revenue, maxOffer,
					mcpo, budgetpo, fcpo,
					pop1,
					sorted_npp, prod_can, popSize, range, hurdleTolerance, penaltie, average_npp);
			else
			{
				build_population(cli, prod, hurdle,
					cost, revenue, maxOffer,
					mcpo, budgetpo, fcpo,
					pop1,
					sorted_npp, prod_can, popSize - 15, range, hurdleTolerance, penaltie, average_npp);

				alternative_constructive_algorithm_GEN(cli, prod, hurdle,
					cost, revenue, maxOffer,
					mcpo, budgetpo, fcpo,
					pop1,
					sorted_npp,
					prod_can, 15, popSize - 15);

			}

			float np = 0;
			for (j = 0; j < prod; j++)
			{
				average_positive_deviation[j] = 0;
				for (i = 0, aux_list2 = sorted_npp[j]->prox; i < prod; i++, aux_list2 = aux_list2->prox)
				{
					if (cost[aux_list2->inf][j])
						np = (float)(revenue[aux_list2->inf][j] / cost[aux_list2->inf][j]);
					if (np < average_npp[j])
						break;
					average_positive_deviation[j] += (float)(np - average_npp[j]);
				}
				average_positive_deviation[j] = average_positive_deviation[j] / i;
			}

			for (i = 0, max = 0, maxi = -1, min = INT_MAX; i < popSize; i++)
			{
				if (pop1[i].fitness > max)
				{
					max = pop1[i].fitness;
					maxi = i;
					if (pop1[i].fitness == 0)
						printf("Sol Nula\n");
				}
				else if (pop1[i].fitness < min)
				{
					min = pop1[i].fitness;
					mini = i;
				}
			}
			elitei = maxi;

			if (maxi < 0)
				elitei = 0;

			elite = &pop1[elitei];
			elite_solution = elite;
		}
		offer_profiling(prod, cli, popSize, pop1, sorted_npp, revenue, cost, average_positive_deviation, average_npp);
		for (i = 0, max = 0, maxi = -1, min = INT_MAX; i < popSize; i++)
		{
			if (pop1[i].fitness > max)
			{
				max = pop1[i].fitness;
				maxi = i;
				if (pop1[i].fitness == 0)
					printf("Sol Nula\n");
			}
			else if (pop1[i].fitness < min)
			{
				min = pop1[i].fitness;
				mini = i;
			}
		}

		elitei = maxi;
		if (maxi < 0)
			elitei = 0;

		elite = &pop1[elitei];
		elite_solution = elite;

		for (i = 0; i < prod; i++)
		{
			if (solp[i] > 0)
				pop1[mini].offeredProducts[i] = true;
			else
				pop1[mini].offeredProducts[i] = false;
			pop1[mini].budget[i] = budgetpo[i];
		}
		for (i = 0; i < cli; i++)
		{
			pop1[mini].clients[i].cost = 0;
			pop1[mini].clients[i].npp = 0;
			pop1[mini].clients[i].revenue = 0;
			pop1[mini].clients[i].offerSlots = maxOffer[i];
			for (j = 0; j < prod; j++)
			{
				pop1[mini].clients[i].offers[j] = solc[i][j];
				if (solc[i][j])
				{
					pop1[mini].clients[i].cost += cost[i][j];
					pop1[mini].clients[i].revenue += revenue[i][j];
					pop1[mini].clients[i].offerSlots--;
					pop1[mini].budget[j] -= cost[i][j];
				}
			}
			if (pop1[mini].clients[i].revenue > 0)
			{
				pop1[mini].clients[i].npp = pop1[mini].clients[i].revenue / pop1[mini].clients[i].cost;
			}
			else
			{
				pop1[mini].clients[i].npp = 0;
			}
		}
		pop1[mini].cost = *sum_cost;
		pop1[mini].revenue = *sum_profit;
		pop1[mini].fitness = pop1[mini].revenue - pop1[mini].cost;

		elite_solution = &pop1[mini];
		if (pop1[mini].fitness > max)
		{
			elitei = mini;
			elite = &pop1[mini];
		}
		verify_solution(&pop1[mini], budgetpo, prod, cli, cost, revenue, mcpo, maxOffer, prod_can);
		for (int c = 0; c < prod; c++)
		{
			budget[c] = pop1[mini].budget[c];
			if (pop2[elitei].offeredProducts[c])
				active_prod[c] = 0;
		}

	}
	//main loop
	for (gen = 0, popflag = true, sumGap = 0; gen < generations; gen++, popflag = !popflag)
	{

		if (gen % 2 == 0)//each 2 generations, makes a new offer proffiling
		{
			if (popflag)
			{
				offer_profiling(prod, cli, popSize, pop1, sorted_npp, revenue, cost, average_positive_deviation, average_npp);
			}
			else
			{
				offer_profiling(prod, cli, popSize, pop1, sorted_npp, revenue, cost, average_positive_deviation, average_npp);
			}
		}
		//checks which population is currently active and applies the GA to it
		if (popflag)
		{
			for (i = 0; i < popSize; i++)
			{
				probabilities[i] = pop1[i].fitness;
				sumGap += probabilities[i];
			}
			for (i = 0; i < popSize; i++)
			{
				probabilities[i] = probabilities[i] / sumGap;
			}
			last_elite = elite;
			last_elitei = elitei;
			elitei = reproduction_CAN(pop1, pop2, prod, cli, mcpo, totalOffer, revenue, cost, profit, sorted_npp, maxOffer, budgetpo, hurdle, penaltie, popSize, probabilities, fcpo, &pop1[elitei], client_order, prod_order, prod_can);
			//Creates a new generation
			if (gen > -1)
			{
				for (i = 0, min = INT_MAX, mini = -1, max = 0, maxi = -1; i < popSize; i++)
				{
					if (pop2[i].fitness < min)
					{
						min = pop2[i].fitness;
						mini = i;
					}
				}
				for (i1 = 0; i1 < popSize; i1++)
				{
					for (i = 0, cost_counter = 0, rev_counter = 0; i < cli; i++)
					{
						for (j = 0; j < prod; j++)
						{
							if (pop2[i1].clients[i].offers[j])
							{
								cost_counter += cost[i][j];
								rev_counter += revenue[i][j];
								if (!pop2[i1].offeredProducts[j])
									printf("Oferta de produto fora da solucao\n");
							}
						}
					}
					for (j = 0; j < prod; j++)
						if (pop2[i1].offeredProducts[j])
							cost_counter += fcpo[j];
					int lucro = rev_counter - cost_counter;
					if (pop2[i1].fitness != pop2[i1].revenue - pop2[i1].cost)
						printf("Inconsistencia2. \n");
					if (pop2[i1].fitness != lucro && ((float)rev_counter / (float)cost_counter) > 1.0 + hurdle)
						printf("Inconsistencia.\n");
				}
				//Replaces the least profitable individual on the population with the elite from the previous population
				if (pop2[mini].fitness < pop1[last_elitei].fitness)
				{
					pop2[mini].cost = pop1[last_elitei].cost;
					pop2[mini].fitness = pop1[last_elitei].fitness;
					pop2[mini].revenue = pop1[last_elitei].revenue;
					for (i = 0; i < prod; i++)
					{
						pop2[mini].budget[i] = pop1[last_elitei].budget[i];
						pop2[mini].offeredProducts[i] = pop1[last_elitei].offeredProducts[i];
					}
					for (i = 0, cost_counter = 0, rev_counter = 0; i < cli; i++)
					{
						pop2[mini].clients[i].cost = pop1[last_elitei].clients[i].cost;
						pop2[mini].clients[i].npp = pop1[last_elitei].clients[i].npp;
						pop2[mini].clients[i].offerSlots = pop1[last_elitei].clients[i].offerSlots;
						pop2[mini].clients[i].revenue = pop1[last_elitei].clients[i].revenue;
						for (j = 0; j < prod; j++)
						{
							pop2[mini].clients[i].offers[j] = pop1[last_elitei].clients[i].offers[j];
							if (pop2[mini].clients[i].offers[j])
							{
								cost_counter += cost[i][j];
								rev_counter += revenue[i][j];
							}
						}
					}
					for (j = 0; j < prod; j++)
						if (pop2[mini].offeredProducts[j])
						{
							cost_counter += fcpo[j];
						}
					if (pop2[elitei].fitness < pop1[last_elitei].fitness)
					{
						elitei = mini;
					}
					else
					{
						if (pop1[last_elitei].fitness == pop2[elitei].fitness && (float)((float)(pop1[last_elitei].revenue) / (float)(pop1[last_elitei].cost)) > (float)((float)(pop2[elitei].revenue) / (float)(pop2[elitei].cost)))
						{
							elitei = mini;
						}
					}

				}
			}
		}
		else
		{
			//Creates a new generation
			for (i = 0; i < popSize; i++)
			{
				probabilities[i] = pop2[i].fitness;
				sumGap += probabilities[i];
			}
			for (i = 0; i < popSize; i++)
			{
				probabilities[i] = probabilities[i] / sumGap;
			}
			last_elitei = elitei;
			last_elite = elite;
			elitei = reproduction_CAN(pop2, pop1, prod, cli, mcpo, totalOffer, revenue, cost, profit, sorted_npp, maxOffer, budgetpo, hurdle, penaltie, popSize, probabilities, fcpo, &pop2[elitei], client_order, prod_order, prod_can);
			for (i = 0, min = INT_MAX, mini = -1, max = 0, maxi = -1; i < popSize; i++)
			{
				if (pop1[i].fitness < min)
				{
					min = pop1[i].fitness;
					mini = i;
				}
			}

			for (i1 = 0; i1 < popSize; i1++)
			{
				for (i = 0, cost_counter = 0, rev_counter = 0; i < cli; i++)
				{
					for (j = 0; j < prod; j++)
					{
						if (pop1[i1].clients[i].offers[j])
						{
							cost_counter += cost[i][j];
							rev_counter += revenue[i][j];
						}
					}
				}
				for (j = 0; j < prod; j++)
					if (pop1[i1].offeredProducts[j])
						cost_counter += fcpo[j];
				if (pop1[i1].fitness != rev_counter - cost_counter && ((float)rev_counter / (float)cost_counter) > 1.0 + hurdle)
					printf("Inconsistencia.\n");
			}
			//Replaces the least profitable individual on the population with the elite from the previous population
			if (pop2[last_elitei].fitness > pop1[mini].fitness)
			{
				pop1[mini].cost = pop2[last_elitei].cost;
				pop1[mini].fitness = pop2[last_elitei].fitness;
				pop1[mini].revenue = pop2[last_elitei].revenue;
				for (i = 0; i < prod; i++)
				{
					pop1[mini].budget[i] = pop2[last_elitei].budget[i];
					pop1[mini].offeredProducts[i] = pop2[last_elitei].offeredProducts[i];
				}
				for (i = 0, cost_counter = 0, rev_counter = 0; i < cli; i++)
				{
					pop1[mini].clients[i].cost = pop2[last_elitei].clients[i].cost;
					pop1[mini].clients[i].npp = pop2[last_elitei].clients[i].npp;
					pop1[mini].clients[i].offerSlots = pop2[last_elitei].clients[i].offerSlots;
					pop1[mini].clients[i].revenue = pop2[last_elitei].clients[i].revenue;
					for (j = 0; j < prod; j++)
					{
						pop1[mini].clients[i].offers[j] = pop2[last_elitei].clients[i].offers[j];
						if (pop1[mini].clients[i].offers[j])
						{
							cost_counter += cost[i][j];
							rev_counter += revenue[i][j];
						}
					}
				}
				for (j = 0; j < prod; j++)
					if (pop1[mini].offeredProducts[j])
						cost_counter += fcpo[j];

				if (pop2[last_elitei].fitness > pop1[elitei].fitness)
				{
					elitei = mini;
				}
				else
				{
					if (pop2[last_elitei].fitness == pop1[elitei].fitness && (float)((float)(pop2[last_elitei].revenue) / (float)(pop2[last_elitei].cost)) > (float)((float)(pop1[elitei].revenue) / (float)(pop1[elitei].cost)))
					{
						elitei = mini;
					}
				}

			}

		}
		if (popflag)
			printf("Gen loop: %d   Fitness: %d    Hurdle: %f\n", gen, pop2[elitei].fitness, (float)((float)(pop2[elitei].revenue) / (float)(pop2[elitei].cost)));
		else
			printf("Gen loop: %d   Fitness: %d    Hurdle: %f\n", gen, pop1[elitei].fitness, (float)((float)(pop1[elitei].revenue) / (float)(pop1[elitei].cost)));
	}
	int return_solution = elitei;
	for (i = 0; i < prod; i++)
		client_po[i] = 0;
	int best_alternative_sol = -1;
	offer_profiling(prod, cli, popSize, pop1, sorted_npp, revenue, cost, average_positive_deviation, average_npp);
	//If the diversification flag is marked, the best solution that meets the required deviation will be selected to be returned. The deviation starts at 3%, in that case, a solution with at fitness at most 3% lower than  the elite solution fitness will be picked. 
	// If no solution ther than the elite solution meets this critea, deviation will be increased by 3%.
	if (diversification)
	{
		if (!popflag)
			for (i = 0; i < popSize; i++)
			{

				if (pop2[elitei].fitness > pop2[i].fitness + pop2[elitei].fitness*deviation && (pop2[return_solution].fitness > best_alternative_sol || best_alternative_sol < 0))
				{
					return_solution = i;
					best_alternative_sol = pop2[i].fitness;
				}

			}
		else
			for (i = 0; i < popSize; i++)
			{

				if (pop1[elitei].fitness <= pop1[i].fitness + pop1[elitei].fitness*(deviation + 0.3) && (pop1[return_solution].fitness > best_alternative_sol || best_alternative_sol < 0))
				{
					return_solution = i;
					best_alternative_sol = pop1[i].fitness;
				}

			}
		if (return_solution == elitei)
			return_solution = (rand() % popSize);
		deviation += 0.03;
	}



	if (!popflag)
	{
		for (i = 0; i < cli; i++)
			for (j = 0; j < prod; j++)
			{
				if (pop2[return_solution].clients[i].offers[j])
				{
					solc[i][j] = 1;
					client_po[j]++;
				}
				else
					solc[i][j] = 0;
			}
		for (j = 0; j < prod; j++)
		{
			if (pop2[return_solution].offeredProducts[j])
			{
				solp[j] = 1;
			}
			else
			{
				solp[j] = 0;
			}
		}
		for (int c = 0; c < prod; c++)
		{
			budget[c] = pop2[return_solution].budget[c];
			if (pop2[return_solution].offeredProducts[c])
				active_prod[c] = 0;
		}
		for (int c = 0; c < cli; c++)
			active_clients[c] = pop2[return_solution].clients[c].offerSlots;
		*sum_cost = pop2[return_solution].cost;
		*sum_profit = pop2[return_solution].revenue;
		
	}
	else
	{
		for (i = 0; i < cli; i++)
			for (j = 0; j < prod; j++)
			{
				if (pop1[return_solution].clients[i].offers[j])
				{
					solc[i][j] = 1;
					client_po[j]++;
				}
				else
					solc[i][j] = 0;
			}
		for (j = 0; j < prod; j++)
		{
			if (pop1[return_solution].offeredProducts[j])
			{
				solp[j] = 1;
			}
			else
			{
				solp[j] = 0;
			}
		}
		for (int c = 0; c < prod; c++)
		{
			budget[c] = pop1[return_solution].budget[c];
			if (pop1[return_solution].offeredProducts[c])
				active_prod[c] = 0;
		}
		for (int c = 0; c < cli; c++)
			active_clients[c] = pop1[return_solution].clients[c].offerSlots;
		*sum_cost = pop1[return_solution].cost;
		*sum_profit = pop1[return_solution].revenue;

	}
	free(average_npp);
	free(probabilities);
}

