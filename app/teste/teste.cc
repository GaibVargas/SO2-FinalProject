#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

static int cas(volatile int & value, int compare, int replacement) {
    register int old;
    if(sizeof(int) == sizeof(unsigned long))
        ASM("1: amoswap.d.aq  %0,  %3, (%1) \n"
            "   beq     %0, %2, 2f       \n"
            "   amoswap.d.rl  %3,  %0, (%1) \n"
            "2:                          \n" : "=&r"(old) : "r"(&value), "r"(compare), "r"(replacement) : "cc", "memory");
    else
        ASM("1: amoswap.w.aq  %0,  %3, (%1) \n"
            "   beq     %0, %2, 2f       \n"
            "   amoswap.w.rl  %3,  %0, (%1) \n"
            "2:                          \n" : "=&r"(old) : "r"(&value), "r"(compare), "r"(replacement) : "cc", "memory");
        // ASM("1: lr.w    %0, (%1)        \n"
        //     "   bne     %0, %2, 2f      \n"
        //     "   sc.w    t3, %3, (%1)    \n"
        //     "   bnez    t3, 1b          \n"
        //     "2:                         \n" : "=&r"(old) : "r"(&value), "r"(compare), "r"(replacement) : "t3", "cc", "memory");
    return old;
}

// Função de teste
void test_cas() {
    volatile int value = 42; // Valor inicial

    // Teste 1: CAS bem-sucedido
    int old_value = cas(value, 42, 84);
    cout << "Teste 1 - Valor antigo: " << old_value << ", Novo valor: " << value << endl;

    // Teste 2: CAS falha porque valor não coincide
    old_value = cas(value, 42, 126);
    cout << "Teste 2 - Valor antigo: " << old_value << ", Novo valor: " << value << endl;

    // Teste 3: CAS bem-sucedido novamente
    old_value = cas(value, 84, 126);
    cout << "Teste 3 - Valor antigo: " << old_value << ", Novo valor: " << value << endl;

}

int main() {
    test_cas();
    return 0;
}