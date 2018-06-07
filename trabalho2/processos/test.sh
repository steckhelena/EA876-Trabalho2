# Script para testes de unidade automaticos

# Configuracao: escolhe se o programa retonara mensagens teste-a-teste
verbose=$2

# A primeira linha indica que o script foi inicializado corretamente
if [ $verbose -eq 1 ]; then
  echo "Testando $1"
fi;

# A variavel dirtestes indica onde os arquivos teste (.in e .out) estao
dirtestes=./images


# tests sera inicializada com o resultado da expressao find,
# encontrando todos os arquivos .in do diretorio dirtestes
tests=`find $dirtestes -name '*.jpg'`

# Para cada teste...
for t in $tests
do
 
  # Encontra o nome do arquivo relacionado a saida do teste
  o=`echo $t | sed $sedexpression`

  # Executa o programa que foi compilado usando o arquivo de teste.in como
  # entrada
  $program < $t > ./$$.out
done
