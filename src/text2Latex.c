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
    return (c == '+' || c == '-' || c == '*' || c == '='|| c == '.' || c ==':' || c == ',' );
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
                while (( isAlphaNum(latex[i]) || latex[i] == '(' || latex[i] == ')' ) && j < TOKEN_TEXT_MAX - 1) {
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
            while (isAlphaNum(latex[i]) && j < TOKEN_TEXT_MAX - 1) {
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
    RenderBox children[32];
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
    RenderBox children[32];
    while(depthContinue(&depthSub)){
        if(depthUpdate(&depthSub,tokens,index)){
        }
        else{
            RenderBox child = rmviMain2Box(tokens, tokenCount, font, fontSize,spacing, index);
            children[childCount++] = child;
            box.width += child.width;                       // ici à modifier ?
            box.height = max(box.height, child.height);
        }
    }
    // ici on peut metre un truc du style si un seul enfant
    box.items = MemAlloc(sizeof(RenderBox) * childCount);
    memcpy(box.items, children, sizeof(RenderBox) * childCount);
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
            rmviLineSkip(&cursor,INTERLIGNE, box->size, height);
            box->height += height + INTERLIGNE*box->size;
            height = box->size;
        }
        if( box->items[i].token && box->items[i].token->type == TOKEN_DISPLACE){
            rmviLineSkip(&cursor,INTERLIGNE_ITEM, box->size, box->size);
            box->items[i].pos = Vector2Add(box->items[i].pos,cursor);
            box->height += box->items[i].height ;
            cursor.y += box->height;
            cursor.x += 0;
        }
        // if passage à la ligne ?
        else {
            box->items[i].pos.x += cursor.x;
            box->items[i].pos.y += cursor.y;
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
    RenderBox children[32];
    box.size = fontSize;
    int childCount = 0;
    if(tokens[*index].type == TOKEN_RBRACKET){
        // cas ou item[qqch]
    }
    else{
        RenderBox child = {0};
        child.token = &tiretToken;
        child.width = rmviMeasureToken(child.token,font,fontSize,spacing,true).x;
        child.size = fontSize;
        children[childCount++] = child;
        // on remplace le premier box par un tiret -
    }
    while(tokens[*index].type != TOKEN_END_ITEMIZE && tokens[*index].type != TOKEN_ITEM){
        ERROR_OUT_OF_INDEX(*index, tokenCount);
        RenderBox child = {0};
        child = rmviMain2Box(tokens,tokenCount,font,fontSize,spacing,index);
        children[childCount++] = child;

    }
    box.items = MemAlloc(sizeof(RenderBox) * childCount);
    memcpy(box.items, children, sizeof(RenderBox) * childCount);
    box.itemCount = childCount;
    rmviFixChildPositionNext(&box);
    return box;
}

RenderBox rmviBuildBeginItemize(Token *tokens,int tokenCount,int *index,Font font,float fontSize,float spacing){
    RenderBox box = {0};
    RenderBox children[32];
    int childCount = 0;
    Depth depth = {0};
    depthUpdate(&depth,tokens,index);
    box.size = fontSize;
    box.token = &displaceToken;
    // ici parametre à imposer
    box.pos = (Vector2) {RATIO_INDENT_BEGIN*fontSize , 0 };
    while (depthContinue(&depth)){
        if(depthUpdate(&depth,tokens,index)){
        }
        else if(tokens[*index].type == TOKEN_ITEM){
            (*index)++; //skip begin
            RenderBox child ={0};
            child = rmviBuildItem(tokens, tokenCount,index,font, fontSize,spacing);
            children[childCount++] = child;
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
   
    else if (tokens[*index].type == TOKEN_BEGIN_ITEMIZE){
        box = rmviBuildBeginItemize(tokens, tokenCount,index,font, fontSize,spacing);
        rmviFixChildPositionUnder(&box,INTERLIGNE_ITEM);
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
    else if(tokens[*index].type == TOKEN_NEXTLINE){
        
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
    int endLineCount = 0;
    int oldIndex = 0;
    float height = fontSize;
    RenderBox box = {0};
    for (int index = 0; index < tokenCount; ){
        height = max(height,box.height);
        if(cursor.x > RATIO_LEN_TEXT){
            rmviLineSkip(&cursor,INTERLIGNE, fontSize,height);
            endLineCount = count;
            height = fontSize;
        }
        if(tokens[index].type == TOKEN_NEXTLINE){
            rmviLineSkip(&cursor,INTERLIGNE, fontSize,height);
            (index)++;
            endLineCount = count;
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

void rmviDrawRenderBoxes(RenderBox *boxes,int count,Vector2 basePos,Font font,float fontSize,float spacing,Color color){
    for (int i = 0; i < count; i++){
        rmviDrawRenderBox(&boxes[i],basePos,font,fontSize,spacing,color);
    }
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

// Calcule la longeur d'un texte entre accolade, on peut en ouvrir et en fermer d'autre en interne
// Permetra la recursivité ex : {{}} = 2
// retoune la position de la dernière accolade fermante
int rmviLenSymbole(const char *latex,int start, char openChar, char closeChar){
    int count = 1;
    int end = start;
    while(count > 0 && latex[end] !='\0'){
        if (latex[end] == openChar) count++;
        else if (latex[end] == closeChar) count--;
        end++;
    }
    end--;
    return end;
}

int rmviLenAccolade(const char *latex,int start){
    return rmviLenSymbole(latex, start, '{', '}');
}
// Calcule la longeur d'un texte entre parenthèse, on peut en ouvrir et en fermer d'autre en interne
// Permetra la recursivité ex : (()) = 2
int rmviLenParenthese(const char *latex,int start){
    return rmviLenSymbole(latex, start, '(', ')');
}

int rmviLenCrochets(const char *latex,int start){
    return rmviLenSymbole(latex, start, '[', ']');
}

int rmviLenItem(const char *latex, int start) {
    int i = start;
    int depth = 0; // profondeur d'imbrication des itemize
    while (latex[i] != '\0') {
        if (strncmp(&latex[i], "/begin(itemize)", 15) == 0) {
            depth++;
            i += 15;
        }
        else if (strncmp(&latex[i], "/end(itemize)", 13) == 0) {
            if (depth == 0) {
                // Fin de l'itemize courant
                return i - start;
            } else {
                depth--;
                i += 13;
            }
        }
        else if (strncmp(&latex[i], "/item", 5) == 0 && depth == 0) {
            // Prochain item au même niveau
            return i - start;
        }
        else {
            i++;
        }
    }
    // Si on atteint la fin de la string
    return i - start;
}

// Calcul la longeur que va prendre le text
float rmviCalcTextWidth(const char *latex, Font font, float sizeText, float spacing){
    float width = 0.0f;
    float max_width = 0.0f;
    int i = 0;
    while (latex[i] != '\0') {
        // exposant simple
        if (latex[i] == '^' && latex[i+1] != '\0') {
            if (latex[i+1] == '{') {
                // Trouver la fin de l'accolade
                int start = i + 2; // après "_{"
                int end = rmviLenAccolade(latex, start);
                int len = end - start;
                char subBuf[64]; // buffer suffisant
                strncpy(subBuf, &latex[start], len);
                subBuf[len] = '\0';
                float subSize = sizeText * RATIO_INDEX; 
                float textWidth = rmviCalcTextWidth(subBuf, font, subSize, spacing);
                width += textWidth;
                i =  end + 1 ; // on saute tout jusqu'à '}'
            } else {
                char subBuf[8];
                int consumed = copy_utf8_char(subBuf, &latex[i+1]);
                float subSize = sizeText * RATIO_INDEX;
                Vector2 ts = MeasureTextEx(font, subBuf, subSize, spacing);
                width += ts.x;
                i += 1 + consumed;
            }
        }
        // indice simple
        else if (latex[i] == '_' && latex[i+1] != '\0') {
            if (latex[i+1] == '{') {
                // Trouver la fin de l'accolade
                int start = i + 2; // après "_{"
                int end = rmviLenAccolade(latex, start);
                int len = end - start;
                char subBuf[64]; // buffer suffisant
                strncpy(subBuf, &latex[start], len);
                subBuf[len] = '\0';
                float subSize = sizeText * RATIO_INDEX; 
                float textWidth = rmviCalcTextWidth(subBuf, font, subSize, spacing);
                width += textWidth;
                i =  end + 1 ; // on saute tout jusqu'à '}'
            } else {
                // simple caractère après '_'
                char subBuf[8];
                int consumed = copy_utf8_char(subBuf, &latex[i+1]);
                float subSize = sizeText * RATIO_INDEX;
                Vector2 ts = MeasureTextEx(font, subBuf, subSize, spacing);
                width += ts.x;
                i += 1 + consumed;
            }
        }
        // commande spéciale
        else if (latex[i] == '/' && latex[i+1] != '\0') {
            if (strncmp(&latex[i+1], "nu", 2) == 0) {
                Vector2 ts = MeasureTextEx(font, "ν", sizeText, spacing);
                width += ts.x;
                i += 3; // /nu
            }
            RMVI_MEASURE_WIDTH_SYMBOLE("int", "∫")
            RMVI_MEASURE_WIDTH_SYMBOLE("delta", "δ")
            RMVI_MEASURE_WIDTH_SYMBOLE("Delta", "Δ")
            RMVI_MEASURE_WIDTH_SYMBOLE("alpha", "α")
            RMVI_MEASURE_WIDTH_SYMBOLE("beta", "β")
            RMVI_MEASURE_WIDTH_SYMBOLE("gamma", "γ")
            RMVI_MEASURE_WIDTH_SYMBOLE("Gamma", "Γ")
            RMVI_MEASURE_WIDTH_SYMBOLE("theta", "θ")
            RMVI_MEASURE_WIDTH_SYMBOLE("Theta", "Θ")
            RMVI_MEASURE_WIDTH_SYMBOLE("pi", "π")
            RMVI_MEASURE_WIDTH_SYMBOLE("Pi", "Π")
            RMVI_MEASURE_WIDTH_SYMBOLE("sigma", "σ")
            RMVI_MEASURE_WIDTH_SYMBOLE("Sigma", "Σ")
            RMVI_MEASURE_WIDTH_SYMBOLE("phi", "φ")
            RMVI_MEASURE_WIDTH_SYMBOLE("Phi", "Φ")
            RMVI_MEASURE_WIDTH_SYMBOLE("psi", "ψ")
            RMVI_MEASURE_WIDTH_SYMBOLE("Psi", "Ψ")
            RMVI_MEASURE_WIDTH_SYMBOLE("omega", "ω")
            RMVI_MEASURE_WIDTH_SYMBOLE("Omega", "Ω")
            RMVI_MEASURE_WIDTH_SYMBOLE("neq", "≠")
            RMVI_MEASURE_WIDTH_SYMBOLE("leq", "≤")
            RMVI_MEASURE_WIDTH_SYMBOLE("geq", "≥")
            RMVI_MEASURE_WIDTH_SYMBOLE("infinity", "∞")
            else if (strncmp(&latex[i+1],"sum",3) == 0){
                Vector2 ts = MeasureTextEx(font, "∑", BIG*sizeText, spacing);
                width += ts.x;
                i += 4; // skip "/sum"
            }
            else if(strncmp(&latex[i+1], "bar",3) == 0){
                i += 4; // skip "/bar"
                while (latex[i] == '{' || latex[i] == ' ') i++;
                int start = i;
                i = rmviLenAccolade(latex, start);
                int len = i - start;
                char *barText = (char*)malloc(len + 1);
                memcpy(barText, &latex[start], len);
                barText[len] = '\0';
                width += rmviCalcTextWidth(barText, font, sizeText, spacing);
                i++;
                free(barText);
            }
            else if (strncmp(&latex[i+1], "exp{", 4) == 0) {
                i += 5; // saute "/exp{"
                int start = i;
                while (latex[i] != '}' && latex[i] != '\0') i++;
                int len = i - start;
                float subSize = sizeText * RATIO_INDEX;
                if (len > 0) {
                    char *exp = (char*)malloc(len+1);
                    memcpy(exp, &latex[start], len);
                    exp[len] = '\0';
                    width += rmviCalcTextWidth(exp, font, subSize, spacing);
                    free(exp);
                }
                if (latex[i] == '}') i++;
            }
            else if(strncmp(&latex[i+1], "begin(", 6) == 0) {
                char *env = rmviGetEnv(&i, latex);
                width += rmviCalcBeginWidth(env, i, latex, font, sizeText, spacing);
                i += rmviLenBegin(latex, i, env);
            }
            else if( latex[i+1] == '/'){
                if(max_width < width) max_width = width;
                width = 0;
                i += 2; // skip "/"
            }
            else if(strncmp(&latex[i+1], "loadImage{",10) == 0){
                // on ignore pour le calcul de largeur
                i += 11; // après "/loadImage{"
                int start = i;
                while(latex[i] != '}' && latex[i] != '\0') i++;
                if(latex[i] == '}') i++;
            }
            else if(strncmp(&latex[i+1], "frac",5) == 0){
                i += 5; // après "/frac"
                if (latex[i] == '{') i += 1; // après '{'
                int start = i;
                i = rmviLenAccolade(latex, start);
                int len = i - start;
                char *numerator = malloc(len + 1);
                memcpy(numerator, &latex[start], len);
                numerator[len] = '\0';
                if (latex[i] == '}') i++;
                if (latex[i] == '{'){
                    i += 1; // après '{'
                    start = i;
                    i = rmviLenAccolade(latex, start);
                    len = i - start;
                    char *denominator = malloc(len + 1);
                    memcpy(denominator, &latex[start], len);
                    denominator[len] = '\0';
                    if (latex[i] == '}') i++;
                    // maintenant on a numerator et denominator
                    width += max( rmviCalcTextWidth(numerator, font, sizeText, spacing),rmviCalcTextWidth(denominator, font, sizeText, spacing));
                    free(denominator);
                    free(numerator);
                }
            }
            else {
                Vector2 ts = MeasureTextEx(font, "/", sizeText, spacing);
                width += ts.x;
                i++;
            }
        }
        else if (latex[i] == '\n') {
            if(max_width < width) max_width = width;
            width = 0;
            i++;
        }

        // caractère normal
        else {
            char buf[8];
            int consumed = copy_utf8_char(buf, &latex[i]);
            Vector2 ts = MeasureTextEx(font, buf, sizeText, spacing);
            width += ts.x;
            i += consumed;
        }
    }
    if(max_width < width) max_width = width;
    return max_width;
}
char *rmviGetEnv(int *i, const char *latex){
    *i += 7; // position après "/begin("
    int start = *i;
    int end = rmviLenParenthese(latex, start);
    int len = end - start;
    char *env = (char*)malloc(len + 1);
    strncpy(env, &latex[start], len);
    env[len] = '\0';
    *i = end + 1; // i placé juste après '}'
    return env;
}

float rmviCalcBeginWidth(const char *env, int start, const char *latex, Font font, float sizeText, float spacing) {
    float widthMemory = 0;
    if (strcmp(env, "itemize") == 0) {
        const char *p = strstr(latex + start, "/item"); // commencer à partir de start
        while (p && (strncmp(p, "/end(itemize)", 13) != 0)) {
            p += 5; // skip "/item"
            
            // Récupérer le texte jusqu’au prochain /item ou /end(itemize)
            const char *next = strstr(p, "/item");
            const char *endEnv = strstr(p, "/end(itemize)");
            if (!next || (endEnv && endEnv < next)) {
                next = endEnv;
            }
            int len = next ? (next - p) : strlen(p);

            char buf[256];
            strncpy(buf, p, len);
            buf[len] = '\0';

            // Calcul largeur de CET item
            float width = 0;
            width += rmviCalcTextWidth("-", font, sizeText, spacing); // puce
            width += sizeText * (RATIO_INDENT +1); // espace après la puce
            width += rmviCalcTextWidth(buf, font, sizeText, spacing);

            if (width > widthMemory) {
                widthMemory = width;
            }

            p = next; // avancer
        }
    } 
    else if(strcmp(env, "equation") == 0) {
        // Pour l'instant, on traite equation comme du texte normal
        State prevState = state;
        state.interline = state.interline * MATH_INTERLINE_RATIO;
        widthMemory = rmviCalcTextWidth(latex + start, font, state.sizeText, state.spacing);
        state.interline = prevState.interline;
    }
    else {
        printf("rmviCalcBeginWidth = bug (env=%s)\n", env);
    }

    return widthMemory;
}


float rmviCalcBeginHeight(const char *env, int start, const char *latex, Font font, float sizeText, float spacing, bool isNested) {
    float height = 0.0f;
    if (strcmp(env, "itemize") == 0) {
        height += rmviCalcItemizeHeight(latex, start, font, sizeText, spacing, isNested);
    }
    else if(strcmp(env, "equation") == 0) {
        height += rmviCalcEquationHeight(latex + start, font, sizeText, spacing);
        
    }
    else {
        printf("rmviCalcBeginHeight = bug (env=%s)\n", env);
    }
    return height;
}
float rmviCalcEquationHeight(const char *latex, Font font, float sizeText, float spacing) {
    float height = 0.0f;
    int i = 0;
    while (latex[i] != '\0') {
        // retour ligne → incrémenter hauteur
        if (latex[i] == '/'){
            if(latex[i+1] == '/'){
                height += sizeText * INTERLIGNE;
                i += 2;
                while (latex[i] == '\n' || latex[i] == ' ') i++; // on compte pas deux fois le saut de ligne    
            }
            else if( strncmp(&latex[i+1], "frac",4) == 0){
                // on ignore pour le calcul de hauteur
                i += 5; // après "/frac"
                if (latex[i] == '{') i += 1; // après '{'
                int start = i;
                i = rmviLenAccolade(latex, start);
                char *numerator = malloc((size_t)(i - start + 1));
                memcpy(numerator, &latex[start], (size_t)(i - start));
                numerator[i - start] = '\0';
                height += rmviCalcTextHeight(numerator, font, sizeText, spacing, false);
                free(numerator);
                if (latex[i] == '}') i++;
                if (latex[i] == '{'){
                    i += 1; // après '{'
                    start = i;
                    i = rmviLenAccolade(latex, start);
                    char *denominator = malloc((size_t)(i - start + 1));
                    memcpy(denominator, &latex[start], (size_t)(i - start));
                    denominator[i - start] = '\0';
                    height += rmviCalcTextHeight(denominator, font, sizeText, spacing, false);
                    free(denominator);
                    if (latex[i] == '}') i++;
                }
            }
            else {
                // unknown /command: ignore it for height calculation
                i++;
            }
        }
        else {
            i++;
        }
    }
    return max(sizeText * INTERLIGNE_ITEM, height);
}
float rmviCalcItemizeHeight(const char *body, int start, Font font, float sizeText, float spacing, bool isNested) {
    const float lineHeight = sizeText * INTERLIGNE_ITEM;
    const float bulletGap  = 10.0f;
    float height = lineHeight; // Pour commencer
    for (int i = 0; body[i] != '\0'; ) {
        if(strncmp(&body[i], "/item", 5) == 0){
            i += 5; // saute "/item"
            if(strncmp(&body[i], "[", 1) == 0) {
                i++; // saute "["
                int end = rmviLenCrochets(body, i);
                i = end + 1; // i placé juste après ']'
            }
            int itemLength = rmviLenItem(body, i);
            const char *bodyStart = body + i;
            const char *bodyEnd   = bodyStart + itemLength;
            // On fabrique une string indépendante pour le body
            char *body_item = (char*)malloc((size_t)(itemLength + 1));
            memcpy(body_item, bodyStart, (size_t)itemLength);
            body_item[itemLength] = '\0';
            // ici il faudrait uniquement fournir ce qui suit après le item
            height +=rmviCalcTextHeight(body_item, font, sizeText, spacing, true);
            i += itemLength;
        }
        else if(strncmp(&body[i], "/begin(", 7) == 0){
            char *env = rmviGetEnv(&i, body);
            int bodyLen  = rmviLenBegin(body, i, env);
            const char *bodyBeginStart = body + i;
            // On fabrique une string indépendante pour le body
            char *bodyBegin = (char*)malloc((size_t)(bodyLen + 1));
            memcpy(bodyBegin, bodyBeginStart, (size_t)bodyLen);
            bodyBegin[bodyLen] = '\0';
            height += rmviCalcBeginHeight(env, i, bodyBegin, font, sizeText, spacing, true);
            i += bodyLen;
            free(bodyBegin);
        }
        /*else if(strncmp(&body[i], "/end(itemize)", 13) == 0){
            // ici ca devrait être fini du coup ?
            i += 13; // saute "/end(itemize)"
            pos->x -= initStart.x; // reset x to initial start
            pos->y += lineHeight; // saut de ligne après la fin de l'itemize
        }*/
        else{
            i++;
        }
    }
    return height;
}


float rmviCalcTextHeight(const char *latex, Font font, float sizeText, float spacing, bool isNested) {
    float height = sizeText * INTERLIGNE;
    int i = 0;
    while (latex[i] != '\0') {
        // retour ligne → incrémenter hauteur
        if (latex[i] == '/'){
            if (strncmp(&latex[i+1], "begin(", 6) == 0) {
                char *env = rmviGetEnv(&i, latex);
                height += rmviCalcBeginHeight(env, i, latex, font, sizeText, spacing, isNested);
                i += rmviLenBegin(latex, i, env);
            }
            else if(latex[i+1] == '/'){
                height += sizeText * INTERLIGNE;
                i += 2;
                while (latex[i] == '\n' || latex[i] == ' ') i++; // on compte pas deux fois le saut de ligne
                
            }
            else {
                // unknown /command: ignore it for height calculation
                i++;
            }
        }
        else if (latex[i] == '\n') {
            height += sizeText * INTERLIGNE;
            i++;
        }
        else {
            i++;
        }
    }
    return height;
}

void rmviWriteLatexLeftCenteredClassic(const char *latex, Vector2 *position) {
    rmviWriteLatexLeftCentered(latex, position, SIZE_TEXT, SIZE_SPACING, WHITE, mathFont);
}
// on va centrée en x mais pas en y
void rmviWriteLatexLeftCentered(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font) {
    if (!latex || !position) return;
    Vector2 initpos = *position;
    float textWidth  = rmviCalcTextWidth(latex, font, sizeText, spacing);
    float textHeight = rmviCalcTextHeight(latex, font, sizeText, spacing, false);
    // position de départ centrée
    position->x -= textWidth  / 2.0f;
    //position->y -= textHeight / 2.0f;
    // écriture
    rmviWriteLatex(latex, position, sizeText, spacing, color, font);
    position->x = initpos.x;
    position->y = position->y + sizeText * INTERLIGNE;
}
// Dessine le text comme si on ecrivait en latex mais il faut remplacer les backslash par /
void rmviWriteLatex(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font) {
    if (!latex || !position) return;
    Vector2 initPos = *position;

    // You might want x,y to be top-left or center — adapt before calling.
    Vector2 ts;
    for (int i = 0; latex[i] != '\0'; ) {
        // handle /commands like /nu or /exp{...}
        //switch case
        if(latex[i] == '/' && latex[i+1] == '/') {
                position->y += sizeText * INTERLIGNE; // interline, ajustable
                position->x = initPos.x;       // reset to line start (you can pre-center per-line if needed)
                if(latex[i+2] == ' ') i++;
                i += 2;
        }
        // newline
        else if (latex[i] == '\n') {
            position->y += sizeText * INTERLIGNE; // interline, ajustable
            position->x = initPos.x;       // reset to line start (you can pre-center per-line if needed)
            i++;
        }

        else if (latex[i] == '/' && latex[i+1] != '\0'){
            int k = 0;
            const char *kstart = &latex[i + 1];
            while (kstart[k] != ' ' && kstart[k] != '\0' &&
                kstart[k] != '/' && kstart[k] != '{' && kstart[k] != '(' &&
                kstart[k] != '}' && kstart[k] != ')'  && kstart[k] != '\n' &&
                kstart[k] != '^' && kstart[k] != '_')
            {
                k++;
            }

            bool handled = false;
            Vector2 ts = {0};
            switch (k)
            {   
                case 0:
                    DrawTextEx(font, "/", *position, sizeText, spacing, color);
                    ts = MeasureTextEx(font, "/", sizeText, spacing);
                    handled = true;
                    break;
                case 1:
                    char tmp[16];
                    snprintf(tmp, sizeof(tmp), "%.*s", k, kstart);
                    DrawTextEx(font, tmp, *position, sizeText, spacing, color);
                    ts = MeasureTextEx(font, tmp, sizeText, spacing);
                    handled = true;
                    break;
                
                case 2:
                    if (strncmp(kstart, "nu", k) == 0){
                        DrawTextEx(font, "ν", *position, sizeText, spacing, color);
                        ts = MeasureTextEx(font, "ν", sizeText, spacing);
                        handled = true;
                    }
                    break;
                case 3:
                    if (strncmp(kstart, "int", k) == 0)
                    {
                        DrawTextEx(font, "∫", *position, sizeText, spacing, color);
                        ts = MeasureTextEx(font, "∫", sizeText, spacing);
                        handled = true;
                    }
                    else if(strncmp(kstart,"neq",k) == 0){
                        DrawTextEx(font, "≠", *position, sizeText, spacing, color);
                        ts = MeasureTextEx(font, "≠", sizeText, spacing);
                        handled = true;
                    }
                    else if(strncmp(kstart,"sum",k) == 0){
                        initPos = *position;
                        position->y -= (sizeText *BIG - sizeText)/2.0f;
                        DrawTextEx(font, "Σ", *position, BIG * sizeText, spacing, color);
                        ts = MeasureTextEx(font, "Σ", BIG * sizeText, spacing);
                        // traitement ^_ après
                        if(latex[i+4] == '^' || latex[i+4] == '_'){
                            int j = i + 4;
                            while(latex[j] == '^' || latex[j] == '_'){
                                bool isSuper = (latex[j] == '^');
                                j++;
                                if (latex[j] == '{'){
                                    j++; // après la {
                                    int start = j;
                                    j = rmviLenAccolade(latex, start);
                                    int len = j - start;
                                    if (len > 0){
                                        char *sub = malloc(len + 1);
                                        memcpy(sub, &latex[start], len);
                                        sub[len] = '\0';
                                        float subSize = sizeText * 0.7f;
                                        Vector2 subPos;
                                        if (isSuper){
                                            subPos = (Vector2){ position->x + ts.x*0.7, position->y - BIG * sizeText };
                                        }
                                        else{
                                            subPos = (Vector2){ position->x + ts.x * 0.7, position->y + BIG * sizeText };
                                        }
                                        rmviWriteLatexLeftCentered(sub, &subPos, subSize, spacing, color, font);
                                        free(sub);
                                    }
                                    if (latex[j] == '}') j++;
                                }
                                else{
                                    char subBuf[8];
                                    int consumed = copy_utf8_char(subBuf, &latex[j]);
                                    float subSize = sizeText * 0.7f;
                                    Vector2 subPos;
                                    if (isSuper){
                                            subPos = (Vector2){ position->x + ts.x * 0.7, position->y - BIG * sizeText };
                                        }
                                        else{
                                            subPos = (Vector2){ position->x + ts.x * 0.7, position->y + BIG * sizeText };
                                        }
                                    rmviWriteLatexLeftCentered(subBuf, &subPos, subSize, spacing, color, font);
                                    j += consumed;
                                }
                            }
                            i = j - k -1;
                        }
                        handled = true;
                    }
                    else if (strncmp(kstart, "exp", k) == 0){
                        handled = true;
                        if (latex[i+4] == '{'){
                            i += 5; // après "/exp{"
                            int start = i;
                            i = rmviLenAccolade(latex, start);
                            int len = i - start;
                            if (len > 0){
                                char *exp = malloc(len + 1);
                                memcpy(exp, &latex[start], len);
                                exp[len] = '\0';
                                float expSize = sizeText * 0.7f;
                                Vector2 expPos = { position->x, position->y - sizeText * RATIO_INDEX_UP };
                                rmviWriteLatex(exp, &expPos, expSize, spacing, color, font);
                                ts = MeasureTextEx(font, exp, expSize, spacing);
                                free(exp);
                            }
                            if (latex[i] == '}') i++;
                            i -= k + 1;
                        }
                        else{
                            DrawTextEx(font, "exp", *position, sizeText, spacing, color);
                            ts = MeasureTextEx(font, "exp", sizeText, spacing);
                        }
                    }
                    else if (strncmp(kstart, "phi", k) == 0){
                        DrawTextEx(font, "φ", *position, sizeText, spacing, color);
                        ts = MeasureTextEx(font, "φ", sizeText, spacing);
                        handled = true;
                    }
                    else if(strncmp(kstart,"bar",k) == 0){
                        handled = true;
                        if (latex[i+4] == '{'){
                            i += 5; // après "/bar{"
                            int start = i;
                            i = rmviLenAccolade(latex, start);
                            int len = i - start;
                            if (len > 0){
                                char *bar = malloc(len + 1);
                                memcpy(bar, &latex[start], len);
                                bar[len] = '\0';
                                Vector2 positionBar = { position->x, position->y};
                                rmviWriteLatex(bar, position,sizeText, spacing, color, font);
                                Vector2 tsBar = MeasureTextEx(font, bar, sizeText, spacing);
                                // dessiner la barre au dessus
                                DrawLineEx((Vector2){positionBar.x, positionBar.y - spacing/4}, (Vector2){positionBar.x + tsBar.x, positionBar.y - spacing/4}, sizeText/20, color);
                                ts = (Vector2) {0,0};
                                free(bar);
                            }
                            if (latex[i] == '}') i++;
                            i -= k + 1;
                        }
                    }
                    break;
                case 4:
                    if(strncmp(kstart,"frac",k) == 0){
                        handled = true;
                        if (latex[i+5] == '{'){
                            i += 6; // après "/frac{"
                            int start = i;
                            i = rmviLenAccolade(latex, start);
                            int len = i - start;
                            char *numerator = malloc(len + 1);
                            memcpy(numerator, &latex[start], len);
                            numerator[len] = '\0';
                            if (latex[i] == '}') i++;
                            if (latex[i] == '{'){
                                i += 1; // après '{'
                                start = i;
                                i = rmviLenAccolade(latex, start);
                                len = i - start;
                                char *denominator = malloc(len + 1);
                                memcpy(denominator, &latex[start], len);
                                denominator[len] = '\0';
                                if (latex[i] == '}') i++;
                                // maintenant on a numerator et denominator
                                rmviWriteFraction(numerator, denominator, position, sizeText, spacing, color, font);
                                free(denominator);
                                free(numerator);
                            }
                        }
                        else{
                            printf("rmviWriteLatex: /frac missing {\n");
                        }
                        i -= k + 1;
                    }
                case 5:
                    if (strncmp(kstart, "theta", k) == 0){
                        i++;
                        DrawTextEx(font, "θ", *position, sizeText, spacing, color);
                        ts = MeasureTextEx(font, "θ", sizeText, spacing);
                        handled = true;
                    }
                    else if(strncmp(kstart, "delta",k) == 0){
                        i++;
                        DrawTextEx(font, "δ", *position, sizeText, spacing, color);
                        ts = MeasureTextEx(font, "δ", sizeText, spacing);
                        handled = true;
                    }
                    else if(strncmp(kstart,"Delta",k) == 0){
                        i++;                        // sauter l'espace si y'en a une
                        DrawTextEx(font, "Δ", *position, sizeText, spacing, color);
                        ts = MeasureTextEx(font, "Δ", sizeText, spacing);
                        handled = true;
                    }
                    else if (strncmp(kstart, "begin", k) == 0)
                    {
                        handled = true;
                        char *env = rmviGetEnv(&i, latex);
                        int bodyFullLen = rmviLenBegin(latex, i, env); //donne la position à la fin de /end(env)
                        int bodyLen = bodyFullLen - (6 + strlen(env)); // longueur du corps entre /begin(env) et /end(env)
                        char *body = malloc(bodyLen + 1);
                        memcpy(body, latex + i, bodyLen);
                        body[bodyLen] = '\0';
                        // body fini par /end(env) faudrait modifier ca mais il commence après le /begin(env)
                        rmviWriteBegin(env, body, position, initPos,
                                    sizeText, spacing, color, font, false);
                        i += bodyFullLen;
                        free(body);
                        i -= k + 1;
                    }
                    break;
                case 8:
                    if(strncmp(kstart,"footnote",k) == 0){
                        // on ignore la footnote pour l'instant
                        handled = true;
                        i += k + 2; // après "/footnote{"
                        int start = i;
                        int end = rmviLenAccolade(latex, start);
                        int len = end - start;
                        Vector2 positionFootNote = {GetScreenWidth()*1/15, GetScreenHeight() - sizeText*2.0f};
                        if (len > 0 && len < 255) {
                            char tmp[256];
                            snprintf(tmp, sizeof(tmp), "%.*s", len, &latex[start]);
                            DrawTextEx(font, tmp, positionFootNote, sizeText*0.75, spacing*0.75, color);
                        }
                        i = end-k +1;
                    }
                    break;
                case 9:
                    if(strncmp(kstart,"loadImage",k) == 0 || strncmp(kstart,"loadimage",k) == 0){
                        bool hasPosition = false;
                        Vector2 imgPos;
                        handled = true;
                        i += k+2; // après "/loadImage("
                        int start = i;
                        int end = rmviLenAccolade(latex, start);
                        int len = end - start;
                        char imgPath[256];
                        strncpy(imgPath, &latex[start], len);
                        imgPath[len] = '\0';
                        end = rmviHasPosition(latex, end+1, &hasPosition, &imgPos);
                        hasPosition ? rmviLoadImage(imgPath, imgPos, sizeText / SIZE_TEXT) : 
                            rmviLoadImage(imgPath, (Vector2){position->x + sizeText, position->y + sizeText / 2}, sizeText / SIZE_TEXT);
                        Vector2 tsImg = {sizeText * 2.0f, sizeText * 1.0f};
                        ts.x = 0;
                        position->x += -MeasureTextEx(font, " ", sizeText, spacing).x;
                        i = end - k ;
                    }
                }
                if (!handled){
                    DrawTextEx(font, " ", *position, sizeText, spacing, color);
                    ts = MeasureTextEx(font, " ", sizeText, spacing);
                }
                position->x += ts.x;
                i += k + 1;
            }

        // exposant '^' (suivi d'un seul "caractère" UTF-8)
        else if (( latex[i] == '^' || latex[i] == '_') && latex[i+1] != '\0') {
            Vector2 ts, subPos;
            if (latex[i+1] == '{') {
                // Trouver la fin de l'accolade
                int start = i + 2; // après "^{"
                int end = rmviLenAccolade(latex, start);
                int len = end - start;
                char subBuf[64]; // buffer suffisant
                strncpy(subBuf, &latex[start], len);
                subBuf[len] = '\0';
                float subSize = sizeText * RATIO_INDEX;

                if (latex[i] == '^') {
                    subPos = (Vector2){ position->x,
                                        position->y - sizeText * RATIO_INDEX_UP };
                } else {
                    subPos = (Vector2){ position->x,
                                        position->y + sizeText * RATIO_INDEX_DOWN };
                }
                rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                ts = MeasureTextEx(font, subBuf, subSize, spacing);
                i =  end + 1 ; // on saute tout jusqu'à '}'
            } else {
                char subBuf[8];
                int consumed = copy_utf8_char(subBuf, &latex[i+1]);
                float subSize = sizeText * RATIO_INDEX;
                if (latex[i] == '^') {
                    subPos = (Vector2){ position->x,
                                        position->y - sizeText * RATIO_INDEX_UP };
                } else {
                    subPos = (Vector2){ position->x,
                                        position->y + sizeText * RATIO_INDEX_DOWN };
                }
                rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                ts = MeasureTextEx(font, subBuf, subSize, spacing);
                i += 1 + consumed;
            }
            if((latex[i] == '_' || latex[i] == '^') && latex[i] != '\0'){
                if (latex[i+1] == '{') {
                    // Trouver la fin de l'accolade
                    int start = i + 2; // après "_{"
                    int end = rmviLenAccolade(latex, start);
                    int len = end - start;
                    char subBuf[64]; // buffer suffisant
                    strncpy(subBuf, &latex[start], len);
                    subBuf[len] = '\0';
                    float subSize = sizeText * RATIO_INDEX;
                    if (latex[i] == '^') {
                        subPos = (Vector2){ position->x,
                                            position->y - sizeText * RATIO_INDEX_UP };
                    } else {
                        subPos = (Vector2){ position->x,
                                            position->y + sizeText * RATIO_INDEX_DOWN };
                    }
                    rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                    ts.x = max(ts.x,MeasureTextEx(font, subBuf, subSize, spacing).x);
                    i =  end + 1 ; // on saute tout jusqu'à '}'
                } else {
                    // simple caractère après '_'
                    char subBuf[8];
                    int consumed = copy_utf8_char(subBuf, &latex[i+1]);
                    float subSize = sizeText * RATIO_INDEX;
                    if (latex[i] == '^') {
                        subPos = (Vector2){ position->x,
                                            position->y - sizeText * RATIO_INDEX_UP };
                    } else {
                        subPos = (Vector2){ position->x,
                                            position->y + sizeText * RATIO_INDEX_DOWN };
                    }
                    rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                    ts.x = max(ts.x,MeasureTextEx(font, subBuf, subSize, spacing).x);
                    i += 1 + consumed;
                }
            }
            position->x += ts.x;
            
        }
        // normal character (may be UTF-8 multibyte)
        else {
            char ch[8];
            int consumed = copy_utf8_char(ch, &latex[i]);
            DrawTextEx(font, ch, *position, sizeText, spacing, color);
            Vector2 ts = MeasureTextEx(font, ch, sizeText, spacing);
            position->x += ts.x;
            i += consumed;
        }
    }
}

void rmviWriteFraction(const char *numerator, const char *denominator, Vector2 *position, float sizeText, float spacing, Color color, Font font){
    // Calculer la largeur maximale entre numérateur et dénominateur
    float numWidth = rmviCalcTextWidth(numerator, font, sizeText, spacing);
    float denWidth = rmviCalcTextWidth(denominator, font, sizeText, spacing);
    float fracWidth = (numWidth > denWidth) ? numWidth : denWidth;
    Vector2 initPosFrac = *position;
    Vector2 numPos = {
        position->x + (fracWidth - numWidth) / 2.0f,
        position->y - sizeText * 0.6f
    };
    rmviWriteLatex(numerator, &numPos, sizeText, spacing, color, font);

    // Dessiner la ligne de fraction
    DrawLineEx((Vector2){position->x, position->y + sizeText * 0.7f}, (Vector2){position->x + fracWidth, position->y + sizeText * 0.7f}, sizeText * RATIO_LINE, color);
    // Positionner le dénominateur
    Vector2 denPos = {
        position->x + (fracWidth - denWidth) / 2.0f,
        position->y + sizeText
    };
    rmviWriteLatex(denominator, &denPos, sizeText, spacing, color, font);
    // Mettre à jour la position pour après la fraction
    position->x += initPosFrac.x + fracWidth + MeasureTextEx(font, " ", sizeText, spacing).x;
    position->y = initPosFrac.y;
}


void rmviLoadImage(const char *path, Vector2 position, float scale){
    static Texture2D texture = {0};
    static char currentPath[512] = {0};
    // Charger seulement si le chemin change
    if (strcmp(currentPath, path) != 0){
        if (texture.id != 0)
            UnloadTexture(texture);

        texture = LoadTexture(path);
        if (texture.id == 0)
        {
            TraceLog(LOG_ERROR, "rmviLoadImage: failed to load %s", path);
            return;
        }

        strcpy(currentPath, path);
    }
    Vector2 size = {
        texture.width * scale,
        texture.height * scale
    };
    Vector2 drawPos = {
        position.x - size.x / 2.0f,
        position.y - size.y / 2.0f
    };
    DrawTextureEx(texture, drawPos, 0.0f, scale, WHITE);
}

int rmviHasPosition(const char *latex, int startIndex, bool *hasPosition,  Vector2 *imgPos){
    *hasPosition = false;
    *imgPos = (Vector2){0};
    int i = startIndex;
    int j = 0;
    // skip spaces
    while (latex[i + j] == ' ') j++;
    // pas de position
    if (latex[i + j] != '{')
        return startIndex;
    // ---- parse X ----
    int start = i + j + 1; // après '{'
    int end = rmviLenAccolade(latex, start);
    int len = end - start;
    if (len <= 0 || len >= 63)
        return startIndex;
    char tmp[64];
    memcpy(tmp, &latex[start], len);
    tmp[len] = '\0';
    (*imgPos).x = strtof(tmp, NULL);
    // ---- chercher Y ----
    j = 1;
    while (latex[end + j] == ' ') j++;
    if (latex[end + j] != '{') {
        *hasPosition = true;
        return end + j;
    }
    // ---- parse Y ----
    start = end + j + 1;
    end = rmviLenAccolade(latex, start);
    len = end - start;
    if (len <= 0 || len >= 63)
        return startIndex;
    memcpy(tmp, &latex[start], len);
    tmp[len] = '\0';
    (*imgPos).y = strtof(tmp, NULL);
    *hasPosition = true;
    return end;
}



void rmviWriteBegin(const char *env, const char *body, Vector2 *pos, Vector2 initPos, float sizeText, float spacing, Color color, Font font, bool isNested){
    if (strcmp(env, "itemize") == 0) {
        pos->x = initPos.x;
        rmviWriteItemize(body, pos, sizeText, spacing, color, font);
        if (!isNested) {
            pos->y += sizeText * INTERLIGNE_ITEM;  // espacement entre environnements
        }
    }
    else if (strcmp(env, "equation") == 0) {
        // centrer l'équation

        float textWidth = rmviCalcTextWidth(body, font, sizeText, spacing);
        pos->x = initPos.x + (GetScreenWidth() - textWidth) / 2.0f;
        rmviWriteEquation(body, pos, sizeText, spacing, color, font);
        if (!isNested) {
            pos->y += sizeText * INTERLIGNE;  // espacement entre environnements
        }
    }
    else {
        rmviWriteLatex(body, pos, sizeText, spacing, color, font);
    }
}


void rmviWriteEquation(const char *body, Vector2 *pos, float sizeText, float spacing, Color color,Font font){
    Vector2 lineStart = *pos;
    rmviWriteLatex(body, pos, sizeText, spacing, color, font);
    pos->x = lineStart.x;
    pos->y += sizeText * INTERLIGNE;
}


void rmviWriteItemize(const char *body, Vector2 *pos, float sizeText, float spacing, Color color, Font font){
    Vector2 initStart = *pos;
    const float indentStep = sizeText * RATIO_INDENT;
    const float lineHeight = sizeText * INTERLIGNE_ITEM;
    const float bulletGap  = 10.0f;
    pos->x += indentStep;
    for (int i = 0; body[i] != '\0'; ) {
        if(strncmp(&body[i], "/item", 5) == 0){
            pos->y += lineHeight;
            i += 5; // saute "/item"
            if(strncmp(&body[i], "[", 1) == 0) {
                i++; // saute "["
                int end = rmviLenCrochets(body, i);
                int len = end - i;
                char *crochet = (char*)malloc(len + 1);
                strncpy(crochet, &body[i], len);
                crochet[len] = '\0';
                i = end + 1; // i placé juste après ']'
                rmviWriteLatex(crochet, pos, sizeText, spacing, color, font);
                // Utiliser 'crochet' ici
                free(crochet);
            }
            else{
                rmviWriteLatex("-", pos, sizeText, spacing, color, font);
            }
            int itemLength = rmviLenItem(body, i);
            const char *bodyStart = body + i;
            const char *bodyEnd   = bodyStart + itemLength;
            // On fabrique une string indépendante pour le body
            char *body_item = (char*)malloc((size_t)(itemLength + 1));
            memcpy(body_item, bodyStart, (size_t)itemLength);
            body_item[itemLength] = '\0';
            // ici il faudrait uniquement fournir ce qui suit après le item
            rmviWriteLatex(body_item, pos, sizeText, spacing, color, font);
            pos->x = initStart.x + indentStep; // reset x position for next item
            i += itemLength;
        }
        else if(strncmp(&body[i], "/begin(", 7) == 0){
            char *env = rmviGetEnv(&i, body);
            int bodyLen  = rmviLenBegin(body, i, env);
            const char *bodyBeginStart = body + i;
            // On fabrique une string indépendante pour le body
            char *bodyBegin = (char*)malloc((size_t)(bodyLen + 1));
            memcpy(bodyBegin, bodyBeginStart, (size_t)bodyLen);
            bodyBegin[bodyLen] = '\0';
            Vector2 posItem = { initStart.x + indentStep , pos->y };
            rmviWriteBegin(env, bodyBegin, pos, posItem, sizeText, spacing, color, font, true);
            pos->x = initStart.x + indentStep;
            i += bodyLen;
        }
        else if(strncmp(&body[i], "/end(itemize)", 13) == 0){
            // ici ca devrait être fini du coup ?
            i += 13; // saute "/end(itemize)"
            pos->x = initStart.x; // reset x to initial start
        }
        else{
            // on ne devrait pas arriver là
            //rmviWriteLatex(&body[i], *pos, sizeText, spacing, color, font);
            i++;
        }
    }
    pos->x = initStart.x;
    pos->y += lineHeight; // saut de ligne après la fin de l'itemize
}

// on donne le nombre de caractère pour arriver à la sortie du /end(env)
int rmviLenBegin(const char *latex, int start, const char *env) {
    int depth = 1; // on part déjà avec 1 /begin(env)
    const char *p = latex + start;

    char beginTag[80];
    char endTag[80];
    snprintf(beginTag, sizeof(beginTag), "/begin(%s)", env);
    snprintf(endTag, sizeof(endTag), "/end(%s)", env);

    while (*p && depth > 0) {
        if (strncmp(p, beginTag, strlen(beginTag)) == 0) {
            depth++;
            p += strlen(beginTag);
        }
        else if (strncmp(p, endTag, strlen(endTag)) == 0) {
            depth--;
            p += strlen(endTag);
        }
        else {
            p++; // avancer caractère par caractère
        }
    }

    // Retourne le nombre de caractères lus jusqu’à la fermeture
    return (int)(p - (latex + start));
}



// coupe une chaine de caractère séparé par par de // ou \n en list de chaine de caractère
char **rmviSplitText(const char *text) {
    if (!text) return NULL;
    size_t capacity = 8;
    size_t count = 0;
    char **result = malloc(capacity * sizeof(char*));
    if (!result) return NULL;
    const char *p = text;
    const char *start = p;
    while (*p) {
        if ((p[0] == '/' && p[1] == '/') || *p == '\n') {
            const char *end = p;
            size_t len = (size_t)(end - start);
            if (len > 0) {
                if(start[0] == ' ') {
                    start++;
                    len--;
                }
                if( start[len - 1 ] == ' ') len --;
                char *segment = malloc(len + 1);
                memcpy(segment, start, len);
                segment[len] = '\0';
                if (count >= capacity) {
                    capacity *= 2;
                    result = realloc(result, capacity * sizeof(char*));
                }
                result[count++] = segment;
            }
            // avancer start après le séparateur
            if (p[0] == '/' && p[1] == '/')
                p += 2;
            else
                p += 1;
            start = p;
        } else {
            p++;
        }
    }
    if (p > start) {
        size_t len = (size_t)(p - start);
        char *segment = malloc(len + 1);
        memcpy(segment, start, len);
        segment[len] = '\0';
        if (segment[0] == ' ')
            memmove(segment, segment + 1, strlen(segment));
        if (count >= capacity) {
            capacity *= 2;
            result = realloc(result, capacity * sizeof(char*));
        }
        result[count++] = segment;
    }
    // terminer le tableau par NULL
    result[count] = NULL;
    return result;
}

void rmviWriteAnimText(const char *text, Vector2 position, float size, Color color,
                       int frameStart, int currentFrame) {
    if (text == NULL || *text == '\0') return;
    int charsToWrite = currentFrame - frameStart;
    if (charsToWrite <= 0) return;
    int byteCount = 0;
    int charCount = 0;
    // Avancer jusqu’au bon nombre de caractères complets
    while (text[byteCount] != '\0' && charCount < charsToWrite) {
         if (text[byteCount] == '/') {
            // commandes LaTeX spéciales
            if (strncmp(&text[byteCount+1], "nu", 2) == 0) {
                byteCount += 3; // saute "/nu"
                charCount++;    // compte comme 1 caractère
            }
            else if (strncmp(&text[byteCount+1], "int", 3) == 0) {
                byteCount += 4; // saute "/int"
                charCount++;
            }
            else if (strncmp(&text[byteCount+1], "begin(", 6) == 0) {
                // consommer jusqu'à la ')'
                byteCount += 7;
                while (text[byteCount] != ')' && text[byteCount] != '\0') byteCount++;
                if (text[byteCount] == ')') byteCount++;
                charCount++; // on considère le begin comme un "bloc"
            }
            else if (strncmp(&text[byteCount+1], "end(", 4) == 0) {
                byteCount += 5;
                while (text[byteCount] != ')' && text[byteCount] != '\0') byteCount++;
                if (text[byteCount] == ')') byteCount++;
                charCount++;
            }
            else {
                // inconnu → afficher juste '/'
                byteCount++;
                charCount++;
            }
        } 
        else {
            // caractère normal UTF-8
            int charLen = utf8_char_len((unsigned char)text[byteCount]);
            byteCount += charLen;
            charCount++;
        }
    }
    // Copier uniquement les bytes nécessaires
    char *subText = (char *)malloc(byteCount + 1);
    memcpy(subText, text, byteCount);
    subText[byteCount] = '\0';
    rmviWriteLatex(subText, &position, size, size / RATIO_SPACE, color, mathFont);
    free(subText);
}
