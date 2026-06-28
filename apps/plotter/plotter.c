// ============================================================================
//  plotter.c  —  Plotador de funcoes interativo (mini-Desmos) com raylib
//
//  Digite uma formula em f(x), ajuste os parametros a/b/c nos sliders e veja
//  a curva se redesenhar ao vivo. Arraste pra mover, scroll pra dar zoom.
//
//  Formula aceita:
//    - variavel:   x
//    - parametros: a, b, c   (controlados pelos sliders)
//    - operadores: + - * / ^   e parenteses ( )
//    - funcoes:    sin cos tan asin acos atan exp ln log log10 sqrt abs floor
//    - constantes: pi  e
//    (multiplicacao precisa ser explicita: use 2*x, nao 2x)
//
//  Compilar no Debian:
//    cc plotter.c -o plotter $(pkg-config --cflags --libs raylib) -lm
//    # se nao tiver pkg-config:  cc plotter.c -o plotter -lraylib -lm
// ============================================================================

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
//  PARTE 1 — Parser de expressao (descida recursiva -> arvore / AST)
//
//  A ideia: em vez de re-interpretar o texto "sin(a*x)" a cada pixel, fazemos
//  o parse UMA vez, montando uma arvore de nos. Depois avaliamos a arvore
//  (rapido) pra cada valor de x. Reparse so quando a formula muda.
// ----------------------------------------------------------------------------

typedef enum {
    N_NUM, N_VAR, N_NEG, N_ADD, N_SUB, N_MUL, N_DIV, N_POW, N_FUNC
} NodeType;

// ids das funcoes matematicas suportadas
enum { F_SIN, F_COS, F_TAN, F_ASIN, F_ACOS, F_ATAN,
       F_EXP, F_LOG, F_LOG10, F_SQRT, F_ABS, F_FLOOR };

typedef struct Node {
    NodeType t;
    double   num;     // usado em N_NUM
    char     var;     // usado em N_VAR: 'x','a','b','c'
    int      fn;      // usado em N_FUNC: um dos F_*
    struct Node *l, *r;
} Node;

static const char *P;        // cursor de leitura sobre a string da formula
static bool        parseOK;  // vira false se encontrar algo invalido

static Node *mk(NodeType t){ Node *n = calloc(1, sizeof(Node)); n->t = t; return n; }
static Node *mkNum(double v){ Node *n = mk(N_NUM); n->num = v; return n; }
static Node *mkVar(char v){ Node *n = mk(N_VAR); n->var = v; return n; }
static Node *mkBin(NodeType t, Node *l, Node *r){ Node *n = mk(t); n->l = l; n->r = r; return n; }

static void freeNode(Node *n){ if(!n) return; freeNode(n->l); freeNode(n->r); free(n); }

static void skipws(void){ while(*P==' ' || *P=='\t') P++; }

static int fnId(const char *s){
    if(!strcmp(s,"sin"))   return F_SIN;
    if(!strcmp(s,"cos"))   return F_COS;
    if(!strcmp(s,"tan"))   return F_TAN;
    if(!strcmp(s,"asin"))  return F_ASIN;
    if(!strcmp(s,"acos"))  return F_ACOS;
    if(!strcmp(s,"atan"))  return F_ATAN;
    if(!strcmp(s,"exp"))   return F_EXP;
    if(!strcmp(s,"ln") || !strcmp(s,"log")) return F_LOG;   // log = log natural aqui
    if(!strcmp(s,"log10")) return F_LOG10;
    if(!strcmp(s,"sqrt"))  return F_SQRT;
    if(!strcmp(s,"abs"))   return F_ABS;
    if(!strcmp(s,"floor")) return F_FLOOR;
    return -1;
}

// declaracoes adiantadas (a gramatica eh recursiva)
static Node *parseExpr(void);
static Node *parseTerm(void);
static Node *parseUnary(void);
static Node *parsePower(void);
static Node *parsePrimary(void);

// primary := numero | variavel | constante | funcao(expr) | ( expr )
static Node *parsePrimary(void){
    skipws();
    char c = *P;

    if(c == '('){
        P++;
        Node *e = parseExpr();
        skipws();
        if(*P == ')') P++; else parseOK = false;
        return e;
    }

    if(isdigit((unsigned char)c) || c == '.'){
        char *end;
        double v = strtod(P, &end);
        if(end == P){ parseOK = false; return mkNum(0); }
        P = end;
        return mkNum(v);
    }

    if(isalpha((unsigned char)c)){
        char id[16]; int n = 0;
        while(isalpha((unsigned char)*P) && n < 15) id[n++] = *P++;
        id[n] = '\0';
        skipws();

        if(*P == '('){                       // eh chamada de funcao
            P++;
            Node *arg = parseExpr();
            skipws();
            if(*P == ')') P++; else parseOK = false;
            Node *f = mk(N_FUNC); f->l = arg; f->fn = fnId(id);
            if(f->fn < 0) parseOK = false;
            return f;
        }
        if(!strcmp(id,"pi")) return mkNum(3.14159265358979323846);
        if(!strcmp(id,"e"))  return mkNum(2.71828182845904523536);
        if(n == 1 && (id[0]=='x'||id[0]=='a'||id[0]=='b'||id[0]=='c'))
            return mkVar(id[0]);

        parseOK = false; return mkNum(0);    // identificador desconhecido
    }

    parseOK = false; return mkNum(0);
}

// power := primary ( '^' unary )?    -> '^' eh associativo a direita
static Node *parsePower(void){
    Node *base = parsePrimary();
    skipws();
    if(*P == '^'){ P++; return mkBin(N_POW, base, parseUnary()); }
    return base;
}

// unary := ('-' | '+')? power
static Node *parseUnary(void){
    skipws();
    if(*P == '-'){ P++; Node *n = mk(N_NEG); n->l = parseUnary(); return n; }
    if(*P == '+'){ P++; return parseUnary(); }
    return parsePower();
}

// term := unary ( ('*' | '/') unary )*
static Node *parseTerm(void){
    Node *n = parseUnary();
    for(;;){
        skipws();
        if(*P == '*'){ P++; n = mkBin(N_MUL, n, parseUnary()); }
        else if(*P == '/'){ P++; n = mkBin(N_DIV, n, parseUnary()); }
        else break;
    }
    return n;
}

// expr := term ( ('+' | '-') term )*
static Node *parseExpr(void){
    Node *n = parseTerm();
    for(;;){
        skipws();
        if(*P == '+'){ P++; n = mkBin(N_ADD, n, parseTerm()); }
        else if(*P == '-'){ P++; n = mkBin(N_SUB, n, parseTerm()); }
        else break;
    }
    return n;
}

static Node *parseFormula(const char *s){
    P = s; parseOK = true;
    Node *n = parseExpr();
    skipws();
    if(*P != '\0') parseOK = false;   // sobrou lixo no fim
    return n;
}

// avalia a arvore para um x (e os parametros a,b,c atuais)
static double ev(Node *n, double x, double a, double b, double c){
    if(!n) return NAN;
    switch(n->t){
        case N_NUM: return n->num;
        case N_VAR: return n->var=='x'?x : n->var=='a'?a : n->var=='b'?b : c;
        case N_NEG: return -ev(n->l,x,a,b,c);
        case N_ADD: return ev(n->l,x,a,b,c) + ev(n->r,x,a,b,c);
        case N_SUB: return ev(n->l,x,a,b,c) - ev(n->r,x,a,b,c);
        case N_MUL: return ev(n->l,x,a,b,c) * ev(n->r,x,a,b,c);
        case N_DIV: return ev(n->l,x,a,b,c) / ev(n->r,x,a,b,c);
        case N_POW: return pow(ev(n->l,x,a,b,c), ev(n->r,x,a,b,c));
        case N_FUNC: {
            double v = ev(n->l,x,a,b,c);
            switch(n->fn){
                case F_SIN:  return sin(v);   case F_COS:  return cos(v);
                case F_TAN:  return tan(v);   case F_ASIN: return asin(v);
                case F_ACOS: return acos(v);  case F_ATAN: return atan(v);
                case F_EXP:  return exp(v);   case F_LOG:  return log(v);
                case F_LOG10:return log10(v); case F_SQRT: return sqrt(v);
                case F_ABS:  return fabs(v);  case F_FLOOR:return floor(v);
            }
        }
    }
    return NAN;
}

// ----------------------------------------------------------------------------
//  PARTE 2 — Camera / transformacao mundo <-> tela
//
//  gCx,gCy = coordenada de mundo no centro da area de plot
//  gScale  = pixels por unidade de mundo
//  gPx..gPh = retangulo da area de plot (abaixo do cabecalho)
// ----------------------------------------------------------------------------

static double gCx = 0, gCy = 0, gScale = 60;
static float  gPx, gPy, gPw, gPh;

static double SX(double wx){ return (gPx + gPw/2.0) + (wx - gCx)*gScale; } // mundo->tela x
static double SY(double wy){ return (gPy + gPh/2.0) - (wy - gCy)*gScale; } // mundo->tela y (invertido)
static double WX(double sx){ return gCx + (sx - (gPx + gPw/2.0))/gScale; } // tela->mundo x
static double WY(double sy){ return gCy - (sy - (gPy + gPh/2.0))/gScale; } // tela->mundo y

// escolhe um "passo bonito" (1,2,5 x 10^k) proximo de um valor bruto
static double niceStep(double rough){
    double e = floor(log10(rough));
    double f = rough / pow(10, e);
    double nf = (f < 1.5) ? 1 : (f < 3) ? 2 : (f < 7) ? 5 : 10;
    return nf * pow(10, e);
}

static float clampf(float v, float lo, float hi){ return v<lo?lo : v>hi?hi : v; }

// ----------------------------------------------------------------------------
//  PARTE 3 — Slider em modo imediato
// ----------------------------------------------------------------------------

static int activeSlider = -1;   // qual slider esta sendo arrastado (-1 = nenhum)

static float slider(int id, float x, float y, float w,
                    float val, float lo, float hi, const char *label){
    float knobR = 8.0f;
    float t = clampf((val - lo) / (hi - lo), 0, 1);
    float kx = x + t * w;
    Vector2 m = GetMousePosition();

    bool over = (m.x >= x - knobR && m.x <= x + w + knobR && fabsf(m.y - y) <= knobR + 4);
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && over) activeSlider = id;
    if(activeSlider == id){
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
            float tt = clampf((m.x - x) / w, 0, 1);
            val = lo + tt * (hi - lo);
            kx  = x + tt * w;
        } else activeSlider = -1;
    }

    DrawLineEx((Vector2){x, y}, (Vector2){x + w, y}, 3, LIGHTGRAY);
    DrawLineEx((Vector2){x, y}, (Vector2){kx, y}, 3, (Color){90,140,230,255});
    DrawCircleV((Vector2){kx, y}, knobR, (Color){55,110,210,255});
    DrawText(TextFormat("%s = %.2f", label, val), (int)x, (int)(y - 24), 18, (Color){70,70,80,255});
    return val;
}

// ----------------------------------------------------------------------------
//  PARTE 4 — Programa principal
// ----------------------------------------------------------------------------

int main(void){
    const Color BG     = (Color){250,250,252,255};
    const Color PANEL  = (Color){244,245,248,255};
    const Color GRIDLN = (Color){233,235,240,255};
    const Color AXIS   = (Color){150,155,165,255};
    const Color CURVE  = (Color){205,45,70,255};
    const Color INK    = (Color){45,48,58,255};

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 640, "Plotador de funcoes — raylib");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   // nao fechar no Esc (atrapalha digitar); use o botao da janela

    char formula[256] = "a * sin(b * x) + c";
    float a = 1.0f, b = 2.0f, c = 0.0f;
    Node *ast = parseFormula(formula);
    bool ok = parseOK;

    const float HEADER = 132.0f;

    while(!WindowShouldClose()){
        int W = GetScreenWidth();
        int H = GetScreenHeight();

        // area de plot = tudo abaixo do cabecalho
        gPx = 0; gPy = HEADER; gPw = (float)W; gPh = (float)H - HEADER;

        // ---- entrada de texto na formula (sempre ativa) ----
        bool changed = false;
        int key = GetCharPressed();
        while(key > 0){
            if(key >= 32 && key <= 125 && strlen(formula) < 255){
                int l = strlen(formula);
                formula[l] = (char)key; formula[l+1] = '\0';
                changed = true;
            }
            key = GetCharPressed();
        }
        if((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))){
            int l = strlen(formula);
            if(l > 0){ formula[l-1] = '\0'; changed = true; }
        }
        if(IsKeyPressed(KEY_ENTER)) changed = true;
        if(changed){ freeNode(ast); ast = parseFormula(formula); ok = parseOK; }

        if(IsKeyPressed(KEY_R)){ gCx = 0; gCy = 0; gScale = 60; }   // resetar view

        // ---- pan + zoom (apenas dentro do plot e se nenhum slider ativo) ----
        Vector2 m = GetMousePosition();
        bool inPlot = (m.y >= gPy && m.x >= gPx && m.x <= gPx + gPw && m.y <= gPy + gPh);
        if(inPlot && activeSlider < 0){
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                Vector2 d = GetMouseDelta();
                gCx -= d.x / gScale;
                gCy += d.y / gScale;
            }
            float wheel = GetMouseWheelMove();
            if(wheel != 0){
                double wxB = WX(m.x), wyB = WY(m.y);          // ponto sob o cursor (antes)
                gScale *= (wheel > 0) ? 1.1 : 1.0/1.1;
                gScale = (gScale < 2) ? 2 : (gScale > 8000) ? 8000 : gScale;
                double wxA = WX(m.x), wyA = WY(m.y);          // (depois)
                gCx += wxB - wxA;                              // manter ponto fixo no cursor
                gCy += wyB - wyA;
            }
        }

        // ====================== DESENHO ======================
        BeginDrawing();
        ClearBackground(BG);

        // ---- grade ----
        double left = WX(gPx), right = WX(gPx + gPw);
        double bot  = WY(gPy + gPh), top = WY(gPy);
        double step = niceStep(80.0 / gScale);

        float axisX = (float)clampf((float)SX(0), gPx + 4, gPx + gPw - 34); // onde ancorar labels de y
        float axisY = (float)clampf((float)SY(0), gPy + 2, gPy + gPh - 14); // onde ancorar labels de x

        for(double gx = ceil(left/step)*step; gx <= right; gx += step){
            float X = (float)SX(gx);
            bool zero = fabs(gx) < step*0.5;
            DrawLineEx((Vector2){X, gPy}, (Vector2){X, gPy + gPh}, zero?1.5f:1.0f, zero?AXIS:GRIDLN);
            if(!zero) DrawText(TextFormat("%g", gx), (int)X + 3, (int)axisY + 2, 10, AXIS);
        }
        for(double gy = ceil(bot/step)*step; gy <= top; gy += step){
            float Y = (float)SY(gy);
            bool zero = fabs(gy) < step*0.5;
            DrawLineEx((Vector2){gPx, Y}, (Vector2){gPx + gPw, Y}, zero?1.5f:1.0f, zero?AXIS:GRIDLN);
            if(!zero) DrawText(TextFormat("%g", gy), (int)axisX + 4, (int)Y - 12, 10, AXIS);
        }

        // ---- curva ----
        if(ast && ok){
            bool pen = false;
            Vector2 prev = {0,0};
            for(int sxp = (int)gPx; sxp <= (int)(gPx + gPw); sxp++){
                double wy = ev(ast, WX((double)sxp), a, b, c);
                if(isfinite(wy)){
                    float Y = (float)SY(wy);
                    Vector2 cur = {(float)sxp, Y};
                    // nao conectar atravessando assintotas (salto gigante = descontinuidade)
                    bool jump = pen && fabsf(Y - prev.y) > gPh * 3.0f;
                    if(pen && !jump) DrawLineEx(prev, cur, 2.0f, CURVE);
                    prev = cur; pen = true;
                } else {
                    pen = false;   // levanta a "caneta" em NaN/inf
                }
            }
        }

        // ---- cabecalho ----
        DrawRectangle(0, 0, W, (int)HEADER, PANEL);
        DrawLine(0, (int)HEADER, W, (int)HEADER, (Color){225,228,234,255});

        DrawText("f(x) =", 16, 22, 22, INK);
        int boxX = 96, boxY = 18, boxW = W - 120, boxH = 30;
        Rectangle box = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };
        DrawRectangleRec(box, RAYWHITE);
        DrawRectangleLinesEx(box, 1.5f, ok ? (Color){120,170,250,255} : (Color){230,90,90,255});
        DrawText(formula, boxX + 8, boxY + 7, 18, INK);
        // cursor piscando no fim do texto
        if(((int)(GetTime()*2)) % 2 == 0){
            int cw = MeasureText(formula, 18);
            DrawText("|", boxX + 8 + cw + 1, boxY + 6, 18, INK);
        }
        if(!ok) DrawText("formula invalida", boxX + 8, boxY + boxH + 2, 12, (Color){210,70,70,255});

        // sliders (a, b, c) numa linha
        float sy0 = 96, sw = (W - 80) / 3.0f;
        a = slider(0, 40,            sy0, sw - 30, a, -5, 5, "a");
        b = slider(1, 40 + sw,       sy0, sw - 30, b, -5, 5, "b");
        c = slider(2, 40 + 2*sw,     sy0, sw - 30, c, -5, 5, "c");

        // dica no rodape
        DrawText("arraste = mover   |   scroll = zoom   |   R = resetar view   |   multiplicacao explicita: 2*x",
                 12, H - 20, 11, (Color){150,155,165,255});

        EndDrawing();
    }

    freeNode(ast);
    CloseWindow();
    return 0;
}
