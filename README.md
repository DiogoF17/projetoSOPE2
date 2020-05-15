Unidade Curricular: Sistemas Operativos

Turma 7 Grupo 3:
- Diogo Filipe Santos
- Jessica Nascimento
- Marcelo Reis

Para esta etapa foram criado semáforos de modo a conseguir implementar o número máximo das casas de banho (-l). Para isto usamos um semáforo para o controlo da casa de banho e outro para exclusão. O semáforo de controlo é utilizado para que os valores atribuídos sejam os corretos, ou seja, não hajam erros quando forem alterados por cada thread. O semáforo de exclusão iniciado com o número de casas de banho serve para que estejam em funcionamento as l casas de banho, dado através do utilizador. Isto permite que o cliente espere enquanto as casas de banho estão ocupadas. Fazendo-o entrar logo que haja um disponível. No final do programa estes semáfors são destruídos.

Para a implementação do -n (numero de threads), criamos um mutex que é utilizado no main e na função thread_func para que seja possível usar a variável countThreadsRunning sem que os valores fiquem errados. Esta é decrementada sempre que o thread acaba a sua tarefa e incrementada quando é usado um novo thread. Esta é uma variável que nos ajuda na destruição do mutex. Sendo utilizada quando a variável n é passada pelo o utilizador ou não. Este n, também usado como variável no interior do programa, gere as threads especificadas pelo o utilizador.

Ao testar o código, deparámo-nos com um erro: libgcc_s.so.1 must be installed for pthread_cancel to work. Isto influenciou na medida em que não sabemos se é algum problema no código ou dos nossos computadores. Tivemos várias tentativas de solução ao problema, mas não conseguimos arranjar.
