#ifndef TEXT2LATEX_H
#define TEXT2LATEX_H

#include "raylib.h"   // pour Vector2, Font, Color, etc.
#include <string.h>  // pour strlen, strncmp, etc.
extern Font mathFont;
typedef enum {
    TOKEN_SYMBOL,     // a, x, y, 1, 2...
    TOKEN_SPACE,
    TOKEN_OTHER,
    TOKEN_OPERATOR,   // + - * / =
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_FRAC,
    TOKEN_ITEM,
    TOKEN_BEGIN_ITEMIZE,
    TOKEN_END_ITEMIZE,
    TOKEN_BEGIN_EQUATION,
    TOKEN_END_EQUATION,
    TOKEN_LOAD_IMAGE,
    TOKEN_SUB,
    TOKEN_NEXTLINE,
    TOKEN_DISPLACE
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
    LOAD_IMAGE,
    ITEM,
    OTHER
} Env;


typedef enum{ 
    FIT_NONE,
    FIT_RENDER ,     // fit the renderbox to the dimension of the image // DEFAULT if something
    FIT_NORENDER,     // Renderbox has not the dimension of the image. 
    FIT_POSITION
} ImageFit;

typedef struct {
    float scale;   // 1.0
    float width;   // -1 = unset
    float height;  // -1 = unset
    float posX;
    float posY;
    float shiftX;
    float shiftY;
    ImageFit fit;  // FIT_NONE
} ImgOpts;

typedef struct RenderBox {
    Vector2 pos;
    bool isPositionned; // has a fix position
    float width;
    float height;
    Token *token;
    float size;
    struct RenderBox *items;
    int itemCount;
    Texture2D *texPtr;
    bool isLine;
    bool isImage;
    float imgScale;       
    float imgW, imgH;
    bool isEndLine;
    ImageFit fit;
} RenderBox;


typedef struct Depth{
    int brace;
    int bracket;
    int item;
} Depth;






void rmviGetCustomFont(const char *path, float fontSize);
void rmviWriteLatex(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font);
float rmviCalcTextWidth(const char *latex, Font font, float sizeText, float spacing);
static int utf8_char_len(unsigned char c);
static int copy_utf8_char(char *dst, const char *src);
char *rmviGetEnv(int *i, const char *latex);
int rmviLenItem(const char *latex, int start);
float rmviCalcItemizeHeight(const char *body, int start, Font font, float sizeText, float spacing, bool isNested);
char **rmviSplitText(const char *text);
void rmviWriteAnimText(const char *text, Vector2 position, float size, Color color,int frameStart, int currentFrame);
void rmviWriteLatexLeftCentered(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font);
void rmviWriteLatexLeftCenteredClassic(const char *latex, Vector2 *position);
float rmviCalcEquationHeight(const char *latex, Font font, float sizeText, float spacing);

int rmviTokenizeLatex(const char *latex, Token *tokens, int maxTokens);
Vector2 rmviMeasureToken(Token *token, Font font, float fontSize, float spacing, bool addSpace);
int rmviBuildRenderBoxes(Token *tokens, int tokenCount, RenderBox *boxes, Font font, float fontSize,float spacing);
void rmviDrawRenderBoxes( RenderBox *boxes, int count, Vector2 basePos, Font font, float fontSize, float spacing, Color color);
char* rmviReadSymbolText(Token *tokens, int tokenCount, int *index,char openChar, char closeChar);
RenderBox rmviMain2Box(Token *tokens,int tokenCount,Font font,float fontSize,float spacing, int *index);
bool depthContinue(const Depth *depth);
bool depthUpdate(Depth *depht, Token *tokens, int *index);
RenderBox rmviBuildBrace(Token *tokens, int *index, int tokenCount, Font font, int fontSize, int spacing);
void rmviLineSkip(Vector2* cursor, float ratio, float fontSize, float height);
int rmviCalcWidthLine(RenderBox *boxes, int boxCount, float **outListWidth);
void rmviDrawRenderBoxesCentered(float *listWidth,RenderBox *boxes, int boxCount, Vector2 centerPos,Font font,float fontSize,float spacing,Color color);

#endif // TEXT2LATEX_H