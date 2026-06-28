# Makefile do monorepo c_training
#
# Detecta automaticamente cada jogo em games/<nome>/ e compila para bin/<nome>.
# Cada jogo é compilado a partir de TODOS os .c dentro da sua pasta.
#
# Uso:
#   make              compila todos os jogos
#   make <nome>       compila um jogo específico (ex: make snake)
#   make list         lista os jogos detectados
#   make clean        apaga os binários

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
BIN     := bin

# Lista os nomes dos jogos: cada subpasta de games/
GAMES := $(notdir $(wildcard games/*))

# Alvo padrão: compila todos os jogos
.PHONY: all
all: $(GAMES)

# "make <nome>" compila games/<nome>/*.c -> bin/<nome>.
# Recompila sempre (os jogos são pequenos), mantendo o Makefile simples.
.PHONY: $(GAMES)
$(GAMES): | $(BIN)
	$(CC) $(CFLAGS) games/$@/*.c -o $(BIN)/$@

# Cria a pasta bin/ se não existir
$(BIN):
	mkdir -p $(BIN)

.PHONY: clean
clean:
	rm -rf $(BIN)

.PHONY: list
list:
	@echo "Jogos disponíveis:"
	@for g in $(GAMES); do echo "  - $$g"; done
