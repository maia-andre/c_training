/*
 * Jogo da Velha (tic-tac-toe) — 2 jogadores no mesmo terminal.
 *
 * Treina lógica de verdade: representar um tabuleiro, validar jogadas,
 * alternar turnos e checar todas as condições de vitória/empate.
 *
 * As casas são numeradas de 1 a 9, como um teclado numérico:
 *
 *      1 | 2 | 3
 *      4 | 5 | 6
 *      7 | 8 | 9
 */

#include <stdio.h>

/* O tabuleiro é um vetor de 9 posições.
 * Cada posição guarda 'X', 'O' ou um espaço ' ' (vazio). */
static char tabuleiro[9];

static void inicializar(void) {
    for (int i = 0; i < 9; i++) {
        tabuleiro[i] = ' ';
    }
}

static void desenhar(void) {
    printf("\n");
    for (int linha = 0; linha < 3; linha++) {
        int base = linha * 3;
        /* Mostra a casa jogada, ou o número da casa se ainda estiver vazia,
         * pra ficar fácil saber onde jogar. */
        for (int col = 0; col < 3; col++) {
            int i = base + col;
            if (tabuleiro[i] == ' ') {
                printf(" %d ", i + 1);
            } else {
                printf(" %c ", tabuleiro[i]);
            }
            if (col < 2) printf("|");
        }
        printf("\n");
        if (linha < 2) printf("---+---+---\n");
    }
    printf("\n");
}

/* Retorna o jogador vencedor ('X' ou 'O'), ou ' ' se ninguém venceu ainda. */
static char vencedor(void) {
    /* As 8 linhas possíveis de vitória: 3 linhas, 3 colunas, 2 diagonais. */
    static const int linhas[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},  /* linhas  */
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},  /* colunas */
        {0, 4, 8}, {2, 4, 6}              /* diagonais */
    };

    for (int i = 0; i < 8; i++) {
        int a = linhas[i][0], b = linhas[i][1], c = linhas[i][2];
        if (tabuleiro[a] != ' ' &&
            tabuleiro[a] == tabuleiro[b] &&
            tabuleiro[b] == tabuleiro[c]) {
            return tabuleiro[a];
        }
    }
    return ' ';
}

/* Verdadeiro se todas as casas estão preenchidas. */
static int tabuleiro_cheio(void) {
    for (int i = 0; i < 9; i++) {
        if (tabuleiro[i] == ' ') return 0;
    }
    return 1;
}

int main(void) {
    inicializar();
    char jogador = 'X';

    printf("=== Jogo da Velha ===\n");
    printf("Dois jogadores. X começa. Escolha a casa de 1 a 9.\n");

    while (1) {
        desenhar();

        int casa = 0;
        printf("Jogador %c, sua casa: ", jogador);

        int lido = scanf("%d", &casa);
        if (lido == EOF) {
            /* Entrada encerrada (ex: Ctrl+D). Sai sem travar. */
            printf("\nJogo encerrado.\n");
            break;
        }
        if (lido != 1) {
            printf("Entrada inválida. Digite um número de 1 a 9.\n");
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) { /* limpa buffer */ }
            continue;
        }

        /* Converte 1..9 (humano) para 0..8 (índice do vetor). */
        int i = casa - 1;

        if (casa < 1 || casa > 9) {
            printf("Casa fora do tabuleiro. Use de 1 a 9.\n");
            continue;
        }
        if (tabuleiro[i] != ' ') {
            printf("Essa casa já está ocupada. Escolha outra.\n");
            continue;
        }

        tabuleiro[i] = jogador;

        char v = vencedor();
        if (v != ' ') {
            desenhar();
            printf("🎉 Jogador %c venceu!\n", v);
            break;
        }
        if (tabuleiro_cheio()) {
            desenhar();
            printf("Deu velha! Empate.\n");
            break;
        }

        /* Alterna o turno. */
        jogador = (jogador == 'X') ? 'O' : 'X';
    }

    return 0;
}
