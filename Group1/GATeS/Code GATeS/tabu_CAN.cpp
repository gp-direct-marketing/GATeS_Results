#include <time.h>
#include "tabu_CAN.h"
#include "definitions_CAN.h"
#include "matriz.h"
#include "Gen.h"

//Flag that controls the cannibalism on the tabu search procedure
int can = 1;
int max_fo = 0;
/*Not utilised. tabu_search2 is utilised for GATeS*/
void tabu_search(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* maxBudgetpo, int* fcostpo,
	int **solc, int* solp, float* solution_cost,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can)

{
	float incumbent_solution = *solution_cost; /* Store the Best Iteration so far */
	float tabu_cost = *solution_cost;
	List* aux_list;
	int **tabu_solc;
	tabu_solc = generate_2D_matrix_int(client, prod);
	int *tabu_solp;
	tabu_solp = new int[prod];
	int **tabu_solc_copy;
	tabu_solc_copy = generate_2D_matrix_int(client, prod);
	int *tabu_solp_copy;
	tabu_solp_copy = new int[prod];
	float tabu_cost_copy;
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
	int **tabu_list1;              /* Tabu List: contains the iteration which the element */
											   /* will be free to be evolved in a movement, one for each neighborhood */
	tabu_list1 = generate_2D_matrix_int(client, prod);
	int **tabu_list2;
	tabu_list2 = generate_2D_matrix_int(client, prod);
	int **tabu_list3;
	tabu_list3 = generate_2D_matrix_int(client, prod);
	int *tabu_list4;
	tabu_list4 = new int[client];
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
	//bool initialized = false;




	int best_j1, best_j2, best_i1, best_i2, best_solution, best_move;

	for (j = 0; j < prod; j++)
	{
		residual_budget[j] = budgetpo[j];
		client_po[j] = 0;
		used_product[j] = 0;
	}
	for (i = 0; i < client; i++)
	{
		tabu_list4[i] = 0;
		for (j = 0; j < prod; j++)
		{
			tabu_list1[i][j] = 0;
			tabu_list2[i][j] = 0;
			tabu_list3[i][j] = 0;
			residual_budget[j] = residual_budget[j] - cost[i][j] * solc[i][j];
			tabu_solc[i][j] = solc[i][j];
			tabu_solp[j] = solp[j];
			client_po[j] += solc[i][j];
		}
	}
	best_i1 = -1;
	best_i2 = -1;
	best_j1 = -1;
	best_j2 = -1;
	best_solution = -10000;
	begin_phase = clock();
	time_elapsed = 0.0;
	begin_phase = clock();
	//** Ambos os algoritmos estao apresentando um problema para o 5-5-1-l e 5-5-3-l, onde ficam melhorando infinitamente ou acham uma solucao pior que a solucao inicial
	while (iter_no_improvement < 100 && time_elapsed <= 3600.0) /* loop for the tabu search until a stopping criteria be found */
	{
		for (j1 = 0; (j1 < prod && !flag_move1); j1++) /* loop for the neighborhood of swap,  i.e., if prod[j] is active and customer[i1][j]=1 and customer[i2][j]=0 */
													/* do customer[i1][j]=0 and customer[i2][j]=1 that improves the solution value */
		{
			//Se o produto j1 esta aberto
			if (tabu_solp[j1] != 0)
			{
				for (i1 = 0; (i1 < client - 1 && !flag_move1); i1++)
				{
					//Se o produto esta sendo ofertado para o cliente i1
					if (tabu_solc[i1][j1] == 1)
					{
						for (i2 = 0; (i2 < client && !flag_move1); i2++)
						{
							//Se o produto j1 nao foi ofertado para o client i2 e se a diferenca de custos entre oferecer o produto j1 para o client i1 e i2 esta dentro do orcamento
							//e i2 ainda nao foi mexido pelo tabu e o lucro provido pela oferta esta dentro do hurdle rate
							if ((tabu_solc[i2][j1] == 0) && ((cost[i2][j1] - cost[i1][j1]) < residual_budget[j1]) &&
								(active_clients[i2] > 0) &&
								(((*sum_profit) - revenue[i1][j1] + revenue[i2][j1]) >=
								((1 + hurdle)*((*sum_cost_fix) + cost[i2][j1] - cost[i1][j1]))))
							{
								//Se o lucro da melhor solucao corrente e menor que o lucro provido pelo movimento e o movimento nao e tabu, considerar esse movimento como candidato para melhor movimento
								if ((best_solution < ((revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]))) &&
									(tabu_list1[i1][j1] < iter) && (tabu_list1[i2][j1] < iter))
								{
									aspiration_criteria = 1;
								}

								else
								{
									//Se o lucro e maior que a melhor solucao da iteracao, considerar como candidato para melhor movimento
									if ((tabu_cost + (revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1])) > incumbent_solution)
										aspiration_criteria = 1;
									else aspiration_criteria = 0;
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
										tabu_solc[i1][j1] = 0;
										tabu_solc[i2][j1] = 1;
										//Poe o client i1 na lista tabu e retira o cliente i2 (removido eh considerado tabu)
										active_clients[i1] += 1;
										active_clients[i2] -= 1;
										//Reajusta orcamento residual
										residual_budget[j1] -= (cost[i2][j1] - cost[i1][j1]);
										//Calcula novo custo da solucao atual
										tabu_cost += best_solution;
										//Marca a solucao atual como melhor solucao encontrada
										incumbent_solution = tabu_cost;
										//Transfere a solucao tabu para solucao definitiva
										for (j = 0; j < prod; j++)
										{
											solp[j] = tabu_solp[j];
											for (i = 0; i < client; i++)
												solc[i][j] = tabu_solc[i][j];
										}
										//Ajusta parametros da solucao definitiva e bota o movimento na lista tabu com um timer (iteracao atual + quantas iteracoes o movimento ficara como tabu)
										*solution_cost = tabu_cost;
										printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
										/*tabu_list1[i1][j1] = iter + tabu_time;
										tabu_list1[i2][j1] = iter + tabu_time;*/
										*sum_profit = *sum_profit - revenue[i1][j1] + revenue[i2][j1];
										*sum_cost_fix = *sum_cost_fix - cost[i1][j1] + cost[i2][j1];
										//Reseta o loop de pois foi achada uma melhora
										best_solution = -10000;
										iter_no_improvement = 1;
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
			if (tabu_solp[j1] != 0)
			{
				for (j2 = 0; (j2 < prod && !flag_move2); j2++)
				{
					//Se o produto j2 esta aberto
					if ((tabu_solp[j2] != 0) && (j1 != j2))
					{
						for (i1 = 0; (i1 < client - 1 && !flag_move2); i1++)
						{
							for (i2 = 0; (i2 < client && !flag_move2); i2++)
							{
								//Se o produto j1 esta sendo ofertado para o cliente i1 e nao esta sendo ofertado para o cliente i2
								//E o produto j2 nao esta sendo ofertado para o cliente i1 e esta sendo ofertado para o cliente i2
								if ((tabu_solc[i1][j1] == 1) && (tabu_solc[i2][j1] == 0) &&
									(tabu_solc[i1][j2] == 0) && (tabu_solc[i2][j2] == 1))
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
											+ ((revenue[i1][j2] - cost[i1][j2]) - (revenue[i2][j2] - cost[i2][j2])))) &&
											(tabu_list2[i1][j1] < iter) && (tabu_list2[i2][j1] < iter) &&
											(tabu_list2[i1][j2] < iter) && (tabu_list2[i2][j2] < iter))
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
												tabu_solc[i1][j1] = 0;
												tabu_solc[i2][j1] = 1;
												tabu_solc[i1][j2] = 1;
												tabu_solc[i2][j2] = 0;
												//Ajusta o orcamento
												residual_budget[j1] -= (cost[i2][j1] - cost[i1][j1]);
												residual_budget[j2] -= (cost[i1][j2] - cost[i2][j2]);
												//Ajusta o lucro e marca o novo custo da melhor solucao jah encontrada
												tabu_cost += best_solution;
												printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
												incumbent_solution = tabu_cost;
												//Copia a solucao para solucao definitiva
												for (j = 0; j < prod; j++)
												{
													solp[j] = tabu_solp[j];
													for (i = 0; i < client; i++)
														solc[i][j] = tabu_solc[i][j];
												}
												*solution_cost = tabu_cost;
												//Insere o movimento na lista tabu
												tabu_list2[i1][j1] = iter + tabu_time;
												tabu_list2[i2][j1] = iter + tabu_time;
												tabu_list2[i1][j2] = iter + tabu_time;
												tabu_list2[i2][j2] = iter + tabu_time;
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
			if (tabu_solp[j1] != 0)
			{
				for (j2 = j1; (j2 < prod && !flag_move3); j2++)
				{
					//Se o produto j2 esta aberto e j1 e j2 sao diferentes
					if ((tabu_solp[j2] != 0) && (j1 != j2))
					{
						for (i1 = 0; (i1 < client && !flag_move3); i1++)
						{
							for (i2 = 0; (i2 < client && !flag_move3); i2++)
							{
								//Se o produto i1 esta sendo ofertado para o cliente i1 e nao esta sendo ofertado para o cliente i2
								//E se o numero de ofertas  realizadas para o produto j1 menos uma ainda atendo o numero de ofertas minimas para o produto j1
								//E se o cliente i2 esta ativo (ainda pode receber ofertas)
								//E se o movimento de 
								if ((tabu_solc[i1][j1] == 1) && (tabu_solc[i2][j2] == 0) && ((client_po[j1] - 1) >= mcpo[j1]) &&
									(active_clients[i2] > 0) &&
									(((*sum_profit) - revenue[i1][j1] + revenue[i2][j2]) >=//Este calculo parece estar errado pois se a ideia eh igualar as oferas entao, ou as duas ofertas estao sendo removidas ou estao sendo adicionadas, nunca uma removida e outra adicionada
									((1 + hurdle)*((*sum_cost_fix) - cost[i1][j1] + cost[i2][j2]))))
								{
									if (cost[i2][j2] <= residual_budget[j2])
									{
										if ((best_solution < ((revenue[i2][j2] - cost[i2][j2]) - (revenue[i1][j1] - cost[i1][j1]))) &&
											(tabu_list3[i1][j1] < iter) && (tabu_list3[i2][j2] < iter))
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
												tabu_solc[i1][j1] = 0;
												active_clients[i1] += 1;
												client_po[j1]--;
												tabu_solc[i2][j2] = 1;
												active_clients[i2] -= 1;
												client_po[j2] ++;
												residual_budget[j1] += cost[i1][j1];
												residual_budget[j2] -= cost[i2][j2];
												tabu_cost += best_solution;
												printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
												incumbent_solution = tabu_cost;
												for (j = 0; j < prod; j++)
												{
													solp[j] = tabu_solp[j];
													for (i = 0; i < client; i++)
														solc[i][j] = tabu_solc[i][j];
												}
												*solution_cost = tabu_cost;
												tabu_list3[i1][j1] = iter + tabu_time;
												tabu_list3[i2][j2] = iter + tabu_time;
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
									if ((tabu_solc[i1][j1] == 0) && (tabu_solc[i2][j2] == 1) && ((client_po[j2] - 1) >= mcpo[j2]) &&
										(active_clients[i1] > 0) &&
										(((*sum_profit) + revenue[i1][j1] - revenue[i2][j2]) >=
										((1 + hurdle)*((*sum_cost_fix) + cost[i1][j1] - cost[i2][j2]))))
									{
										if (cost[i1][j1] <= residual_budget[j1])
										{
											if ((best_solution < ((revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j2] - cost[i2][j2]))) &&
												(tabu_list3[i1][j1] < iter) && (tabu_list3[i2][j2] < iter))
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
													tabu_solc[i1][j1] = 1;
													tabu_solc[i2][j2] = 0;
													active_clients[i1] -= 1;
													active_clients[i2] += 1;
													client_po[j1]++;
													client_po[j2]--;
													residual_budget[j1] -= cost[i1][j1];
													residual_budget[j2] += cost[i2][j2];
													tabu_cost += best_solution;
													printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
													incumbent_solution = tabu_cost;
													for (j = 0; j < prod; j++)
													{
														solp[j] = tabu_solp[j];
														for (i = 0; i < client; i++)
															solc[i][j] = tabu_solc[i][j];
													}
													*solution_cost = tabu_cost;
													tabu_list3[i1][j1] = iter + tabu_time;
													tabu_list3[i2][j2] = iter + tabu_time;
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

		//for (i1 = 0; ((i1 < client-1) && (!flag_move1 && !flag_move2 && !flag_move3 && !flag_move4)) ; i1++)
		//{
		//    for (i2 = i1; (i2 < client && !flag_move4) ; i2++)
		//    {
		//      flag_double = 0;
		//      cont_double = 0;

			  ////Trava em dois clientes e percorre a lista de produtos relacionando  produto que foram ofertados para um cliente e nao foram para o outro
			  ////Se for esse o caso, verifica se trocar a oferta entre os dois clientes gera lucro, se gerar adiciona um ao contador
			  ////Sai assim que uma troca que nao gere beneficios foi achada
		//      for (j1 = 0, conti1=0, conti2=0; (j1 < prod && !flag_move4 && !flag_double); j1++) //Vai sair assim q a flag de troca nao encontrada for acionada
		//       {
				 // //Se o produto j1 esta sendo ofertado para o cliente i1 e nao esta para o cliente i2 ou vice-versa
		//           if (tabu_solc[i1][j1] != tabu_solc[i2][j1])
		//           {
				 //	  //Se o produto j1 esta sendo ofertado para o cliente i1 e nao esta sendo ofertado para o cliente i2
				 //	  //E o cliente i2 esta ativo (ainda pode receber ofertas)
				 //	  //E o custo de oferecer o produto j1 para o cliente i2 esta dentro do orcamento
				 //	  //E o movimento respeita o hurdle rate do produto j1
		//               if ((tabu_solc[i1][j1] == 1) && (tabu_solc[i2][j1] == 0) &&
		//                    (cost[i2][j1] -cost[i1][j1]<= residual_budget[j1]) && active_clients[i2]>0 &&
		//                   (((*sum_profit) - revenue[i1][j1] + revenue[i2][j1]) >=
		//                   ((1 + hurdle)*((*sum_cost_fix) - cost[i1][j1] + cost[i2][j1]))))
		//               {
				 //		  //Marca um movimento
		//                    cont_double++;
				 //		   conti2++;
				 //		   conti1--;
		//               }
		//               else
		//               {
				 //		  //Se o produto j1 nao esta sendo ofertado para o cliente i1 e esta sendo ofertado para o cliente i2
				 //		  //E o cliente i1 esta ativo (ainda pode receber ofertas)
				 //		  //E a oferta do produto j1 para o cliente i1 esta dentro do orcamento do produto j1
				 //		  //E o movimento respeita o hurdle rate do produto j1
		//                 if ((tabu_solc[i1][j1] == 0) && (tabu_solc[i2][j1] == 1) &&
		//                     (cost[i1][j1]-cost[i2][j1] <= residual_budget[j1]) && active_clients[i1]>0 &&
		//                    (((*sum_profit) + revenue[i1][j1] - revenue[i2][j1]) >=
		//                    ((1 + hurdle)*((*sum_cost_fix) + cost[i1][j1] - cost[i2][j1]))))
		//                    {
				 //			//Marca um movimento
		//                        cont_double++;
				 //			   conti1++;
				 //			   conti2--;
		//                    }
		//                    else
		//                    {
				 //			   //Caso nenhuma das trocas satisca todas condicoes marca a flag de que nao houve troca
		//                        flag_double = 1;
		//                        break;
		//                    }
		//               }
		//          }
		//       }
			  ////Se a flag foi marcada (entrou no loop de produtos e saiu de la por ter achado uma troca que nao provenha beneficios)
			  ////E se foi encontrada mais de uma troca que gere beneficios
		//       if (cont_double > 1 && !flag_double) //&& active_clients[i2]-conti2 >=0 && active_clients[i1]-conti1>=0 ) /* The test is greather than 1 because of neighborhood type 3 */
		//       {
		//           sum_cost_i1 = 0;
		//           sum_cost_i2 = 0;
		//           sum_revenue_i1 = 0;
		//           sum_revenue_i2 = 0;
				 //  //Calcula o custo de lucro gerado pelas trocas em cada um dos clientes
		//           for (j1 = 0; j1 < prod ; j1++)
		//           {
		//               if (tabu_solc[i1][j1] != tabu_solc[i2][j1])
		//               {
		//                   if (((tabu_solc[i1][j1] == 1) &&
				 //			  (cost[i2][j1] - cost[i1][j1] <= residual_budget[j1]) && active_clients[i2]>0 &&
				 //			  (((*sum_profit) - revenue[i1][j1] + revenue[i2][j1]) >=
				 //			  ((1 + hurdle)*((*sum_cost_fix) - cost[i1][j1] + cost[i2][j1])))))
		//                   {
		//                       sum_cost_i2 += cost[i2][j1];
		//                       sum_revenue_i2 += revenue[i2][j1];
		//                       sum_cost_i1 -= cost[i1][j1];
		//                       sum_revenue_i1 -= revenue[i1][j1];
		//                   }
		//                   else
		//                   {
				 //			  if ((tabu_solc[i2][j1] == 1) &&
				 //				  (cost[i1][j1] - cost[i2][j1] <= residual_budget[j1]) && active_clients[i1] > 0 &&
				 //				  (((*sum_profit) + revenue[i1][j1] - revenue[i2][j1]) >=
				 //				  ((1 + hurdle)*((*sum_cost_fix) + cost[i1][j1] - cost[i2][j1]))))
				 //			  {


				 //				  sum_cost_i1 += cost[i1][j1];
				 //				  sum_revenue_i1 += revenue[i1][j1];
				 //				  sum_cost_i2 -= cost[i2][j1];
				 //				  sum_revenue_i2 -= revenue[i2][j1];
				 //			  }
		//                   }
		//               }
		//           }
				 //  //Compara o resultado dos multiplos movimentos em termos de lucro com o lucro do melhor movimento atual
		//           if (best_solution < (sum_revenue_i1+sum_revenue_i2-sum_cost_i1-sum_cost_i2))
		//           {
				 //	  //Se nenhum dos clientes esta marcado como tabu marca o movimento como candidato de melhora
		//              if ((tabu_list4[i1] < iter) && (tabu_list4[i2] < iter))
		//              {
		//                  aspiration_criteria = 1;
		//              }
		//              else
		//              {
				 //		 //Se algum dos clientes esta marcado como tabu, verifica se o movimento geraria uma solucao melhor que a melhor solucao achada
		//                  if ((tabu_cost+(sum_revenue_i1+sum_revenue_i2-sum_cost_i1-sum_cost_i2))
		//                           > incumbent_solution)
		//                       aspiration_criteria = 1;//Se for melhor marca como candidato a melhor movimento
		//                  else aspiration_criteria = 0;//Senao nao marca
		//              }
		//           }
				 //  else{
				 //	  aspiration_criteria = 0;
				 //  }
		//           if (aspiration_criteria == 1)//Se acho um candidato
		//           {
				 //	  //Marca os candidatos envolvidos no movimento e o tipo de movimento (movimento do tipo 5)
		//               best_i1 = i1;
		//               best_i2 = i2;
		//               best_move = 5;
				 //	  best_cont_double = cont_double;
		//               best_solution = sum_revenue_i1 + sum_revenue_i2 - sum_cost_i1 - sum_cost_i2;
				 //	  //Se o movimento gera uma solucao melhor que a melhor solucao encontrada, transfere os movimentos para solucao atual e solucao definitiva
		//               if ((best_solution > 0 && (tabu_cost+best_solution) > incumbent_solution))
		//               {
		//                   flag_move4 = 1;
		//                   tabu_cost += best_solution;
				 //		  printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
		//                   incumbent_solution = tabu_cost;
		//                   for (j1 = 0; j1 < prod; j1++)
		//                   {
		//                     if (tabu_solc[i1][j1] != tabu_solc[i2][j1])
		//                     {
		//                       if (tabu_solc[i1][j1] == 1 && residual_budget[j1] >= cost[i2][j1] - cost[i1][j1])
		//                       {
		//                          tabu_solc[i1][j1] = 0;
		//                          tabu_solc[i2][j1] = 1;
		//                          active_clients[i1] += 1;
		//                          active_clients[i2] -= 1;
		//                          residual_budget[j1] -= cost[i1][j1];
		//                          residual_budget[j1] += cost[i2][j1];
		//                          *sum_profit = (*sum_profit) - revenue[i1][j1] + revenue[i2][j1];
		//                          *sum_cost_fix = (*sum_cost_fix) - cost[i1][j1] + cost[i2][j1];
		//                       }
		//                       else
		//                       {
				 //				  if (tabu_solc[i1][j1] == 0)
				 //				  {
				 //					  if (residual_budget[j1] >= cost[i1][j1] - cost[i2][j1])
				 //					  {
				 //						  tabu_solc[i1][j1] = 1;
				 //						  tabu_solc[i2][j1] = 0;
				 //						  active_clients[i1] -= 1;
				 //						  active_clients[i2] += 1;
				 //						  residual_budget[j1] += cost[i1][j1];
				 //						  residual_budget[j1] -= cost[i2][j1];
				 //						  *sum_profit = (*sum_profit) + revenue[i1][j1] - revenue[i2][j1];
				 //						  *sum_cost_fix = (*sum_cost_fix) + cost[i1][j1] - cost[i2][j1];
				 //					  }
				 //				  }
		//                       }
		//                     }
		//                   }
		//                   for (j = 0; j < prod ; j++)
		//                   {
		//                      solp[j] = tabu_solp[j];
		//                      for (i = 0; i < client ; i++) solc[i][j] = tabu_solc[i][j];
		//                   }
		//                   *solution_cost = tabu_cost;
		//                   tabu_list4[i1] = iter + tabu_time;
		//                   tabu_list4[i2] = iter + tabu_time;
		//                   iter_no_improvement = 1;
		//                   best_solution = -10000;
		//                   /*printf("sol. Incumbente MOVE 4 = %f\n",incumbent_solution);*/
		//               }
		//           }
		//       }
		//     }
		//   }






		if (!flag_move1 && !flag_move2 && !flag_move3 && !flag_move4
			&& best_solution > -9000) /* It means that no improvement in the incumbent solution was found, */
											   /* then perform the best movement without improvement */
		{
			switch (best_move)
			{
			case 1:
				tabu_solc[best_i1][best_j1] = 0;
				tabu_solc[best_i2][best_j1] = 1;
				active_clients[best_i1] += 1;
				active_clients[best_i2] -= 1;
				residual_budget[best_j1] -= (cost[best_i2][best_j1] - cost[best_i1][best_j1]);
				tabu_cost += best_solution;
				printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
				tabu_list1[best_i1][best_j1] = iter + tabu_time;
				tabu_list1[best_i2][best_j1] = iter + tabu_time;
				best_solution = -10000;
				*sum_profit = (*sum_profit) - revenue[best_i1][best_j1] + revenue[best_i2][best_j1];
				*sum_cost_fix = (*sum_cost_fix) - cost[best_i1][best_j1] + cost[best_i2][best_j1];
				break;
			case 2:
				tabu_solc[best_i1][best_j1] = 0;
				tabu_solc[best_i2][best_j1] = 1;
				tabu_solc[best_i1][best_j2] = 1;
				tabu_solc[best_i2][best_j2] = 0;
				residual_budget[best_j1] -= (cost[best_i2][best_j1] - cost[best_i1][best_j1]);
				residual_budget[best_j2] -= (cost[best_i1][best_j2] - cost[best_i2][best_j2]);
				tabu_cost += best_solution;
				printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
				tabu_list2[best_i1][best_j1] = iter + tabu_time;
				tabu_list2[best_i2][best_j1] = iter + tabu_time;
				tabu_list2[best_i1][best_j2] = iter + tabu_time;
				tabu_list2[best_i2][best_j2] = iter + tabu_time;
				best_solution = -10000;
				*sum_profit = (*sum_profit) - revenue[best_i1][best_j1] + revenue[best_i2][best_j1] +
					revenue[best_i1][best_j2] - revenue[best_i2][best_j2];
				*sum_cost_fix = (*sum_cost_fix) - cost[best_i1][best_j1] + cost[best_i2][best_j1] +
					cost[best_i1][best_j2] - cost[best_i2][best_j2];
				break;
			case 3:
				tabu_solc[best_i1][best_j1] = 0;
				tabu_solc[best_i2][best_j2] = 1;
				active_clients[best_i1] += 1;
				active_clients[best_i2] -= 1;
				client_po[best_j1]--;
				client_po[best_j2]++;
				residual_budget[best_j1] += cost[best_i1][best_j1];
				residual_budget[best_j2] -= cost[best_i2][best_j2];
				tabu_cost += best_solution;
				printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
				tabu_list3[best_i1][best_j1] = iter + tabu_time;
				tabu_list3[best_i2][best_j2] = iter + tabu_time;
				*sum_profit = (*sum_profit) - revenue[best_i1][best_j1] + revenue[best_i2][best_j2];
				*sum_cost_fix = (*sum_cost_fix) - cost[best_i1][best_j1] + cost[best_i2][best_j2];
				best_solution = -10000;
				break;
			case 4:
				tabu_solc[best_i1][best_j1] = 1;
				tabu_solc[best_i2][best_j2] = 0;
				active_clients[best_i1] -= 1;
				active_clients[best_i2] += 1;
				client_po[best_j1]++;
				client_po[best_j2]--;
				residual_budget[best_j1] -= cost[best_i1][best_j1];
				residual_budget[best_j2] += cost[best_i2][best_j2];
				tabu_cost += best_solution;
				printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
				tabu_list3[best_i1][best_j1] = iter + tabu_time;
				tabu_list3[best_i2][best_j2] = iter + tabu_time;
				best_solution = -10000;
				*sum_profit = (*sum_profit) + revenue[best_i1][best_j1] - revenue[best_i2][best_j2];
				*sum_cost_fix = (*sum_cost_fix) + cost[best_i1][best_j1] - cost[best_i2][best_j2];
				break;
			case 5:
				//Aparentemente o movimento 5 esta gerando um estado incosistente na solucao possibilitando melhoras infinitas
				//Refletir as modificacoes aqui no outro codigo
				tabu_cost += best_solution;
				incumbent_solution = tabu_cost;
				for (j1 = 0; j1 < prod; j1++)
				{
					if (tabu_solc[best_i1][j1] != tabu_solc[best_i2][j1])
					{
						if (tabu_solc[best_i1][j1] == 1 && residual_budget[j1] >= cost[best_i2][j1] - cost[best_i1][j1] && active_clients[i2] > 0)
						{
							tabu_solc[best_i1][j1] = 0;
							tabu_solc[best_i2][j1] = 1;
							active_clients[best_i1] += 1;
							active_clients[best_i2] -= 1;
							residual_budget[j1] -= cost[best_i1][j1];
							residual_budget[j1] += cost[best_i2][j1];
							*sum_profit = (*sum_profit) - revenue[best_i1][j1] + revenue[best_i2][j1];
							*sum_cost_fix = (*sum_cost_fix) - cost[best_i1][j1] + cost[best_i2][j1];
						}
						else
						{
							if (tabu_solc[best_i1][j1] == 0 && active_clients[i1] > 0)
							{
								if (residual_budget[j1] >= cost[best_i1][j1] - cost[best_i2][j1])
								{
									tabu_solc[best_i1][j1] = 1;
									tabu_solc[best_i2][j1] = 0;
									active_clients[best_i1] -= 1;
									active_clients[best_i2] += 1;
									residual_budget[j1] += cost[best_i1][j1];
									residual_budget[j1] -= cost[best_i2][j1];
									*sum_profit = (*sum_profit) + revenue[best_i1][j1] - revenue[best_i2][j1];
									*sum_cost_fix = (*sum_cost_fix) + cost[best_i1][j1] - cost[best_i2][j1];
								}
							}
						}
					}
				}
				printf("Bestmove: %d Tabu_cost: %f Best_Solution: %d\n", best_move, tabu_cost, best_solution);
				tabu_list4[best_i1] = iter + tabu_time;
				tabu_list4[best_i2] = iter + tabu_time;
				best_solution = -10000;

				break;
			default:
				printf("Invalid Tabu movement\n");
				break;
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

		/* Diversification Procedure */

	  //  if (iter_no_diversification > 5 )
	  //  {
			////Ajustar o budget
			//if (prod_cic <= prod && div2_flag && false)
			//{
			//	iter_no_diversification = 1;
			//	for (j = 0; j < prod; j++)
			//	{
			//		active_products_aux[j] = 1;
			//		aux_budget[j] = maxBudgetpo[j];
			//		for (i = 0; i < client; i++)
			//		{
			//			active_clients_aux[i] = maxofer[i];
			//			sol_aux[i][j] = 0;
			//		}
			//		solp_aux[j] = 0;

			//	}

			//	if (mutate_solution(client, prod, hurdle, blocked_prod,
			//		cost, revenue, maxofer,
			//		mcpo, aux_budget, fcostpo,
			//		sol_aux, solp_aux, sorted_npp, active_clients_aux, active_products_aux,
			//		&sum_cost_fix_aux, &sum_profit_aux, prod_can))
			//	{
			//		compute_solution_value(client, prod, hurdle,
			//			cost, revenue, maxofer,
			//			mcpo, aux_budget, fcostpo,
			//			sol_aux, solp_aux, &tabu_cost, prod_can);
			//		int ** sub = tabu_solc;
			//		tabu_solc = sol_aux;
			//		sol_aux = sub;

			//		for (int c = 0;c < prod;c++)
			//		{
			//			tabu_solp[c] = solp_aux[c];
			//			active_products[c] = active_products_aux[c];
			//			residual_budget[c] = aux_budget[c];
			//		}
			//		for (int c = 0; c < client;c++)
			//		{
			//			active_clients[c] = active_clients_aux[c];
			//		}
			//		for (int c = 0;c < client;c++)
			//		{
			//			for (int p = 0;p < prod;p++)
			//			{
			//				tabu_solc[c][p] = sol_aux[c][p];
			//			}
			//		}
			//		*sum_cost_fix = sum_cost_fix_aux;
			//		*sum_profit = sum_profit_aux;
			//		//tabu_cost = (int)(*sum_profit - *sum_cost_fix);
			//		prod_cic++;
			//		div2_flag = false;
			//	}
			//	printf("Diversification2. Tabu_cost: %f\n", tabu_cost);
			//}
			//else
			//{
			//	tabu_cost_copy = tabu_cost;
			//	for (j = 0; j < prod; j++)
			//	{
			//		tabu_solp_copy[j] = tabu_solp[j];
			//		sum_cost[j] = (float)fcostpo[j] * tabu_solp[j];
			//		sum_revenue[j] = 0.0;
			//		for (i = 0; i < client; i++)
			//		{
			//			tabu_solc_copy[i][j] = tabu_solc[i][j];
			//			sum_revenue[j] += tabu_solc[i][j] * revenue[i][j];
			//			sum_cost[j] += tabu_solc[i][j] * (float)cost[i][j];
			//		}
			//	}

			//	for (j1 = 0; j1 < prod && flag_diver == 0; j1++)
			//	{
			//		//Se produto j1 esta aberto
			//		if (tabu_solp[j1] != 0)
			//		{
			//			*sum_cost_fix -= sum_cost[j1];
			//			*sum_profit -= sum_revenue[j1];
			//			//Fecha j1
			//			tabu_solp[j1] = 0;
			//			client_po[j1] = 0;
			//			//Ajusta o custo atual
			//			tabu_cost = (int)(*sum_profit - *sum_cost_fix);

			//			//remove as ofertas do produto j1 para todos os clientes e abre uma oferta disponivel para eles
			//			for (i = 0; i < client; i++)
			//			{
			//				if (tabu_solc[i][j1] == 1)
			//				{
			//					residual_budget[j1] += cost[i][j1];
			//					active_clients[i] += 1;
			//					tabu_solc[i][j1] = 0;
			//				}
			//			}

			//			for (j2 = 0; j2 < prod && flag_diver == 0; j2++)
			//			{
			//				flag_can = 0;
			//				//Procura um produto j2 nao aberto que nao possua uma relacao de canibalismo com o produto j1
			//				if (can) {
			//					for (j = 0; ((j < prod) && (prod_can[j2] >= 0)); j++)
			//					{
			//						if ((prod_can[j2] == prod_can[j]) && (tabu_solp[j] == 1) && (j1 != j2))
			//							flag_can = 1;
			//					}
			//				}
			//				//Se o produto j2 nao esta aberto e j1 e diferente de j2
			//				//E o produto j2 nao canibaliza o produto j1
			//				if ((tabu_solp[j2] == 0) && (used_product[j2] == 0) && (j1 != j2) && !flag_can)
			//				{
			//					cont = 0;
			//					aux_list = sorted_npp[j2]->prox;
			//					sum_cost[j2] = fcostpo[j2];
			//					sum_revenue[j2] = 0.0;
			//					tabu_solp[j2] = 1;
			//					tabu_cost += fcostpo[j2];
			//					//Insere a oferta do novo produto aberto para os clientes na ordem de maior lucro individual de cliente, respeitando o orcamento
			//					while (aux_list != NULL && residual_budget[j2] >= 0)
			//					{
			//						if (active_clients[aux_list->inf] > 0 && residual_budget[j2] - cost[aux_list->inf][j2] >= 0)
			//						{
			//							sum_revenue[j2] += revenue[aux_list->inf][j2];
			//							sum_cost[j2] += cost[aux_list->inf][j2];
			//							residual_budget[j2] -= cost[aux_list->inf][j2]; //Possivel que o residual budget fique negativo, deve ser verificado antes de inserir a oferta
			//							tabu_solc[aux_list->inf][j2] = 1;
			//							active_clients[aux_list->inf]--;
			//							cont++;
			//						}
			//						aux_list = aux_list->prox;
			//					}
			//					sorted_npp[j2]->inf = cont;
			//					client_po[j2] = cont;

			//					*sum_cost_fix += sum_cost[j2];
			//					*sum_profit += sum_revenue[j2];
			//					//Se foi possivel antingir o numero minimo de ofertas
			//					if ((cont >= mcpo[j2]) &&
			//						(*sum_profit >= ((1 + hurdle)*(*sum_cost_fix))))
			//					{
			//						//Marca que a diversificacao foi feita e ajusta os parametros
			//						flag_diver = 1;
			//						used_product[j2] = 1;
			//						iter_no_diversification = 0;
			//						tabu_solp[j2] = 1;
			//						tabu_cost = (int)(*sum_profit - *sum_cost_fix);

			//					}
			//					else
			//					{
			//						//Senao remove a oferta de j2 e reajusta os parametros
			//						residual_budget[j2] = budgetpo[j2];
			//						*sum_cost_fix -= sum_cost[j2];
			//						*sum_profit -= sum_revenue[j2];
			//						sum_cost[j2] = 0.0;
			//						sum_revenue[j2] = 0.0;
			//						tabu_solp[j2] = 0;
			//						tabu_cost -= fcostpo[j2];
			//						sorted_npp[j2]->inf = 0;
			//						client_po[j2] = 0;
			//						for (i = 0; i < client; i++)
			//						{
			//							if (tabu_solc[i][j2] == 1)
			//							{
			//								tabu_solc[i][j2] = 0;
			//								active_clients[i]++;
			//							}
			//						}
			//					}
			//				}
			//			}

			//		}
			//		//Se nao foi achada diversificacao, reoferece o produto j1, restaurando a solucao ao estado original
			//		if ((flag_diver == 0) && (tabu_solp_copy[j1] != tabu_solp[j1]))
			//		{
			//			*sum_cost_fix += sum_cost[j1];
			//			*sum_profit += sum_revenue[j1];
			//			tabu_cost = (int)(*sum_profit - *sum_cost_fix);
			//			tabu_solp[j1] = tabu_solp_copy[j1];
			//			iter_no_diversification++;
			//			for (i = 0; i < client; i++)
			//			{
			//				if (tabu_solc_copy[i][j1] == 1)
			//				{
			//					residual_budget[j1] -= cost[i][j1];
			//					active_clients[i] -= 1;
			//					client_po[j1]++;
			//					tabu_solc[i][j1] = 1;
			//				}
			//				else tabu_solc[i][j1] = 0;
			//			}
			//			sorted_npp[j1]->inf = client_po[j1];

			//		}

			//	}
			//	if (flag_diver == 0) iter_no_improvement = 1000;
			//	else
			//	{
			//		flag_diver = 0;
			//		for (i = 0;i < client;i++)
			//		{
			//			for (j = 0;j < prod;j++)
			//			{
			//				tabu_list1[i][j] = 0;
			//				tabu_list2[i][j] = 0;
			//				tabu_list3[i][j] = 0;
			//			}
			//			tabu_list4[i] = 0;
			//		}
			//	}
			//	div2_flag = true;
			//}
			//printf("Diversification. Tabu_cost: %f\n", tabu_cost);
	  //  }



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
		delete tabu_list1[i];
	}
	delete tabu_list1;

	for (int i = 0; i < client; i++) {
		delete tabu_list2[i];
	}
	delete tabu_list2;
	for (int i = 0; i < client; i++) {
		delete tabu_list3[i];
	}
	delete tabu_list3;
	delete tabu_list4;

	for (int i = 0; i < client; i++) {
		delete sol_aux[i];
	}
	delete sol_aux;
	/*    printf("sol. Incumbente = %f\n",incumbent_solution);
		for (j=0;j<prod;j++) printf("client po [%d] = %d\n",j,client_po[j]);*/
}

/*Perforns tabu search utilising GA as a diversification mechanism*/
void tabu_search2(int client, int prod, float hurdle,
	int **cost, int **revenue, int* maxofer,
	int* mcpo, int* budgetpo, int* maxBudgetpo, int* fcostpo,
	int **solc, int* solp, float* solution_cost,
	List** sorted_npp, int* active_clients, int* active_products,
	float* sum_cost_fix, float* sum_profit, int* prod_can, int **profit, cromo *p1, cromo *p2, int* npp_sorted_prod_list, bool newpop, clock_t *initial_time, int *global_best, float *global_best_time, bool initialized)

{
	float incumbent_solution = *solution_cost; /* Store the Best Iteration so far */
	float tabu_cost = *solution_cost;
	List* aux_list;
	int **tabu_solc;
	tabu_solc = generate_2D_matrix_int(client, prod);
	int *tabu_solp;
	tabu_solp = new int[prod];
	int **tabu_solc_copy;
	tabu_solc_copy = generate_2D_matrix_int(client, prod);
	int *tabu_solp_copy;
	tabu_solp_copy = new int[prod];
	float tabu_cost_copy;
	float *sum_revenue;
	sum_revenue = new float[prod];
	float *sum_cost;
	sum_cost = new float[prod];
	int sum_cost_i1, sum_cost_i2, sum_revenue_i1, sum_revenue_i2;
	int aspiration_criteria = 0;
	int *used_product;
	used_product = new int[prod];
	int i_, i, j_, j, i1, i2, j1, j2, j1_, j2_;
	int iter = 1;                              /* Iteration counter */
	int iter_no_improvement = 1;               /* Iterations without improvement */
	int iter_no_diversification = 1;          /* Iterations without diversification */
	int diver_not_found = 1;
	int cont;
	int tabu_time = 2;
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
	int sum_prof_dif, sum_cost_dif;
	float previous_solution = tabu_cost;
	clock_t begin_phase, end_phase;
	float time_elapsed, last_time_mark;
	int conti1 = 0;
	int conti2 = 0;
	int iteration_counter;
	float initial_speed = 0;
	float total_profit;
	int val;
	float interval = 0.7;//**Testar intervalo de 100%
	int **CVP = create_cvp_npp_matrix(cost, revenue, prod, client);//Npp ordered list of client vs products
	int *VV = create_variance_vector(cost, revenue, client, prod, solp);//Vector of client indexes ordered by profit variance
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
	bool better_solution_found = false;
	int aux_budget[MAXPROD];
	float desvio_construtivo = 0.00;
	List *aux, *aux2;
	List* last_client[MAXPROD];
	bool new_pop = true;
	int **ic1 = (int**)malloc(sizeof(int*)*prod);
	int **ic2 = (int**)malloc(sizeof(int*)*prod);
	int total_prods = 0;
	bool initialized2 = initialized;
	int tabu[MAXPROD][MAXPROD][MAXC];
	for (j = 0; j < prod; j++)
	{

		ic2[j] = (int*)malloc(sizeof(int)*prod);
		ic1[j] = (int*)malloc(sizeof(int)*prod);
		for (i = 0; i < prod; i++)
		{
			ic1[j][i] = -1;
			ic2[j][i] = -1;
		}

		for (aux = sorted_npp[j]; aux->prox; aux = aux->prox);
		last_client[j] = aux;
	}
	for (i = 0; i < prod; i++)
	{
		if (solp[i] == 1)
			total_prods++;
		for (j = 0; j < prod; j++)
		{
			if (i != j)
			{
				for (i1 = 0; i1 < client; i1++)
				{
					tabu[i][j][i1] = -1;
				}
			}
		}
	}
	int limit = 3600;

	int best_j1, best_j2, best_i1, best_i2, best_solution, best_move, best_losing_j1, best_losing_j2, best_losing_i1, best_losing_i2, best_losing_move, best_losing_solution;
	bool hurdle_flag = false;
	for (j = 0; j < prod; j++)
	{
		residual_budget[j] = budgetpo[j];
		client_po[j] = 0;
		used_product[j] = 0;
	}
	if ((float)(*sum_profit / (*sum_cost_fix)) >= hurdle) hurdle_flag = true;
	best_i1 = -1;
	best_i2 = -1;
	best_j1 = -1;
	best_j2 = -1;
	best_solution = -10000;
	begin_phase = clock();
	time_elapsed = 0.0;
	int try_number;

	sum_cost_dif = 0;
	sum_prof_dif = 0;
	sum_cost_fix_aux = *sum_cost_fix;
	sum_profit_aux = *sum_profit;
	for (j = 0; j < prod; j++)
	{
		tabu_solp[j] = solp[j];
		for (i = 0; i < client; i++)
		{
			tabu_solc[i][j] = solc[i][j];
		}
	}
	try_number = 0;
	int last_result = 0;
	bool newpop2 = newpop;
	bool climb = true;
	int iter2 = 0;
	iter = 0;
	bool flag_tabu = true;
	int time_limit = 220;
	if (client == 100)
	{
		time_limit = 10;
	}
	else if (client == 500)
	{
		time_limit = 60;
	}
	else if (client < 10000)
	{
		time_limit = 110;
	}
	if (max_fo == 0)
		max_fo = *solution_cost;
	float loop_time = 0;
	clock_t aux_time = 0;
	
	compute_solution_value(client, prod, hurdle,
		cost, revenue, maxofer,
		mcpo, budgetpo, fcostpo,
		solc, solp, solution_cost, prod_can,false);

	/* loop for the tabu search until a stopping criteria be found (30 iterations with no improvement on the current solution or the time limit. */
	//In order to improve the speed of exploration, multiple movements can be done in a single loop.
	while (iter_no_improvement < 30 && time_elapsed <= time_limit )
	{
		best_losing_solution = INT_MIN;
		best_losing_move = -1;
		best_solution = -1000;
		if (max_fo < tabu_cost)
			max_fo = tabu_cost;
		if (iter_no_improvement % 10 == 0)//each 10 interations without improvement, a new initial population will be generate for the GA to avoid stagnation
		{
			newpop2 = true;
			desvio_construtivo += 0.20;
			if (desvio_construtivo > 0.7)
				desvio_construtivo = 0.7;
			interval += 0.1;
			if (interval > 1.0)
			{
				interval = 1;
			}
		}
		
		if (!better_solution_found && iter_no_improvement > 0 && hurdle_flag )
		{
			flag_tabu = false;

			//If it is the first iteration (initial_speed =0) or it is not the first try since the last improvement or diversitification, runs the GA (4 generations).
			if (try_number >= 0 || initial_speed == 0)
			{
				sum_cost_fix_aux = *sum_cost_fix + sum_cost_dif;
				sum_profit_aux = *sum_profit + sum_prof_dif;

				if (newpop2)//if the new population flag is true, generates a new population for the GA and runs the GA
				{
					Genloop_CAN(p1, p2, prod, client, mcpo, prod_can, budgetpo, maxofer, revenue, cost, profit, sorted_npp, hurdle, 0.10, 4, POPSIZE, fcostpo, 0.10, 0.03, tabu_solc, tabu_solp, residual_budget, &sum_cost_fix_aux, &sum_profit_aux, active_clients, active_products, newpop2, client_po, false, false, initialized2);
					newpop2 = false;
				}
				else
					Genloop_CAN(p1, p2, prod, client, mcpo, prod_can, budgetpo, maxofer, revenue, cost, profit, sorted_npp, hurdle, 0.10, 4, POPSIZE, fcostpo, 0.10, 0.03, tabu_solc, tabu_solp, residual_budget, &sum_cost_fix_aux, &sum_profit_aux, active_clients, active_products, newpop2, client_po, false, true, initialized2);
				//On the second and 4th GA try without improvements, uses the alternative constructive to generate part of the population and starts accepting worse solutions (diversification)
				if (try_number == 2  || try_number == 4 )
				{
					Genloop_CAN(p1, p2, prod, client, mcpo, prod_can, budgetpo, maxofer, revenue, cost, profit, sorted_npp, hurdle, 0.10, 2, POPSIZE, fcostpo, 0.3 + desvio_construtivo, 0.03, tabu_solc, tabu_solp, residual_budget, &sum_cost_fix_aux, &sum_profit_aux, active_clients, active_products, newpop2, client_po, true, false, initialized2);

					iter2 = iter + 1;
					max_fo = max_fo * (0.9);
				}
				else
					initial_speed = 0;//resets conververgence speed

				sum_cost_dif = sum_cost_fix_aux - *sum_cost_fix;
				sum_prof_dif = sum_profit_aux - *sum_profit;
				if (!initialized2)
				{
					initialized2 = true;
				}
				if (*solution_cost < (sum_profit_aux - sum_cost_fix_aux))
				{
					iter_no_improvement = 0;
					try_number = 0;
				}
				tabu_cost = sum_profit_aux - sum_cost_fix_aux;

				newpop2 = false;
				if (tabu_cost > *global_best)
				{
					*global_best_time = (clock() - *initial_time) / CLOCKS_PER_SEC;
					*global_best = tabu_cost;
				}
				last_result = (sum_profit_aux - sum_cost_fix_aux);
				for (i = 0; i < prod; i++)
				{
					if (tabu_solp[i] != solp[i])
					{
						VV = create_variance_vector(cost, revenue, client, prod, tabu_solp);
						if (!ic1[i])
						{
							ic1[i] = (int*)malloc(sizeof(int)*prod);
							ic2[i] = (int*)malloc(sizeof(int)*prod);

							for (j = 0; j < prod; j++)
							{
								ic1[i][j] = -1;
								ic2[i][j] = -1;
							}
						}
						break;
					}
				}
				if (try_number == 4)
				{
					try_number = 0;

				}

				try_number++;
			}
		}
		if (iter2 < iter)
		{
			flag_tabu = false;
		}
		better_solution_found = false;
		total_profit = 0;

		aux_time = clock();
		compute_solution_value(client, prod, hurdle,
			cost, revenue, maxofer,
			mcpo, budgetpo, fcostpo,
			solc, solp, solution_cost, prod_can,false);
		if ((float)(sum_profit_aux / sum_cost_fix_aux) >= hurdle) hurdle_flag = true;
		//Neighborhood 1
		if (1)
		{
			for (i = 0; i < prod; i++)
			{
				for (j = 0; j < prod; j++)
				{
					ic1[i][j] = -1;
					ic2[i][j] = -1;
				}
			}
			for (i1 = 0; i1 < client; i1++)
			{
				i = VV[i1];
				//Matrixes ic1 and ic2 are utilized record possible offer swaps (removing the clients i offer to product j1 and adding the offer of product j2 for the client i) that are not allowed to be performed due to budget restrictions
				//Starting with the client with the higher offer profit variability between products
				//Try to find a profitable offer swap 
				//If this offer swap does not violate budget restrictions, perform the swap
				//If the budget does not allow the swap, look for a recorded offer swap with contrary direction that would free up enough budget to make the current offer swap possible
				//If there is a second offer swap that allows the first one to be performed, perform both offer swaps
				//If there is no second offer swap that would allow the first one to be performed, record the offer swap possibility on the ic1 or ic2 matrix 
				for (j = 0, j1 = -1, j2 = -1; j < prod && (j1 < 0 || j2 < 0); j++)
				{
					if (tabu_solp[j]) //(tabu_solp[CVP[i][j]])
					{
						if (tabu_solc[i][j] == 0/*!(tabu_solc[i][CVP[i][j]] > 0)*/ /*&& j1 < 0 && j2 < 0*/)
						{
							j2 = j /*CVP[i][j]*/;
						}
						/*else if (tabu_solc[i][CVP[i][j]] && j2 < 0)
						{
							break;
						}*/
						else if (tabu_solc[i][j] > 0/*tabu_solc[i][CVP[i][j]]*/)
						{
							j1 = j/*CVP[i][j]*/;
						}
						if (j1 >= 0 && j2 >= 0)
							break;
					}
				}
				if (j1 >= 0 && j2 >= 0)//If a profitable offer swap was found for the client i
				{

					//If there is an reverse direction offer swap recorded in ic1
					if (ic1[j2][j1] >= 0)
					{	//If the offer record in ic1 frees enought budget to allow the current swap to be performed, perform both swaps
						if ((tabu[j2][j1][ic1[j2][j1]] < iter && tabu[j1][j2][i] < iter) || !flag_tabu)
							if (cost[ic1[j2][j1]][j1] <= cost[i][j1] + residual_budget[j1] && cost[i][j2] <= cost[ic1[j2][j1]][j2] + residual_budget[j2] && (revenue[i][j2] + revenue[ic1[j2][j1]][j1] - revenue[i][j1] - revenue[ic1[j2][j1]][j2] - cost[i][j2] - cost[ic1[j2][j1]][j1] + cost[i][j1] + cost[ic1[j2][j1]][j2] > 0))
							{
								tabu_solc[ic1[j2][j1]][j1] = 1;
								tabu_solc[ic1[j2][j1]][j2] = 0;
								tabu_solc[i][j2] = 1;
								tabu_solc[i][j1] = 0;

								residual_budget[j1] += cost[i][j1];
								residual_budget[j1] -= cost[ic1[j2][j1]][j1];
								residual_budget[j2] += cost[ic1[j2][j1]][j2];
								residual_budget[j2] -= cost[i][j2];

								tabu_cost += revenue[ic1[j2][j1]][j1] - cost[ic1[j2][j1]][j1] + revenue[i][j2] - cost[i][j2] - (revenue[i][j1] - cost[i][j1] + revenue[ic1[j2][j1]][j2] - cost[ic1[j2][j1]][j2]);
								sum_cost_dif += cost[ic1[j2][j1]][j1] + cost[i][j2] - cost[ic1[j2][j1]][j2] - cost[i][j1];
								sum_prof_dif += revenue[ic1[j2][j1]][j1] + revenue[i][j2] - revenue[ic1[j2][j1]][j2] - revenue[i][j1];
								total_profit += revenue[ic1[j2][j1]][j1] + revenue[i][j2] - revenue[ic1[j2][j1]][j2] - revenue[i][j1] - (cost[ic1[j2][j1]][j1] + cost[i][j2] - cost[ic1[j2][j1]][j2] - cost[i][j1]);
								best_solution = revenue[ic1[j2][j1]][j1] - cost[ic1[j2][j1]][j1] + revenue[i][j2] - cost[i][j2] - (revenue[i][j1] - cost[i][j1] + revenue[ic1[j2][j1]][j2] - cost[ic1[j2][j1]][j2]);
								if (flag_tabu)//If tabu is on, record the movement
								{
									tabu[j1][j2][ic1[j2][j1]] = iter + tabu_time;
									tabu[j2][j1][i] = iter + tabu_time;
								}
								if (ic2[j2][j1] >= 0)//if there is a offer swap recorded in ic2, move it to ic1.
								{
									ic1[j2][j1] = ic2[j2][j1];
									ic2[j2][j1] = -1;
								}
								else
									ic1[j2][j1] = -1;//Remove the performed swap from ic1
								better_solution_found = true;
								try_number = 0;

							}//If the offer swap on recorded on ic1 would not make the current offer swap possible, and there is an offer swap recorded on ic2 that would allow the current offer swap to be performed
							 //Perform both offer swaps
							else if (ic2[j2][j1] >= 0 && ((tabu[j2][j1][ic2[j2][j1]] < iter || tabu[j1][j2][i] < iter || !flag_tabu)) &&
								cost[ic2[j2][j1]][j1] <= cost[i][j1] + residual_budget[j1] && cost[i][j2] <= cost[ic2[j2][j1]][j2] + residual_budget[j2] && (revenue[i][j2] + revenue[ic2[j2][j1]][j1] - revenue[i][j1] - revenue[ic2[j2][j1]][j2] - cost[i][j2] - cost[ic2[j2][j1]][j1] + cost[i][j1] + cost[ic2[j2][j1]][j2] > 0))
							{

								tabu_solc[ic2[j2][j1]][j1] = 1;
								tabu_solc[ic2[j2][j1]][j2] = 0;
								tabu_solc[i][j2] = 1;
								tabu_solc[i][j1] = 0;

								residual_budget[j1] += cost[i][j1];
								residual_budget[j1] -= cost[ic2[j2][j1]][j1];
								residual_budget[j2] += cost[ic2[j2][j1]][j2];
								residual_budget[j2] -= cost[i][j2];

								tabu_cost += revenue[ic2[j2][j1]][j1] - cost[ic2[j2][j1]][j1] + revenue[i][j2] - cost[i][j2] - (revenue[i][j1] - cost[i][j1] + revenue[ic2[j2][j1]][j2] - cost[ic2[j2][j1]][j2]);
								sum_cost_dif += cost[ic2[j2][j1]][j1] + cost[i][j2] - cost[ic2[j2][j1]][j2] - cost[i][j1];
								sum_prof_dif += revenue[ic2[j2][j1]][j1] + revenue[i][j2] - revenue[ic2[j2][j1]][j2] - revenue[i][j1];
								total_profit += revenue[ic2[j2][j1]][j1] + revenue[i][j2] - revenue[ic2[j2][j1]][j2] - revenue[i][j1] - (cost[ic2[j2][j1]][j1] + cost[i][j2] - cost[ic2[j2][j1]][j2] - cost[i][j1]);
								best_solution = revenue[ic2[j2][j1]][j1] - cost[ic2[j2][j1]][j1] + revenue[i][j2] - cost[i][j2] - (revenue[i][j1] - cost[i][j1] + revenue[ic2[j2][j1]][j2] - cost[ic2[j2][j1]][j2]);
								ic2[j2][j1] = -1;
								better_solution_found = true;
								if (flag_tabu)
								{
									tabu[j1][j2][ic2[j2][j1]] = iter + tabu_time;
									tabu[j2][j1][i] = iter + tabu_time;
								}
								try_number = 0;
							}//Otherwise, if there is no offer swap recorded on ic2 or the profit from the current swap is higher than the offer swap recored in ic2, record current swap on ic2
							else if (ic2[j1][j2] < 0 && (revenue[i][j2] - cost[i][j2] >= revenue[i][j1] - cost[i][j1]) && client_po[j1]>mcpo[j1])
							{
								ic2[j1][j2] = i;
							}
							else if (ic2[j1][j2] >= 0 && (revenue[i][j2] - cost[i][j2] >= revenue[i][j1] - cost[i][j1]) && client_po[j1] > mcpo[j1])	
							{
								if ((revenue[i][j2] - revenue[i][j1] + cost[i][j1] - cost[i][j2] < revenue[ic2[j1][j2]][j2] - revenue[ic2[j1][j2]][j1] + cost[ic2[j1][j2]][j1] - cost[ic2[j1][j2]][j2]))
									ic2[j1][j2] = i;
							}
					}
					else//If there is no swap recorded on ic1
					{
						//If the budget allows the current swap to be performed, perform the current swap.
						if (tabu[j1][j2][i] < iter || !flag_tabu)
							if (revenue[i][j2] - revenue[i][j1] - cost[i][j2] + cost[i][j1] > 0 && client_po[j1] > mcpo[j1] && residual_budget[j2] >= cost[i][j2] &&
								((hurdle_flag == false) || (((*sum_profit) + sum_prof_dif + revenue[i][j2] - revenue[i][j1]) >=
								((1 + hurdle)*((*sum_cost_fix) + sum_cost_dif + cost[i][j2] - cost[i][j1])))))
							{
								tabu_solc[i][j2] = 1;
								tabu_solc[i][j1] = 0;
								residual_budget[j2] -= cost[i][j2];
								residual_budget[j1] += cost[i][j1];

								sum_cost_dif += cost[i][j2] - cost[i][j1];
								sum_prof_dif += revenue[i][j2] - revenue[i][j1];
								tabu_cost += revenue[i][j2] - revenue[i][j1] - (cost[i][j2] - cost[i][j1]);
								total_profit += revenue[i][j2] - revenue[i][j1] - (cost[i][j2] - cost[i][j1]);
								client_po[j1]--;
								client_po[j2]++;
								if (flag_tabu)
								{
									tabu[j2][j1][i] = iter + tabu_time;
								}
								better_solution_found = true;
								try_number = 0;
							}
							else//Otherwise, record the current swap on ic1
							{
								if ((revenue[i][j2] - cost[i][j2] >= revenue[i][j1] - cost[i][j1]) && client_po[j1] > mcpo[j1])
									ic1[j1][j2] = i;
							}
					}
				}
			}
			for (j = 0; j < prod; j++)//Try to fit new offers in the current solution.
			{
				if (tabu_solp[j] && residual_budget[j] > 0)
				{
					for (aux_list = sorted_npp[j]->prox; aux_list; aux_list = aux_list->prox)
					{
						if (tabu_solc[aux_list->inf][j] == 0 && active_clients[aux_list->inf] > 0 && residual_budget[j] >= cost[aux_list->inf][j] && cost[aux_list->inf][j] < revenue[aux_list->inf][j])
							if (!(tabu_solc[aux_list->inf][j] > 0) &&
								((hurdle_flag == false) || (((*sum_profit) + sum_prof_dif + revenue[aux_list->inf][j]) >=
								((1 + hurdle)*((*sum_cost_fix) + sum_cost_dif + cost[aux_list->inf][j])))))
							{
								tabu_solc[aux_list->inf][j] = 1;
								residual_budget[j] -= cost[aux_list->inf][j];
								sum_cost_dif += cost[aux_list->inf][j];
								sum_prof_dif += revenue[aux_list->inf][j];
								tabu_cost += revenue[aux_list->inf][j] - cost[aux_list->inf][j];
								total_profit += revenue[aux_list->inf][j] - cost[aux_list->inf][j];
								active_clients[aux_list->inf]--;
								client_po[j]++;
								better_solution_found = true;
								//printf("Bestmove: alt1 Tabu_cost: %f Best_Solution: %d\n", tabu_cost, revenue[aux_list->inf][j] - cost[aux_list->inf][j]);
							}
					}
				}
			}
			if (tabu_cost > *solution_cost)//If the series of alterations performed on the previous neighborhood created a solution with a profit higher than the incumbent solution, save the alterations on the incunbent solution.
			{
				if (tabu_cost > *global_best)
				{
					*global_best_time = (clock() - *initial_time) / CLOCKS_PER_SEC;
					*global_best = tabu_cost;
				}
				try_number = 0;
				for (j = 0; j < prod; j++)
				{
					solp[j] = tabu_solp[j];
					for (i = 0; i < client; i++)
					{
						solc[i][j] = tabu_solc[i][j];
					}
				}
				//Verifies solution correctness
				for (i = 0; i < client; i++)
				{
					for (j = 0, val = maxofer[i]; j < prod; j++)
					{
						if (solc[i][j] != 0)
						{
							val--;
						}
					}
					if (val != active_clients[i])
					{
						printf("Solucao inconsistente com os clientes ativos.\n");
					}
				}
				*solution_cost = tabu_cost;
				*sum_profit += sum_prof_dif;
				*sum_cost_fix += sum_cost_dif;
				total_profit += sum_prof_dif - sum_cost_dif;
				sum_cost_dif = 0;
				sum_prof_dif = 0;
				iter_no_improvement = 0;
			}
		}
		/*Hurdle rate was met on the current solution*/
		if ((float)(sum_profit_aux / sum_cost_fix_aux) >= hurdle) hurdle_flag = true;



		//Neighborhood 2
		for (j1 = 0; (j1 < prod); j1++) /* loop for the neighborhood of swap,  i.e., if prod[j] is active and customer[i1][j]=1 and customer[i2][j]=0 */
													/* do customer[i1][j]=0 and customer[i2][j]=1 that improves the solution value */
		{
			//If product j1 is being offered on the current solution
			iteration_counter = 0;
			if (tabu_solp[j1] != 0)
			{

				//Parar busca local quando forem testados clientes sem oferta equivalentes a 20% do tamanho da populacao de clientes 
				//Explores the client qeue decreasing order of NPP (average of return for each monetary unity invested.)
				for (aux = sorted_npp[j1]->prox, i_ = 0; (i_ < client - 1 && iteration_counter < interval*client); i_++, aux = aux->prox)
				{
					i1 = aux->inf;

					//If the product j1 is not offered to the client i1
					if (tabu_solc[i1][j1] == 0 && (active_clients[i1] > 0))
					{
						iteration_counter++;
						//Explores the client qeue in increasing order of NPP (average of return for each monetary unity invested.)
						for (aux2 = last_client[j1]; (aux2); aux2 = aux2->ant)
						{
							i2 = aux2->inf;
							//If the product j1 is offered to the client i2 and the swap of the product j1 from the client i2 to the client i1 would be profitable
							//Perform the offer swap start looking for a new client without an offer to the product j1
							if (tabu[j1][j1][i2] < iter || !flag_tabu)
								if (/*(tabu_list1[i2][j1] < iter) && (tabu_list1[i1][j1] < iter) && */(tabu_solc[i2][j1] == 1) && (cost[i1][j1] < residual_budget[j1] + cost[i2][j1]) &&
									((hurdle_flag == false) || (((*sum_profit) + sum_prof_dif - revenue[i2][j1] + revenue[i1][j1]) >=
									((1 + hurdle)*((*sum_cost_fix) + sum_cost_dif + cost[i1][j1] - cost[i2][j1])))))
								{
									/*If the movement is profitable, peforms the movement and starts exploring a new product*/
									if ((0 < ((revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j1] - cost[i2][j1]))))
									{
										better_solution_found = true;
										best_i1 = i2;
										best_i2 = i1;
										best_j1 = j1;
										best_move = 1;
										if (flag_tabu)
										{
											tabu[j1][j1][i1] = iter + tabu_time;
										}
										best_solution = (revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j1] - cost[i2][j1]);
										flag_move1 = 1;
										tabu_solc[i1][j1] = 1;
										tabu_solc[i2][j1] = 0;
										active_clients[i2] += 1;
										active_clients[i1] -= 1;
										residual_budget[j1] += cost[i2][j1];
										residual_budget[j1] -= cost[i1][j1];
										tabu_cost += best_solution;
										incumbent_solution = tabu_cost;
										sum_prof_dif += revenue[i1][j1] - revenue[i2][j1];
										sum_cost_dif += cost[i1][j1] - cost[i2][j1];
										best_solution = -10000;
										try_number = 0;
										break;
									}
									else if (!better_solution_found && best_losing_solution < (revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j1] - cost[i2][j1]) && (revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j1] - cost[i2][j1]) != 0)
									{
										best_losing_i1 = i2;
										best_losing_i2 = i1;
										best_losing_j1 = j1;
										best_losing_move = 1;
										best_losing_solution = (revenue[i1][j1] - cost[i1][j1]) - (revenue[i2][j1] - cost[i2][j1]);
									}
								}
						}
					}
				}
			}
		}
		//If the series of alterations performed on the previous neighborhood created a solution with a profit higher than the incumbent solution, save the alterations on the incunbent solution.
		if (tabu_cost > *solution_cost)
		{
			if (tabu_cost > *global_best)
			{
				*global_best_time = (clock() - *initial_time) / CLOCKS_PER_SEC;
				*global_best = tabu_cost;
			}
			try_number = 0;
			for (j = 0; j < prod; j++)
			{
				solp[j] = tabu_solp[j];
				for (i = 0; i < client; i++)
				{
					solc[i][j] = tabu_solc[i][j];
				}
			}
			//Verifies solution correctness
			for (i = 0; i < client; i++)
			{
				for (j = 0, val = maxofer[i]; j < prod; j++)
				{
					if (solc[i][j] != 0)
					{
						val--;
					}
				}
				if (val != active_clients[i])
				{
					printf("Solucao inconsistente com os clientes ativos.\n");
				}
			}
			*solution_cost = tabu_cost;
			*sum_profit += sum_prof_dif;
			*sum_cost_fix += sum_cost_dif;
			sum_cost_dif = 0;
			sum_prof_dif = 0;
			iter_no_improvement = 0;
		}
		if ((float)(sum_profit_aux / sum_cost_fix_aux) >= hurdle) hurdle_flag = true;

			//Neighborhood 3
		for (j1 = 0; (j1 < prod - 1 && !flag_move1 && !flag_move2); j1++) /* loop for the neighborhood of swap,  i.e., if prod[j1] and prod[j2] is active and */
													 /*    customer[i1][j1]=1 and customer[i2][j1]=0  and customer[i1][j2]=0 and customer[i2][j2]=1*/
													 /* do customer[i1][j1]=0 and customer[i2][j1]=1  and customer[i1][j2]=1 and customer[i2][j2]=0 that improves the solution value */
		{
			
			//If the product j1 is offered on the current solution
			if (tabu_solp[j1] != 0)
			{
				for (j2 = j1 + 1; (j2 < prod && !flag_move2); j2++)
				{
					//If the product j2 is offered on the current solution
					if ((tabu_solp[j2] != 0) && (j1 != j2))
					{
						//Explores j1 offers in order of profitability
						for (aux = sorted_npp[j1]->prox; (aux); aux = aux->prox)
						{
							i1 = aux->inf;
							//Explores j1 offers in order of profitabily
							//Only a percentage of the offers are explored. This percentage is defined by interval.
							for (aux2 = sorted_npp[j2]->prox, j_ = 0; (aux2 && j_ < client*interval); aux2 = aux2->prox)
							{
								i2 = aux2->inf;
								//If the product j1 is being offered to the client i1 and it is not being offered to the client i2
								//And the product j2 is not being offered to the client i1 and it is being offered to the client i2
								if ((tabu_solc[i1][j1] == 1) && (tabu_solc[i2][j1] == 0) &&
									(tabu_solc[i1][j2] == 0) && (tabu_solc[i2][j2] == 1))
								{
									j_++;
									//If the offer swap satifies the hurdle rate and budget restraints, perform the swap and continue to look for new swap.
									if ((tabu[j1][j2][i1] < iter && tabu[j2][j1][i2] < iter) || !flag_tabu)
										if (((cost[i2][j1] - cost[i1][j1]) < residual_budget[j1]) &&
											((cost[i1][j2] - cost[i2][j2]) < residual_budget[j2]) &&
											((hurdle_flag == false) || (((*sum_profit) + sum_prof_dif - revenue[i1][j1] + revenue[i2][j1] + revenue[i1][j2] - revenue[i2][j2]) >=
											((1 + hurdle)*((*sum_cost_fix) + sum_cost_dif - cost[i1][j1] + cost[i2][j1] + cost[i1][j2] - cost[i2][j2])))))
										{
											//Verifies if the new solution will be better than the incumbent solution.
											//If it is better, saves the new incumbent solution and stops the neighborhood exploration
											if ((0 < (((revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]))
												+ ((revenue[i1][j2] - cost[i1][j2]) - (revenue[i2][j2] - cost[i2][j2])))))
											{
												better_solution_found = true;
												best_solution = ((revenue[i2][j1] - cost[i2][j1]) - (revenue[i1][j1] - cost[i1][j1]))
													+ ((revenue[i1][j2] - cost[i1][j2]) - (revenue[i2][j2] - cost[i2][j2]));
												tabu_solc[i1][j1] = 0;
												tabu_solc[i2][j1] = 1;
												tabu_solc[i1][j2] = 1;
												tabu_solc[i2][j2] = 0;
												if (flag_tabu)
												{
													tabu[j1][j2][i2] = iter + tabu_time;
													tabu[j2][j1][i1] = iter + tabu_time;
												}
												residual_budget[j1] -= (cost[i2][j1] - cost[i1][j1]);
												residual_budget[j2] -= (cost[i1][j2] - cost[i2][j2]);
												tabu_cost += best_solution;
												incumbent_solution = tabu_cost;
												sum_prof_dif += revenue[i2][j1] - revenue[i1][j1] + revenue[i1][j2] - revenue[i2][j2];
												sum_cost_dif += cost[i2][j1] - cost[i1][j1] + cost[i1][j2] - cost[i2][j2];
												best_solution = -10000;
												try_number = 0;
												break;
											}
										}

								}
							}
						}
					}
				}
			}
		}
		//If the series of alterations performed on the previous neighborhood created a solution with a profit higher than the incumbent solution, save the alterations on the incunbent solution.
		if (tabu_cost > *solution_cost)
		{
			if (tabu_cost > *global_best)
			{
				*global_best_time = (clock() - *initial_time) / CLOCKS_PER_SEC;
				*global_best = tabu_cost;
			}
			try_number = 0;
			for (j = 0; j < prod; j++)
			{
				solp[j] = tabu_solp[j];
				for (i = 0; i < client; i++)
				{
					solc[i][j] = tabu_solc[i][j];
				}
			}
			//Verifies solution correctness
			for (i = 0; i < client; i++)
			{
				for (j = 0, val = maxofer[i]; j < prod; j++)
				{
					if (solc[i][j] != 0)
					{
						val--;
					}
				}
				if (val != active_clients[i])
				{
					printf("Solucao inconsistente com os clientes ativos.\n");
				}
			}
			*solution_cost = tabu_cost;
			*sum_profit += sum_prof_dif;
			*sum_cost_fix += sum_cost_dif;
			total_profit += sum_prof_dif - sum_cost_dif;
			sum_cost_dif = 0;
			sum_prof_dif = 0;
			iter_no_improvement = 0;


		}
		if ((float)(sum_profit_aux / sum_cost_fix_aux) >= hurdle) hurdle_flag = true;


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




		if (!better_solution_found)
		{
			iter_no_improvement++;
		}
		else
		{
			interval = 0.7;
		}


		flag_move1 = 0;
		flag_move2 = 0;
		flag_move3 = 0;
		flag_move4 = 0;
		end_phase = clock();
		loop_time = (end_phase - aux_time) / CLOCKS_PER_SEC;
		time_elapsed = (end_phase - begin_phase) / CLOCKS_PER_SEC;
		last_time_mark = end_phase - begin_phase;
		if (initial_speed == 0)
		{
			initial_speed = total_profit / last_time_mark;
		}

		//printf("Time_elapsed=%f\n",time_elapsed);

		iter++;
		printf("Custo no final do loop: %f\n", tabu_cost);
		if (time_elapsed > time_limit && tabu_cost > max_fo*1.005 && time_elapsed < limit/**(client / 10000)*/)
		{
			time_limit += time_elapsed - time_limit;
			time_limit += 4 * loop_time;

		}
		if (time_elapsed >= limit)
			break;


	}



	for (int i = 0; i < client; i++) {
		delete sol_aux[i];
	}
	delete sol_aux;

	for (j = 0; j < prod; j++)
	{
		/*if (solp[j])
			credits[j] = (short int*)malloc(sizeof(short int)*prod);*/

		free(ic2[j]);
		free(ic1[j]);
	}

	free(ic1);
	free(ic2);

	for (i = 0; i < prod; i++)
	{
		delete tabu_solc_copy[i];
		delete tabu_solc[i];
	}
	delete tabu_solc_copy;
	delete tabu_solp_copy;
	delete tabu_solc;
	delete tabu_solp;
	delete sum_revenue;
	delete used_product;
	delete sum_cost;

}

