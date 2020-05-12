Unidade Curricular: Sistemas Operativos

Turma 7 Grupo 3:
- Diogo Filipe Santos
- Jessica Nascimento
- Marcelo Reis

Para esta etapa foi criado semáforos de modo a conseguir implementar o numero máximo das casas de banho. Para isto usamos um semáforo para o controlo da casa de banho e outro para exclusão. Antes de chamar-mos a função choose_WC fazemos sem_wait para que seja possível usar o arrWC sem que este seja interferido por outro thread e danifiquem os valores guardados. Após esta escolha colocamos sem_post para que os próximos threads que precisem de aceder ao array o possam fazer. 
