# Lê o arquivo que será passado através da linha de comando.
# Exemplo: zimpl.exe -D FILE="teste.txt" "mktdircan.zpl"
param arquivo := FILE;

# Lê o numero de clientes na primeira linha, primeira coluna.
param CLIENTES := read arquivo as "1n" use 1;
# Lê o numero de produtos na primeira linha, segunda coluna.
param PRODUTOS := read arquivo as "2n" use 1;
# Lê o hurdle rate na primeira linha, terceira coluna.
param R := read arquivo as "3n" use 1;
param COLUNAS := PRODUTOS * 2 + 1;
# Quantidade de pares de produtos canibais.
param QUANTIDADE_PARES_PRODUTOS_CANIBAIS := PRODUTOS / 5;

set I := { 1..CLIENTES };
set J := { 1..PRODUTOS };
set Z := { 1..COLUNAS };
set T := { I * Z };

# Lê todo o arquivo a partir da segunda linha, até o número de clientes.
param MATRIZ[T] := read arquivo as "n+" skip 1 use CLIENTES;
# Lê as três linhas após o fim dos clientes.
param OFB[<i,j> in { 1..3 } * J] := read arquivo as "n+" skip CLIENTES + 1 use 3;
# Lê a última linha com os pares de produtos que comem carne humana (canibais).
param PRODUTOS_CANIBAIS[<i,j> in { 1..QUANTIDADE_PARES_PRODUTOS_CANIBAIS } * { 1..2 }] := read arquivo as "n+" skip CLIENTES + 4 use 1;

# Custo de oferecer o produto j para o cliente i.
param c[<i,j> in I * J] := MATRIZ[i, j];
# Retorno esperado do cliente i quando o produto j é ofertado.
param p[<i,j> in I * J] := MATRIZ[i, j + PRODUTOS];
# Número máximo de ofertas que o cliente i pode receber.
param M[<i> in I] := MATRIZ[i, COLUNAS];
# Número mínimo de clientes que devem receber oferta do produto.
param O[<i> in J] := OFB[1, i];
# Orçamento disponível por produto.
param B[<i> in J] := OFB[2, i];
# Custo fixo do produto.
param f[<i> in J] := OFB[3, i];

var x[I * J] binary;
var y[J] binary;

# Função objetivo.
maximize LUCRO: 
	sum <i,j> in I * J : ((p[i, j] - c[i, j]) * x[i, j]) - sum <j> in J: (f[j] * y[j]);

# Restrições.
subto TX_MIN_RETORNO5:
	sum <i,j> in I * J : (p[i, j] * x[i, j]) - (1 + R) * ((sum <i, j> in I * J : c[i, j] * x[i, j]) + (sum <j> in J: f[j] * y[j])) >= 0;

subto ORCAMENTO6:
	forall <j> in J do sum <i> in I:
		c[i, j] * x[i, j] <= B[j];

subto OFERTA_MAX7:
	forall <i> in I do sum <j> in J:
		x[i, j] <= M[i];

subto OFERTA_MIN8:
	forall <j> in J do sum <i> in I:
		x[i, j] <= CLIENTES * y[j];

subto OFERTA_MIN9:
	forall <j> in J do sum <i> in I:
		x[i, j] >= O[j] * y[j];

subto CANIB10:
 	forall <i> in { 1..QUANTIDADE_PARES_PRODUTOS_CANIBAIS } do
		y[(PRODUTOS_CANIBAIS[i, 1] + 1)] + y[(PRODUTOS_CANIBAIS[i, 2] + 1)] <= 1;