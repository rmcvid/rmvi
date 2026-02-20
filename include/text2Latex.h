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
    TOKEN_BREAK,
    TOKEN_FRAC,
    TOKEN_ITEM,
    TOKEN_BEGIN_ITEMIZE,
    TOKEN_END_ITEMIZE,
    TOKEN_BEGIN_EQUATION,
    TOKEN_END_EQUATION,
    TOKEN_LOAD_IMAGE,
    TOKEN_SUB,
    TOKEN_NEXTLINE,
    TOKEN_DISPLACE,
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
    Vector2 pos;                // is the position relative to the parent box
    bool isPositionned;         // possibly to delete to see
    float width;                // width of the box, can be 0 if isImage and fit == FIT_NORENDER  fit still need ?
    float height;       
    Token *token;               // token take the text and the type
    float size;                 // is the font size
    struct RenderBox *items;    // list of children
    int itemCount;              // number of children
    Texture2D *texPtr;          // if isImage, pointer to the texture
    bool isEndLine;             // if true, the next box should be on a new line (for \frac and \item)
    bool isLine;                // if true, the box is a line (for fraction)    
    bool isImage;               // if true, the box is an image
    float imgScale;             // parameter de scale simple pour les images
    float imgW, imgH;           // dimension de l'image à afficher
    ImageFit fit;               // pour les images, comment gérer les dimensions du renderbox
    float time;                 // pour les animations sert au break, temps d'apparition du box
} RenderBox;


typedef struct Depth{
    int brace;
    int bracket;
    int item;
} Depth;

typedef struct AnimText{
    int letterCount;
    float animTime;
    int boxEnd;
    bool hasChanged;
}AnimText;

// structure pour la lecture de fichier texte
typedef struct Lecture{
    const char *path;
    char *content;
    int  oldParagraph;
    int  currentParagraph;
} Lecture;

#define ANIM_CLAVIE_SPEED 1.0f/12.0f




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
void rmviDrawRenderBoxesCentered(float *listWidth,float height, RenderBox *boxes, int boxCount, Vector2 centerPos,Font font,float fontSize,float spacing,Color color);
float rmviCalcHeightTotal(RenderBox *boxes, int boxCount);
void rmviDrawRenderBoxesAnimed(RenderBox *boxes,int boxCount,Vector2 basePos,Font font,float fontSize,float spacing, Color color, AnimText *anim);
AnimText* initAnimText();
void resetAnimText(AnimText *anim);
void rmviDrawRenderBoxesCenteredAnimed(RenderBox *boxes,int boxCount,Vector2 basePos,Font font,float fontSize,float spacing, Color color, AnimText* anim);

int readScenario(Lecture *lecture);
Lecture rmviGetLecture(const char *path);
#endif // TEXT2LATEX_H