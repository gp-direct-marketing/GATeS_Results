# Paper
#Cetin, F. and Alabas-Uslu, C. (2015). Heuristic solution to the product targeting problem based
#on mathematical programming. Int. Journal of Production Research. 55(1):3–17.
# Implemented by Felipe Martins Müller

using JuMP, Gurobi, DelimitedFiles
function solucionar_modelo(instancia, output_instancia)
    let
        # Reading the external file according to readme.txt
        arq = open(instancia, "r")
        linha = readline(arq, keep=true)
        pieces = split(linha, ' ', keepempty=false)
        #The first line with three columns contains successively the number of clients, the number of products and the hurdle rate.
        m=parse(Int64, pieces[1])   #number o clients
        n=parse(Int64, pieces[2])   #number of products
        R=parse(Float64, pieces[3]) #Hurdle rate
        #The next m lines each have 2n+1 columns.
        #The ith line contains the data corresponding to the  ith client.
        #At that line, the first n columns represent the cost of offering each product to that client
        #while the next n columns report expected revenue per product if the considered product is accepted by that client.
        #Finally, the last column contains an integer representing the maximum number of offers that can be made to that client.
        c = Array{Int64}(undef,m+1,n) #unit cost of offering product j to client i
        c = zeros(m+1,n)
        p = Array{Int64}(undef,m+1,n) #expected return to the firm from offering of product j to client i
        p = zeros(m+1,n)
        M = Array{Int64}(undef,m)   #maximum number of offers that can be exhibited to client i
        O = Array{Int64}(undef,n)   #minimum quantity commitment bound of product j
        B = Array{Int64}(undef,n)   #budget for product j
        f = Array{Int64}(undef,n)   #fixed cost of including product j in promotion campaign
        #println("m=$m, n=$n, R=$R")

        for i=1 : m
            linha = readline(arq, keep=true)
            pieces = split(linha, ' ', keepempty=false)
            for j=1 : n
                c[i,j]=parse(Int64, pieces[j])
                p[i,j]=parse(Int64, pieces[j+n])
                #println("i=$i  j=$j, c=",c[i,j],"  p=",p[i,j])
            end
            M[i]=parse(Int64, pieces[n*2+1])
            #println("i=$i  M=",M[i])
        end
        #line m+2 gives for each product the minimum number of clients who must receive an offer of that product is used during the campaign
        linha = readline(arq, keep=true)
        pieces = split(linha, ' ', keepempty=false)
        for j=1 : n
            O[j] = parse(Int64, pieces[j])
            #println("j=$j  O=",O[j])
        end
        #The last but one line reports for each product the budget available
        linha = readline(arq, keep=true)
        pieces = split(linha, ' ', keepempty=false)
        for j=1 : n
            B[j] = parse(Int64, pieces[j])
            #println("j=$j  B=",B[j])
        end
        #The last line reports the fixed cost for each product.
        linha = readline(arq, keep=true)
        pieces = split(linha, ' ', keepempty=false)
        for j=1 : n
            f[j] = parse(Int64, pieces[j])
            #println("j=$j  f=",f[j])
        end
        close(arq)

        itime_to_model2 = time_ns() #Start to count time to execute the relaxed model called Model 2
        # MODELO
        modelo = Model(with_optimizer(Gurobi.Optimizer, Presolve=0))
        @variables modelo begin
            x[1:m+1,1:n] >= 0 #xij is one if product j if offered to the client i, zero otherwise (Now the variables are relaxed)
            #y[1:n] >= 0 the variable yj is replaced by yj=1-(xm+1j/Oj)
        end
        @objective(modelo, Max, sum(sum((p[i,j]-c[i,j])*x[i,j] for i=1:m) for j=1:n) - sum(f[j]*(1-x[m+1,j]/O[j]) for j=1:n))
        @constraints modelo begin
            c1, sum(sum(p[i,j]*x[i,j] for i=1:m) for j=1:n) >= (1+R)*(sum(sum(c[i,j]*x[i,j] for i=1:m) for j=1:n) + sum(f[j]*(1-x[m+1,j]/O[j]) for j=1:n))
            c2[j=1:n], sum(c[i,j]*x[i,j] for i=1:m) <= B[j]*(1-x[m+1,j]/O[j])
            c3[i=1:m], sum(x[i,j] for j=1:n) <= M[i]
            c4[j=1:n], sum(x[i,j] for i=1:m+1) >= O[j]
            c5[j=1:n], x[m+1,j] <= O[j]
            c6[i=1:m,j=1:n], x[i,j] <= 1
        end
        #print(modelo)
        status = optimize!(modelo)
        ftime_to_model2 = time_ns() # stop to count time to execute relaxed model called Model 2
        elaptime_to_model2 = (ftime_to_model2 - itime_to_model2) / 1.0e9
        # storing results
        arq_saida = open(output_instancia, "w")
        println(arq_saida,"Result of Relaxed Model2 of Cetin, F. and Alabas-Uslu, C. (2015).article")
        println(arq_saida,"Number of Products = $n   Number of Clients = $m    Hurdle Rate = $R")
        println(arq_saida," ")
        println(arq_saida,"Custo total: ", objective_value(modelo))
        println(arq_saida,"Elapsed Time: $elaptime_to_model2 sec")
        #=
        for j = 1 : n
            for i = 1 : m+1
            println("x[$i , $j] = ",value(x[i,j]))
        end
        end
        =#
        #Now we are goign to prepare the Phase 2 of the Heuristic of Cetin, Alabas-Uslu
        #computing the constant C, and X[m+1,j]/O[j] in order to compute H-R1
        itime_to_model3 = time_ns() #Start to count time to execute the heuristic solution H-R1
        C = sum(O) - sum(M)
        #   println("C = $C")
        #Compute (xm+1j/Oj) for each j
        MplusOne = Array{Float64}(undef,n)
        N=[1:1:n;]
        Nlinha=copy(N)
        for j in N
            MplusOne[j] = value(x[m+1,j])/O[j]
            #    println("MplusOne[$j] = ",MplusOne[j])
        end
        k = argmax(MplusOne)
        Ne = Int64[]
        valor_fo = 0
        Ne = push!(Ne,k)
        SomaOj = O[k]
        MplusOne[k] = 0.0
        Nlinha = deleteat!(Nlinha,k)
        while SomaOj < C
            k = argmax(MplusOne)
            Ne = push!(Ne,k)
            SomaOj += O[k]
            MplusOne[k] = 0.0
            k=findall(x->x==k, Nlinha)
            Nlinha = deleteat!(Nlinha,k)
        end

        #Now we will solve the Model 3 until find a feasible solution to obtain a heuristic solution
        flag_of_infeasibility = true
        while flag_of_infeasibility
            # MODELO3
            model3 = Model(with_optimizer(Gurobi.Optimizer, Presolve=1))
            @variables model3 begin
                x[1:m, Nlinha], Bin #xij is one if product j if offered to the client i, zero otherwise (Now the variables are relaxed)
                #y[1:n] >= 0 the variable yj is replaced by yj=1-(xm+1j/Oj)
            end
            @objective(model3, Max, sum(sum((p[i,j]-c[i,j])*x[i,j] for i=1:m) for j in Nlinha) - sum(f[j] for j in Nlinha))
            @constraints model3 begin
                c1, sum(sum(p[i,j]*x[i,j] for i=1:m) for j in Nlinha) >= (1+R)*(sum(sum(c[i,j]*x[i,j] for i=1:m) for j in Nlinha) + sum(f[j] for j in Nlinha))
                c2[j in Nlinha], sum(c[i,j]*x[i,j] for i=1:m) <= B[j]
                c3[i=1:m], sum(x[i,j] for j in Nlinha) <= M[i]
                c4[j in Nlinha], sum(x[i,j] for i=1:m) >= O[j]
            end
            #print(modelo)
            status = optimize!(model3)
            if (termination_status(model3) == MOI.OPTIMAL)
                flag_of_infeasibility = false
                valor_fo = objective_value(model3)         else
                # now we adjust the parameters from the model 3 withdrawing one product each
                # time until find a feasible solution
                k = argmax(MplusOne)
                Ne = push!(Ne,k)
                MplusOne[k] = 0.0
                k=findall(x->x==k, Nlinha)
                Nlinha = deleteat!(Nlinha,k)
                if isempty(Nlinha)
                    flag_of_infeasibility = false
                end
            end
        end # end while flag_of_infeasibility
        ftime_to_model3 = time_ns() #Stop to count time to execute the heuristic solution H-R1
        elaptime_to_model3 = (ftime_to_model3 - itime_to_model3) / 1.0e9
        println(arq_saida," ")
        println(arq_saida,"Result of Relaxed Model3 (H-R1) of Cetin, F. and Alabas-Uslu, C. (2015).article")
        println(arq_saida," ")
        if isempty(Nlinha)
            println(arq_saida,"IMPOSSIBLE TO SOLVE THE PROBLEM: HEURISTIC REMOVE ALL PRODUCTS FROM CAMPAIGN")
        else
            println(arq_saida,"Custo total: ", valor_fo)
        end
        println(arq_saida,"Elapsed Time: $elaptime_to_model3 sec")

        #Now we are goign to prepare the Phase 2 of the Heuristic of Cetin, Alabas-Uslu
        #computing the constant C (we use the same as above), and f[j]/B[j] in order to compute H-R2
        itime_to_model4 = time_ns()
        foverB = Array{Float64}(undef,n)
        Nlinha=copy(N)
        for j in N
            foverB[j] = f[j]/B[j]
            #println("foverB[$j] = ",foverB[j])
        end
        k = argmax(foverB)
        Ne = Int64[]
        Ne = push!(Ne,k)
        SomaOj = O[k]
        foverB[k] = 0.0
        Nlinha = deleteat!(Nlinha,k)
        while SomaOj < C
            k = argmax(foverB)
            Ne = push!(Ne,k)
            SomaOj += O[k]
            foverB[k] = 0.0
            k=findall(x->x==k, Nlinha)
            Nlinha = deleteat!(Nlinha,k)
        end
        #Now we will solve the Model 3 until find a feasible solution to obtain a heuristic solution
        flag_of_infeasibility = true
        while flag_of_infeasibility
            #        MODELO4 Heuristic Rule H-R2
            modelo4 = Model(with_optimizer(Gurobi.Optimizer, Presolve=0))
            @variables modelo4 begin
                x[1:m, Nlinha], Bin #xij is one if product j if offered to the client i, zero otherwise (Now the variables are relaxed)
                #y[1:n] >= 0 the variable yj is replaced by yj=1-(xm+1j/Oj)
            end
            @objective(modelo4, Max, sum(sum((p[i,j]-c[i,j])*x[i,j] for i=1:m) for j in Nlinha) - sum(f[j] for j in Nlinha))
            @constraints modelo4 begin
                c1, sum(sum(p[i,j]*x[i,j] for i=1:m) for j in Nlinha) >= (1+R)*(sum(sum(c[i,j]*x[i,j] for i=1:m) for j in Nlinha) + sum(f[j] for j in Nlinha))
                c2[j in Nlinha], sum(c[i,j]*x[i,j] for i=1:m) <= B[j]
                c3[i=1:m], sum(x[i,j] for j in Nlinha) <= M[i]
                c4[j in Nlinha], sum(x[i,j] for i=1:m) >= O[j]
            end
            #print(modelo)
            status = optimize!(modelo4)
            if (termination_status(modelo4) == MOI.OPTIMAL)
                flag_of_infeasibility = false
                valor_fo = objective_value(modelo4)
            else
                # now we adjust the parameters from the model 3 withdrawing one product each
                # time until find a feasible solution
                k = argmax(foverB)
                Ne = push!(Ne,k)
                foverB[k] = 0.0
                k=findall(x->x==k, Nlinha)
                Nlinha = deleteat!(Nlinha,k)
                if isempty(Nlinha)
                    flag_of_infeasibility = false
                end
            end
        end # end while flag_of_infeasibility
        ftime_to_model4 = time_ns()
        elaptime_to_model4 = (ftime_to_model4 - itime_to_model4) / 1.0e9
        println(arq_saida," ")
        println(arq_saida,"Result of Relaxed Model4 (H-R2) of Cetin, F. and Alabas-Uslu, C. (2015).article")
        println(arq_saida," ")
        if isempty(Nlinha)
            println(arq_saida,"IMPOSSIBLE TO SOLVE THE PROBLEM: HEURISTIC REMOVE ALL PRODUCTS FROM CAMPAIGN")
        else
            println(arq_saida,"Custo total: ", valor_fo)
        end
        println(arq_saida,"Elapsed Time: $elaptime_to_model4 sec")

        #for j = 1 : n
        #    for i = 1 : m
        #        write(arq_saida, string(value(x[i,j]), "\n"))
        #        #write(arq_saida, string(i, j, "\n"))
        #    end
        #    write(arq_saida, "\n")
        #end

        close(arq_saida)

    end #end do let
end

# Seleciona todos os arquivos de instancias (walkdir), cria a pasta onde serao salvos os resultados da solucao (linha 98) e chama
# a funcao para solucionar o modelo (linha 100)
for (root, dirs, files) in walkdir("C:/Users/Greici da Rosa/Documents/JULIA/Instancias")
    for file in files
        instancia = joinpath(root, file)
        output_instancia = joinpath("C:/Users/Greici da Rosa/Documents/JULIA/Resultado_Modelo3_4", basename(root))
        if ! ispath(output_instancia)
            mkdir(output_instancia)
        end
        solucionar_modelo(instancia, output_instancia * "/SOL_" * file)
    end
end
