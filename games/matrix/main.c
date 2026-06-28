/*
 * Matrix — um mundo procedural com "blocos" sencientes (nivel 2).
 *
 * Experimento meio filosofico, meio de programacao: criar uma Matrix de
 * brinquedo onde "blocos" parecem vivos sem nenhuma fisica hiper-realista.
 *
 * A ideia e uma "escada de senciencia" (do mais barato ao mais caro):
 *
 *      0  reatividade        responde a estimulo local
 *      1  memoria            o passado importa (estado interno)
 *   -> 2  valencia           coisas sao boas/ruins (energia: viver x morrer)   <- ESTAMOS AQUI
 *      3  modelo de mundo    preve o proximo instante
 *      4  agencia            age PARA mudar a propria valencia
 *      5  auto-modelo        se representa dentro do mundo
 *      6  aprendizado        a politica muda com a experiencia
 *
 * Este arquivo vai ate o nivel 2: cada bloco-agente tem uma "energia" (sua
 * valencia). Comida no chao = bom; chegar a zero = morte. O bloco enxerga so
 * a vizinhanca 3x3 e caminha na direcao de mais comida. Nada sabe do mundo
 * inteiro; o comportamento global (manadas, escassez, ciclos) EMERGE das
 * regras locais. Voce, ao escolher a "seed", e o relojoeiro: o universo
 * inteiro e f(seed) — deterministico, mas de dentro parece aberto.
 *
 * O codigo segue um pipeline em 4 partes, igual ao do plotter:
 *   PART 1  Mundo procedural   ruido por hash -> campos de comida
 *   PART 2  Blocos / cognicao  percepcao 3x3 e decisao (a "mente" do bloco)
 *   PART 3  Simulacao          o tick: LER -> RESOLVER -> ESCREVER
 *   PART 4  Render + main       desenho ASCII, HUD e o laco principal
 *
 * Importante: a regra padrao do Makefile compila os .c de cada pasta em
 * games/ SEM passar -lm, entao aqui nao usamos <math.h>: o ruido e feito com
 * inteiros e o resto com polinomios. Assim "make matrix" funciona sem flags
 * extras, igual aos outros jogos.
 *
 * Uso:
 *   make matrix
 *   ./bin/matrix [seed] [ticks] [delay_ms]
 *      seed     semente do universo   (default 20260628)
 *      ticks    quantos ticks rodar   (default 0 = infinito, Ctrl+C para sair)
 *      delay_ms pausa entre frames    (default 80)
 */

#define _POSIX_C_SOURCE 199309L  /* libera nanosleep/signal com -std=c11 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/*  Configuracao do universo (tudo ajustavel — mude e veja a vida     */
/*  mudar; cada numero abaixo e uma "lei da fisica" deste mundinho).   */
/* ------------------------------------------------------------------ */
#define LARG        64      /* largura do mundo em celulas  */
#define ALT         22      /* altura  do mundo em celulas  */

#define MAX_COMIDA  5.0f    /* teto absoluto de comida por celula            */
#define REGROW      0.06f   /* fracao do "faltante" que a comida recompoe/tick */

#define N_INICIAL   60      /* quantos blocos nascem no comeco               */
#define ENERGIA0    6.0f    /* energia inicial de um bloco                    */
#define INGESTAO    2.0f    /* quanto de comida um bloco come por tick        */
#define METABOLISMO 0.35f   /* energia gasta so por existir, a cada tick      */
#define REPRO       12.0f   /* a partir desta energia o bloco se divide       */

#define MAX_AG      (LARG * ALT)   /* no maximo um bloco por celula           */

/* ------------------------------------------------------------------ */
/*  Estado global do mundo (file-static, no estilo do plotter).        */
/* ------------------------------------------------------------------ */
static float comida[ALT][LARG];      /* energia do solo — o "recurso"        */
static float capacidade[ALT][LARG];  /* teto de comida da celula (do ruido)  */
static int   ocup[ALT][LARG];        /* indice do bloco ali, ou -1 se vazio  */

/* Um "bloco" senciente. No nivel 2 sua mente cabe num numero: a energia. */
typedef struct {
    int   x, y;       /* posicao no mundo                         */
    float energia;    /* a VALENCIA: cair a zero = morte           */
    int   vivo;       /* 0 = slot livre / bloco morto             */
} Bloco;

static Bloco blocos[MAX_AG];
static int   n_blocos;            /* slots em uso (com buracos de mortos)    */

/* Intencao de movimento decidida na fase de LEITURA, aplicada na ESCRITA.
 * Separar as duas fases evita que a ordem de varredura vire "fisica fantasma"
 * (o classico bug do organismo que anda mais rapido para um lado so porque
 * o laco o visita antes dos vizinhos). */
static int alvo_x[MAX_AG];
static int alvo_y[MAX_AG];
static int reivindicado[ALT][LARG];  /* quem ganhou o direito de entrar aqui */

/* Gerador pseudo-aleatorio do proprio universo (LCG). Semeado pela seed,
 * entao toda a simulacao — inclusive nascimentos — e reproduzivel: f(seed). */
static uint32_t rng_estado;
static uint32_t rng(void) {
    rng_estado = rng_estado * 1664525u + 1013904223u;
    return rng_estado;
}
static float rng01(void) { return (rng() >> 8) / 16777216.0f; }  /* 0..1 */

static float menor(float a, float b) { return a < b ? a : b; }

/* ================================================================== */
/*  PART 1 — MUNDO PROCEDURAL                                          */
/*  O mundo nao e desenhado a mao: ele e f(seed, x, y). Um hash       */
/*  transforma coordenadas em ruido, e o ruido vira manchas de comida. */
/* ================================================================== */

/* Hash inteiro: (x, y, seed) -> numero "aleatorio" mas DETERMINISTICO. */
static uint32_t hash2(int x, int y, uint32_t s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u
               + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float hash01(int x, int y, uint32_t s) {
    return (hash2(x, y, s) >> 8) / 16777216.0f;   /* 0..1 */
}

/* Value noise: ruido suave (manchas, nao chuvisco). Amostra os 4 cantos
 * da celula do reticulado e interpola com suavizacao (smoothstep).
 * Como as coordenadas aqui sao sempre >= 0, (int) ja funciona como floor. */
static float ruido(float fx, float fy, uint32_t s) {
    int x0 = (int)fx, y0 = (int)fy;
    float tx = fx - x0, ty = fy - y0;
    /* smoothstep: suaviza as bordas pra mancha nao ficar quadriculada */
    float ux = tx * tx * (3.0f - 2.0f * tx);
    float uy = ty * ty * (3.0f - 2.0f * ty);

    float c00 = hash01(x0,     y0,     s);
    float c10 = hash01(x0 + 1, y0,     s);
    float c01 = hash01(x0,     y0 + 1, s);
    float c11 = hash01(x0 + 1, y0 + 1, s);

    float a = c00 + (c10 - c00) * ux;   /* interpola na horizontal (topo)  */
    float b = c01 + (c11 - c01) * ux;   /* interpola na horizontal (base)  */
    return a + (b - a) * uy;            /* e entao na vertical             */
}

/* Gera o mundo a partir da seed: define a capacidade de comida de cada
 * celula (manchas ferteis x desertos) e enche todas ate o teto. */
static void gerar_mundo(uint32_t seed) {
    for (int y = 0; y < ALT; y++) {
        for (int x = 0; x < LARG; x++) {
            /* Duas oitavas: manchas grandes + textura fina por cima. */
            float n = 0.65f * ruido(x * 0.12f, y * 0.12f, seed)
                    + 0.35f * ruido(x * 0.30f, y * 0.30f, seed + 99u);
            /* Elevar ao quadrado deixa o mundo mais "binario": muito
             * deserto e algumas manchas bem ferteis, em vez de cinza geral. */
            float cap = n * n * MAX_COMIDA;
            capacidade[y][x] = cap;
            comida[y][x]     = cap;   /* o mundo comeca cheio              */
            ocup[y][x]       = -1;    /* e sem ninguem em cima             */
        }
    }
}

/* Espalha os primeiros blocos, de preferencia onde ja ha comida. */
static void semear_blocos(void) {
    n_blocos = 0;
    int tentativas = 0;
    while (n_blocos < N_INICIAL && tentativas < N_INICIAL * 50) {
        tentativas++;
        int x = (int)(rng01() * LARG);
        int y = (int)(rng01() * ALT);
        if (x >= LARG) x = LARG - 1;
        if (y >= ALT)  y = ALT - 1;
        if (ocup[y][x] != -1) continue;          /* celula ja ocupada     */
        if (capacidade[y][x] < 1.0f) continue;   /* evita largar no deserto */

        Bloco *b = &blocos[n_blocos];
        b->x = x; b->y = y; b->energia = ENERGIA0; b->vivo = 1;
        ocup[y][x] = n_blocos;
        n_blocos++;
    }
}

/* ================================================================== */
/*  PART 2 — BLOCOS / COGNICAO                                         */
/*  Toda a "mente" de um bloco no nivel 2: olhe os 8 vizinhos + a sua  */
/*  propria celula e queira ir para onde houver mais comida. Percepcao */
/*  estritamente LOCAL — nenhum bloco conhece o mundo inteiro.         */
/* ================================================================== */
static void decidir(int i) {
    Bloco *b = &blocos[i];

    /* Comeca querendo ficar parado, comendo o que tem debaixo de si. */
    int   melhor_x = b->x, melhor_y = b->y;
    float melhor   = comida[b->y][b->x];

    /* Varre a vizinhanca 3x3 (a "visao" do bloco). */
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = b->x + dx, ny = b->y + dy;
            if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
            if (comida[ny][nx] > melhor) {   /* estritamente melhor        */
                melhor   = comida[ny][nx];
                melhor_x = nx; melhor_y = ny;
            }
        }
    }
    alvo_x[i] = melhor_x;
    alvo_y[i] = melhor_y;
}

/* ================================================================== */
/*  PART 3 — SIMULACAO (o tick)                                        */
/*  Um tick tem fases bem separadas para ser deterministico e justo:   */
/*    A. DECIDIR     todos leem a vizinhanca e escolhem um alvo         */
/*    B. RESOLVER    conflitos: dois blocos, uma celula -> so um entra  */
/*    C. APLICAR     move quem ganhou, depois come e paga metabolismo   */
/*    D. REPRODUZIR  blocos saciados se dividem                         */
/*    E. MUNDO       a comida rebrota em direcao a capacidade           */
/* ================================================================== */

/* B. Resolve quem pode entrar em cada celula. Para ser simples e sem
 * bugs de "troca de lugar", so permitimos mover para celulas que estavam
 * VAZIAS no inicio do tick, e cada celula e dada a um unico pretendente
 * (o de menor indice). Os demais ficam parados. */
static void resolver(void) {
    for (int y = 0; y < ALT; y++)
        for (int x = 0; x < LARG; x++)
            reivindicado[y][x] = -1;

    for (int i = 0; i < n_blocos; i++) {
        if (!blocos[i].vivo) continue;
        int tx = alvo_x[i], ty = alvo_y[i];
        if (tx == blocos[i].x && ty == blocos[i].y) continue;  /* fica      */

        if (ocup[ty][tx] == -1 && reivindicado[ty][tx] == -1) {
            reivindicado[ty][tx] = i;        /* concedido                  */
        } else {
            alvo_x[i] = blocos[i].x;         /* negado: permanece          */
            alvo_y[i] = blocos[i].y;
        }
    }
}

/* C. Move os blocos que ganharam o direito, e em seguida cada bloco vivo
 * come a comida da celula onde esta e paga o custo de viver. */
static void aplicar_e_comer(void) {
    /* mover */
    for (int i = 0; i < n_blocos; i++) {
        if (!blocos[i].vivo) continue;
        int tx = alvo_x[i], ty = alvo_y[i];
        if (tx == blocos[i].x && ty == blocos[i].y) continue;
        if (reivindicado[ty][tx] != i) continue;   /* nao foi quem ganhou  */

        ocup[blocos[i].y][blocos[i].x] = -1;
        blocos[i].x = tx; blocos[i].y = ty;
        ocup[ty][tx] = i;
    }

    /* comer + metabolismo (a valencia em acao) */
    for (int i = 0; i < n_blocos; i++) {
        if (!blocos[i].vivo) continue;
        int x = blocos[i].x, y = blocos[i].y;

        float garfada = menor(comida[y][x], INGESTAO);
        comida[y][x]      -= garfada;
        blocos[i].energia += garfada;
        blocos[i].energia -= METABOLISMO;   /* existir custa               */

        if (blocos[i].energia <= 0.0f) {     /* morte por inanicao          */
            blocos[i].vivo = 0;
            ocup[y][x] = -1;
        }
    }
}

/* D. Reproducao: um bloco saciado se divide num vizinho vazio, partilhando
 * a energia. E uma "lei do mundo", nao cognicao — mas e o que faz a
 * populacao oscilar e a tela ganhar vida. */
static void reproduzir(void) {
    int total = n_blocos;   /* fixa o limite ANTES de adicionar filhotes    */
    for (int i = 0; i < total; i++) {
        if (!blocos[i].vivo) continue;
        if (blocos[i].energia < REPRO) continue;
        if (n_blocos >= MAX_AG) break;

        /* Coleta as celulas vizinhas livres. */
        int lx[8], ly[8], nlivres = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = blocos[i].x + dx, ny = blocos[i].y + dy;
                if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
                if (ocup[ny][nx] != -1) continue;
                lx[nlivres] = nx; ly[nlivres] = ny; nlivres++;
            }
        }
        if (nlivres == 0) continue;          /* sem espaco, sem cria        */

        int e = (int)(rng01() * nlivres);
        if (e >= nlivres) e = nlivres - 1;

        int j = n_blocos++;                  /* novo slot                   */
        blocos[i].energia *= 0.5f;           /* a energia se divide         */
        blocos[j].x = lx[e]; blocos[j].y = ly[e];
        blocos[j].energia = blocos[i].energia;
        blocos[j].vivo = 1;
        ocup[ly[e]][lx[e]] = j;
    }
}

/* E. O mundo se recompoe: a comida rebrota em direcao a capacidade da
 * celula. Manchas ferteis se enchem rapido; desertos seguem desertos. */
static void rebrotar(void) {
    for (int y = 0; y < ALT; y++) {
        for (int x = 0; x < LARG; x++) {
            float falta = capacidade[y][x] - comida[y][x];
            if (falta > 0.0f) comida[y][x] += REGROW * falta;
        }
    }
}

/* ================================================================== */
/*  PART 4 — RENDER + LACO PRINCIPAL                                   */
/* ================================================================== */

/* Codigos ANSI: limpar tela, esconder/mostrar cursor, cores. */
#define CLS      "\033[H\033[2J"
#define CUR_OFF  "\033[?25l"
#define CUR_ON   "\033[?25h"
#define RESET    "\033[0m"

static volatile sig_atomic_t parar = 0;
static void ao_interromper(int s) { (void)s; parar = 1; }

/* Estatisticas para o HUD. */
static int populacao(void) {
    int n = 0;
    for (int i = 0; i < n_blocos; i++) if (blocos[i].vivo) n++;
    return n;
}

/* Desenha um frame inteiro num buffer e cospe de uma vez (menos piscada). */
static void desenhar(uint32_t seed, long tick) {
    static char buf[ALT * LARG * 16 + 4096];
    int p = 0;
    p += sprintf(buf + p, CLS);
    p += sprintf(buf + p,
        "  M A T R I X  —  mundo procedural, blocos sencientes (nivel 2)\n");

    /* borda superior */
    p += sprintf(buf + p, "  +");
    for (int x = 0; x < LARG; x++) buf[p++] = '-';
    p += sprintf(buf + p, "+\n");

    for (int y = 0; y < ALT; y++) {
        p += sprintf(buf + p, "  |");
        for (int x = 0; x < LARG; x++) {
            int i = ocup[y][x];
            if (i != -1 && blocos[i].vivo) {
                /* Bloco: cor pela energia (verde=forte, amarelo, vermelho=fraco). */
                float e = blocos[i].energia;
                const char *cor = e > 10.0f ? "\033[92m"   /* verde forte  */
                                : e >  4.0f ? "\033[93m"   /* amarelo      */
                                            : "\033[91m";  /* vermelho     */
                p += sprintf(buf + p, "%s@" RESET, cor);
            } else {
                /* Solo: densidade de comida vira textura, em verde fraco. */
                float c = comida[y][x];
                char g = c < 0.4f ? ' ' : c < 1.3f ? '.' : c < 2.8f ? ':' : '*';
                if (g == ' ') buf[p++] = ' ';
                else p += sprintf(buf + p, "\033[2;32m%c" RESET, g);
            }
        }
        p += sprintf(buf + p, "|\n");
    }

    /* borda inferior */
    p += sprintf(buf + p, "  +");
    for (int x = 0; x < LARG; x++) buf[p++] = '-';
    p += sprintf(buf + p, "+\n");

    /* HUD */
    int pop = populacao();
    float soma_e = 0.0f, soma_c = 0.0f;
    for (int i = 0; i < n_blocos; i++) if (blocos[i].vivo) soma_e += blocos[i].energia;
    for (int y = 0; y < ALT; y++) for (int x = 0; x < LARG; x++) soma_c += comida[y][x];

    p += sprintf(buf + p,
        "  seed %-10u  tick %-6ld  pop %-4d  energia media %5.1f  comida %6.0f\n",
        seed, tick, pop, pop ? soma_e / pop : 0.0f, soma_c);
    p += sprintf(buf + p,
        "  legenda: \033[92m@\033[0m forte  \033[93m@\033[0m ok  \033[91m@\033[0m fraco"
        "   \033[2;32m. : *\033[0m comida    (Ctrl+C para sair)\n");

    fwrite(buf, 1, p, stdout);
    fflush(stdout);
}

int main(int argc, char **argv) {
    uint32_t seed  = (argc > 1) ? (uint32_t)strtoul(argv[1], NULL, 10) : 20260628u;
    long    ticks  = (argc > 2) ? strtol(argv[2], NULL, 10) : 0;   /* 0 = infinito */
    long    delay  = (argc > 3) ? strtol(argv[3], NULL, 10) : 80;  /* ms por frame */
    if (delay < 0) delay = 0;

    rng_estado = seed ? seed : 1u;   /* o RNG do universo nasce da seed      */
    signal(SIGINT, ao_interromper);

    gerar_mundo(seed);
    semear_blocos();

    struct timespec pausa = { delay / 1000, (delay % 1000) * 1000000L };

    fputs(CUR_OFF, stdout);

    long t = 0;
    while (!parar && (ticks == 0 || t < ticks)) {
        desenhar(seed, t);

        if (populacao() == 0) {                 /* extincao total           */
            printf("\n  Silencio. A populacao se extinguiu no tick %ld.\n", t);
            break;
        }

        /* O tick, na ordem: ler -> resolver -> escrever -> reproduzir -> mundo. */
        for (int i = 0; i < n_blocos; i++)
            if (blocos[i].vivo) decidir(i);
        resolver();
        aplicar_e_comer();
        reproduzir();
        rebrotar();

        if (delay > 0) nanosleep(&pausa, NULL);
        t++;
    }

    fputs(CUR_ON, stdout);
    if (parar) printf("\n  Encerrado no tick %ld. O universo era f(seed=%u).\n", t, seed);
    return 0;
}
