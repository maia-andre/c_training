/*
 * Adivinhação — o jogo mais simples do repo.
 *
 * O computador sorteia um número de 1 a 100 e você tenta adivinhar.
 * A cada palpite ele diz se o número secreto é maior ou menor.
 *
 * Serve pra validar que o ambiente compila e roda, e pra treinar o básico:
 * entrada do usuário, laço, comparação e contagem.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN 1
#define MAX 100

int main(void) {
    /* Semeia o gerador de números aleatórios com o relógio,
     * senão o "sorteio" seria o mesmo toda vez. */
    srand((unsigned int)time(NULL));

    int secreto = MIN + rand() % (MAX - MIN + 1);
    int palpite = 0;
    int tentativas = 0;

    printf("=== Adivinhação ===\n");
    printf("Pensei num número entre %d e %d. Tente adivinhar!\n", MIN, MAX);

    while (palpite != secreto) {
        printf("\nSeu palpite: ");

        /* scanf retorna quantos itens leu com sucesso.
         * Se o usuário digitar algo que não é número, tratamos o erro. */
        int lido = scanf("%d", &palpite);
        if (lido == EOF) {
            /* Entrada encerrada (ex: Ctrl+D). Sai sem travar. */
            printf("\nAté a próxima!\n");
            break;
        }
        if (lido != 1) {
            printf("Isso não é um número. Tente de novo.\n");
            /* Descarta o que sobrou na entrada pra não travar o laço. */
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { /* limpa o buffer */ }
            continue;
        }

        tentativas++;

        if (palpite < secreto) {
            printf("Mais ALTO. ⬆\n");
        } else if (palpite > secreto) {
            printf("Mais BAIXO. ⬇\n");
        } else {
            printf("\n🎉 Acertou! O número era %d.\n", secreto);
            printf("Você usou %d tentativa%s.\n",
                   tentativas, tentativas == 1 ? "" : "s");
        }
    }

    return 0;
}
