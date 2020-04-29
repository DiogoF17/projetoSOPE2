Unidade Curricular: Sistemas Operativos

Turma 7 Grupo 3:
- Diogo Filipe Santos
- Jessica Nascimento
- Marcelo Reis

Neste parte do trabalho, mostramos o nosso conhecimento de threads e de fifos utilizando ambos num mesmo programa. 

Para a comunicação entre Q1 e U1 e vice versa utilizamos fifos criados dentro de ambos, e como forma de passar os dados de um lado para o outro criamos uma estrutura de dados denominada "ParametrosParaFifo". Para passar os argumentos para as threads criamos outra struck: "ParametrosParaThread" com o identificador de cada thread e o nome do fifo.

Em ambos os ficheiros, utilizamos uma variável "end" que é colocado a 1 quando recebe um sinal, fazendo o ciclo while acabar a criação de threads.
