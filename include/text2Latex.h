#ifndef TEXT2LATEX_H
#define TEXT2LATEX_H

#include "raylib.h"   // pour Vector2, Font, Color, etc.
#include <string.h>  // pour strlen, strncmp, etc.
extern Font mathFont;
typedef enum {
    TOKEN_SYMBOL,     // a, x, y, 1, 2...
    TOKEN_COMMAND,    // /frac, etc etc
    TOKEN_OPERATOR,   // + - * / =
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_SUB,
    TOKEN_NEXTLINE
} TokenType;

typedef enum {
    POLICE_LEFT,     
    POLICE_RIGHT,    
    POLICE_CENTERED 
} PoliceStyle;


#define TOKEN_TEXT_MAX 32

typedef struct {
    TokenType type;
    char text[TOKEN_TEXT_MAX];
} Token;

typedef enum {
    FRAC,     
    BEGIN_EQUATION,    
    BEGIN_ITEMIZE,
    OTHER
} Env;

typedef struct{
    Env env;
    int nBracket;
    bool addSpace;
} Command;

typedef struct RenderBox {
    Vector2 pos;
    float width;
    float height;
    Token *token;
    float size;
    Command command; 
    struct RenderBox *items;    // enfants linéaires (exposant, indice)
    int itemCount;
    bool line;
} RenderBox;






void rmviGetCustomFont(const char *path, float fontSize);
void rmviWriteLatex(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font);
float rmviCalcTextWidth(const char *latex, Font font, float sizeText, float spacing);
static int utf8_char_len(unsigned char c);
static int copy_utf8_char(char *dst, const char *src);
float rmviCalcBeginHeight(const char *env, int start, const char *latex, Font font, float sizeText, float spacing, bool isNested);
float rmviCalcTextHeight(const char *latex, Font font, float sizeText, float spacing, bool isNested);
void rmviWriteBegin(const char *env, const char *body, Vector2 *pos, Vector2 initPos, float sizeText, float spacing, Color color, Font font, bool isNested);
int rmviLenBegin(const char *latex, int start, const char *env);
float rmviCalcBeginWidth(const char *env, int start, const char *latex, Font font, float sizeText, float spacing);
void rmviWriteItemize(const char *body, Vector2 *pos, float sizeText, float spacing, Color color, Font font);
char *rmviGetEnv(int *i, const char *latex);
int rmviLenItem(const char *latex, int start);
float rmviCalcItemizeHeight(const char *body, int start, Font font, float sizeText, float spacing, bool isNested);
char **rmviSplitText(const char *text);
void rmviWriteAnimText(const char *text, Vector2 position, float size, Color color,int frameStart, int currentFrame);
void rmviWriteLatexLeftCentered(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font);
void rmviWriteLatexLeftCenteredClassic(const char *latex, Vector2 *position);
void rmviLoadImage(const char *path, Vector2 position, float scale);
int rmviHasPosition(const char *latex, int startIndex, bool *hasPosition,  Vector2 *imgPos);
void rmviWriteEquation(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font);
void rmviWriteFraction(const char *numerator, const char *denominator, Vector2 *position, float sizeText, float spacing, Color color, Font font);
float rmviCalcEquationHeight(const char *latex, Font font, float sizeText, float spacing);

int rmviTokenizeLatex(const char *latex, Token *tokens, int maxTokens);
Vector2 rmviMeasureToken(Token *token, Font font, float fontSize, float spacing, bool addSpace);
int rmviBuildRenderBoxes(Token *tokens, int tokenCount, RenderBox *boxes, Font font, float fontSize,float spacing);
void rmviDrawRenderBoxes( RenderBox *boxes, int count, Vector2 basePos, Font font, float fontSize, float spacing, Color color);
RenderBox rmviBuildCommandBox(Token *tokens,int tokenCount,int *index,Font font,float fontSize,float spacing);
RenderBox rmviBuildSubBox(Token *tokens,int tokenCount,int *index,Font font,float fontSize,float spacing);
#endif // TEXT2LATEX_H