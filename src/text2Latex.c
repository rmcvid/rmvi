#include <stdio.h>
#include <stdlib.h>
#include "text2Latex.h"
Font mathFont;

#define MAX_CODEPOINTS 512
#define INTERLIGNE_ITEM 1.4f // interligne pour les items
#define INTERLIGNE 1.2f     // Line spacing ratio with \n
#define RATIO_INDEX 0.6f    // Size ratio for index text such H_3
#define RATIO_INDEX_DOWN 0.5f // Ratio for subscript positioning
#define RATIO_INDEX_UP 0.35f   // Ratio for superscript positioning
#define RATIO_INDENT 2.0f      // Ratio for item indentation
#define RATIO_SPACE 15.0f

void rmviGetCustomFont(const char *path, float fontSize) {
    int codepoints[MAX_CODEPOINTS];
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
    codepoints[count++] = 0x222B; // ∫
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
int rmviLenAccolade(const char *latex,int start){
    int count = 1;
    int end = start;
    while(count > 0 && latex[end] !='\0'){
        if (latex[end] == '{') count++;
        else if (latex[end] == '}') count--;
        end++;
    }
    end--;
    return end;
}
// Calcule la longeur d'un texte entre parenthèse, on peut en ouvrir et en fermer d'autre en interne
// Permetra la recursivité ex : (()) = 2
int rmviLenParenthese(const char *latex,int start){
    int count = 1;
    int end = start;
    while(count > 0 && latex[end] !='\0'){
        if (latex[end] == '(') count++;
        else if (latex[end] == ')') count--;
        end++;
    }
    end--;
    return end;
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
            else if (strncmp(&latex[i+1], "int", 3) == 0) {
                Vector2 ts = MeasureTextEx(font, "∫", sizeText, spacing);
                width += ts.x;
                i += 4; // skip "/int"
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
            else {
                Vector2 ts = MeasureTextEx(font, "/", sizeText, spacing);
                width += ts.x;
                i++;
            }
        }
        // retour ligne → on arrête (car mesure ligne par ligne)
        else if (latex[i] == '\n') {
            break;
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
    } else {
        printf("rmviCalcBeginWidth = bug (env=%s)\n", env);
    }

    return widthMemory;
}

float rmviCalcBeginHeight(const char *env, int start, const char *latex, Font font, float sizeText, float spacing, bool isNested) {
    float height = 0.0f;
    if (strcmp(env, "itemize") == 0) {
        height += rmviCalcItemizeHeight(latex, start, font, sizeText, spacing, isNested);
    }
    return height;
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
            const char *body_begin_Start = body + i;
            // On fabrique une string indépendante pour le body
            char *body_begin = (char*)malloc((size_t)(bodyLen + 1));
            memcpy(body_begin, body_begin_Start, (size_t)bodyLen);
            body_begin[bodyLen] = '\0';
            height += rmviCalcBeginHeight(env, i, body_begin, font, sizeText, spacing, true);
            i += bodyLen;
            free(body_begin);
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

// Dessine le text comme si on ecrivait en latex mais il faut remplacer les backslash par /
void rmviWriteLatex(const char *latex, Vector2 *position, float sizeText, float spacing, Color color, Font font) {
    if (!latex || !position) return;
    Vector2 initPos = *position;

    // You might want x,y to be top-left or center — adapt before calling.
    for (int i = 0; latex[i] != '\0'; ) {
        // handle /commands like /nu or /exp{...}
        //switch case
        if (latex[i] == '/' && latex[i+1] != '\0') {
            if (strncmp(&latex[i+1], "nu", 2) == 0) {
                DrawTextEx(font, "ν", *position, sizeText, spacing, color);
                Vector2 ts = MeasureTextEx(font, "ν", sizeText, spacing);
                position->x += ts.x;
                i += 3; // skip "/nu"
            }
            else if (latex[i+1] == '/') {
                position->y += sizeText * INTERLIGNE; // interline, ajustable
                position->x = initPos.x;       // reset to line start (you can pre-center per-line if needed)
                if(latex[i+2] == ' ') i++;
                i += 2;
            }
            else if (strncmp(&latex[i+1], "int", 3) == 0) {
                DrawTextEx(font, "∫", *position, sizeText, spacing, color);
                Vector2 ts = MeasureTextEx(font, "∫", sizeText, spacing);
                position->x += ts.x;
                i += 4; // skip "/int"
            }
            else if (strncmp(&latex[i+1], "exp{", 4) == 0) {
                i += 5; // position après "/exp{"
                int start = i;
                i = rmviLenAccolade(latex, start);
                int len = i - start;
                if (len > 0) {
                    char *exp = (char*)malloc(len + 1);
                    memcpy(exp, &latex[start], len);
                    exp[len] = '\0';
                    float expSize = sizeText * 0.7f;
                    Vector2 expPos = { position->x, position->y - sizeText * RATIO_INDEX_UP };
                    rmviWriteLatex(exp, &expPos, expSize, spacing, color, font);
                    Vector2 ts = MeasureTextEx(font, exp, expSize, spacing);
                    position->x += ts.x;
                    free(exp);
                }
                if (latex[i] == '}') i++; // skip closing brace if any
            }
           else if (strncmp(&latex[i+1], "begin(", 6) == 0) {
                char *env = rmviGetEnv(&i, latex);
                int bodyLen  = rmviLenBegin(latex, i, env);
                const char *bodyStart = latex + i;        
                const char *bodyEnd   = bodyStart + bodyLen;
                // On fabrique une string indépendante pour le body
                char *body = (char*)malloc((size_t)(bodyLen + 1));
                memcpy(body, bodyStart, (size_t)bodyLen);
                body[bodyLen] = '\0';
                rmviWriteBegin(env, body, position, initPos, sizeText, spacing, color, font, false);
                i += bodyLen;
                if(latex[i] == ' ') i++;
                free(body);
            }

            else {
                // unknown /command: draw the '/' char itself
                DrawTextEx(font, "/", *position, sizeText, spacing, color);
                Vector2 ts = MeasureTextEx(font, "/", sizeText, spacing);
                position->x += ts.x;
                i++;
            }
        }
        // exposant '^' (suivi d'un seul "caractère" UTF-8)
        else if (latex[i] == '^' && latex[i+1] != '\0') {
            if (latex[i+1] == '{') {
                // Trouver la fin de l'accolade
                int start = i + 2; // après "^{"
                int end = rmviLenAccolade(latex, start);
                int len = end - start;
                char subBuf[64]; // buffer suffisant
                strncpy(subBuf, &latex[start], len);
                subBuf[len] = '\0';
                float subSize = sizeText * RATIO_INDEX;
                Vector2 subPos = { position->x, position->y - sizeText * RATIO_INDEX_UP };
                rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                Vector2 ts = MeasureTextEx(font, subBuf, subSize, spacing);
                position->x += ts.x;
                i =  end + 1 ; // on saute tout jusqu'à '}'
            } else {
                // simple caractère après '_'
                char subBuf[8];
                int consumed = copy_utf8_char(subBuf, &latex[i+1]);
                float subSize = sizeText * RATIO_INDEX;
                Vector2 subPos = { position->x, position->y - sizeText * RATIO_INDEX_UP };
                rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                Vector2 ts = MeasureTextEx(font, subBuf, subSize, spacing);
                position->x += ts.x;
                i += 1 + consumed;
            }
            
        }
        // indice '_' (suivi d'un seul "caractère" UTF-8)
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
                Vector2 subPos = { position->x, position->y + sizeText * RATIO_INDEX_DOWN };
                rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                Vector2 ts = MeasureTextEx(font, subBuf, subSize, spacing);
                position->x += ts.x;
                i =  end + 1 ; // on saute tout jusqu'à '}'
            } else {
                // simple caractère après '_'
                char subBuf[8];
                int consumed = copy_utf8_char(subBuf, &latex[i+1]);
                float subSize = sizeText * RATIO_INDEX;
                Vector2 subPos = { position->x, position->y + sizeText * RATIO_INDEX_DOWN };
                rmviWriteLatex(subBuf, &subPos, subSize, spacing, color, font);
                Vector2 ts = MeasureTextEx(font, subBuf, subSize, spacing);
                position->x += ts.x;
                i += 1 + consumed;
            }
        }
        // newline
        else if (latex[i] == '\n') {
            position->y += sizeText * INTERLIGNE; // interline, ajustable
            position->x = initPos.x;       // reset to line start (you can pre-center per-line if needed)
            i++;
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

void rmviWriteBegin(const char *env, const char *body, Vector2 *pos, Vector2 initPos, float sizeText, float spacing, Color color, Font font, bool isNested){
    if (strcmp(env, "itemize") == 0) {
        pos->x = initPos.x;
        rmviWriteItemize(body, pos, sizeText, spacing, color, font);
        if (!isNested) {
            pos->y += sizeText * INTERLIGNE_ITEM;  // espacement entre environnements
        }
    }
    else {
        rmviWriteLatex(body, pos, sizeText, spacing, color, font);
    }
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
            const char *body_begin_Start = body + i;
            // On fabrique une string indépendante pour le body
            char *body_begin = (char*)malloc((size_t)(bodyLen + 1));
            memcpy(body_begin, body_begin_Start, (size_t)bodyLen);
            body_begin[bodyLen] = '\0';
            Vector2 posItem = { initStart.x + indentStep , pos->y };
            rmviWriteBegin(env, body_begin, pos, posItem, sizeText, spacing, color, font, true);
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
                // Fin de l'item courant
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

int rmviLenCrochets(const char *latex,int start){
    int count = 1;
    int end = start;
    while(count > 0 && latex[end] !='\0'){
        if (latex[end] == '[') count++;
        else if (latex[end] == ']') count--;
        end++;
    }
    end--;
    return end;
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
