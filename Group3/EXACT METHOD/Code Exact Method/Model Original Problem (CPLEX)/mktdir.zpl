#lê o arquivo que será passado através da linha de comando
#p. ex.: zimpl-3.3.0.win.x86_64.vc10.normal.opt.exe -D FILE="teste.txt" "mktdir (06.15).zpl"
param arquivo := FILE;

#lê o numero de clientes na primeira coluna, primeira linha
param CLIENTES := read arquivo as "1n" use 1;
#lê o numero de produtos na segunda coluna, primeira linha
param PRODUTOS := read arquivo as "2n" use 1;
#lê o hurdle rate na terceira coluna, primeira linha
param R := read arquivo as "3n" use 1;
param COLUNAS := PRODUTOS * 2 + 1;

set I := { 1..CLIENTES };
set J := { 1..PRODUTOS };
set Z := { 1..COLUNAS };
set T := { I * Z };

#lê todo o arquivo a partir da segunda linha até o número de clientes
param MATRIZ[T] := read arquivo as "n+" skip 1 use CLIENTES;
#lê as linhas restantes após o fim dos clientes
param OFB[<i,j> in {1..3} * J] := read arquivo as "n+" skip CLIENTES + 1;

#custo de oferecer o produto j para o cliente i
param c[<i,j> in I * J] := MATRIZ[i,j];
#retorno esperado do cliente i quando o produto j é ofertado
param p[<i,j> in I * J] := MATRIZ[i,j + PRODUTOS];
#número máximo de ofertas que o cliente i pode receber
param M[<i> in I] := MATRIZ[i, COLUNAS];
#número minimo de clientes que devem receber oferta do produto
param O[<i> in J] := OFB[1,i];
#orçamento disponível por produto
param B[<i> in J] := OFB[2,i];
#custo fixo do produto
param f[<i> in J] := OFB[3,i];

#lucro potencial liquido
#param NPP[<i,j> in I * J] := (p[i,j] - c[i,j]) / c[i,j];

#do forall <j> in J do print f[j];

var x[I * J] binary;
var y[J] binary;

# função objetivo

maximize LUCRO: 
	sum <i,j> in I * J : ((p[i,j] - c[i,j]) * x[i,j]) - sum <j> in J: (f[j] * y[j]);

# restrições

subto TX_MIN_RETORNO5:
	sum <i,j> in I * J : (p[i,j] * x[i,j]) >= (1 + R) * ((sum <i,j> in I * J : c[i,j] * x[i,j]) + (sum <j> in J: f[j] * y[j]));

subto ORCAMENTO6:
	forall <j> in J do sum <i> in I :
		c[i,j] * x[i,j] <= B[j];

subto OFERTA_MAX7:
	forall <i> in I do sum <j> in J :
		x[i,j] <= M[i];

subto OFERTA_MIN8:
	forall <j> in J do sum <i> in I :
		x[i,j] <= CLIENTES*y[j];

subto OFERTA_MIN9:
	forall <j> in J do sum <i> in I :
		x[i,j] >= O[j]*y[j];