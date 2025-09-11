#ifndef TEXT2LATEX_H
#define TEXT2LATEX_H

#include "raylib.h"   // pour Vector2, Font, Color, etc.
#include <string.h>  // pour strlen, strncmp, etc.
extern Font mathFont;
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
#endif // TEXT2LATEX_H