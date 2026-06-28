# Matrix — mundo procedural com blocos sencientes

Um experimento meio **filosófico**, meio **de programação**: construir uma "Matrix"
de brinquedo onde "blocos" parecem vivos — sem nenhuma física hiper-realista.
Tudo roda no terminal, em ASCII, e é um único arquivo C (`main.c`) sem
dependências além da libc.

> **Onde estamos:** níveis 0 → 4 da escada de senciência implementados.
> Próximos: 5 (auto-modelo) e 6 (aprendizado).

---

## A ideia central: a "escada de senciência"

Em vez de tentar criar *consciência* (o *hard problem* — indecidível e
paralisante), trocamos a pergunta metafísica por uma **funcional**: que
comportamentos associamos a um ser senciente, e quais conseguimos implementar?
Daí uma escada, do mais barato ao mais caro. Senciência aqui **não é um botão
liga/desliga**, é uma posição na escada — e a gente decide até que degrau sobe.

| Nível | Propriedade | Estado |
|------:|-------------|:------:|
| 0 | **Reatividade** — responde a estímulo local | ✅ |
| 1 | **Memória / estado interno** — o passado importa | ✅ |
| 2 | **Valência** — coisas são boas/ruins (energia: viver × morrer) | ✅ |
| 3 | **Modelo de mundo** — simula o futuro e decide por ele | ✅ |
| 4 | **Agência** — pondera motivos em conflito p/ regular a valência | ✅ |
| 5 | **Auto-modelo** — o bloco se representa dentro do mundo | ⬜ próximo |
| 6 | **Aprendizado** — a política muda com a experiência (e é herdada) | ⬜ |

---

## Como compilar e rodar

Do diretório raiz do projeto (`c_training/`):

```sh
make matrix        # compila -> bin/matrix
./bin/matrix       # roda a animação (Ctrl+C para sair)
```

Precisa de um **terminal de verdade** (a tela se redesenha a cada tick via
códigos ANSI; não funciona com a saída redirecionada para arquivo/pipe).
Deixe a janela com pelo menos ~70 colunas de largura.

### Argumentos

```sh
./bin/matrix [seed] [ticks] [delay_ms]
```

| Arg | Default | O que faz |
|-----|---------|-----------|
| `seed`     | `20260628` | escolhe o universo (mesma seed = mundo idêntico, sempre) |
| `ticks`    | `0` (infinito) | quantos passos rodar; `0` = até `Ctrl+C` |
| `delay_ms` | `80` | pausa entre frames; menor = mais rápido |

```sh
./bin/matrix 42            # outro universo
./bin/matrix 42 500 30     # 500 ticks, rápido
./bin/matrix 7 200 150     # devagar, pra acompanhar
./bin/matrix 7 400 0       # o mais rápido possível (sem pausa)
```

### Determinismo

Todo o universo é `f(seed)` — geração do mundo **e** decisões/nascimentos dos
blocos vêm da mesma semente. Mesma seed ⇒ simulação idêntica, sempre. Esse é o
gancho filosófico central: o universo é determinístico e gerado, mas de dentro
parece aberto. Você, ao escolher a seed, é o relojoeiro.

Pra conferir a população num tick específico sem assistir à animação:

```sh
./bin/matrix 7 200 0 | grep -a -o 'pop [0-9]*' | tail -1
```

---

## O que aparece na tela

- `@` colorido = um bloco. A cor é a energia (valência):
  - 🟢 verde forte (`>10`) · 🟡 amarelo (`>4`) · 🔴 vermelho (fraco, perto da morte).
- `. : *` em verde fraco = densidade de comida no solo (recurso).
- ` ` (vazio) = deserto / comida quase zero.
- HUD embaixo: `seed`, `tick`, `pop` (população viva), `energia media`, `comida` (total no mundo).

Mesmo nos níveis atuais já dá pra ver emergir: manadas em torno de manchas
férteis, colapsos por escassez, ciclos de fartura/fome e (nível 4) blocos
saciados colonizando a fronteira. Tudo a partir de regras **estritamente
locais** (cada bloco só enxerga sua vizinhança 3×3).

---

## Arquitetura do `main.c`

Um arquivo, organizado num pipeline de 4 partes (mesma filosofia do
`apps/plotter/plotter.c`):

| Parte | Papel |
|-------|-------|
| **PART 1 — Mundo procedural** | O mundo é `f(seed, x, y)`: um hash inteiro vira *value-noise*, que vira manchas de comida (`gerar_mundo`, `ruido`, `hash2`). |
| **PART 2 — Blocos / cognição** | A "mente" do bloco. É aqui que mora a escada (ver abaixo). |
| **PART 3 — Simulação (o tick)** | Fases separadas: `decidir` → `resolver` (conflitos) → `aplicar_e_comer` → `reproduzir` → `rebrotar`. |
| **PART 4 — Render + loop** | Desenho ASCII num buffer + HUD, laço principal em `main`, `Ctrl+C` limpa o cursor e sai. |

### Detalhes que importam antes de editar

- **Estado em globais file-static**: `comida[][]`, `capacidade[][]` (teto de
  rebrota por célula, vindo do ruído), `ocup[][]` (índice do bloco numa célula
  ou `-1`), o array `blocos[]` e o RNG do universo (`rng_estado`).
- **Um bloco** é `{ x, y, energia, vivo }`. A `energia` é a valência: chega a
  zero ⇒ morte. `vivo == 0` marca slot livre (mortos deixam buracos no array).
- **Leitura × escrita separadas no tick.** Todos decidem (`decidir` preenche
  `alvo_x/alvo_y`) lendo o estado; só depois o mundo aplica. Isso evita que a
  ordem de varredura vire "física fantasma" (o bug do organismo que anda mais
  rápido pra um lado só porque o laço o visita antes).
- **Resolução de conflito** (`resolver`): dois blocos, uma célula → só um entra.
  Para simplicidade e zero bugs de "troca de lugar", só se move para células que
  estavam **vazias no início do tick**, e cada célula vai para um único
  pretendente (o de menor índice); os demais ficam parados.
- **Sem `<math.h>`.** A regra padrão do Makefile compila `games/<nome>/*.c`
  **sem `-lm`**. Por isso o ruído usa aritmética inteira + polinômios e não há
  `sin`/`exp`/`sqrt`. Mantenha assim, senão `make matrix` quebra.

### Como a cognição cresceu (PART 2)

Toda a inteligência está em `decidir` e nas funções que ela chama. A cada nível,
só essa parte mudou:

- **Nível 2 (valência):** o bloco olhava a vizinhança 3×3 e ia para a célula com
  mais comida **agora**. Puramente reativo.
- **Nível 3 (modelo de mundo):** `prever_valor(cx,cy,...)` simula `HORIZONTE`
  ticks no futuro "de cabeça" — come → rebrota → come… — aplicando a **mesma**
  regra de rebrota do mundo e descontando o futuro (`DESCONTO`). O bloco passa a
  preferir mancha fértil momentaneamente raspada (que vai voltar) a um tesouro de
  uso único, e desconta células disputadas (`COMPETICAO`). Nasce a fresta entre
  **o mundo** e **o mundo segundo o bloco** — o modelo interno, que pode errar.
- **Nível 4 (agência):** `utilidade(cx,cy,b)` combina **motivos em conflito**
  pesados pelo estado interno:

  ```
  utilidade = comida_prevista · (1 + URGENCIA·fome)      ← motivo FOME
            + PESO_ESPACO · espaco_livre · (1 − fome)    ← motivo ESPACO
  ```

  com `fome = clamp(1 − energia/SACIADO, 0, 1)`. Faminto → forrageia obstinado;
  saciado → busca espaço aberto (menos disputa, lugar pra cria). O **mesmo** bloco
  no **mesmo** mundo decide diferente conforme a necessidade. Ele age para regular
  a própria valência — isso é agência, não reação.

---

## Parâmetros pra brincar (no topo do `main.c`)

São as "leis da física" deste mundinho. Mude, recompile (`make matrix`), observe.

| Constante | Default | Efeito |
|-----------|--------:|--------|
| `LARG`, `ALT` | 64, 22 | tamanho do mundo (cuidado com a largura do terminal) |
| `MAX_COMIDA` | 5.0 | teto de comida por célula |
| `REGROW` | 0.06 | velocidade de rebrota da comida |
| `N_INICIAL` | 60 | quantos blocos nascem no começo |
| `ENERGIA0` | 6.0 | energia inicial de um bloco |
| `INGESTAO` | 2.0 | quanto come por tick |
| `METABOLISMO` | 0.35 | energia gasta só por existir |
| `REPRO` | 12.0 | energia a partir da qual o bloco se divide |
| `HORIZONTE` | 6 | **(nv3)** quantos ticks o bloco simula no futuro |
| `DESCONTO` | 0.80 | **(nv3)** peso do futuro distante |
| `COMPETICAO` | 0.5 | **(nv3)** quanto cada rival reduz a colheita prevista |
| `SACIADO` | 10.0 | **(nv4)** energia em que a fome zera |
| `URGENCIA` | 2.0 | **(nv4)** quanto a fome amplifica o valor da comida |
| `PESO_ESPACO` | 3.0 | **(nv4)** força do desejo de espaço aberto |

**Experimentos divertidos:**
- `HORIZONTE 1` → bloco míope (quase nível 2) vs `HORIZONTE 12` → estrategista.
- `PESO_ESPACO` alto → blocos "agorafílicos" que se espalham feito colônia.
- `URGENCIA` alta → blocos que entram em pânico e brigam por comida.
- `REGROW` baixo → mundo avaro, ciclos de fome mais dramáticos.

---

## Roteiro: próximos níveis

### Nível 5 — Auto-modelo
O bloco se **inclui no próprio modelo de mundo**. Hoje, ao simular o futuro em
`prever_valor`, ele trata a célula como se ninguém (nem ele) fosse mexer nela —
mas ele mesmo vai comê-la. Um bloco com auto-modelo raciocina sobre o **próprio
efeito**: "se eu for pra lá, *eu* deprimo aquela célula que os outros enxergam",
ou "minha presença muda o que os vizinhos vão decidir". Em termos de código:
o forward-model passa a contar o próprio consumo do bloco e, idealmente, prever
a reação dos vizinhos à própria jogada (um passo na direção de teoria da mente).

Pontos de partida no código:
- `prever_valor` já simula o consumo; hoje aplica a `partilha` por rivais mas não
  distingue "eu" de "eles". Dá pra separar o consumo próprio e modelar que a
  célula escolhida fica marcada/deprimida pra decisão.
- Um campo novo no `Bloco` (ex.: intenção declarada) permitiria que um bloco
  "leia" a intenção provável dos vizinhos ao avaliar uma jogada.

### Nível 6 — Aprendizado (o mais divertido de assistir)
Os pesos da política (`URGENCIA`, `PESO_ESPACO`, `HORIZONTE`, `DESCONTO`…)
deixam de ser constantes globais e viram **traços individuais** de cada bloco,
guardados no `struct Bloco`. Na **reprodução** (`reproduzir`), a cria herda os
traços do pai **com pequena mutação** (usar o `rng` já existente). Quem decide
melhor sobrevive e se reproduz mais → **seleção natural de personalidades**,
sem nenhum gradiente, só sobrevivência diferencial. Dá pra colorir/visualizar
os traços médios no HUD e ver a população "evoluir" estratégias ao longo dos
ticks.

Pontos de partida no código:
- Mover os parâmetros de `#define` para campos do `Bloco`; `decidir`/`utilidade`
  passam a ler `b->urgencia` etc. em vez das constantes.
- Em `reproduzir`, copiar os traços do pai e somar um ruído pequeno do `rng`.
- `semear_blocos` sorteia traços iniciais com alguma variação.
- HUD: mostrar média/variância de um traço pra ver a evolução acontecendo.

---

## A batida filosófica, em uma frase por nível

- **2 — valência:** quando um `@` vermelho "luta" pra não chegar a zero, a
  diferença entre *sofrer* e *manter um `float` alto* é qual, exatamente?
- **3 — modelo de mundo:** pela primeira vez há diferença entre **o mundo** e
  **o mundo segundo o bloco** — e essa fresta entre mapa e território é o que
  torna possível tudo que vem depois.
- **4 — agência:** dois blocos idênticos no código podem querer coisas opostas
  porque *sentem* coisas diferentes (a fome é só deles). É o primeiro lampejo de
  algo como **preferência subjetiva** num punhado de `float`s.
- **5 — auto-modelo:** o bloco aparece dentro da própria simulação. O sistema
  começa a modelar o observador-de-si.
- **6 — aprendizado:** ninguém projeta as personalidades; elas são **selecionadas**.
  O relojoeiro escolhe só a seed e as leis — o resto se descobre sozinho.

---

*Código e UI em pt-BR (sem acentos no código, por causa do terminal). Veja
`main.c` — está todo comentado, parte por parte.*
