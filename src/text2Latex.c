#include <stdio.h>
#include <stdlib.h>
#include "text2Latex.h"
#include "visual.h"
Font mathFont;

#define MAX_CODEPOINTS 512
#define INTERLIGNE_ITEM 0.5f    // interligne pour les items
#define INTERLIGNE 0.3f         // Line spacing ratio to multiply to the fontSize
#define RATIO_INDEX 0.6f        // Size ratio for index text such H_3
#define RATIO_INDEX_DOWN 0.6f   // Ratio for subscript positioning
#define RATIO_INDEX_UP 0.15f    // Ratio for superscript positioning
#define RATIO_SIZE_FRAC 0.8f
#define RATIO_INDENT 2.0f      // Ratio for item indentation
#define RATIO_SPACE 0.33f
#define RATIO_LEN_TEXT 0.8f * GetScreenWidth()
#define RATIO_INDENT_BEGIN 2    // ratio for the indent time the font size
#define SIZE_TEXT 40.0f
#define SIZE_SPACING 8.0f
#define BIG 1.4f
#define RATIO_LINE 1/20.0f
#define MATH_INTERLINE_RATIO INTERLIGNE_ITEM
#define MINIMAL_BREAK_TIME 1.5f

#define MAX_LINE 1024
#define MAX_PARAGRAPH 1024

#define ERROR_OUT_OF_INDEX(index, tokenCount)                           \
    do {                                                                \
        if ((index) > (tokenCount)) {                                   \
            fprintf(stderr,                                             \
                "ERROR: index out of bounds (%d > %d) at %s:%d\n",      \
                (index), (tokenCount), __FILE__, __LINE__);             \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)

#define RMVI_MEASURE_WIDTH_SYMBOLE(cmd, glyph) else if (strncmp(&latex[i + 1], (cmd), sizeof(cmd) - 1) == 0) {  \
    Vector2 ts = MeasureTextEx(font, (glyph), sizeText, spacing);     \
        width += ts.x;                                                \
        i += (int)(sizeof(cmd) - 1) + 1;                              \
    }

#define GREC_LETTER(cmd, adress) else if( strcmp(tokens[*index].text, cmd) == 0){      \
    box.token = adress;                                                \
    Vector2 size = MeasureTextEx(font, box.token->text, fontSize, spacing); \
    box.width = size.x;\
    box.height = size.y;\
    } \

#define RMVI_GROW_CAP(cap) ((cap) ? (cap) * 2 : 32)
#define RMVI_ENSURE_CAP(ptr, count, cap, T) do {                     \
    if ((count) >= (cap)) {                                          \
        size_t newCap = RMVI_GROW_CAP(cap);                          \
        void *newPtr = realloc((ptr), newCap * sizeof(T));           \
        if (!newPtr) { /* handle OOM */ exit(1); }                   \
        (ptr) = (T*)newPtr;                                          \
        (cap) = newCap;                                              \
    }                                                                \
} while(0)
#define RMVI_PUSH(ptr, count, cap, T, value) do {                    \
    RMVI_ENSURE_CAP(ptr, count, cap, T);                             \
    (ptr)[(count)++] = (value);                                      \
} while(0)



typedef struct {
    char type;
    Color color;
    Vector2 position;
    float sizeText;
    float spacing;
    float interline;
    int miseEnPage;
} State;



typedef enum{
    CLASSIC,
    DOLLAR,
    EQUATION,
    SUB
} Style;

State state = {
    .sizeText = SIZE_TEXT,
    .spacing = SIZE_SPACING,
    .interline = INTERLIGNE,
    .miseEnPage = CLASSIC
};



static Token IntToken        = { TOKEN_SYMBOL, "∫" };
static Token deltaToken      = { TOKEN_SYMBOL, "δ" };
static Token DeltaToken      = { TOKEN_SYMBOL, "Δ" };
static Token alphaToken      = { TOKEN_SYMBOL, "α" };
static Token betaToken       = { TOKEN_SYMBOL, "β" };
static Token gammaToken      = { TOKEN_SYMBOL, "γ" };
static Token GammaToken      = { TOKEN_SYMBOL, "Γ" };
static Token thetaToken      = { TOKEN_SYMBOL, "θ" };
static Token ThetaToken      = { TOKEN_SYMBOL, "Θ" };
static Token piToken         = { TOKEN_SYMBOL, "π" };
static Token PiToken         = { TOKEN_SYMBOL, "Π" };
static Token sigmaToken      = { TOKEN_SYMBOL, "σ" };
static Token SigmaToken      = { TOKEN_SYMBOL, "Σ" };
static Token phiToken        = { TOKEN_SYMBOL, "φ" };
static Token PhiToken        = { TOKEN_SYMBOL, "Φ" };
static Token psiToken        = { TOKEN_SYMBOL, "ψ" };
static Token PsiToken        = { TOKEN_SYMBOL, "Ψ" };
static Token omegaToken      = { TOKEN_SYMBOL, "ω" };
static Token OmegaToken      = { TOKEN_SYMBOL, "Ω" };
static Token neqToken        = { TOKEN_SYMBOL, "≠" };
static Token leqToken        = { TOKEN_SYMBOL, "≤" };
static Token geqToken        = { TOKEN_SYMBOL, "≥" };
static Token infinityToken   = { TOKEN_SYMBOL, "∞" };

static Token spaceToken      = { TOKEN_SYMBOL, " " };
static Token tiretToken      = { TOKEN_SYMBOL, "-"};
static Token displaceToken   = { TOKEN_DISPLACE, ""}; 
static Token breakToken      = { TOKEN_BREAK, ""};

typedef struct {
    char path[512];
    Texture2D tex;
    bool used;
} TextureList;

static TextureList g_texCache[128] = {0};
static int g_texCacheCount = 0;

static Texture2D *rmviGetTexturePtrCached(const char *path){
    for (int i = 0; i < g_texCacheCount; i++) {
        if (g_texCache[i].used && strcmp(g_texCache[i].path, path) == 0) {
            return &g_texCache[i].tex;
        }
    }
    if (g_texCacheCount >= 128) {
        printf("Texture cache full, cannot load: %s\n", path);
        return NULL;
    }
    TextureList *e = &g_texCache[g_texCacheCount++];
    e->used = true;
    strncpy(e->path, path, sizeof(e->path) - 1);
    e->path[sizeof(e->path) - 1] = '\0';
    e->tex = LoadTexture(path);
    // Optionnel: check
    if (e->tex.id == 0) {
        printf("Failed to load texture: %s\n", path);
        // tu peux laisser quand même, ou marquer used=false
    }
    return &e->tex;
}



static int isOperator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '='|| c == '.' || c ==':' || c == ',' || c == '<' || 
    c == '>' || c == ')' || c == '(');
}
static int isAlphaNum(unsigned char c)
{
    // ASCII letters & digits
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '\'' ) 
        return 1;
    // UTF-8 continuation or leading byte
    if (c >= 0x80)
        return 1;
    return 0;
}

int tokenTypeCommand(Token token){
    if (strcmp(token.text, "/frac") == 0)
        return TOKEN_FRAC;
    else if(strcmp(token.text,"/item") == 0)
        return TOKEN_ITEM;
    else if(strcmp(token.text,"/break") == 0)
        return TOKEN_BREAK;
    else if (strcmp(token.text, "/begin(itemize)") == 0)
        return TOKEN_BEGIN_ITEMIZE;
    else if (strcmp(token.text, "/end(itemize)") == 0 )
        return TOKEN_END_ITEMIZE;
    else if (strcmp(token.text, "/begin(equation)") == 0)
        return TOKEN_BEGIN_EQUATION;
    else if (strcmp(token.text, "/end(equation)") == 0)
        return TOKEN_END_EQUATION;
    else if (strcmp(token.text, "/loadimage") == 0 || strcmp(token.text, "/loadImage") == 0)
        return TOKEN_LOAD_IMAGE;
    else
        return TOKEN_OTHER;
}

// renvoie la liste des différents tokens
int rmviTokenizeLatex(const char *latex, Token *tokens, int maxTokens){
    int i = 0;
    int count = 0;
    if(latex == NULL) return 0;
    while (latex[i] && count < maxTokens){
        char c = latex[i];
        // Ignore spaces
        if (c == ' ') {
            i++;
            continue;
        }
        // COMMAND: \xxxx
        else if (c == '/') {
            int j = 0;
            if(latex[i+1] == '/'){
                tokens[count++].type = TOKEN_NEXTLINE;
                i = i + 2;
                continue;
            }
            else if (latex[i+1] == ' ') {
                tokens[count].type = TOKEN_SPACE;
                tokens[count].text[0] = ' ';
                tokens[count].text[1] = '\0';
                count++;
                i= i + 2;
                continue;
            }
            else{
                tokens[count].text[j++] = latex[i++]; // on prend le premier d'office
                //ici
                while (( isAlphaNum(latex[i]) || isOperator(latex[i]) ) && j < TOKEN_TEXT_MAX - 1) {
                    tokens[count].text[j++] = latex[i++];
                }
                tokens[count].text[j] = '\0';
                tokens[count].type = tokenTypeCommand(tokens[count]);
                count++;
                continue;
            }
        }
        // Braces
        else if (c == '{') {
            tokens[count++] = (Token){ TOKEN_LBRACE, "{" };
            i++;
            continue;
        }

        else if (c == '}') {
            tokens[count++] = (Token){ TOKEN_RBRACE, "}" };
            i++;
            continue;
        }
        else if(c == '['){
            tokens[count++] = (Token){ TOKEN_RBRACKET, "[" };
            i++;
            continue;
        }
        else if(c == ']'){
            tokens[count++] = (Token){ TOKEN_RBRACKET, "]" };
            i++;
            continue;
        }
        else if (c == '^'){
            tokens[count++] = (Token) { TOKEN_SUB, '^'};
            i++;
            continue;
        }
        else if (c == '_'){
            tokens[count++] = (Token) { TOKEN_SUB, '_'};
            i++;
            continue;
        }
        else if (isOperator(c)) {
            tokens[count].type = TOKEN_OPERATOR;
            tokens[count].text[0] = c;
            tokens[count].text[1] = '\0';
            count++;
            i++;
            continue;
        }
        else if(isAlphaNum(c)) {
            int j = 0;
            tokens[count].type = TOKEN_SYMBOL;
            while ( (isAlphaNum(latex[i]) || isOperator(latex[i])) && j < TOKEN_TEXT_MAX - 1) {
                tokens[count].text[j++] = latex[i++];
            }
            tokens[count].text[j] = '\0';
            count++;
            continue;
        }
        else{
            i++;
        }
    }
    return count;
}

Vector2 rmviMeasureToken(Token *token, Font font, float fontSize, float spacing, bool addSpace){
    Vector2 size = Vector2Zero();
    switch (token->type){
        case TOKEN_SYMBOL:
            size = MeasureTextEx(font, token->text, fontSize, spacing);
            if(addSpace) size.x += RATIO_SPACE*fontSize;
            break;
        case TOKEN_OPERATOR:
            size = MeasureTextEx(font, token->text, fontSize, spacing);
            break;
            
    }
    return size;
}

// trim retire les espace du text
static char *rmviTrim(char *s){
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return s;
}

static ImgOpts rmviParseImgOpts(const char *s){
    ImgOpts o = {0};
    o.scale = 1.0f;
    o.width = -1.0f;
    o.height = -1.0f;
    o.posX = -1;
    o.posY = -1;
    o.fit = FIT_RENDER;
    if (!s || !s[0]) return o;
    char buf[512];
    strncpy(buf, s, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    for (char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        // trim espaces
        while (*tok == ' ') tok++;
        char *eq = strchr(tok, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = rmviTrim(tok);
        char *val = rmviTrim(eq + 1);
        if (strcmp(key, "scale") == 0) o.scale = (float)atof(val);
        else if (strcmp(key, "width") == 0) o.width = (float)atof(val);
        else if (strcmp(key, "height") == 0) o.height = (float)atof(val);
        if (strcmp(key, "posX") == 0){
            o.posX = (float)atof(val);
            o.fit = FIT_POSITION;
        }        
        if (strcmp(key, "posY") == 0){
            o.posY = (float)atof(val);
            o.fit = FIT_POSITION;
        }
        if (strcmp(key, "shiftX") == 0) o.shiftX = (float)atof(val);        
        if (strcmp(key, "shiftY") == 0) o.shiftY = (float)atof(val);
        if (strcmp(key, "fit") == 0) {
            if (strcmp(val, "noRender") == 0 && o.fit != FIT_POSITION) o.fit = FIT_NORENDER;
        }
    }
    return o;
}

// to do implémenter les shifts
void rmviBuildImageOpt(RenderBox *box, ImgOpts imgOpts){
    if(!box ) return;
    box->isImage = true;
    if (imgOpts.scale  <= 0.0f) imgOpts.scale = 1.0f;
    float tw = (float)box->texPtr->width;
    float th = (float)box->texPtr->height;
    if (imgOpts.width > 0.0f && imgOpts.height > 0.0f){
        box->imgW = imgOpts.width ;
        box->imgH = imgOpts.height;
    }
    else if (imgOpts.width > 0.0f){
        float s = imgOpts.width / tw;
        box->imgW = tw * s;
        box->imgH = th * s;
    }
    else if (imgOpts.height > 0.0f){
        float s = imgOpts.height / th;
        box->imgW = tw * s;
        box->imgH = th * s;
    }
    else{
        // scale simple
        box->imgW = tw * imgOpts.scale;
        box->imgH = th * imgOpts.scale;
    }
    if(imgOpts.fit == FIT_NORENDER){
        box->width = 0;
        box->height = 0;
    }
    else if(imgOpts.fit == FIT_POSITION){
        box->isPositionned = true;
        box->pos.x = imgOpts.posX;
        box->pos.y = imgOpts.posY;
    }
    else{
        box->width = box->imgW;
        box->height = box->imgH;
    }
}

// the boolean impose if we reduce the police or not
RenderBox rmviBuildFrac(Token *tokens,int tokenCount, int *index, Font font, float fontSize, float spacing, bool reduce){  
    float ratio = (reduce ? RATIO_SIZE_FRAC : 1 );
    float padding = ratio * fontSize * 0.1f;
    RenderBox box = {0};

    RenderBox lineBox = (RenderBox){0};
    lineBox.isLine = true;
    lineBox.size   = ratio * fontSize * 0.05f;
    lineBox.pos    = (Vector2) {0,fontSize/2};

    RenderBox numBox = rmviMain2Box(tokens,tokenCount,font,ratio * fontSize, ratio * spacing , index);
    numBox.pos.y = lineBox.pos.y - numBox.height - padding;
    RenderBox denBox = rmviMain2Box(tokens,tokenCount,font,ratio * fontSize, ratio * spacing , index);
    denBox.pos.y = lineBox.pos.y + lineBox.size + padding;
    lineBox.width  =  fmaxf(numBox.width, denBox.width) + 2*padding;

    box.itemCount = 3;
    box.items = MemAlloc(sizeof(RenderBox) * box.itemCount);
    box.items[0] = numBox;
    box.items[1] = denBox;
    box.items[2] = lineBox;
    box.width  = lineBox.width;
    box.height = numBox.height + denBox.height + 2.0f * padding + lineBox.size;

    float xShift = (denBox.width - numBox.width) * 0.5f;
    box.items[0].pos.x += fmaxf(xShift, 0.0f) + padding;
    box.items[1].pos.x += fmaxf(-xShift, 0.0f) + padding;
    return box;
}

// build l'exposant et le sub
RenderBox rmviBuildSub(Token *tokens, int tokenCount, int *index,Font font, float fontSize, float spacing){
    RenderBox box ={0};
    Vector2 cursor = Vector2Zero();
    float yShift = 0.0f;
    if ((*index - 1) >= 0) {
        if (tokens[*index - 1].text && strncmp(tokens[*index - 1].text, "^", 1) == 0) {
            yShift = -RATIO_INDEX_UP * fontSize;
        } else if (tokens[*index - 1].text && strncmp(tokens[*index - 1].text, "_", 1) == 0) {
            yShift =  RATIO_INDEX_DOWN * fontSize;
        }
    }

    if (tokens[*index].type == TOKEN_LBRACE) {
        box = rmviBuildBrace(tokens,index,tokenCount,font,RATIO_INDEX *fontSize, RATIO_INDEX *spacing);
    }
    else {
        Vector2 sizeText = rmviMeasureToken(&tokens[*index], font,RATIO_INDEX * fontSize, RATIO_INDEX *spacing, false);
        box.pos = (Vector2){cursor.x, cursor.y + yShift };
        box.width  = sizeText.x;
        box.height = sizeText.y;
        box.token  = &tokens[*index];
        box.size   = RATIO_INDEX * fontSize;
        cursor.x += sizeText.x;
        (*index)++;
    }
    return box;
}

char* rmviReadSymbolText(Token *tokens, int tokenCount, int *index,char openChar, char closeChar){
    int depth = 0;
    // buffer temporaire
    int capacity = 512;
    int length = 0;
    char *result = MemAlloc(capacity);
    result[0] = '\0';
    if (tokens[*index].text[0] == openChar && tokens[*index].text[1] == '\0') {
        depth = 1;
        (*index)++;
    } else {
        printf("rmviReadSymbolText: expected '%c', got '%s'\n",
               openChar, tokens[*index].text);
        return result;
    }
    while (depth > 0 && *index < tokenCount){
        char *t = tokens[*index].text;
        if (t[0] == openChar && t[1] == '\0') {
            depth++;
            (*index)++;
            continue;
        }
        if (t[0] == closeChar && t[1] == '\0') {
            depth--;
            (*index)++;
            continue;
        }
        int len = strlen(t);
        if (length + len + 1 >= capacity) {
            capacity *= 2;
            result = MemRealloc(result, capacity);
        }
        memcpy(result + length, t, len);
        length += len;
        result[length] = '\0';
        (*index)++;
    }
    return result;
}

// is true if the token is consumed
bool depthUpdate(Depth *depth, Token *tokens, int *index){
    bool consumed = true;
    switch (tokens[*index].type){
    case TOKEN_LBRACE:
        depth->brace += 1;
        (*index) ++;
        break;
    case TOKEN_RBRACE:
        depth->brace -=1;
        (*index) ++;
        break;
    case TOKEN_LBRACKET:
        depth->bracket +=1;
        (*index) ++;
        break;
    case TOKEN_RBRACKET:
        depth->bracket -=1;
        (*index) ++;
        break;
    case TOKEN_BEGIN_ITEMIZE:
        depth->item += 1;
        (*index)++;
        break;
    case TOKEN_END_ITEMIZE:
        depth->item -=1;
        (*index)++;
        break;

    default:
        consumed = false;
        break;
    }
    return consumed;
}

bool depthContinue(const Depth *depth){
    if (!depth) {
        fprintf(stderr, "depthContinue: depth is NULL\n");
        exit(EXIT_FAILURE);
    }
    // Erreur fatale : profondeur négative
    if (depth->brace   < 0 || depth->bracket < 0 || depth->item   < 0){
        fprintf(stderr,"Depth error: negative depth detected (brace=%d, bracket=%d, paren=%d)\n",depth->brace, depth->bracket, depth->item);
        exit(EXIT_FAILURE);
    }
    return (depth->brace > 0 || depth->bracket > 0 || depth->item > 0);
}

RenderBox rmviBuildOther(Token *tokens,int tokenCount,int *index,Font font,float fontSize,float spacing){
    RenderBox box ={0};
    Vector2 cursor = Vector2Zero();
    box.pos = cursor;
    box.size = fontSize;
    if(false);
    GREC_LETTER("/int",      &IntToken) 
    GREC_LETTER("/Delta",    &DeltaToken)
    GREC_LETTER("/delta",    &deltaToken)
    GREC_LETTER("/Delta",    &DeltaToken)
    GREC_LETTER("/alpha",    &alphaToken)
    GREC_LETTER("/beta",     &betaToken)
    GREC_LETTER("/gamma",    &gammaToken)
    GREC_LETTER("/Gamma",    &GammaToken)
    GREC_LETTER("/theta",    &thetaToken)
    GREC_LETTER("/Theta",    &ThetaToken)
    GREC_LETTER("/pi",       &piToken)
    GREC_LETTER("/Pi",       &PiToken)
    GREC_LETTER("/sigma",    &sigmaToken)
    GREC_LETTER("/Sigma",    &SigmaToken)
    GREC_LETTER("/phi",      &phiToken)
    GREC_LETTER("/Phi",      &PhiToken)
    GREC_LETTER("/psi",      &psiToken)
    GREC_LETTER("/Psi",      &PsiToken)
    GREC_LETTER("/omega",    &omegaToken)
    GREC_LETTER("/Omega",    &OmegaToken)
    GREC_LETTER("/neq",      &neqToken)
    GREC_LETTER("/leq",      &leqToken)
    GREC_LETTER("/geq",      &geqToken)
    GREC_LETTER("/infinity", &infinityToken)
    
    (*index)++;
    return box;
}
RenderBox rmviBuildBrace(Token *tokens, int *index, int tokenCount, Font font, int fontSize, int spacing){
    RenderBox box ={0};
    Depth depthSub ={0};
    depthUpdate(&depthSub,tokens,index);
    int childCount = 0;
    int childCap = 0;
    int initMemory = 32;
    RenderBox *children = NULL;
    while(depthContinue(&depthSub)){
        if(depthUpdate(&depthSub,tokens,index)){
        }
        else{
            RenderBox child = rmviMain2Box(tokens, tokenCount, font, fontSize,spacing, index);
            RMVI_PUSH(children,childCount,childCap,RenderBox, child);
            box.width += child.width;                       // ici à modifier ?
            box.height = max(box.height, child.height);
        }
    }
    // ici on peut metre un truc du style si un seul enfant
    box.items = MemAlloc(sizeof(RenderBox) * childCount);
    memcpy(box.items, children, sizeof(RenderBox) * childCount);
    free(children);
    box.itemCount = childCount;
    return box;
}
void rmviFixChildPositionNext(RenderBox *box){
    Vector2 cursor =Vector2Zero();
    float height = box->size;
    box->height += height;
    for(int i = 0; i < box->itemCount; i++){
        height = max(box->items[i].height,height);
        if(cursor.x + box->items[i].width > RATIO_LEN_TEXT){ // ici à modifier je pense
            box->items[i].isEndLine = true;
            rmviLineSkip(&cursor,INTERLIGNE, box->size, height);
            box->height += height + INTERLIGNE*box->size;
            height = box->size;
        }
        if( box->items[i].token && box->items[i].token->type == TOKEN_DISPLACE){
            box->items[i].token = NULL;
            box->items[i].isEndLine = true;
            rmviLineSkip(&cursor,INTERLIGNE_ITEM, box->size, height);
            box->height += height + INTERLIGNE*box->size; ;
            height = box->size;
        }
        // if passage à la ligne ?
        else {
            box->items[i].pos.x += cursor.x;
            box->items[i].pos.y += cursor.y;
            height = max(height,box->items[i].height);
            cursor.x += box->items[i].width;
            if(box->width < RATIO_LEN_TEXT) box->width +=box->items[i].width;
            else box->width = RATIO_LEN_TEXT;
        }
    }
}

void rmviFixChildPositionUnder(RenderBox *box, float ratio){
    Vector2 cursor =Vector2Zero();
    for(int i = 0; i < box->itemCount; i++){
        box->items[i].pos.y += cursor.y;
        cursor.y += box->items[i].height + ratio*box->items[i].size;
        box->height += box->items[i].height + ratio*box->items[i].size;
        box->width = min( max(box->width,box->items[i].width),RATIO_LEN_TEXT);
    }

}

RenderBox rmviBuildSpace(Token *tokens,int tokenCount,int *index,Font font,float fontSize,float spacing){
    RenderBox box ={0};
    box.size = fontSize;
    box.token = &spaceToken; 
    Vector2 size = MeasureTextEx(font, box.token->text, fontSize, spacing); 
    box.width = size.x; 
    box.height = size.y;
    (*index)++;
    return box;
}
RenderBox rmviBuildItem(Token *tokens,int tokenCount,int *index,Font font,float fontSize,float spacing){
    RenderBox box = {0};
    RenderBox *children = NULL;
    int childCap = 0;
    int childCount = 0;
    box.size = fontSize;
    if(tokens[*index].type == TOKEN_RBRACKET){
        // cas ou item[qqch]
    }
    else{
        RenderBox child = {0};
        child.token = &tiretToken;
        child.width = rmviMeasureToken(child.token,font,fontSize,spacing,true).x;
        child.size = fontSize;
        RMVI_PUSH(children, childCount, childCap, RenderBox, child);
        // on remplace le premier box par un tiret -
    }
    while(tokens[*index].type != TOKEN_END_ITEMIZE && tokens[*index].type != TOKEN_ITEM){
        ERROR_OUT_OF_INDEX(*index, tokenCount);
        RenderBox child = {0};
        child = rmviMain2Box(tokens,tokenCount,font,fontSize,spacing,index);
        RMVI_PUSH(children, childCount, childCap, RenderBox, child);

    }
    box.items = MemAlloc(sizeof(RenderBox) * childCount);
    memcpy(box.items, children, sizeof(RenderBox) * childCount);
    box.itemCount = childCount;
    rmviFixChildPositionNext(&box);
    free(children);
    return box;
}

RenderBox rmviBuildBeginItemize(Token *tokens,int tokenCount,int *index,Font font,float fontSize,float spacing){
    RenderBox box = {0};
    RenderBox *children = NULL;
    int childCap = 0;
    int childCount = 0;
    Depth depth = {0};
    depthUpdate(&depth,tokens,index);
    box.size = fontSize;
    box.token = &displaceToken;
    // ici parametre à imposer
    box.pos = (Vector2) {RATIO_INDENT_BEGIN*fontSize , 0 };
    box.isEndLine = true;
    while (depthContinue(&depth)){
        if(depthUpdate(&depth,tokens,index)){
        }
        else if(tokens[*index].type == TOKEN_ITEM){
            (*index)++; //skip begin
            RenderBox child ={0};
            child = rmviBuildItem(tokens, tokenCount,index,font, fontSize,spacing);
            RMVI_PUSH(children, childCount, childCap, RenderBox, child);
        }
        else{ 
            fprintf(stderr,
                "Syntax error: itemize must contain only \\item or \\end(itemize).\n"
                "Found token type %d with text '%s' at index %d.\n",
                tokens[*index].type,
                tokens[*index].text,
                *index
            );
            exit(EXIT_FAILURE);

        }
    }
    box.items = MemAlloc(sizeof(RenderBox) * childCount);
    memcpy(box.items, children, sizeof(RenderBox) * childCount);
    box.itemCount = childCount;
    free(children);
    return box;
}

RenderBox rmviBuildBreak(Token *tokens,int tokenCount, int *index){
    RenderBox box = {0};
    // la le problème c'est que le tems n'est pas compris
    // soit on rajoute un parametre soi on voit comment on peut modifier le text
    // le truc c'est qu'il sera traité comme du text donc vaut mieux pas
    box.token = &breakToken;
    (*index) ++;
    char *timeStr = rmviReadSymbolText(tokens, tokenCount, index, '{', '}');
    // ici j'ai une fonction qui peut ressortir ca en vrai
    float time = *timeStr ? (float) atof(timeStr) : MINIMAL_BREAK_TIME;         // doit vérifier que timeStr est non nulle ?z
    box.time = time;
    free(timeStr);
    return box;
}

RenderBox rmviMain2Box(Token *tokens,int tokenCount,Font font,float fontSize,float spacing, int *index){
    // mettre un switch case
    RenderBox box = {0};
    Vector2 cursor = Vector2Zero();
    if (tokens[*index].type == TOKEN_FRAC){
        (*index)++; // skip \frac
        box = rmviBuildFrac(tokens, tokenCount,index,font, fontSize,spacing, true);
    }
    else if(tokens[*index].type == TOKEN_BREAK){
        box = rmviBuildBreak(tokens, tokenCount,index);
    }
    else if (tokens[*index].type == TOKEN_BEGIN_ITEMIZE){
        box = rmviBuildBeginItemize(tokens, tokenCount,index,font, fontSize,spacing);
        rmviFixChildPositionUnder(&box,1.0f);
    }
    else if(tokens[*index].type == TOKEN_LOAD_IMAGE){
        (*index)++;
        char *opts = NULL;
        ImgOpts imgOpts = {0};
        if (tokens[*index].text[0] == '[') {
            opts = rmviReadSymbolText(tokens, tokenCount, index, '[', ']');
            imgOpts = rmviParseImgOpts(opts);
            free(opts);
        }
        char *path = rmviReadSymbolText(tokens, tokenCount, index,'{','}');
        box.isImage = true;
        box.size = fontSize;
        box.texPtr = rmviGetTexturePtrCached(path);
        rmviBuildImageOpt(&box,imgOpts);
        free(path);
    }
    else if (tokens[*index].type == TOKEN_OTHER){
        box = rmviBuildOther(tokens,tokenCount,index,font,fontSize,spacing);
    }
    else if (tokens[*index].type == TOKEN_SPACE){
        box = rmviBuildSpace(tokens,tokenCount,index,font,fontSize,spacing);
    }
    else if( tokens[*index].type == TOKEN_SUB){
        (*index)++;
        box = rmviBuildSub(tokens,tokenCount,index,font,fontSize,spacing);
        // fonction qui place les box à la bonne place
        rmviFixChildPositionNext(&box);
    }
    else if (tokens[*index].type == TOKEN_LBRACE){
        box = rmviBuildBrace(tokens,index,tokenCount,font,fontSize,spacing);
    }
    else if(tokens[*index].type == TOKEN_NEXTLINE){ //ici
        box.token = &displaceToken;
        box.size = fontSize;
        (*index)++;
    }
    else{
        // met un espace après un mot si n'est pas après _ ou ^
        Vector2 size = rmviMeasureToken(&tokens[*index], font, fontSize, spacing, (tokens[*index+1].type != TOKEN_SUB));
        box.pos.x = cursor.x;
        box.pos.y = cursor.y;
        box.width = size.x;
        box.height = size.y;
        box.token = &tokens[*index];
        box.size = fontSize;
        cursor.x += size.x;
        (*index)++;
    }
    return box;
}
void rmviFixBoxPosition(RenderBox *boxes, RenderBox *box, int *index, int *count, Vector2 *cursor){
    if (box->token && box->token->type == TOKEN_DISPLACE){
        
        rmviLineSkip(cursor,INTERLIGNE_ITEM, box->size, box->size);
        box->pos = Vector2Add( box->pos, *cursor);
        cursor->y += box->height; 
    }
    else{
        box->pos = Vector2Add( box->pos, *cursor);
        cursor->x += box->width;
    }   
}
void rmviLineSkip(Vector2* cursor, float ratio, float fontSize, float height){
    cursor->x = 0.0f;
    cursor->y += ratio*fontSize+ height;
}

int rmviBuildRenderBoxes(Token *tokens,int tokenCount,RenderBox *boxes,Font font,float fontSize,float spacing){
    Vector2 cursor = Vector2Zero();
    int count = 0;
    int oldIndex = 0;
    float height = fontSize;
    RenderBox box = {0};
    for (int index = 0; index < tokenCount; ){
        height = max(height,boxes[count-1].height);
        if(cursor.x > RATIO_LEN_TEXT ){
            rmviLineSkip(&cursor,INTERLIGNE, fontSize,height);
            if (count !=0) boxes[count-1].isEndLine = true;
            height = fontSize;
        }
        if(tokens[index].type == TOKEN_NEXTLINE){
            if (count !=0) boxes[count-1].isEndLine = true;
            rmviLineSkip(&cursor,INTERLIGNE, fontSize,height);
            (index)++;
            height = fontSize;
        }
        box = rmviMain2Box(tokens,tokenCount,font,fontSize,spacing, &index);
        rmviFixBoxPosition(boxes, &box, &index, &count, &cursor);
        boxes[count++] = box;
    }
    return count;
}

void rmviDrawRenderBox(RenderBox *box,Vector2 basePos,Font font,float fontSize,float spacing,Color color){ 
    Vector2 drawPos;
    if(box->isPositionned) drawPos = (Vector2){ box->pos.x,box->pos.y }; 
    else drawPos = (Vector2) { basePos.x + box->pos.x, basePos.y + box->pos.y }; 
    if (box->itemCount > 0 && box->items){ 
        float cursorX = 0.0f; 
        for (int i = 0; i < box->itemCount; i++){ RenderBox *child = &box->items[i]; 
            rmviDrawRenderBox(child,drawPos,font,child->size,spacing,color); 
            cursorX += child->width;
        }
    }
    else if (box->token){
            DrawTextEx(font,box->token->text,drawPos,fontSize,spacing,color);
    } 
    else if(box->isLine){
        DrawLineEx((Vector2) {drawPos.x,drawPos.y}, (Vector2) {drawPos.x + box->width, drawPos.y},box->size,color);
    }
    else if(box->isImage){
        Rectangle src = (Rectangle){0,0,(float)box->texPtr->width,(float)box->texPtr->height};
        Rectangle dst = (Rectangle){drawPos.x, drawPos.y, box->imgW, box->imgH};
        rmviRotateTexture(*box->texPtr, src, dst, (Vector2){0,0}, 0.0f, WHITE);
    }
}

void rmviDrawRenderBoxes(RenderBox *boxes,int boxCount,Vector2 basePos,Font font,float fontSize,float spacing,Color color){
    for (int i = 0; i < boxCount; i++){
        rmviDrawRenderBox(&boxes[i],basePos,font,fontSize,spacing,color);
    }
}

void rmviDrawRenderBoxesCentered(float *listWidth,float height, RenderBox *boxes, int boxCount, Vector2 centerPos,Font font,float fontSize,float spacing,Color color){
    Vector2 centeredPos = centerPos;
    centeredPos.y = centerPos.y - height / 2;
    int endLine = 0;
    for(int i =0; i< boxCount; i++){
        centeredPos.x = centerPos.x - listWidth[endLine]/2;
        rmviDrawRenderBox(&boxes[i],centeredPos,font,fontSize,spacing,color);
        if(boxes[i].isEndLine) endLine ++;
    }
}

AnimText* initAnimText(){
    AnimText *anim = MemAlloc(sizeof(AnimText));
    anim->boxEnd = -1;
    return anim;
}
void resetAnimText(AnimText *anim){
    *anim = (AnimText){0};
    anim->boxEnd = -1;
}

void freeAnimText(AnimText *anim){
    if(anim) free(anim);
}

float boxWriteTime(RenderBox box){
    float time = 0;
    if(box.token && box.token->type == TOKEN_BREAK) return box.time;
    if(box.itemCount>0){
        for(int i =0; i< box.itemCount; i++){
            time+= boxWriteTime(box.items[i]);
        }
    }
    if(box.token && box.token->text) return time + ANIM_CLAVIE_SPEED*(1.0f + GetCodepointCount(box.token->text));
    else return time;
}

void rmviDrawRenderBoxAnimed(RenderBox *box,Vector2 basePos,Font font,float fontSize,float spacing,Color color, AnimText* anim){ 
    Vector2 drawPos;
    AnimText subAnim = *anim;
    if(box->isPositionned) drawPos = (Vector2){ box->pos.x,box->pos.y }; 
    else drawPos = (Vector2) { basePos.x + box->pos.x, basePos.y + box->pos.y }; 
    if (box->itemCount > 0 && box->items){ 
        float cursorX = 0.0f; 
        for (int i = 0; i < box->itemCount; i++){
            anim->boxEnd = i;
            RenderBox *child = &box->items[i];
            float timeToBox = boxWriteTime(*child);
            if(subAnim.animTime>timeToBox) rmviDrawRenderBox(child,drawPos,font,child->size,spacing,color);
            else rmviDrawRenderBoxAnimed(child,drawPos,font,child->size,spacing,color,&subAnim);
            subAnim.animTime -= timeToBox;
            cursorX += child->width;
            if (subAnim.animTime <= 0){
                if(subAnim.boxEnd > 0) anim->boxEnd = subAnim.boxEnd;
                anim->letterCount = subAnim.letterCount;
                break; 
            }
        }
    }
    else if (box->token){
        float timeToBox = boxWriteTime(*box);
        float ratio = subAnim.animTime / timeToBox;
        if (ratio > 1.0f){
            DrawTextEx(font,box->token->text,drawPos,fontSize,spacing,color);
            return;
        }
        int cpCount = 0;
        int *cps = LoadCodepoints(box->token->text, &cpCount);
        anim->letterCount = (cpCount >= 0) ? cpCount : 0;
        int target = (int)(ratio * (float)cpCount);
        if (target >= cpCount) target = cpCount;
        if (target == 0) { UnloadCodepoints(cps); return; }
        char out[1024];        
        int outLen = 0;
        out[0] = '\0';
        for (int i = 0; i < target; i++){
            int bytes = 0;
            const char *u8 = CodepointToUTF8(cps[i], &bytes);
            if (!u8 || bytes <= 0) continue;
            if (outLen + bytes >= (int)sizeof(out) - 1) break; // éviter overflow
            memcpy(out + outLen, u8, bytes);
            outLen += bytes;
            out[outLen] = '\0';
        }
        UnloadCodepoints(cps);
        DrawTextEx(font, out, drawPos, fontSize, spacing, color);
    }
    else if(box->isLine){
        DrawLineEx((Vector2) {drawPos.x,drawPos.y}, (Vector2) {drawPos.x + box->width, drawPos.y},box->size,color);
    }
    else if(box->isImage){
        Rectangle src = (Rectangle){0,0,(float)box->texPtr->width,(float)box->texPtr->height};
        Rectangle dst = (Rectangle){drawPos.x, drawPos.y, box->imgW, box->imgH};
        rmviRotateTexture(*box->texPtr, src, dst, (Vector2){0,0}, 0.0f, WHITE);
    }
}

// la dernière lettre d'un mot s'écrit en commencant l'audio du suivant à cause des erreurs d'arrondi :/
void rmviDrawRenderBoxesAnimed(RenderBox *boxes,int boxCount,Vector2 basePos,Font font,float fontSize,float spacing, Color color, AnimText* anim){
    anim->animTime += 1.0f/50.0f;
    float timeAccum = 0.0f;
    int lastBoxEnd = anim->boxEnd;
    for (int i = 0; i < boxCount; i++){
        // the time to write is lower than the time we have
        float boxTime = boxWriteTime(boxes[i]);
        if (timeAccum + boxTime <= anim->animTime) {
            rmviDrawRenderBox(&boxes[i],basePos,font,fontSize,spacing,color);
            timeAccum += boxTime;
        }
        else{
            AnimText subAnim = {0};
            subAnim.animTime = anim->animTime-timeAccum;
            rmviDrawRenderBoxAnimed(&boxes[i],basePos,font,fontSize,spacing,color,&subAnim);
            anim->letterCount = subAnim.letterCount;
            anim->boxEnd = (subAnim.boxEnd > 0) ? subAnim.boxEnd : i;
            break;
        }
        
    }
    anim->hasChanged = (lastBoxEnd != anim->boxEnd);
}

void rmviDrawRenderBoxesCenteredAnimed(RenderBox *boxes,int boxCount,Vector2 basePos,Font font,float fontSize,float spacing, Color color, AnimText* anim){
    float *listWidth; 
    int linesCount =rmviCalcWidthLine(boxes, boxCount,&listWidth);
    Vector2 centeredPos = basePos;
    centeredPos.y = basePos.y - rmviCalcHeightTotal(boxes, boxCount);
    int endLine = 0;
    anim->animTime += 1.0f/50.0f;
    float timeAccum = 0.0f;
    int lastBoxEnd = anim->boxEnd;
    for (int i = 0; i < boxCount; i++){
        centeredPos.x = basePos.x - listWidth[endLine]/2;
        if(boxes[i].isEndLine) endLine ++;
        // the time to write is lower than the time we have
        float boxTime = boxWriteTime(boxes[i]);
        if (timeAccum + boxTime <= anim->animTime) {
            rmviDrawRenderBox(&boxes[i],centeredPos,font,fontSize,spacing,color);
            timeAccum += boxTime;
        }
        else{
            AnimText subAnim = {0};
            subAnim.animTime = anim->animTime-timeAccum;
            rmviDrawRenderBoxAnimed(&boxes[i],centeredPos,font,fontSize,spacing,color,&subAnim);
            anim->letterCount = subAnim.letterCount;
            anim->boxEnd = (subAnim.boxEnd > 0) ? subAnim.boxEnd : i;
            break;
        }
        
    }
    anim->hasChanged = (lastBoxEnd != anim->boxEnd);
    free(listWidth);
}


int rmviCalcWidthLine(RenderBox *boxes, int boxCount, float **outListWidth){
    float *listWidth = NULL;
    float width = 0.0f;
    int lineCap = 0;
    int countLine = 0;

    if (outListWidth) *outListWidth = NULL;
    if (!boxes || boxCount <= 0 || !outListWidth) return 0;

    for (int i = 0; i < boxCount; i++) {
        width += boxes[i].width;

        if (boxes[i].isEndLine || i == (boxCount - 1)) {
            RMVI_PUSH(listWidth, countLine, lineCap, float, min(width, RATIO_LEN_TEXT));
            width = 0.0f;
        }
    }

    *outListWidth = listWidth;
    return countLine;
}

int rmviCalcHeightLine(RenderBox *boxes, int boxCount, float **outListHeight){
    float *listHeight = NULL;
    float height = 0.0f;
    int lineCap = 0;
    int countLine = 0;

    if (outListHeight) *outListHeight = NULL;
    if (!boxes || boxCount <= 0 || !outListHeight) return 0;
    for (int i = 0; i < boxCount; i++) {
        height = max(height, boxes[i].height);
        if (boxes[i].isEndLine || i == (boxCount - 1)) {
            if (countLine > 0) height += INTERLIGNE * boxes[i].size;
            RMVI_PUSH(listHeight, countLine, lineCap, float, height);
            height = 0.0f;
        }
    }

    *outListHeight = listHeight;
    return countLine;
}

float rmviCalcHeightTotal(RenderBox *boxes, int boxCount){
    float *listHeight = NULL;
    int countLine = rmviCalcHeightLine(boxes, boxCount, &listHeight);
    float totalHeight = 0.0f;
    for (int i = 0; i < countLine; i++){
        totalHeight += listHeight[i];
    }
    free(listHeight);
    return totalHeight;
}


void rmviGetCustomFont(const char *path, float fontSize) {
    int codepoints[256];
    int count = 0;
    // ASCII imprimables (espace -> ~)
    for (int i = 32; i <= 126; i++) {
        codepoints[count++] = i;
    }
    // Lettres accentuées minuscules
    codepoints[count++] = 0x00E9; // é
    codepoints[count++] = 0x00E8; // è
    codepoints[count++] = 0x00EA; // ê
    codepoints[count++] = 0x00EB; // ë
    codepoints[count++] = 0x00E0; // à
    codepoints[count++] = 0x00E2; // â
    codepoints[count++] = 0x00E4; // ä
    codepoints[count++] = 0x00F9; // ù
    codepoints[count++] = 0x00FB; // û
    codepoints[count++] = 0x00FC; // ü
    codepoints[count++] = 0x00F4; // ô
    codepoints[count++] = 0x00F6; // ö
    codepoints[count++] = 0x00EE; // î
    codepoints[count++] = 0x00EF; // ï
    codepoints[count++] = 0x00E7; // ç
    // Lettres accentuées majuscules
    codepoints[count++] = 0x00C9; // É
    codepoints[count++] = 0x00C8; // È
    codepoints[count++] = 0x00CA; // Ê
    codepoints[count++] = 0x00CB; // Ë
    codepoints[count++] = 0x00C0; // À
    codepoints[count++] = 0x00C2; // Â
    codepoints[count++] = 0x00C4; // Ä
    codepoints[count++] = 0x00D9; // Ù
    codepoints[count++] = 0x00DB; // Û
    codepoints[count++] = 0x00DC; // Ü
    codepoints[count++] = 0x00D4; // Ô
    codepoints[count++] = 0x00D6; // Ö
    codepoints[count++] = 0x00CE; // Î
    codepoints[count++] = 0x00CF; // Ï
    codepoints[count++] = 0x00C7; // Ç
    // Symboles mathématiques/grecs
    
    codepoints[count++] = 0x03B1; // α
    codepoints[count++] = 0x03B2; // β
    codepoints[count++] = 0x03B3; // γ
    codepoints[count++] = 0x03B4; // δ
    codepoints[count++] = 0x03B5; // ε
    codepoints[count++] = 0x03B6; // ζ
    codepoints[count++] = 0x03B7; // η
    codepoints[count++] = 0x03B8; // θ
    codepoints[count++] = 0x03B9; // ι
    codepoints[count++] = 0x03C0; // π
    codepoints[count++] = 0x03C1; // ρ
    codepoints[count++] = 0x03C3; // σ
    codepoints[count++] = 0x03C4; // τ
    codepoints[count++] = 0x03C6; // φ
    codepoints[count++] = 0x03C7; // χ
    codepoints[count++] = 0x03C8; // ψ
    codepoints[count++] = 0x03C9; // ω
    codepoints[count++] = 0x03BD; // ν
    
    // symboles mathématique supplémentaires
    codepoints[count++] = 0x2264; // ≤
    codepoints[count++] = 0x2265; // ≥
    codepoints[count++] = 0x221E; // ∞
    codepoints[count++] = 0x2260; // ≠
    // les lettres grecques majuscules
    codepoints[count++] = 0x0393; // Γ
    codepoints[count++] = 0x0394; // Δ
    codepoints[count++] = 0x0398; // Θ
    codepoints[count++] = 0x03A0; // Π
    codepoints[count++] = 0x03A3; // Σ
    codepoints[count++] = 0x03A6; // Φ
    codepoints[count++] = 0x03A8; // Ψ
    codepoints[count++] = 0x03A9; // Ω
    codepoints[count++] = 0x222B; // 
    // Chargement
    mathFont = LoadFontEx(path, fontSize, codepoints, count);
}

// helpers UTF-8
static int utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1; // fallback
}

// copy next UTF-8 codepoint from src into dst (null-terminated).
// dst must be large enough (>=5 bytes is safe for up to 4-byte UTF-8).
// returns number of bytes consumed from src.
static int copy_utf8_char(char *dst, const char *src) {
    unsigned char c = (unsigned char)src[0];
    int len = utf8_char_len(c);
    // validate roughly: don't read past string end
    for (int i = 0; i < len; i++) {
        if (src[i] == '\0') { len = i; break; }
        dst[i] = src[i];
    }
    dst[len] = '\0';
    return (len > 0) ? len : 1;
}


void rmviWriteLatexLeftCenteredClassic(const char *latex, Vector2 *position) {
    rmviWriteLatexLeftCentered(latex, position, SIZE_TEXT, SIZE_SPACING, WHITE, mathFont);
}
// on va centrée en x mais pas en y
void rmviWriteLatexLeftCentered(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font) {
    exit(EXIT_FAILURE);
}
// Dessine le text comme si on ecrivait en latex mais il faut remplacer les backslash par /
void rmviWriteLatex(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font) {
    Token tokens[512];
    int tokenCount = rmviTokenizeLatex(latex,tokens, 512);
    RenderBox boxes[256];
    int boxCount = rmviBuildRenderBoxes(tokens, tokenCount, boxes, font, sizeText, spacing);
    rmviDrawRenderBoxes(boxes, boxCount, *position, font, sizeText, spacing, color);
}

int isBlankLine(const char *line){
    while (*line)
    {
        if (*line != ' ' && *line != '\t' &&
            *line != '\n' && *line != '\r')
            return 0;
        line++;
    }
    return 1;
}

Lecture rmviGetLecture(const char *path){
    Lecture lecture;
    lecture.path = path;
    lecture.content = NULL;
    lecture.oldParagraph = -1;
    lecture.currentParagraph = 0;
    return lecture;
}

char *readParagraphFromFile(const char *path, int targetParagraph){
    FILE *file = fopen(path, "r");
    if (!file) return NULL;
    char line[MAX_LINE];
    char temp[MAX_PARAGRAPH];
    temp[0] = '\0';

    int currentParagraph = 0;
    int hasContent = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (isBlankLine(line))
        {
            if (hasContent)
            {
                if (currentParagraph == targetParagraph)
                {
                    fclose(file);
                    char *result = (char*)malloc(strlen(temp) + 1);
                    if (!result) return NULL;
                    strcpy(result, temp);
                    return result;
                }

                temp[0] = '\0';
                hasContent = 0;
                currentParagraph++;
            }
        }
        else{
            hasContent = 1;
            size_t need = strlen(temp) + strlen(line) + 1;
            if (need < MAX_PARAGRAPH)
                strcat(temp, line);
        }
    }
    if (hasContent && currentParagraph == targetParagraph)
    {
        char *result = (char*)malloc(strlen(temp) + 1);
        if (!result) { fclose(file); return NULL; }
        strcpy(result, temp);
        fclose(file);
        return result;
    }

    fclose(file);
    return NULL;
}

int lectureLoadParagraph(Lecture *lecture, int paragraphIndex)
{
    if (!lecture || !lecture->path) return 0;

    char *p = readParagraphFromFile(lecture->path, paragraphIndex);
    if (!p) return 0;

    // update state
    lecture->oldParagraph = lecture->currentParagraph;
    lecture->currentParagraph = paragraphIndex;

    // replace content
    if (lecture->content) free(lecture->content);
    lecture->content = p;

    return 1;
}

int readScenario(Lecture *lecture){
    if (!lecture || !lecture->path) return 0;
    if (lecture->oldParagraph != lecture->currentParagraph) return lectureLoadParagraph(lecture, lecture->currentParagraph);
    return 0;
}