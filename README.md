# c_training

Monorepo de brincadeiras em **C** — feito pra treinar lógica de programação e a
linguagem na prática: jogos de terminal pra exercitar lógica pura, e apps
gráficas pra quando bater vontade de pixel.

A ideia é simples: cada projeto é uma pasta independente. Você compila, roda,
brinca, quebra, conserta e aprende.

## Por que C?

C te força a entender o que acontece "por baixo": memória, ponteiros, laços,
estruturas de dados na unha. Não tem rede de segurança — e é exatamente aí que
mora o aprendizado. Jogos de terminal são o playground perfeito: lógica pura,
sem se afogar em setup de gráficos. Quando quiser visual (janela, cores,
animação), o caminho mais divertido em C é a [raylib](https://www.raylib.com/) —
e já tem um exemplo dela aqui (o `plotter`).

## O que tem aqui

### 🎮 Jogos de terminal (`games/`)

| Jogo | O que treina |
|------|--------------|
| `adivinhacao`   | Adivinhe o número — mínimo, valida o ambiente |
| `jogo_da_velha` | Jogo da velha p/ 2 jogadores — lógica de verdade |

### 🖼️ Apps gráficas (`apps/`)

| App | O que é |
|-----|---------|
| `plotter` | Plotador de funções interativo (mini-Desmos) com **raylib** — digite `f(x)`, mexa nos sliders `a`/`b`/`c`, arraste e dê zoom |

## Estrutura

```
c_training/
├── README.md          # este guia
├── Makefile           # compila jogos e o plotter (saída em bin/)
├── CLAUDE.md          # notas técnicas (build da raylib, arquitetura do plotter)
├── games/
│   ├── adivinhacao/   # adivinhe o número
│   └── jogo_da_velha/ # jogo da velha 2 jogadores
└── apps/
    └── plotter/       # plotador de funções com raylib
```

## Como compilar e rodar

Você precisa de um compilador C (`gcc` ou `clang`) e `make`. Todos os binários
vão para a pasta `bin/`.

### Jogos de terminal

```sh
make                  # compila todos os jogos
make jogo_da_velha    # compila um jogo específico
./bin/adivinhacao     # roda
./bin/jogo_da_velha
```

### Plotter (gráfico, raylib)

O plotter depende da **raylib**, instalada por usuário em `~/.local` (não é
pacote do sistema). Os detalhes de como ela foi compilada do código-fonte estão
no [`CLAUDE.md`](CLAUDE.md). Com a raylib no lugar:

```sh
make plotter          # compila apps/plotter/plotter.c -> bin/plotter
./bin/plotter         # abre a janela
```

No plotter: digite a fórmula em `f(x)` (multiplicação explícita: `2*x`), arraste
os sliders `a`/`b`/`c`, **arraste** pra mover, **scroll** pra zoom, **R** pra
resetar a vista.

### Limpar

```sh
make clean            # apaga a pasta bin/
make list             # lista jogos e apps detectados
```

## Como adicionar um jogo novo

1. Crie a pasta `games/nome_do_jogo/` com um `main.c` dentro.
2. Pronto — o `Makefile` detecta sozinho. Rode `make nome_do_jogo`.

## Ideias de próximos jogos

Em ordem aproximada de dificuldade, ótimos pra treinar lógica:

- **Forca** — manipulação de strings e estado.
- **Pedra, papel e tesoura** — laços e `enum`.
- **Campo minado** — matrizes e busca (flood fill).
- **Snake** — entrada não-bloqueante e movimento no terminal.
- **2048** — manipulação de matriz e regras de combinação.
- **Roguelike simples** — mapa, colisão, turnos.
