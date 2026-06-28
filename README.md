# c_training

Monorepo de brincadeiras em **C** — feito pra treinar lógica de programação e a
linguagem através de pequenos jogos de terminal.

A ideia é simples: cada jogo é uma pasta independente dentro de `games/`, com seu
próprio `main.c`. Você compila, roda, brinca, quebra, conserta e aprende.

## Por que C?

C te força a entender o que acontece "por baixo": memória, ponteiros, laços,
estruturas de dados na unha. Não tem rede de segurança — e é exatamente aí que
mora o aprendizado. Jogos de terminal são o playground perfeito: lógica pura,
sem se afogar em setup de gráficos.

Quando quiser partir pro visual (janela, cores, animação), o caminho mais
divertido em C é a [raylib](https://www.raylib.com/). A gente adiciona quando
você quiser.

## Estrutura

```
c_training/
├── README.md
├── Makefile          # compila qualquer jogo
├── games/
│   ├── adivinhacao/  # adivinhe o número (mínimo, valida o ambiente)
│   └── jogo_da_velha/# jogo da velha 2 jogadores (lógica de verdade)
└── common/           # (futuro) código reutilizável entre jogos
```

## Como compilar e rodar

Você precisa de um compilador C (`gcc` ou `clang`) e `make`.

Compilar **todos** os jogos:

```sh
make
```

Os binários vão pra pasta `bin/`. Pra rodar:

```sh
./bin/adivinhacao
./bin/jogo_da_velha
```

Compilar **um** jogo específico:

```sh
make adivinhacao
make jogo_da_velha
```

Limpar os binários:

```sh
make clean
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
