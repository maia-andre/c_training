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
 *      2  valencia           coisas sao boas/ruins (energia: viver x morrer)
 *      3  modelo de mundo    simula o futuro e decide por ele
 *      4  agencia            pondera motivos em conflito p/ regular a valencia
 *      5  auto-modelo        se inclui na simulacao e le a intencao dos vizinhos
 *   -> 6  aprendizado        os tracos sao herdados com mutacao -> selecao natural  <- ESTAMOS AQUI
 *
 * Este arquivo vai ate o nivel 6: cada bloco-agente tem uma "energia" (sua
 * valencia; nivel 2), um MODELO DO MUNDO que simula o futuro (nivel 3),
 * AGENCIA (nivel 4) — pondera motivos em conflito cujos pesos mudam com o
 * proprio estado interno — um AUTO-MODELO (nivel 5): se inclui na cena, com o
 * tick em DUAS passagens (todos DECLARAM a intencao, depois cada um RECONSIDERA
 * lendo a dos vizinhos e cede a celula disputada) — e, agora, APRENDIZADO
 * (nivel 6): os pesos da politica (urgencia, espaco, horizonte, desconto)
 * deixam de ser constantes e viram TRACOS herdados pela cria COM MUTACAO. Quem
 * decide melhor vive e se reproduz mais, entao a media da populacao EVOLUI
 * sozinha — selecao natural de personalidades, sem ninguem projetar nada.
 * Comida no chao = bom; chegar a zero = morte.
 * A percepcao continua estritamente LOCAL (3x3): nada sabe do mundo
 * inteiro, e ainda assim o comportamento global (manadas, escassez, ciclos)
 * EMERGE das regras locais. Voce, ao escolher a "seed", e o relojoeiro: o
 * universo inteiro e f(seed) — deterministico, mas de dentro parece aberto.
 *
 * O codigo segue um pipeline em 4 partes, igual ao do plotter:
 *   PART 1  Mundo procedural   ruido por hash -> campos de comida
 *   PART 2  Blocos / cognicao  percepcao + modelo + agencia + auto-modelo + aprendizado
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
 *   ./bin/matrix [seed] [ticks] [delay_ms] [foco]
 *      seed     semente do universo   (default 20260628)
 *      ticks    quantos ticks rodar   (default 0 = infinito, Ctrl+C para sair)
 *      delay_ms pausa entre frames    (default 80)
 *      foco     entra ja em 1a pessoa nesse bloco (default -1 = visao de deus)
 *
 * Num terminal de verdade, durante a animacao (a "pilula vermelha"):
 *   p / TAB   alterna visao de DEUS  <->  PRIMEIRA PESSOA (entrar num bloco)
 *   , / .     em 1a pessoa, troca o bloco habitado
 *   espaco    pausa/retoma o tempo
 *   q         sai
 */

#define _POSIX_C_SOURCE 199309L  /* libera nanosleep/signal com -std=c11 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>    /* isatty, read, STDIN/STDOUT_FILENO (teclado nao-bloqueante) */
#include <termios.h>   /* modo cru do terminal: ler tecla a tecla, sem Enter nem eco */
#include <string.h>    /* strcmp: separar flags (--log) dos argumentos posicionais  */

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

/* Parametros do MODELO DE MUNDO (nivel 3): como o bloco imagina o futuro.
 * HORIZONTE e DESCONTO viram TRACOS individuais no nivel 6 — aqui ficam os
 * valores MEDIOS de partida (semear_blocos sorteia em torno deles). */
#define HORIZONTE   6       /* media inicial: ticks que o bloco simula "de cabeca" */
#define DESCONTO    0.80f   /* media inicial: peso do futuro (+1 tick vale 0.8...)  */
#define COMPETICAO  0.5f    /* o quanto cada vizinho rival reduz a colheita prevista */

/* Parametros da AGENCIA (nivel 4): como o bloco pondera motivos em conflito.
 * URGENCIA e PESO_ESPACO tambem viram tracos (nivel 6); aqui, a media inicial. */
#define SACIADO     10.0f   /* energia acima da qual a fome zera                  */
#define URGENCIA    2.0f    /* media inicial: a fome amplifica o valor da comida  */
#define PESO_ESPACO 3.0f    /* media inicial: peso do desejo de espaco aberto     */

/* Parametro do AUTO-MODELO (nivel 5): o bloco se ve entre os concorrentes. */
#define ANTECIPACAO 0.5f    /* quanto cada vizinho que MIRA a mesma celula a desvaloriza */

/* Parametros do APRENDIZADO (nivel 6): os 4 tracos acima sao herdados COM
 * MUTACAO a cada nascimento; a selecao natural (quem sobrevive e se reproduz
 * mais) faz a media da populacao evoluir sozinha — sem gradiente, so vida. */
#define MUTACAO       0.12f /* magnitude da mutacao herdada (0 = clones perfeitos) */
#define HORIZONTE_MAX 12    /* teto do horizonte de planejamento de um bloco       */

#define MAX_AG      (LARG * ALT)   /* no maximo um bloco por celula           */

/* ------------------------------------------------------------------ */
/*  Estado global do mundo (file-static, no estilo do plotter).        */
/* ------------------------------------------------------------------ */
static float comida[ALT][LARG];      /* energia do solo — o "recurso"        */
static float capacidade[ALT][LARG];  /* teto de comida da celula (do ruido)  */
static int   ocup[ALT][LARG];        /* indice do bloco ali, ou -1 se vazio  */

/* Um "bloco" senciente. No nivel 2 sua mente cabia num numero (a energia);
 * do nivel 6 em diante ele tambem carrega uma "personalidade" herdavel. */
typedef struct {
    int   x, y;       /* posicao no mundo                         */
    float energia;    /* a VALENCIA: cair a zero = morte           */
    int   vivo;       /* 0 = slot livre / bloco morto             */
    /* (nivel 6) TRACOS herdaveis — a politica de decisao deste bloco. A cria
     * os herda do pai com mutacao; a selecao natural faz a media evoluir. */
    float urgencia;     /* peso da fome           (era #define URGENCIA)    */
    float peso_espaco;  /* desejo de espaco livre (era #define PESO_ESPACO) */
    float desconto;     /* desconto do futuro     (era #define DESCONTO)    */
    int   horizonte;    /* profundidade do plano  (era #define HORIZONTE)   */
} Bloco;

static Bloco blocos[MAX_AG];
static int   n_blocos;            /* slots em uso (com buracos de mortos)    */

/* Intencao de movimento decidida na fase de LEITURA, aplicada na ESCRITA.
 * Separar as duas fases evita que a ordem de varredura vire "fisica fantasma"
 * (o classico bug do organismo que anda mais rapido para um lado so porque
 * o laco o visita antes dos vizinhos). */
static int alvo_x[MAX_AG];
static int alvo_y[MAX_AG];

/* (nivel 5) Intencao DECLARADA na 1a passagem do tick: para onde cada bloco
 * pretende ir antes de saber o que os vizinhos querem. Na 2a passagem cada um
 * le estas intencoes — que ficam num array separado de alvo_x/y justamente
 * para a 2a passagem nunca ler uma decisao ja atualizada de um vizinho de
 * indice menor (senao a ordem de varredura voltaria a ser "fisica fantasma"). */
static int intencao_x[MAX_AG];
static int intencao_y[MAX_AG];

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

/* Raiz quadrada sem <math.h> (o projeto compila sem -lm, igual ao resto da
 * matematica daqui). Newton-Raphson: a iteracao x <- (x + v/x)/2 converge
 * QUADRATICAMENTE para sqrt(v) — cada passo ~dobra os digitos certos, entao
 * meia duzia ja satura a precisao de um float. Usada so para o desvio-padrao. */
static float raiz(float v) {
    if (v <= 0.0f) return 0.0f;       /* variancia nunca e negativa; protege /0 */
    float x = v > 1.0f ? v : 1.0f;    /* chute inicial >= sqrt(v) para todo v>0 */
    for (int i = 0; i < 12; i++)
        x = 0.5f * (x + v / x);
    return x;
}

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
        /* (nivel 6) tracos iniciais sorteados em torno das medias, com folga,
         * pra populacao comecar DIVERSA — sem variedade nao ha o que selecionar. */
        b->urgencia    = URGENCIA    * (0.5f + rng01());          /* ~0.5x..1.5x */
        b->peso_espaco = PESO_ESPACO * (0.5f + rng01());          /* ~0.5x..1.5x */
        b->desconto    = DESCONTO + (rng01() - 0.5f) * 0.2f;      /* +-0.1       */
        b->horizonte   = 1 + (int)(rng01() * HORIZONTE_MAX);      /* 1..MAX      */
        if (b->horizonte > HORIZONTE_MAX) b->horizonte = HORIZONTE_MAX;
        ocup[y][x] = n_blocos;
        n_blocos++;
    }
}

/* ================================================================== */
/*  PART 2 — BLOCOS / COGNICAO  (niveis 3 a 6)                         */
/*                                                                     */
/*  Nivel 3 — MODELO DE MUNDO: o bloco nao olha so a comida de agora;  */
/*  carrega a dinamica do mundo dentro de si e simula HORIZONTE ticks  */
/*  no futuro "de cabeca" (prever_valor), aplicando a propria regra de */
/*  rebrota e descontando o futuro. Prefere mancha fertil raspada (que */
/*  vai voltar) a tesouro de uso unico, e desconta celulas disputadas. */
/*                                                                     */
/*  Nivel 4 — AGENCIA: maximizar comida nao basta. O bloco tem MOTIVOS */
/*  EM CONFLITO e os pondera segundo o proprio estado interno:         */
/*     - FOME    com energia baixa, comer e quase tudo (urgencia);     */
/*     - ESPACO  saciado, comida rende pouco na margem e o que pesa e  */
/*               espaco aberto — menos disputa e lugar para a cria.    */
/*  Os pesos deslizam com a energia, entao o MESMO bloco no MESMO      */
/*  mundo decide diferente conforme sua necessidade: faminto forrageia */
/*  obstinado, saciado se espalha pela fronteira. Ele age para regular */
/*  a propria valencia — isso e agencia, nao mera reacao.              */
/*                                                                     */
/*  Nivel 5 — AUTO-MODELO: ate aqui o bloco modelava o mundo mas se    */
/*  esquecia de si — avaliava uma celula como se fosse o unico a       */
/*  cobica-la. Agora se inclui na cena: o tick tem DUAS passagens.     */
/*  1) todos DECLARAM a intencao (a decisao do nivel 4). 2) cada um    */
/*  RECONSIDERA lendo a intencao dos vizinhos: se varios miram a mesma */
/*  celula, so um entra (resolver), entao desvaloriza alvos disputados */
/*  e cede para a livre. So insiste no alvo cobicado se ele for MUITO  */
/*  melhor. E um lampejo de teoria da mente — decidir contando que os  */
/*  outros tambem decidem.                                             */
/*                                                                     */
/*  Nivel 6 — APRENDIZADO: ate aqui todos os blocos partilhavam a      */
/*  MESMA politica (constantes globais URGENCIA, HORIZONTE...). Agora  */
/*  esses pesos sao TRACOS de cada bloco, no struct, e a cria os herda */
/*  do pai com uma mutacaozinha (reproduzir). Ninguem projeta a melhor */
/*  estrategia: quem por acaso herda uma politica que come mais vive   */
/*  mais e deixa mais filhos, entao a media da populacao DERIVA para o */
/*  que funciona naquele mundo. Evolucao por selecao natural — sem     */
/*  gradiente, sem recompensa explicita, so sobrevivencia diferencial. */
/*  Veja os "tracos medios" no HUD mudarem ao longo dos ticks.         */
/* ================================================================== */

/* Conta vizinhos ocupados de (cx,cy), ignorando o proprio bloco. Serve a
 * dois propositos: a disputa por comida (nivel 3) e o desejo de espaco
 * aberto (nivel 4). */
static int rivais_em(int cx, int cy, int self_x, int self_y) {
    int rivais = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
            if (nx == self_x && ny == self_y) continue;   /* nao se conta */
            if (ocup[ny][nx] != -1) rivais++;
        }
    }
    return rivais;
}

/* (nivel 3, com tracos do nivel 6) Quanto de comida o bloco PREVE colher se
 * ocupar (cx,cy) pelos proximos b->horizonte ticks. Roda, na cabeca do bloco,
 * a propria dinamica do mundo — comer -> rebrotar -> comer... — descontando o
 * futuro pelo proprio b->desconto e partilhando a colheita com os rivais.
 * Quanto enxerga adiante (horizonte) e quanto liga pro futuro (desconto) sao
 * tracos individuais: cada bloco "imagina" o futuro de um jeito. */
static float prever_valor(int cx, int cy, const Bloco *b) {
    int rivais = rivais_em(cx, cy, b->x, b->y);
    float partilha = 1.0f / (1.0f + COMPETICAO * rivais);

    float food = comida[cy][cx];
    float cap  = capacidade[cy][cx];
    float valor = 0.0f, peso = 1.0f;
    for (int h = 0; h < b->horizonte; h++) {
        float garfada = menor(food, INGESTAO) * partilha;
        valor += peso * garfada;
        food  -= garfada;
        food  += REGROW * (cap - food);   /* a regra de rebrota, prevista  */
        peso  *= b->desconto;
    }
    return valor;
}

/* (nivel 4, com tracos do nivel 6) Utilidade de uma celula para ESTE bloco
 * agora: combina os dois motivos com pesos que dependem da fome E dos tracos
 * herdados (b->urgencia, b->peso_espaco). E aqui que mora a agencia — e e
 * exatamente aqui que a personalidade herdada muda quem o bloco e. */
static float utilidade(int cx, int cy, Bloco *b) {
    /* fome em 0..1: 1 = a beira da morte, 0 = saciado. */
    float fome = 1.0f - b->energia / SACIADO;
    if (fome < 0.0f) fome = 0.0f;
    if (fome > 1.0f) fome = 1.0f;

    float comida_prev = prever_valor(cx, cy, b);
    float espaco = (8 - rivais_em(cx, cy, b->x, b->y)) / 8.0f;   /* 0..1 */

    /* Faminto: a comida vale ainda mais (urgencia escala com a fome).
     * Saciado: a comida pesa pouco e entra o desejo de espaco aberto. */
    return comida_prev * (1.0f + b->urgencia * fome)
         + b->peso_espaco * espaco * (1.0f - fome);
}

/* (nivel 5) Quantos OUTROS blocos declararam que querem entrar em (cx,cy)?
 * So vizinhos imediatos de (cx,cy) podem mira-la (ninguem anda mais que um
 * passo), entao basta varrer o 3x3 ao redor e ler a intencao de cada um. E o
 * bloco "lendo a mente" dos vizinhos a partir das intencoes ja declaradas. */
static int pretendentes_em(int cx, int cy, int self_i) {
    int n = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
            int j = ocup[ny][nx];
            if (j == -1 || j == self_i) continue;       /* vazio ou eu mesmo  */
            if (intencao_x[j] == cx && intencao_y[j] == cy) n++;
        }
    }
    return n;
}

/* Varre as celulas 3x3 alcancaveis (mais ficar parado) e devolve a de maior
 * pontuacao. Pontuar = utilidade do nivel 4; se 'antecipar', o nivel 5 ainda
 * desvaloriza alvos que os vizinhos tambem declararam querer (so um entra). */
static void melhor_celula(Bloco *b, int i, int antecipar, int *bx, int *by) {
    int   melhor_x = b->x, melhor_y = b->y;   /* ficar parado e sempre valido */
    float melhor   = utilidade(b->x, b->y, b);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = b->x + dx, ny = b->y + dy;
            if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
            if (ocup[ny][nx] != -1) continue;        /* ocupada: inalcancavel */

            float u = utilidade(nx, ny, b);
            if (antecipar) {                         /* nivel 5: prever a disputa */
                int pret = pretendentes_em(nx, ny, i);
                u /= 1.0f + ANTECIPACAO * pret;
            }
            if (u > melhor) {
                melhor = u;
                melhor_x = nx; melhor_y = ny;
            }
        }
    }
    *bx = melhor_x; *by = melhor_y;
}

/* 1a passagem (nivel 5): cada bloco DECLARA, sem olhar os vizinhos, para onde
 * pretende ir — e exatamente a decisao do nivel 4. */
static void declarar(int i) {
    melhor_celula(&blocos[i], i, 0, &intencao_x[i], &intencao_y[i]);
}

/* 2a passagem (nivel 5): cada bloco RECONSIDERA, agora antecipando a disputa
 * lida nas intencoes dos vizinhos, e fixa o alvo final do tick. */
static void decidir(int i) {
    melhor_celula(&blocos[i], i, 1, &alvo_x[i], &alvo_y[i]);
}

/* ================================================================== */
/*  PART 3 — SIMULACAO (o tick)                                        */
/*  Um tick tem fases bem separadas para ser deterministico e justo:   */
/*    A. DECIDIR     2 passagens (nv5): declarar a intencao, depois     */
/*                   reconsiderar lendo a intencao dos vizinhos          */
/*    B. RESOLVER    conflitos: dois blocos, uma celula -> so um entra  */
/*    C. APLICAR     move quem ganhou, depois come e paga metabolismo   */
/*    D. REPRODUZIR  blocos saciados se dividem; a cria herda os        */
/*                   tracos do pai com mutacao (nivel 6)                 */
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

/* (nivel 6) Mutacao de um traco continuo: um empurraozinho uniforme +-passo,
 * mantido numa faixa saudavel. passo = 0 -> a cria e um clone exato do pai. */
static float muta_traco(float v, float passo, float lo, float hi) {
    v += (rng01() * 2.0f - 1.0f) * passo;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

/* (nivel 6) Mutacao do horizonte (inteiro): de vez em quando, um passo de +-1. */
static int muta_horizonte(int h) {
    if (rng01() < 2.0f * MUTACAO) h += (rng01() < 0.5f) ? -1 : 1;
    if (h < 1) h = 1;
    if (h > HORIZONTE_MAX) h = HORIZONTE_MAX;
    return h;
}

/* D. Reproducao: um bloco saciado se divide num vizinho vazio, partilhando a
 * energia. No nivel 6 isto deixa de ser "so uma lei do mundo": e o canal da
 * HERANCA — a cria recebe os tracos do pai com mutacao, e a selecao natural
 * que daqui emerge e o que faz a populacao APRENDER ao longo das geracoes. */
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
        Bloco *pai = &blocos[i], *cria = &blocos[j];
        pai->energia *= 0.5f;                /* a energia se divide         */
        cria->x = lx[e]; cria->y = ly[e];
        cria->energia = pai->energia;
        cria->vivo = 1;
        /* (nivel 6) a cria HERDA os tracos do pai, cada um com uma mutacaozinha.
         * Quem herdou uma politica melhor come mais, vive mais e tem mais
         * filhos — e a media da populacao deriva para estrategias vencedoras. */
        cria->urgencia    = muta_traco(pai->urgencia,    1.5f * MUTACAO, 0.0f, 6.0f);
        cria->peso_espaco = muta_traco(pai->peso_espaco, 2.0f * MUTACAO, 0.0f, 8.0f);
        cria->desconto    = muta_traco(pai->desconto,    0.4f * MUTACAO, 0.30f, 0.98f);
        cria->horizonte   = muta_horizonte(pai->horizonte);
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
/*  Duas janelas para o mesmo mundo: a visao de DEUS (o mundo inteiro  */
/*  de fora, desenhar) e a PRIMEIRA PESSOA (descer para dentro de um   */
/*  unico bloco e ver so o que ele percebe e sente, desenhar_1p) — os  */
/*  dois lados do "explanatory gap", a um aperto de tecla um do outro. */
/* ================================================================== */

/* Codigos ANSI: limpar tela, esconder/mostrar cursor, cores. */
#define CLS      "\033[H\033[2J"
#define CUR_OFF  "\033[?25l"
#define CUR_ON   "\033[?25h"
#define RESET    "\033[0m"

static volatile sig_atomic_t parar = 0;
static void ao_interromper(int s) { (void)s; parar = 1; }

/* ------------------------------------------------------------------ */
/*  Interacao: a "pilula vermelha" (entrar/sair de um bloco).          */
/*  Estado da JANELA, nao do mundo — por isso fica fora da simulacao.  */
/* ------------------------------------------------------------------ */
static int modo_1p    = 0;   /* 0 = visao de deus; 1 = primeira pessoa     */
static int foco       = -1;  /* indice do bloco "habitado" em 1a pessoa    */
static int pausado    = 0;   /* congela o tempo sem fechar a janela        */
static int interativo = 0;   /* so liga teclado se stdin E stdout sao tty  */

/* Terminal em modo CRU: ler tecla a tecla, sem esperar Enter e sem eco.
 * Guardamos o estado original para devolver o terminal intacto na saida. */
static struct termios termios_orig;
static int termios_ativo = 0;
static void termios_restaura(void) {
    if (termios_ativo) {
        tcsetattr(STDIN_FILENO, TCSANOW, &termios_orig);
        termios_ativo = 0;
    }
}
static void termios_cru(void) {
    if (tcgetattr(STDIN_FILENO, &termios_orig) != 0) return;
    struct termios cru = termios_orig;
    cru.c_lflag &= ~(ICANON | ECHO);   /* sem buffer de linha, sem eco  */
    cru.c_cc[VMIN]  = 0;               /* read() devolve na hora...     */
    cru.c_cc[VTIME] = 0;               /* ...com 0 bytes se nada chegou */
    if (tcsetattr(STDIN_FILENO, TCSANOW, &cru) == 0) termios_ativo = 1;
}

/* Le uma tecla ja disponivel; devolve 0 se nao ha nenhuma (nao bloqueia). */
static int ler_tecla(void) {
    unsigned char c;
    return (read(STDIN_FILENO, &c, 1) == 1) ? (int)c : 0;
}

/* Acha um bloco VIVO a partir de 'de', andando na direcao 'dir' (+1/-1) e
 * dando a volta no array. Devolve -1 se nao restou ninguem vivo. */
static int bloco_vivo(int de, int dir) {
    if (n_blocos == 0) return -1;
    for (int passo = 0; passo < n_blocos; passo++) {
        de = (de + dir + n_blocos) % n_blocos;
        if (blocos[de].vivo) return de;
    }
    return -1;
}

/* Garante que 'foco' aponta para um bloco vivo (ou -1 se a vida acabou). */
static void foco_valido(void) {
    if (foco < 0 || foco >= n_blocos || !blocos[foco].vivo)
        foco = bloco_vivo(foco < 0 ? -1 : foco, +1);
}

/* Estatisticas para o HUD. */
static int populacao(void) {
    int n = 0;
    for (int i = 0; i < n_blocos; i++) if (blocos[i].vivo) n++;
    return n;
}

/* ------------------------------------------------------------------ */
/*  Phi (Φ): a "luz acesa" — um PROXY de integracao, sabor IIT.        */
/*                                                                     */
/*  A IIT (Tononi) diz que consciencia e informacao INTEGRADA: o       */
/*  quanto o todo e irredutivel a soma das partes. O Phi de verdade e  */
/*  incomputavel aqui (e, a rigor, em quase tudo) — entao isto e uma   */
/*  CARICATURA honesta, nao a coisa. Mede UMA intuicao: o quanto a     */
/*  escolha integrada e IRREDUTIVEL ao reflexo puro (ir a celula com   */
/*  mais comida AGORA, o nivel 2). Sobre as jogadas possiveis, compara */
/*  a ordem de preferencia do valor COMPLETO (modelo de mundo+agencia  */
/*  +auto-modelo+tracos) com a do valor REATIVO (so a comida do        */
/*  instante). Quanto mais as duas ordens discordam, mais "trabalho de */
/*  integracao" entrou na decisao. Reflexo puro -> 0; muita reorgani-  */
/*  zacao -> alto. (distancia de Kendall entre as ordens, escala 0..10) */
/* ------------------------------------------------------------------ */

/* Coleta as jogadas alcancaveis (ficar + vizinhos vazios) e devolve, em 0..10,
 * o quanto a ordem de preferencia INTEGRADA (utilidade) discorda da ordem
 * REATIVA (comida do instante) — a distancia de Kendall entre as duas. */
static float phi_proxy(Bloco *b) {
    float u[9], f[9];
    int n = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int nx = b->x + dx, ny = b->y + dy;
            if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
            if (!(dx == 0 && dy == 0) && ocup[ny][nx] != -1) continue;  /* inalcancavel */
            u[n] = utilidade(nx, ny, b);     /* valor integrado (modelo+agencia+...) */
            f[n] = comida[ny][nx];           /* valor reativo  (so a comida de agora) */
            n++;
        }
    if (n < 2) return 0.0f;

    int disc = 0, tot = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) {
            float du = u[i] - u[j], df = f[i] - f[j];
            if (du * df < 0.0f) disc++;     /* as duas ordens discordam neste par */
            tot++;
        }
    return tot ? 10.0f * (float)disc / (float)tot : 0.0f;
}

/* ------------------------------------------------------------------ */
/*  Agregados de uma populacao num tick: a "leitura" do instrumento.   */
/*  Um SO lugar calcula tudo (DRY) — o HUD e o log de dados (--log)     */
/*  consomem a mesma struct, entao a tela e o CSV nunca se contradizem. */
/* ------------------------------------------------------------------ */
typedef struct {
    int   pop;
    float energia_media;
    float comida_total;
    float phi_media;
    /* media (_m) e desvio-padrao (_sd) de cada traco herdavel do nivel 6 */
    float hor_m,  hor_sd;
    float desc_m, desc_sd;
    float urg_m,  urg_sd;
    float esp_m,  esp_sd;
} Stats;

static Stats coletar_stats(void) {
    Stats s = {0};                 /* zera tudo: pop=0, medias=0...           */

    /* 1a passagem: somas -> medias. Precisamos da media ANTES da variancia. */
    float se = 0, su = 0, sp = 0, sd = 0, sh = 0, sphi = 0;
    for (int i = 0; i < n_blocos; i++) if (blocos[i].vivo) {
        s.pop++;
        se   += blocos[i].energia;
        su   += blocos[i].urgencia;
        sp   += blocos[i].peso_espaco;
        sd   += blocos[i].desconto;
        sh   += blocos[i].horizonte;     /* int -> float na soma              */
        sphi += phi_proxy(&blocos[i]);
    }
    float inv = s.pop ? 1.0f / s.pop : 0.0f;
    s.energia_media = se   * inv;
    s.phi_media     = sphi * inv;
    s.hor_m  = sh * inv;  s.desc_m = sd * inv;
    s.urg_m  = su * inv;  s.esp_m  = sp * inv;

    /* 2a passagem: variancia = media dos desvios ao quadrado. Populacional
     * (/N, nao /N-1) porque temos a populacao INTEIRA, nao uma amostra dela.
     * Duas passagens em vez da formula de uma so ("media dos quadrados menos
     * quadrado da media"): aquela subtrai dois numeros grandes e quase iguais
     * — cancelamento catastrofico, que come os digitos significativos. */
    float vh = 0, vd = 0, vu = 0, ve = 0;
    for (int i = 0; i < n_blocos; i++) if (blocos[i].vivo) {
        float dh = blocos[i].horizonte   - s.hor_m;   vh += dh * dh;
        float dd = blocos[i].desconto    - s.desc_m;  vd += dd * dd;
        float du = blocos[i].urgencia    - s.urg_m;   vu += du * du;
        float de = blocos[i].peso_espaco - s.esp_m;   ve += de * de;
    }
    s.hor_sd  = raiz(vh * inv);  s.desc_sd = raiz(vd * inv);
    s.urg_sd  = raiz(vu * inv);  s.esp_sd  = raiz(ve * inv);

    /* comida total do solo: varre o reticulado, nao os blocos */
    float sc = 0;
    for (int y = 0; y < ALT; y++)
        for (int x = 0; x < LARG; x++) sc += comida[y][x];
    s.comida_total = sc;

    return s;     /* struct devolvida por VALOR: copia barata, sem ponteiro solto */
}

/* ------------------------------------------------------------------ */
/*  Bateria de desbotamento: para cada palavra mental, um mostrador    */
/*  [0,1] que mede se a FACULDADE correspondente carrega o             */
/*  comportamento. Duas familias: ABLACAO (arranco a faculdade — a     */
/*  decisao muda?) e CALIBRACAO (uma estrutura interna bate com a      */
/*  realidade?). Se arrancar nao muda nada, a palavra desbota. Mede    */
/*  FUNCAO (papel causal), nunca experiencia — cada um e caricatura.   */
/* ------------------------------------------------------------------ */
typedef struct {
    float modelo;       /* CALIBRACAO "preve/sabe": o mapa bate com o territorio  */
    float agencia;      /* ABLACAO "quer/escolhe": a decisao muda com a fome      */
    float automodelo;   /* ABLACAO "eu, um entre outros": muda ao antecipar       */
} Bateria;

static Bateria ultima_bateria;   /* ultimo tick medido; lido pelo HUD e pelo log */

/* Estado do mostrador 'modelo' (arrays paralelos a blocos[]): ao decidir, cada
 * bloco PREVE a colheita do 1o passo na celula escolhida; um tick depois a gente
 * confere com o que ele DE FATO colheu. So da pra pontuar uma previsao depois do
 * desfecho — dai o atraso natural de 1 tick. */
static float pred_colheita[MAX_AG];   /* colheita prevista no alvo cognitivo */
static float energia_antes[MAX_AG];   /* energia na hora de decidir          */
static int   pred_valido[MAX_AG];     /* este slot previu algo neste tick?   */

/* FASE 1 (apos decidir, ANTES de resolver): os dois mostradores de ablacao, e
 * guarda a previsao do modelo. Le o alvo COGNITIVO — antes de resolver() trocar
 * alvos negados pela posicao atual (queremos o efeito da mente, nao do bloqueio). */
static void medir_decisao(void) {
    int   n = 0;
    float ag = 0.0f, au = 0.0f;
    for (int i = 0; i < n_blocos; i++) {
        if (!blocos[i].vivo) { pred_valido[i] = 0; continue; }
        n++;

        /* AUTO-MODELO (nv5), de graca: antecipar os rivais mudou a escolha?
         * intencao = pre-social (declarar) x alvo = pos-social (decidir). */
        if (intencao_x[i] != alvo_x[i] || intencao_y[i] != alvo_y[i]) au += 1.0f;

        /* AGENCIA (nv4): so mudando a fome, a decisao muda? Reroda a escolha em
         * dois clones (faminto x saciado) no mesmo mundo. melhor_celula nao tem
         * efeito colateral (so escreve nos out-params): o contrafactual e seguro. */
        Bloco faminto = blocos[i], saciado = blocos[i];
        faminto.energia = 0.10f * SACIADO;   /* a beira da morte */
        saciado.energia = 1.00f * SACIADO;   /* fome zero        */
        int fx, fy, sx, sy;
        melhor_celula(&faminto, i, 1, &fx, &fy);
        melhor_celula(&saciado, i, 1, &sx, &sy);
        if (fx != sx || fy != sy) ag += 1.0f;

        /* MODELO (nv3): guarda a colheita prevista (1 passo, so o recurso) no
         * alvo escolhido. O 'real' chega no medir_modelo, depois de comer. */
        pred_colheita[i] = menor(comida[alvo_y[i]][alvo_x[i]], INGESTAO);
        energia_antes[i] = blocos[i].energia;
        pred_valido[i]   = 1;
    }
    float inv = n ? 1.0f / n : 0.0f;
    ultima_bateria.agencia    = ag * inv;
    ultima_bateria.automodelo = au * inv;
}

/* FASE 2 (apos aplicar_e_comer, ANTES de reproduzir reusar slots): confere a
 * previsao contra a colheita real. real = variacao de energia + metabolismo pago
 * (Δe = colheita - METABOLISMO). Erro simetrico normalizado -> acc em [0,1]:
 * 1 = previu certo, 0 = errou tudo. Quem chegou ao alvo acerta na mosca (nada
 * mexe na comida ali no meio-tempo); o erro vem de quem foi BARRADO e colheu
 * noutra celula — o mapa nao previu perder a disputa. */
static void medir_modelo(void) {
    int   n = 0;
    float acc = 0.0f;
    for (int i = 0; i < n_blocos; i++) {
        if (!pred_valido[i]) continue;
        n++;
        float real = (blocos[i].energia - energia_antes[i]) + METABOLISMO;
        if (real < 0.0f) real = 0.0f;
        float pred = pred_colheita[i];
        float soma = pred + real;
        float dif  = pred > real ? pred - real : real - pred;
        acc += soma > 0.0f ? 1.0f - dif / soma : 1.0f;  /* previu 0 e colheu 0 = ok */
    }
    ultima_bateria.modelo = n ? acc / n : 0.0f;
}

/* Desenha um frame inteiro num buffer e cospe de uma vez (menos piscada). */
static void desenhar(uint32_t seed, long tick) {
    static char buf[ALT * LARG * 16 + 4096];
    int p = 0;
    p += sprintf(buf + p, CLS);
    p += sprintf(buf + p,
        "  M A T R I X  —  mundo procedural, blocos sencientes (nivel 6)\n");

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

    /* HUD — uma so coleta de estatisticas alimenta a tela e o log (coletar_stats). */
    Stats st = coletar_stats();

    p += sprintf(buf + p,
        "  seed %-10u  tick %-6ld  pop %-4d  energia media %5.1f  comida %6.0f  Φ~ %4.1f\n",
        seed, tick, st.pop, st.energia_media, st.comida_total, st.phi_media);
    /* media ± desvio: o desvio revela se a populacao CONVERGE (todos parecidos,
     * desvio -> 0) ou se DIVERSIFICA em nichos (desvio cresce). */
    p += sprintf(buf + p,
        "  tracos media±desvio:  horizonte %4.1f±%-3.1f  desconto %4.2f±%-4.2f"
        "  urgencia %4.1f±%-3.1f  espaco %4.1f±%-3.1f\n",
        st.hor_m, st.hor_sd, st.desc_m, st.desc_sd,
        st.urg_m, st.urg_sd, st.esp_m, st.esp_sd);
    /* bateria de desbotamento (0..1): quanto cada faculdade carrega o comportamento.
     * modelo = mapa bate com o territorio; agencia = decisao muda com a fome;
     * auto-modelo = decisao muda ao antecipar rivais. */
    p += sprintf(buf + p,
        "  bateria (0..1):  modelo %.2f   agencia %.2f   auto-modelo %.2f\n",
        ultima_bateria.modelo, ultima_bateria.agencia, ultima_bateria.automodelo);
    p += sprintf(buf + p,
        "  legenda: \033[92m@\033[0m forte  \033[93m@\033[0m ok  \033[91m@\033[0m fraco"
        "   \033[2;32m. : *\033[0m comida    (Ctrl+C para sair)\n");

    fwrite(buf, 1, p, stdout);
    fflush(stdout);
}

/* Uma barrinha ASCII de 10 casas para um valor 0..1 (clampado). */
static void barra(char *out, float frac) {
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int n = (int)(frac * 10.0f + 0.5f);
    for (int i = 0; i < 10; i++) out[i] = (i < n) ? '#' : '.';
    out[10] = '\0';
}

/* (Ponto 2 — o bloco que diz "eu") Monta, SO a partir do que o bloco percebe e
 * sente, algumas frases em PRIMEIRA PESSOA: a "voz" dele. Nada de novo entra no
 * mundo — e a mesma informacao da pilula vermelha, dita como auto-relato. mdx/mdy
 * e a jogada ja escolhida (para a fala bater com a decisao exibida). O ponto
 * filosofico (o teste do zumbi) esta no FILOSOFIA.md. */
static void auto_relato(const Bloco *b, int mdx, int mdy, char *out, int cap) {
    int n = 0;
    float fome = 1.0f - b->energia / SACIADO;
    if (fome < 0.0f) fome = 0.0f;
    if (fome > 1.0f) fome = 1.0f;

    const char *sentir =
        fome > 0.66f ? "Estou fraco; algo me puxa para a comida. Chamo isto de fome." :
        fome > 0.33f ? "A fome cutuca. Sigo de pe, mas penso em comer."               :
                       "Estou saciado. O que me move agora e outra coisa: espaco.";

    int rivais = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = b->x + dx, ny = b->y + dy;
            if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
            if (ocup[ny][nx] != -1) rivais++;
        }
    const char *ver =
        rivais >= 3 ? "A minha volta, gente demais — disputamos o mesmo chao." :
        rivais >= 1 ? "Ha outro por perto; sei que ele tambem decide."        :
                      "Estou sozinho aqui, so eu e a terra.";

    const char *querer =
        (mdx == 0 && mdy == 0)   ? "Fico onde estou; aqui ainda e o melhor que prevejo." :
        (mdx == 0 && mdy == -1)  ? "Vou para cima — imagino que la rende mais."          :
        (mdx == 0 && mdy ==  1)  ? "Vou para baixo — imagino que la rende mais."         :
        (mdx == -1 && mdy == 0)  ? "Vou para a esquerda — imagino que la rende mais."    :
        (mdx ==  1 && mdy == 0)  ? "Vou para a direita — imagino que la rende mais."     :
                                   "Vou para um canto — imagino que la rende mais.";

    n += snprintf(out + n, cap - n, "      \"%s\"\n", sentir);
    n += snprintf(out + n, cap - n, "      \"%s\"\n", ver);
    snprintf(out + n, cap - n, "      \"%s\"\n", querer);
}

/* A "PILULA VERMELHA": a vista em PRIMEIRA PESSOA de um unico bloco. O mundo
 * encolhe para o que ELE percebe (a vizinhanca 3x3) e para o que ELE sente
 * (energia, fome, tracos) e quer (a utilidade imaginada de cada jogada).
 * Onde a visao de deus mostra o mecanismo de fora, esta mostra o lado de
 * dentro — encena, em ASCII, a pergunta "como e ser este bloco?". */
static void desenhar_1p(uint32_t seed, long tick, int f) {
    static char buf[8192];
    int p = 0;
    Bloco *b = &blocos[f];

    p += sprintf(buf + p, CLS);
    p += sprintf(buf + p,
        "  PILULA VERMELHA  —  dentro do bloco #%d   (seed %u, tick %ld)\n\n",
        f, seed, tick);

    /* 1) O mundo como ELE o percebe: nada alem da vizinhanca 3x3. */
    p += sprintf(buf + p, "  O universo, do lado de dentro (tudo que ele percebe):\n\n");
    for (int dy = -1; dy <= 1; dy++) {
        p += sprintf(buf + p, "      ");
        for (int dx = -1; dx <= 1; dx++) {
            int nx = b->x + dx, ny = b->y + dy;
            if (dx == 0 && dy == 0)
                p += sprintf(buf + p, "\033[96m[EU]\033[0m ");          /* ciano: ele  */
            else if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT)
                p += sprintf(buf + p, "\033[90m####\033[0m ");          /* o fim do mundo */
            else if (ocup[ny][nx] != -1)
                p += sprintf(buf + p, "\033[91m @  \033[0m ");          /* outro: um rival */
            else {
                float c = comida[ny][nx];
                char g = c < 0.4f ? ' ' : c < 1.3f ? '.' : c < 2.8f ? ':' : '*';
                p += sprintf(buf + p, "\033[2;32m %c  \033[0m ", g);    /* solo: comida   */
            }
        }
        p += sprintf(buf + p, "\n");
    }

    /* 2) Como ele se SENTE: valencia e fome (a valencia em acao). */
    float fome = 1.0f - b->energia / SACIADO;
    if (fome < 0.0f) fome = 0.0f;
    if (fome > 1.0f) fome = 1.0f;
    char be[11], bf[11];
    barra(be, b->energia / REPRO);   /* REPRO ~ energia "cheia" (vai se dividir) */
    barra(bf, fome);
    p += sprintf(buf + p, "\n  Como ele se sente:\n");
    p += sprintf(buf + p, "      energia (valencia) %5.1f  [%s]\n", b->energia, be);
    p += sprintf(buf + p, "      fome               %4.2f  [%s]\n", fome, bf);
    p += sprintf(buf + p, "      personalidade: horizonte %d  desconto %.2f"
                          "  urgencia %.1f  espaco %.1f\n",
                 b->horizonte, b->desconto, b->urgencia, b->peso_espaco);
    char bphi[11];
    float phi = phi_proxy(b);
    barra(bphi, phi / 8.0f);
    p += sprintf(buf + p, "      integracao (Φ proxy) %4.1f  [%s]"
                          "   <- a decisao depende do todo?\n", phi, bphi);

    /* 3) O que ele QUER: a utilidade imaginada de cada jogada possivel. */
    float melhor = -1e30f; int mdx = 0, mdy = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int nx = b->x + dx, ny = b->y + dy;
            if (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT) continue;
            if (!(dx == 0 && dy == 0) && ocup[ny][nx] != -1) continue;  /* inalcancavel */
            float u = utilidade(nx, ny, b);
            if (u > melhor) { melhor = u; mdx = dx; mdy = dy; }
        }
    p += sprintf(buf + p, "\n  O que ele quer (utilidade imaginada de cada jogada):\n\n");
    for (int dy = -1; dy <= 1; dy++) {
        p += sprintf(buf + p, "      ");
        for (int dx = -1; dx <= 1; dx++) {
            int nx = b->x + dx, ny = b->y + dy;
            int fora    = (nx < 0 || nx >= LARG || ny < 0 || ny >= ALT);
            int ocupada = !fora && !(dx == 0 && dy == 0) && ocup[ny][nx] != -1;
            if (fora || ocupada) { p += sprintf(buf + p, "  ---  "); continue; }
            float u = utilidade(nx, ny, b);
            int eh_melhor = (dx == mdx && dy == mdy);
            const char *fmt = (dx == 0 && dy == 0)
                ? (eh_melhor ? "\033[93m(%4.1f)*\033[0m" : "(%4.1f) ")
                : (eh_melhor ? "\033[93m %4.1f* \033[0m" : " %4.1f  ");
            p += sprintf(buf + p, fmt, u);
        }
        p += sprintf(buf + p, "\n");
    }
    const char *dir =
        (mdx == 0 && mdy == 0)   ? "FICAR onde esta"       :
        (mdx == 0 && mdy == -1)  ? "subir (norte)"         :
        (mdx == 0 && mdy ==  1)  ? "descer (sul)"          :
        (mdx == -1 && mdy == 0)  ? "ir a esquerda (oeste)" :
        (mdx ==  1 && mdy == 0)  ? "ir a direita (leste)"  :
        (mdx == -1 && mdy == -1) ? "noroeste"              :
        (mdx ==  1 && mdy == -1) ? "nordeste"              :
        (mdx == -1 && mdy ==  1) ? "sudoeste"              : "sudeste";
    p += sprintf(buf + p, "\n      decisao: %s  (utilidade %.1f)\n", dir, melhor);

    /* 4) O que ele DIZ de si — em primeira pessoa (ponto 2: o bloco que diz "eu"). */
    char fala[512];
    auto_relato(b, mdx, mdy, fala, (int)sizeof fala);
    p += sprintf(buf + p, "\n  O que ele diz de si (em primeira pessoa):\n");
    p += sprintf(buf + p, "\033[3m%s\033[0m", fala);   /* italico, se o terminal apoiar */

    /* 5) O fechamento — o ponto filosofico, agora com a voz dele dentro. */
    p += sprintf(buf + p,
        "\n  Isto e tudo que ele sabe — e, ainda assim, ele diz \"eu\".\n"
        "  Voce ouve uma mente, ou um eco?  (FILOSOFIA.md: o teste do zumbi)\n");
    p += sprintf(buf + p,
        "  [p] visao de deus   [, .] trocar de bloco   [espaco] %s   [q] sair\n",
        pausado ? "retomar" : "pausa");

    fwrite(buf, 1, p, stdout);
    fflush(stdout);
}

int main(int argc, char **argv) {
    /* Separa FLAGS (--log ARQUIVO) dos argumentos POSICIONAIS, numa passagem.
     * Os posicionais sobrevivem na ordem em 'pos[]'; assim "seed ticks delay
     * foco" continua valendo com ou sem a flag, em qualquer posicao. */
    const char *log_path = NULL;
    char *pos[8]; int np = 0;
    pos[np++] = argv[0];
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            log_path = argv[++i];                 /* consome o proximo arg     */
        } else if (np < 8) {
            pos[np++] = argv[i];
        }
    }

    uint32_t seed  = (np > 1) ? (uint32_t)strtoul(pos[1], NULL, 10) : 20260628u;
    long    ticks  = (np > 2) ? strtol(pos[2], NULL, 10) : 0;   /* 0 = infinito */
    long    delay  = (np > 3) ? strtol(pos[3], NULL, 10) : 80;  /* ms por frame */
    if (delay < 0) delay = 0;
    /* 4o posicional opcional: entrar ja em 1a pessoa neste bloco (util para
     * inspecionar sem teclado, inclusive com a saida redirecionada). -1 = deus. */
    if (np > 4) { foco = (int)strtol(pos[4], NULL, 10); modo_1p = (foco >= 0); }

    /* --log ARQUIVO: abre um CSV e escreve uma linha de estatisticas por tick.
     * A simulacao vira um DATASET reproduzivel — a base do "instrumento". O log
     * roda independente da animacao (plano de DADOS x plano de APRESENTACAO). */
    FILE *logf = NULL;
    if (log_path) {
        logf = fopen(log_path, "w");
        if (!logf) {
            fprintf(stderr, "matrix: nao consegui abrir '%s' para escrita\n", log_path);
            return 1;
        }
        fprintf(logf, "seed,tick,pop,energia_media,comida_total,"
                      "hor_m,hor_sd,desc_m,desc_sd,urg_m,urg_sd,esp_m,esp_sd,"
                      "modelo,agencia,automodelo,phi\n");
    }

    rng_estado = seed ? seed : 1u;   /* o RNG do universo nasce da seed      */
    signal(SIGINT, ao_interromper);

    gerar_mundo(seed);
    semear_blocos();

    /* Teclado so faz sentido num terminal de verdade (stdin E stdout tty).
     * Com a saida num pipe/arquivo seguimos nao-interativos, igual a antes —
     * os scripts de amostragem (./bin/matrix ... | grep) continuam valendo. */
    interativo = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    if (interativo) { termios_cru(); atexit(termios_restaura); }

    struct timespec pausa = { delay / 1000, (delay % 1000) * 1000000L };
    struct timespec respiro = { 0, 50 * 1000000L };   /* 50ms enquanto pausado */

    fputs(CUR_OFF, stdout);

    long t = 0;
    while (!parar && (ticks == 0 || t < ticks)) {
        /* 1) consumir o teclado (pode chegar mais de uma tecla por frame) */
        if (interativo) {
            int c;
            while ((c = ler_tecla())) {
                if      (c == 'q')                  parar = 1;
                else if (c == 'p' || c == '\t')   { modo_1p = !modo_1p; if (modo_1p) foco_valido(); }
                else if (c == ' ')                  pausado = !pausado;
                else if (modo_1p && (c == '.' || c == 'm')) { foco_valido(); foco = bloco_vivo(foco, +1); }
                else if (modo_1p && (c == ',' || c == 'n')) { foco_valido(); foco = bloco_vivo(foco, -1); }
            }
        }

        /* 2) em 1a pessoa, se a vida acabou, volta para a visao de deus */
        if (modo_1p) { foco_valido(); if (foco < 0) modo_1p = 0; }

        /* 3) desenhar conforme o modo */
        if (modo_1p) desenhar_1p(seed, t, foco);
        else         desenhar(seed, t);

        if (populacao() == 0) {                 /* extincao total           */
            printf("\n  Silencio. A populacao se extinguiu no tick %ld.\n", t);
            break;
        }

        /* 4) o tick — congelado enquanto 'pausado'. declarar -> reconsiderar ->
         * resolver -> escrever -> reproduzir -> mundo (as 2 passagens do nv5
         * leem o mesmo estado estavel: a 1a preenche intencoes, a 2a consulta). */
        if (!pausado) {
            for (int i = 0; i < n_blocos; i++)
                if (blocos[i].vivo) declarar(i);
            for (int i = 0; i < n_blocos; i++)
                if (blocos[i].vivo) decidir(i);

            /* FASE 1 da bateria: mostradores de ablacao (agencia, auto-modelo) e
             * guarda a previsao do modelo. So paga o custo quando alguem vai
             * consumir o resultado: a tela (interativo) ou o CSV. */
            if (interativo || logf) medir_decisao();

            if (logf) {
                Stats st = coletar_stats();   /* estado no inicio do tick t        */
                /* modelo vem com 1 tick de atraso por natureza: so da pra pontuar
                 * uma previsao DEPOIS que o desfecho acontece (ver medir_modelo). */
                fprintf(logf,
                    "%u,%ld,%d,%.3f,%.1f,"
                    "%.3f,%.3f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,"
                    "%.3f,%.3f,%.3f,%.3f\n",
                    seed, t, st.pop, st.energia_media, st.comida_total,
                    st.hor_m, st.hor_sd, st.desc_m, st.desc_sd,
                    st.urg_m, st.urg_sd, st.esp_m, st.esp_sd,
                    ultima_bateria.modelo, ultima_bateria.agencia,
                    ultima_bateria.automodelo, st.phi_media / 10.0f);
                fflush(logf);   /* descarrega ja: Ctrl+C no meio nao perde a cauda */
            }

            resolver();
            aplicar_e_comer();
            /* FASE 2: agora que os blocos comeram (mas ANTES de reproduzir reusar
             * slots), confere a previsao do modelo contra a colheita real. */
            if (interativo || logf) medir_modelo();
            reproduzir();
            rebrotar();
            t++;
        }

        if      (!pausado && delay > 0) nanosleep(&pausa, NULL);
        else if (pausado)               nanosleep(&respiro, NULL);
    }

    fputs(CUR_ON, stdout);
    if (interativo) termios_restaura();
    if (logf) fclose(logf);   /* fclose tambem descarrega o buffer pendente */
    if (parar) printf("\n  Encerrado no tick %ld. O universo era f(seed=%u).\n", t, seed);
    return 0;
}
