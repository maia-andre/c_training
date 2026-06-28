# Makefile do monorepo c_training
#
# Jogos de terminal: detectados automaticamente em games/<nome>/ -> bin/<nome>.
#   Cada jogo é compilado a partir de TODOS os .c dentro da sua pasta.
# App gráfica (plotter): alvo separado, pois depende da raylib (instalada em
#   ~/.local — ver README.md / CLAUDE.md). Por isso NÃO entra no "make all".
#
# Uso:
#   make              compila todos os jogos de terminal -> bin/
#   make <nome>       compila um jogo específico (ex: make jogo_da_velha)
#   make plotter      compila o plotter gráfico (precisa da raylib) -> bin/plotter
#   make list         lista os jogos e apps detectados
#   make clean        apaga todos os binários (bin/)

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
BIN     := bin

# raylib instalada por usuário em ~/.local (ver "Rebuilding raylib" no CLAUDE.md)
RAYLIB_PREFIX := $(HOME)/.local
PLOTTER_LDFLAGS := -I$(RAYLIB_PREFIX)/include -L$(RAYLIB_PREFIX)/lib \
                   -Wl,-rpath,$(RAYLIB_PREFIX)/lib -lraylib -lm

# Jogos = cada subpasta de games/
GAMES := $(notdir $(wildcard games/*))

# Alvo padrão: compila todos os jogos de terminal (o plotter fica de fora de
# propósito, pra que "make" funcione mesmo sem a raylib instalada).
.PHONY: all
all: $(GAMES)

# "make <nome>" compila games/<nome>/*.c -> bin/<nome>
.PHONY: $(GAMES)
$(GAMES): | $(BIN)
	$(CC) $(CFLAGS) games/$@/*.c -o $(BIN)/$@

# App gráfica com raylib (alvo separado; não entra no "all")
.PHONY: plotter
plotter: | $(BIN)
	$(CC) $(CFLAGS) apps/plotter/plotter.c -o $(BIN)/plotter $(PLOTTER_LDFLAGS)

# Cria a pasta bin/ se não existir
$(BIN):
	mkdir -p $(BIN)

.PHONY: clean
clean:
	rm -rf $(BIN)

.PHONY: list
list:
	@echo "Jogos de terminal (games/):"
	@for g in $(GAMES); do echo "  - $$g"; done
	@echo "Apps gráficas (apps/):"
	@echo "  - plotter  (raylib; use: make plotter)"
