- [ ] Escolher os locais corretos para usar as barreiras de CPU.
- [ ] Entender quais tarefas devem ser realizadas apenas por um core.
- [ ] Como carregar o primeiro contexto de um core.
- [ ] Como fazer interrupção de software entre cores.

Quando voce usa a função IPI para setar a interrupção em outro core (_||_), talvez isso gere automaticamente uma interrupção de Software no core alvo.

// Pagina 114: Guia pratico RISCV
O modo S não controla diretamente o temporizador e interrupções de software mas, em vez disso, usa a instrução ecall para solicitar ao modo M que configure temporizadores ou enviar interrupções de interprocessador em seu nome. Esta convenção de software faz parte da Interface Binária do Supervisor.