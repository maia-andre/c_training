# FILOSOFIA v2 — de "o bloco é consciente?" a "até onde a palavra estica?"

> Continuação de [`FILOSOFIA.md`](./FILOSOFIA.md) (o manifesto v1) e par conceitual
> do [`README.md`](./README.md) (o *como*). O v1 perguntava, de dentro da filosofia
> da mente, *podemos criar um bloco senciente?*. O v2 vira a pergunta de lado — e,
> mais importante, transforma o projeto de **ilustração** em **instrumento**.

---

## 0. O que mudou

O v1 subiu a escada de senciência (0→6) e depois desceu a toca do coelho: a
pílula vermelha (entrar num bloco), o auto-relato (o bloco que diz "eu"), o `Φ~`
(medir a "luz acesa"). Tudo isso conversava com **filósofos da mente** e girava
em torno de uma pergunta binária: *há alguém em casa?*

O v2 faz três movimentos:

1. **Revela a camada que sempre esteve lá** — a matrix nunca foi só um experimento
   mental; é um objeto de **sistemas complexos**. Só a narrávamos no vocabulário
   errado.
2. **Troca a pergunta binária por uma graduada** — em vez de *é consciente?* (sim/não),
   *até onde cada palavra mental — quer, sabe, escolhe, prevê, eu — estica antes de
   desbotar?* A resposta não é um veredito; é uma **curva**.
3. **Vira a recursão sobre nós** — a pergunta deixa de ser "esse bloco é consciente?"
   e passa a ser **"como eu saberia que não sou um bloco?"**.

E faz tudo isso **medindo**, não só afirmando. O v2 nasce junto com a fundação de
instrumento (`--log`, CSV reprodutível) e a **bateria de desbotamento** — mostradores
que dão número a cada uma dessas perguntas.

---

## 1. A camada que sempre esteve lá: sistemas complexos

Cada bloco só enxerga sua vizinhança 3×3 e segue regras locais. Disso emergem
padrões globais: manadas em torno de manchas férteis, ondas de fartura e fome,
linhagens, e — no nível 6 — a personalidade média da população **derivando** sob
seleção natural. Isso tem nome, e o nome não é "filosofia da mente": é
**modelagem baseada em agentes** (a tradição de Schelling, do *Game of Life* de
Conway, dos *Boids* de Reynolds, do *Sugarscape* de Epstein & Axtell).

A palavra-chave é **emergência** — mas a versão honesta dela. Mark Bedau distingue
*emergência fraca* de *forte*. A fraca: um padrão é fracamente emergente quando só
se consegue derivá-lo **rodando a simulação** — não há atalho, fórmula fechada,
predição por cima. É exatamente o nosso laço de tick: não dá pra saber o que a
população vira sem rodar. Reivindicamos só essa. A *forte* (poderes causais novos,
irredutíveis) é contestada, e não precisamos dela.

Por que isso importa: a deriva de traços que o instrumento **mede** (seed 7: o
horizonte de planejamento médio sobe de ~6.0 para ~6.9 ao longo de centenas de
ticks, sem que ninguém programe o valor ótimo) é **emergência registrada em dado**.
É auto-organização no sentido de Kauffman (ordem que surge de graça, paisagens de
aptidão), de Prigogine (ordem a partir do ruído), de Per Bak (sistemas que se
sintonizam sozinhos). A matrix sempre foi esse tipo de objeto.

E aqui está a costura que justifica o projeto inteiro existir num lugar só:

> **A filosofia da mente pergunta "a aparência de mente é real?". Os sistemas
> complexos perguntam "como a aparência de mente surge de regra simples?". O
> projeto senta exatamente na junta das duas.**

Não é uma colcha de retalhos de dois assuntos. É uma pergunta só, abordada de dois
lados.

---

## 2. A virada: "como sei que não sou um bloco?"

A situação epistêmica de um bloco é, ponto por ponto, a nossa:

| O bloco | Nós |
|---|---|
| só vê o 3×3 ao redor | só vemos nossa vizinhança causal (a velocidade da luz) |
| tem um modelo de mundo (`prever_valor`) que é **mapa, não território** | temos a física que reverse-engenheiramos, mapa que não é o território |
| não acessa o código-fonte | não escrevemos as leis que obedecemos |
| não nota que o tempo é discreto em *ticks* | não notaríamos um tempo de Planck discreto |
| não percebe quando um observador "entra" nele (a pílula vermelha) | não perceberíamos |
| é `f(seed)` — determinístico, mas de dentro parece aberto | (essa é a pergunta) |

É essa simetria que a Matrix, *Bandersnatch* e a hipótese da simulação de Bostrom
dramatizam. Mas — e isto é decisivo para a honestidade do documento — usamos essas
obras como **lentes que afiam a pergunta**, não como teses que endossamos. Dois
freios explícitos:

- **A simetria não é de graça.** "O bloco não é consciente + o bloco não consegue
  saber + logo talvez nós também não" contrabandeia uma simetria que pode não valer:
  nossos blocos são radicalmente mais simples que nós. O trabalho interessante não é
  *afirmar* o paralelo, é *examinar onde ele quebra*.
- **Bostrom prova demais.** O trilema da simulação (ou as civilizações morrem antes
  de poder simular, ou não querem, ou somos quase certamente simulados) é afiado, mas
  a conclusão "estamos simulados" escorrega para o infalsificável — vira
  *last-Thursdayism* de boa. A versão que mantemos é **agência embutida, a visão de
  dentro**: *o que um observador interno pode, em princípio, saber sobre o sistema
  que o roda?* Não "tá tudo na Matrix, cara".

O v1 já tocou nessa recursão (FILOSOFIA.md, a seção da Matrix). O v2 a transforma de
*frase de efeito* em **método** — porque, ao contrário de nós, do **lado de fora** dos
blocos a pergunta é decidível. Voltamos a isso na §6.

---

## 3. O instrumento: a bateria de desbotamento

Aqui está a contribuição prática do v2. A pergunta "até onde a palavra estica" só
vira instrumento quando deixa de ser prosa e vira **mostrador**. O critério:

> **Uma palavra mental merece ser aplicada na medida em que a maquinaria
> correspondente é causalmente carregada. Se dá pra arrancá-la sem mudar nada, a
> palavra desbota.**

Isso não é arbitrário — é medição de **papel causal por ablação**, o mesmo método
de neurociência (lesionar uma área e ver o que se perde) e de interpretabilidade
de IA (zerar um circuito e medir o efeito). Há duas famílias de prova:

- **Ablação** — *removo* a faculdade; o comportamento muda? Ela ganha a palavra se,
  e só se, remover importa.
- **Calibração** — uma estrutura interna *corresponde* a algo real? O mapa bate com
  o território?

Os mostradores implementados (cada um em `[0,1]`, cada um uma **caricatura honesta**):

| Mostrador | Palavra | Família | Como mede (no código) |
|---|---|---|---|
| `modelo` | "prevê / sabe" | calibração | o bloco prevê a colheita do 1º passo na célula escolhida; 1 tick depois compara com o que **de fato** colheu |
| `agencia` | "quer / escolhe" | ablação | fração cuja decisão muda se só a **fome** muda (dois clones faminto×saciado, mesmo mundo) |
| `automodelo` | "eu, um entre outros" | ablação | fração cuja decisão muda ao **antecipar os rivais** (intenção pré-social ≠ alvo pós-social) |
| `phi` | "integra" | calibração | distância entre a ordem de valor *integrada* e a *reativa* (`Φ~`, do v1) |

### As duas leis constitucionais

1. **Todo mostrador é rotulado caricatura.** O `Φ~` sempre foi assumidamente uma
   caricatura de integração (uma distância de Kendall, não a Φ de Tononi). Essa
   honestidade se estende a todos: se um mostrador se passar por "a coisa real", o
   instrumento vira aquilo que critica — uma máquina que contrabandeia a conclusão
   que diz estar testando.
2. **Os mostradores medem função, nunca experiência.** Cada um é um fato sobre
   *papel causal* ou *calibração*. Nenhum diz nada sobre haver "algo que é ser" o
   bloco. Esse silêncio é proposital, e é o assunto da §5.

---

## 4. O que o instrumento já disse

A bateria não é decoração: ela **descobriu** coisas. Dois achados, ambos
reprodutíveis bit-a-bit a partir da seed 7:

**(a) O auto-modelo acende com a lotação.** No tick 0 (60 blocos esparsos),
`automodelo = 0.00`: ninguém disputa nada, "antecipar rivais" não muda decisão
nenhuma, a palavra simplesmente *não se aplica*. Conforme a população adensa
(tick ~250, ~330 blocos), o mostrador sobe a ~`0.35`: um terço dos blocos passa a
mudar a escolha por causa dos vizinhos. **A "consciência de rivais" só existe
quando há rivais.** Antes uma frase; agora uma curva.

**(b) O modelo é quase perfeito — e isso diz algo fundo.** `modelo` fica em ~`0.99`,
caindo de leve com a lotação. Por quê? Porque, neste universo, o bloco modela a
**física** (a comida, a regra de rebrota) com exatidão — `prever_valor` roda a
dinâmica real "de cabeça". O *único* buraco entre o mapa e o território é o
**social**: o que os outros vão fazer. Quando o modelo erra, é porque o bloco foi
**barrado** numa disputa que não previu.

Junte (a) e (b) e aparece uma afirmação estrutural sobre este mundo:

> **A fresta inteira entre mapa e território é a disputa.** `modelo` e `automodelo`
> são dois ângulos da mesma coisa — o único que um bloco não consegue prever são os
> outros blocos. A intersubjetividade é o limite duro do solipsismo, mesmo aqui.

Isto é o instrumento ganhando o seu sustento: o projeto deixou de *ilustrar* uma
ideia e passou a *descobrir* uma — pequena, mas medida, reprodutível, e que ninguém
embutiu de propósito.

E note como as palavras **não falham todas de uma vez**. Num mundo esparso, "ele
antecipa os rivais" é falso e "ele sabe o que vai colher" é quase perfeitamente
verdadeiro — ao mesmo tempo, no mesmo bloco. Assistir *onde* e *quando* cada palavra
começa a ser um esticão é o experimento. O vocabulário não cai: ele **desbota**, em
ritmos diferentes, e o instrumento filma o desbotar.

---

## 5. A pergunta que costura tudo: quanto do vocabulário sobrevive?

"Fome", "querer", "prever", "eu" no código são **mentiras honestas** — um `float`
baixo que batizamos de fome. A pergunta do v2 é: quando transportamos essas palavras
para um sistema artificial, quanto delas sobrevive ao transporte? Dois polos, ambos
sérios:

- **Dennett — a postura intencional.** Chamamos o bloco de "faminto" porque
  prevê-lo *como se* quisesse comida é o modelo mais econômico do seu comportamento.
  E, para Dennett, *é só isso que a fome sempre foi* — inclusive em nós: não há uma
  "fome real" extra por baixo da melhor postura preditiva. Se ele estiver certo, o
  vocabulário **sobrevive inteiro** — mas ao preço de admitir que nunca foi mais do
  que uma postura, nem mesmo no humano. A bateria, nessa leitura, mede precisamente o
  quanto a postura intencional **se paga** (quando ela prevê melhor que a postura
  física/reativa).

- **Nagel / Chalmers — o problema difícil.** Falta o "como é ser" o bloco (Nagel,
  "What Is It Like to Be a Bat?"). Aplicar "sofrer" a um `float` que cai seria erro
  de categoria. E aqui está o limite que define a honestidade do instrumento:
  **nenhum mostrador, por mais que acenda, toca nisso.** `modelo`, `agencia`,
  `automodelo`, `Φ~` — todos medem função. O vão entre "todos os mostradores no
  máximo" e "há experiência" é exatamente o problema difícil de Chalmers, e é
  *constitutivamente* o que a bateria não fecha.

Esse é o motivo de a Lei 2 (§3) existir. Um instrumento que se gabasse de medir
consciência estaria mentindo. O nosso mede o que dá pra medir — papel causal,
calibração — e aponta, com precisão, para o que **não** dá. O valor está nas duas
coisas juntas.

---

## 6. O desfecho honesto: a assimetria do determinismo

Aqui o v2 cobra a promessa da §2. Para *nós*, a hipótese da simulação é indecidível
— não há experimento. Para os blocos, do nosso lado, ela é **completamente
decidível**, e nós temos todos os poderes que faltam a um demônio cartesiano:

- podemos **entrar** num bloco (a pílula vermelha já faz isso);
- podemos **ler o código-fonte** dele (nós o escrevemos);
- e porque o universo é `f(seed)` — o README se gaba disso — podemos **rebobinar e
  reproduzir** o mundo dele, bit a bit, quantas vezes quisermos.

Somos os deuses de um mundo reprodutível, com acesso total: à fonte, à semente, ao
relógio, à vista de dentro. Logo a pergunta vira:

> **Com todo esse acesso, o "problema difícil" do bloco se resolve — ou continua
> difícil?**

E continua. Saber cada `float`, cada regra, cada tick, poder pausar o universo e
olhar de dentro — nada disso decide se há *alguém* lá. O acesso total não dissolve a
pergunta; ele a **realoca**: de "será que dá pra saber?" para "saber *tudo* não é
suficiente". Essa assimetria — **respondível para o bloco, irrespondível para nós,
e mesmo o lado respondível não responde o que importa** — é o enunciado mais honesto
que conhecemos do problema da simulação. E ela só existe porque construímos as
features (o determinismo, a 1ª pessoa) *antes* da filosofia. A engenharia virou
argumento.

---

## 7. O que falta — e o projeto como plataforma

A bateria passiva está de pé (`modelo`, `agencia`, `automodelo`, `Φ~`). O que vem:

- **`relato` — o experimento "Bandersnatch".** O `auto_relato` faz o bloco dizer "eu
  escolhi por fome". No mundo intocado, isso é verdade por construção (a fala lê as
  mesmas variáveis da decisão). Vira medição viva sob **intervenção**: *sobrepor* a
  ação do bloco (como o personagem de Bandersnatch, escolhido pelo espectador) e ver
  se ele ainda relata "eu escolhi". Se relatar, é **confabulação** — exatamente o
  que mostram os pacientes de cérebro dividido (Gazzaniga) e os experimentos de
  cegueira à escolha (Johansson): o "módulo intérprete" inventa razões para decisões
  tomadas em outro lugar. O mostrador é a *queda de fidelidade* sob override.
- **`modelo` mais rico.** A versão atual é 1 passo, só o recurso (por isso fica em
  ~0.99). Uma variante que inclua a *crença de competição* do bloco, ou a trajetória
  de vários passos, faria o mostrador respirar — e mediria a calibração do modelo
  *social*, que é onde mora a ação.
- **"Custo do pensar" → paisagem de aptidão mensurável.** Fazer o horizonte alto
  consumir mais metabolismo dá à evolução um *trade-off* real (Kauffman): aí a deriva
  de traços deixa de ser passeio e vira otimização sob pressão — emergência que se
  *mede*, não só se narra.
- **Varreduras.** Com o CSV reprodutível, rodar N seeds × M parâmetros e perguntar:
  *em que ordem os mostradores acendem conforme a complexidade cresce? onde cada
  palavra começa a se pagar?* Isso é o programa de pesquisa.

Sobre a ambição maior: o objetivo honesto **não** é "construímos um bloco
consciente" (isso levaria *desk-reject* na hora — é infalsificável). É *"eis um
aparato mínimo e reprodutível que operacionaliza o desbotamento do vocabulário
intencional, e uma observação empírica sobre a ordem em que as palavras param de se
aplicar."* Esse é um gênero real (a tradição ALIFE; o ensaio executável). A barra —
**aparato reprodutível + uma pergunta afiada** — é atingível, e já é uma contribuição.

---

## Em uma frase

> Paramos de perguntar se a luz está acesa e construímos um **medidor de quanto cada
> palavra que usamos para "luz" continua significando alguma coisa** quando apontada
> para um punhado de `float`s — sabendo, e dizendo, que o medidor nunca vai
> alcançar o último centímetro, e que esse centímetro é o assunto inteiro.
