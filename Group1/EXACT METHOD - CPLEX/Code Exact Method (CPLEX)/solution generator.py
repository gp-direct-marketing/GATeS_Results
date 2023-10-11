import cplex
import sys
import os
import xml.etree.ElementTree


caminho_projeto = "C:/endereço/"
caminho_todas_instancias = os.path.join(
    caminho_projeto, "instancias-gerar-solucao")
caminho_resultado = os.path.join(caminho_projeto, "resultados-solucao")
caminho_logs = os.path.join(caminho_projeto, "resultados-log")


def arquivo_txt_padronizado(caminho_resultado_instancia, nome_instancia, solve_file, solution_time, clientes, produtos):
    solve_xml = xml.etree.ElementTree.parse(solve_file).getroot()
    objective_value = solve_xml.find("header").attrib["objectiveValue"]
    variables = solve_xml.find("variables")
    ofertar_produtos = [0 for x in range(produtos)]
    matriz_variaveis = [[0 for x in range(clientes)] for y in range(produtos)]
    for variable in variables:
        variable_name = variable.attrib["name"].split("#")
        value = int(float(variable.attrib["value"]))
        if variable_name[0] == "x":
            x = int(variable_name[1]) - 1
            y = int(variable_name[2]) - 1
            matriz_variaveis[y][x] = value
        elif variable_name[0] == "y":
            y = int(variable_name[1]) - 1
            ofertar_produtos[y] = value

    arquivo_txt = os.path.join(
        caminho_resultado_instancia, f"Sol_Cplex_{nome_instancia}.txt")
    file = open(arquivo_txt, "w")
    file.write(f"Sol_ObjectiveValue = {objective_value}\n")
    file.write(f"Time_Solve = {solution_time}\n")
    for y, y_val in enumerate(ofertar_produtos):
        file.write(f"\nprod[{y}] = {y_val}\n")
        for x, x_val in enumerate(matriz_variaveis[y]):
            file.write(f"client[{x}][{y}] = {x_val}\n")
    file.close()


def get_clientes_produtos(arquivo_instancia):
    dados = arquivo_instancia.split("-")
    if dados[0] == "S1":
        clientes = 100
    elif dados[0] == "S2":
        clientes = 200
    elif dados[0] == "S3":
        clientes = 300
    elif dados[0] == "M1":
        clientes = 1000
    elif dados[0] == "M2":
        clientes = 2000
    elif dados[0] == "L":
        clientes = 10000
    return clientes, int(dados[2])


for instancia in os.listdir(caminho_todas_instancias):
    caminho_resultado_instancia = os.path.join(caminho_resultado, instancia)
    caminho_instancia = os.path.join(caminho_todas_instancias, instancia)
    if not os.path.exists(caminho_resultado_instancia):
        os.mkdir(caminho_resultado_instancia)
    caminho_resultado_logs = os.path.join(caminho_logs, instancia)
    if not os.path.exists(caminho_resultado_logs):
        os.mkdir(caminho_resultado_logs)
    for i in os.listdir(caminho_instancia):
        if i.endswith(".lp"):
            nome_instancia = i.split('.lp')[0]
            arquivo_lp = os.path.join(caminho_instancia, i)
            c = cplex.Cplex(arquivo_lp)
            c.set_results_stream(os.path.join(caminho_resultado_logs,
                                              nome_instancia + ".log"))
            start_time = c.get_time()
            c.solve()
            end_time = c.get_time()
            clientes, produtos = get_clientes_produtos(nome_instancia)
            solve_file = os.path.join(
                caminho_resultado_instancia, f"{nome_instancia}.sol")
            c.solution.write(solve_file)
            arquivo_txt_padronizado(caminho_resultado_instancia, nome_instancia, solve_file, end_time -
                                    start_time, clientes, produtos)

print("\nFinalizado.")
