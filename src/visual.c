#include "visual.h"
#include "ffmpeg.h"
#include "text2Latex.h"
#include "fft_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
//#include <fftw3.h>
#define DRAWCOLOR WHITE
#define BG BLACK
#define RATIODEFAULT 4.5      // ratio entre la hauteur du rectangle et la hauteur du texte
#define RATIO_SPACE 15
#define NBPOINTS 1000
#define SPEED 10
#define PRECISIONS 35 // Give more precision to the plot
#define SIZETEXT 50.0f      // taille du text standar
#define ARROWSIZE 20.0f     // gives the size arrow divided by ratio i think
#define SPLIT_LEFT_ARROWS 0.2f // gives the left split for arrowsRect
#define RATIOWRITEARROWS 0.5f
//---------------------------------- RECTANGLE AND FRAMES ---------------------------------
void rmviMyDrawText(const char *text, Vector2 position, float size, Color color) {
    if (text == NULL || strlen(text) == 0) return;
    rmviWriteLatex(text, &position, size, size / RATIO_SPACE, color, mathFont);
}

void rmviDrawTextMid(const char *text, Rectangle rec, Color color, float ratio, Font font) {
        rmviDrawTextRec(text, rec, color, ratio, 0.5f, 0.5f, font);
}
void rmviDrawTextRec(const char *text, Rectangle rec, Color color, float ratio,
                     float ratioWidth, float ratioHeight, Font font) {
    if (ratio <= 0) ratio = RATIODEFAULT;  // valeur par défaut
    float sizeText = rec.height / ratio;
    float spacing = (sizeText / RATIO_SPACE) * ratioWidth;  // scaling horizontal correct
    // Compter le nombre de lignes
    // Mesurer le texte avec spacing
    // Donne la position centrale du rectangle
    float posX = rec.x + (rec.width - rmviCalcTextWidth(text, font, sizeText, spacing)) * ratioWidth;
    float posY = rec.y + (rec.height - rmviCalcTextHeight(text, font, sizeText, spacing, false)) * ratioHeight;
    // Dessiner
    char **splittxt = rmviSplitText(text);
    for (int i = 0; splittxt[i] != NULL; i++) {
        posX = rec.x + (rec.width - rmviCalcTextWidth(splittxt[i], font, sizeText, spacing)) * ratioWidth;
        if(i!=0) posY = posY + rmviCalcTextHeight(splittxt[i-1], font, sizeText, spacing, false);
        rmviWriteLatex(splittxt[i], &(Vector2){ posX, posY }, sizeText, spacing, color, font);
    }
}

Rectangle rmviGetRectangleCenteredRatio(float x, float y, float ratio_x, float ratio_y) {
    Rectangle rec = {
        x - (float)GetScreenWidth() / ratio_x / 2,
        y - (float)GetScreenHeight() / ratio_y / 2,
        (float)GetScreenWidth() / ratio_x,
        (float)GetScreenHeight() / ratio_y
    };
    return rec;
}
// Construct a frame with the given parameters
rmviFrame rmviGetFrame(float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor, const char *text, Font font,float roundness) {
    rmviFrame frame;
    frame.outerRect = (Rectangle){ posX, posY, width, height };
    frame.innerRect = (Rectangle){ posX + lineThick, posY + lineThick, width - 2 * lineThick, height - 2 * lineThick };
    frame.outerColor = outerColor;
    frame.innerColor = innerColor;
    frame.text = text;
    frame.font = font;
    frame.lineThick = lineThick;
    frame.roundness = roundness;  // Default roundness
    return frame;
}
// Construct a round frame with the given parameters
rmviFrame rmviGetRoundFrame(float posX, float posY, float radius, float lineThick, Color innerColor, Color outerColor, const char *text, Font font) {
    rmviFrame frame = rmviGetFrame(posX, posY, radius * 2, radius * 2, lineThick, innerColor, outerColor, text, font, 1.0f);
    return frame;
}

rmviFrame rmviGetRoundFrameBGCentered(float posX, float posY, float ratio, float lineThick, const char *text, Font font){
    float radius = (float)GetScreenWidth() / ratio;
    Rectangle rec = {
        posX - radius,
        posY - radius,
        radius * 2,
        radius * 2
    };
    rmviFrame frame = rmviGetFrame(rec.x, rec.y, rec.width, rec.height, lineThick, BG, DRAWCOLOR, text, font, 1.0f);
    return frame;
}

// Construct a frame with the given parameters, with a background color
rmviFrame rmviGetFrameBG(float posX, float posY, float width, float height, float lineThick, const char *text, Font font) {
    rmviFrame frame = rmviGetFrame(posX, posY, width, height, lineThick, BG, DRAWCOLOR, text, font, 0.0f);
    return frame;
}
// Construct a frame centered about x, y with a width and height based on the ratio and with the draw and background color
rmviFrame rmviGetFrameBGCentered(float posX, float posY, float ratio_x, float ratio_y, float lineThick, const char *text, Font font) {
    Rectangle rec = rmviGetRectangleCenteredRatio(posX, posY, ratio_x, ratio_y);
    rmviFrame frame = rmviGetFrame(rec.x, rec.y, rec.width, rec.height, lineThick, BG, DRAWCOLOR, text, font, 0.0f);
    return frame;
}
// Draw a frame
void rmviDrawFrame(rmviFrame frame, float ratio) {
    DrawRectangleRounded(frame.outerRect, frame.roundness, PRECISIONS, frame.outerColor);
    DrawRectangleRounded(frame.innerRect, frame.roundness, PRECISIONS, frame.innerColor);
    if (frame.text != NULL) {
        rmviDrawTextMid(frame.text, frame.innerRect, frame.outerColor, ratio, mathFont);
    }
}

// fait tourner une frame autour de son centre
void rmviDrawFramePro(rmviFrame frame, float ratio, float rotation, Vector2 origin, Vector2 translation){
    rmviRotateRectangle(frame.outerRect, origin, rotation, frame.outerColor);
    rmviRotateRectangle(frame.innerRect, origin, rotation, frame.innerColor);
    if (frame.text != NULL) {
        float nLines = 1;
        for (int i = 0; i < strlen(frame.text); i++) {
            if (frame.text[i] == '\n') nLines++;
        }
        float sizeText = frame.innerRect.height / ratio;
        Vector2 translationVector = (Vector2){ 
            MeasureText(frame.text, sizeText)/2 + origin.x + translation.x,
            sizeText /2.0f + origin.y + translation.y };
        rmviRotateText(frame.text, 
            (Vector2){ 
                frame.innerRect.x + frame.innerRect.width / 2.0f - MeasureText(frame.text, sizeText)/2.0f,
                frame.innerRect.y + frame.innerRect.height / 2.0f - nLines* sizeText / 2.0f
            },
            translationVector, - rotation, frame.innerRect.height / ratio, frame.outerColor, frame.font);
    }

}

// Update parameters frames
void rmviUpdateFrame(rmviFrame *frame, float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor, const char *text, Font font) {
    frame->outerRect = (Rectangle){ posX, posY, width, height };
    frame->innerRect = (Rectangle){ posX + lineThick, posY + lineThick, width - 2 * lineThick, height - 2 * lineThick };
    frame->outerColor = outerColor;
    frame->innerColor = innerColor;
    frame->text = text;
    frame->font = font;
    frame->lineThick = lineThick;
}

// Update a frame's zoom
void rmviZoomFrame(rmviFrame *frame, float zoomFactor) {
    float hypothenuse = sqrtf(frame->outerRect.width * frame->outerRect.width + frame->outerRect.height * frame->outerRect.height);
    float rateX = frame->outerRect.width / hypothenuse;
    float rateY = frame->outerRect.height / hypothenuse;
    rmviUpdateFrame(frame, 
        frame->outerRect.x - (zoomFactor * rateX)/2.0f, 
        frame->outerRect.y - (zoomFactor * rateY)/2.0f, 
        frame->outerRect.width + (zoomFactor * rateX), 
        frame->outerRect.height + (zoomFactor * rateY), 
        frame->lineThick , //+ (rate* frame->lineThick/hypothenuse)
        frame->innerColor, 
        frame->outerColor, 
        frame->text,
        frame->font);
}

void rmviRewriteFrame(rmviFrame *frame,const char *text) {
    rmviUpdateFrame(frame, frame->outerRect.x, frame->outerRect.y, frame->outerRect.width, frame->outerRect.height, frame->lineThick, frame->innerColor, frame->outerColor, text, frame->font);
}

// fait tourner un rectangle autour du point origin. 0,0 est le centre du rectangle
void rmviRotateRectangle(Rectangle rec, Vector2 origin, float rotation, Color color){
    Vector2 topLeft = { 0 };
    Vector2 topRight = { 0 };
    Vector2 bottomLeft = { 0 };
    Vector2 bottomRight = { 0 };
    // Only calculate rotation if needed
    if (rotation == 0.0f)
    {
        topLeft = (Vector2){ rec.x, rec.y };
        topRight = (Vector2){ rec.x + rec.width, rec.y };
        bottomLeft = (Vector2){ rec.x, rec.y + rec.height };
        bottomRight = (Vector2){ rec.x + rec.width, rec.y + rec.height };
    }
    else
    {
        float sinRotation = sinf(rotation*DEG2RAD);
        float cosRotation = cosf(rotation*DEG2RAD);
        float x = rec.x;
        float y = rec.y;
        float rx =  rec.x + rec.width/2 + origin.x;
        float ry =  rec.y + rec.height/2 + origin.y;
        topLeft.x = x * cosRotation + y*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
        topLeft.y = - x *sinRotation + y*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
        topRight.x = (x + rec.width)*cosRotation + y*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
        topRight.y = -(x + rec.width)*sinRotation + y*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
        bottomLeft.x = x * cosRotation + (y+rec.height)*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
        bottomLeft.y = - x*sinRotation + (y+rec.height)*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
        bottomRight.x = (x + rec.width)*cosRotation + (y + rec.height)*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
        bottomRight.y = -(x + rec.width)*sinRotation + (y + rec.height)*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
    }
    rlBegin(RL_TRIANGLES);

        rlColor4ub(color.r, color.g, color.b, color.a);

        rlVertex2f(topLeft.x, topLeft.y);
        rlVertex2f(bottomLeft.x, bottomLeft.y);
        rlVertex2f(topRight.x, topRight.y);

        rlVertex2f(topRight.x, topRight.y);
        rlVertex2f(bottomLeft.x, bottomLeft.y);
        rlVertex2f(bottomRight.x, bottomRight.y);

    rlEnd();
}
// fait tourner une rectangle autour du point origin. 0,0 est le centre de la texture
void rmviRotateTexture(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint){
    if(texture.id > 0){   
        float width = (float)texture.width;
        float height = (float)texture.height;

        bool flipX = false;

        if (source.width < 0) { flipX = true; source.width *= -1; }
        if (source.height < 0) source.y -= source.height;

        if (dest.width < 0) dest.width *= -1;
        if (dest.height < 0) dest.height *= -1;

        Vector2 topLeft = { 0 };
        Vector2 topRight = { 0 };
        Vector2 bottomLeft = { 0 };
        Vector2 bottomRight = { 0 };
        // Only calculate rotation if needed
        if (rotation == 0.0f)
        {
            topLeft = (Vector2){ dest.x, dest.y };
            topRight = (Vector2){ dest.x + dest.width, dest.y };
            bottomLeft = (Vector2){ dest.x, dest.y + dest.height };
            bottomRight = (Vector2){ dest.x + dest.width, dest.y + dest.height };
        }
        else
        {
            float sinRotation = sinf(rotation*DEG2RAD);
            float cosRotation = cosf(rotation*DEG2RAD);
            float x = dest.x;
            float y = dest.y;
            float rx =  dest.x + dest.width/2 + origin.x;
            float ry =  dest.y + dest.height/2 + origin.y;
            topLeft.x = x * cosRotation + y*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
            topLeft.y = - x *sinRotation + y*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
            topRight.x = (x + dest.width)*cosRotation + y*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
            topRight.y = -(x + dest.width)*sinRotation + y*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
            bottomLeft.x = x * cosRotation + (y+dest.height)*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
            bottomLeft.y = - x*sinRotation + (y+dest.height)*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
            bottomRight.x = (x + dest.width)*cosRotation + (y + dest.height)*sinRotation - rx * (cosRotation-1) - ry * sinRotation;
            bottomRight.y = -(x + dest.width)*sinRotation + (y + dest.height)*cosRotation - ry * (cosRotation-1) + rx * sinRotation;
        }
        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);

            rlColor4ub(tint.r, tint.g, tint.b, tint.a);
            rlNormal3f(0.0f, 0.0f, 1.0f); 

             // Top-left corner for texture and quad
            if (flipX) rlTexCoord2f((source.x + source.width)/width, source.y/height);
            else rlTexCoord2f(source.x/width, source.y/height);
            rlVertex2f(topLeft.x, topLeft.y);

            // Bottom-left corner for texture and quad
            if (flipX) rlTexCoord2f((source.x + source.width)/width, (source.y + source.height)/height);
            else rlTexCoord2f(source.x/width, (source.y + source.height)/height);
            rlVertex2f(bottomLeft.x, bottomLeft.y);

            // Bottom-right corner for texture and quad
            if (flipX) rlTexCoord2f(source.x/width, (source.y + source.height)/height);
            else rlTexCoord2f((source.x + source.width)/width, (source.y + source.height)/height);
            rlVertex2f(bottomRight.x, bottomRight.y);

            // Top-right corner for texture and quad
            if (flipX) rlTexCoord2f(source.x/width, source.y/height);
            else rlTexCoord2f((source.x + source.width)/width, source.y/height);
            rlVertex2f(topRight.x, topRight.y);

        rlEnd();
        rlSetTexture(0);
    }
}


// ---------------------------- Object build ----------------------------
// Draw an arrow from start to end with a given arrow size and a ratio which sizes the line compare to the triangle
void rmviDrawArrow2(Vector2 start, Vector2 end, float arrowSize, float  ratio, Color color) {
    Vector2 direction = Vector2Normalize(Vector2Subtract(end, start));
    if (Vector2Length(direction) == 0) return;
    Vector2 perp = { -direction.y, direction.x };
    Vector2 tip = end;
    Vector2 left = {
        end.x - direction.x * arrowSize + perp.x * (arrowSize / 2),
        end.y - direction.y * arrowSize + perp.y * (arrowSize / 2)
    };
    Vector2 right = {
        end.x - direction.x * arrowSize - perp.x * (arrowSize / 2),
        end.y - direction.y * arrowSize - perp.y * (arrowSize / 2)
    };
    //Vector2Subtract(end, Vector2Scale(direction, arrowSize/2))
    DrawLineEx(start, Vector2Subtract(end, Vector2Scale(direction, arrowSize/2)), arrowSize / ratio, color);
    DrawTriangle(left, tip, right, color);
}

// draw an arrow with corner
// ratio left gives the proportion left base arrow
void rmviDrawArrowRect2(Vector2 start, Vector2 end, float arrowSize, float  ratio, float ratioLeft, Color color) {
    DrawLineEx((Vector2){start.x, start.y}, (Vector2){start.x + (end.x - start.x)*(ratioLeft + arrowSize/(ratio*2*abs(end.x - start.x))) , start.y}, arrowSize / ratio, color);
    DrawLineEx((Vector2){start.x + ((end.x - start.x)*ratioLeft), start.y}, (Vector2){start.x + ((end.x - start.x)*ratioLeft), end.y}, arrowSize / ratio, color);
    DrawLineEx((Vector2){start.x + (end.x - start.x)*(ratioLeft - arrowSize/(ratio*2*abs(end.x - start.x))), end.y}, end, arrowSize / ratio, color);
    Vector2 tip = end;
    Vector2 left = {
        end.x - arrowSize,
        end.y - arrowSize
    };
    Vector2 right = {
        end.x - arrowSize,
        end.y + arrowSize
    };
    DrawTriangle(left, tip, right, color);
}

// --------------------------------------------- Text and Font ---------------------------------------------

// rotate text around a the center of the text
void rmviRotateText(const char *text, Vector2 position, Vector2 origin, float rotation, float sizeText, Color color, Font font){
    rlPushMatrix();
        rlTranslatef(position.x  + origin.x, position.y  + origin.y, 0.0f);
        rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
        rlTranslatef(-origin.x, -origin.y, 0.0f);
        Vector2 pos = {0, 0};
        rmviWriteLatex(text, &pos, sizeText, sizeText / RATIO_SPACE, color, font);
    rlPopMatrix();
}

// Create a carthesian system with an origin, the size is the half long and unit place the 
rmviCarthesian rmviGetCarthesian(Vector2 origin, Vector2 size, Vector2 unit) {
    rmviCarthesian carthesian;
    carthesian.origin = origin;
    carthesian.size = size;
    carthesian.unit = unit;
    return carthesian;
}

// Draw the ticks on the carthesian system, length is the length of the ticks; log coordinate system to add
void rmviDrawTick(rmviCarthesian carthesian, float length, Color color) {
    int nbMax = floorf(max(carthesian.size.x / carthesian.unit.x, carthesian.size.y / carthesian.unit.y));
    for (int i = - nbMax + 1 ; i < nbMax; i ++) {
        if (abs(i) < floorf(carthesian.size.x / carthesian.unit.x)) {
            Vector2 positionThickX = (Vector2) { carthesian.origin.x + i * carthesian.unit.x, carthesian.origin.y};
            Vector2 lengthY = (Vector2) {0 , length/2};
            DrawLineV(Vector2Add(positionThickX, lengthY) , Vector2Subtract(positionThickX, lengthY), color);
        }
        if (abs(i) < floorf(carthesian.size.y / carthesian.unit.y)){
            // Draw horizontal lines
            Vector2 positionThickY = (Vector2) { carthesian.origin.x, carthesian.origin.y + i * carthesian.unit.y};
            Vector2 lengthX = (Vector2) {length/2 , 0};
            DrawLineV(Vector2Add(positionThickY, lengthX) , Vector2Subtract(positionThickY, lengthX), color);
        }
    }
}

// Draw the grids on the carthesian system, should be modified for the thickness
void rmviDrawGrids(rmviCarthesian carthesian, Color color){
    int nbMax = floorf(max(carthesian.size.x / carthesian.unit.x, carthesian.size.y / carthesian.unit.y));
    for (int i = - nbMax + 1 ; i < nbMax; i ++) {
        if (abs(i) < floorf(carthesian.size.x / carthesian.unit.x)) {
            Vector2 positionThickX = (Vector2) { carthesian.origin.x + i * carthesian.unit.x, carthesian.origin.y};
            Vector2 lengthY = (Vector2) {0 , carthesian.size.y};
            DrawLineV(Vector2Add(positionThickX, lengthY) , Vector2Subtract(positionThickX, lengthY), color);
        }
        if (abs(i) < floorf(carthesian.size.y / carthesian.unit.y)){
            // Draw horizontal lines
            Vector2 positionThickY = (Vector2) { carthesian.origin.x, carthesian.origin.y + i * carthesian.unit.y};
            Vector2 lengthX = (Vector2) {carthesian.size.x , 0};
            DrawLineV(Vector2Add(positionThickY, lengthX) , Vector2Subtract(positionThickY, lengthX), color);
        }
    }
}

// Draw the carthesian system
void rmviDrawCarthesianFull(rmviCarthesian carthesian, float arrowSize, float ratio, Color color,bool drawTicks, bool drawGrids) {
    rmviDrawArrow2(
        (Vector2){ carthesian.origin.x - carthesian.size.x, carthesian.origin.y },
        (Vector2){ carthesian.origin.x + carthesian.size.x , carthesian.origin.y },
        arrowSize, ratio, color);
    rmviDrawArrow2(
        (Vector2){ carthesian.origin.x, carthesian.origin.y + carthesian.size.y },
        (Vector2){ carthesian.origin.x, carthesian.origin.y - carthesian.size.y },
        arrowSize, ratio, color);
    if (drawGrids) rmviDrawGrids(carthesian, color);
    if (drawTicks) rmviDrawTick(carthesian, arrowSize / 2, color);
}
// Update the carthesian system
void rmviUpdateCarthesian(rmviCarthesian *carthesian, Vector2 origin, Vector2 unit , Vector2 size) {
    carthesian->origin = origin;
    carthesian->unit = unit;
    carthesian->size = size;
}

// Draw a function on the carthesian system
void rmviDrawFunction(rmviCarthesian carthesian, MathFunction fct, Color color) {
    int nbPoints = NBPOINTS; // smoothness
    float xRange = carthesian.size.x / carthesian.unit.x; // half width in units
    float step = (xRange * 2) / nbPoints; // step in units

    float x1, x2, y1, y2;
    Vector2 p1, p2;
    for (int i = 0; i < nbPoints; i++) {
        if (i != 0) {
            x1 = x2;
            p1 = p2;
            y1 = fct(x1);
            x2 = -xRange + i * step;
            y2 = fct(x2);
            p2 = (Vector2){
                carthesian.origin.x + x2 * carthesian.unit.x,
                carthesian.origin.y - y2 * carthesian.unit.y
            };
            if (fabsf(y2) <= carthesian.size.y / carthesian.unit.y) DrawLineV(p1, p2, color);
        }
        else {
            x2 = -xRange + i * step;
            y2 = fct(x2);
            p2 = (Vector2){
                carthesian.origin.x + x2 * carthesian.unit.x,
                carthesian.origin.y - y2 * carthesian.unit.y
            };
        }
    }
}
// circles axes projections
float circleX(float t, float radius) {
    return radius * cosf(t);
}
float circleY(float t, float radius) {
    return radius * sinf(t);
}

// Draw a parametric curve defined by two functions which takes 2 args
void rmviDraw2Parametric(rmviCarthesian carthesian,float fx(float,float),float fy(float,float),float radius, float tMin, float tMax, int n, Color color) {
    float dt = (tMax - tMin) / n;
    for (int i = 0; i < n; i++) {
        float t1 = tMin + i * dt;
        float t2 = t1 + dt;
        float x1 = fx(t1, radius);
        float y1 = fy(t1, radius);
        float x2 = fx(t2, radius);
        float y2 = fy(t2, radius);
        Vector2 p1 = { carthesian.origin.x + x1 * carthesian.unit.x, carthesian.origin.y - y1 * carthesian.unit.y };
        Vector2 p2 = { carthesian.origin.x + x2 * carthesian.unit.x, carthesian.origin.y - y2 * carthesian.unit.y };
        DrawLineV(p1, p2, color);
    }
}



// ----------------- Fonctions pour vidéos, à mettre dans un fichier séparé -----------------
// Draw a point a trigonometric circle and its projections on the axes 
void rmviDrawTrigo(rmviCarthesian carthesian, float x, float y, Color color, float radius) {
    float size = 7.0f;
    Vector2 point = { carthesian.origin.x + x * carthesian.unit.x, carthesian.origin.y - y * carthesian.unit.y };
    DrawLineEx(carthesian.origin, point, size ,color);
    DrawLineEx(carthesian.origin, (Vector2){ point.x, carthesian.origin.y },size, GREEN);
    DrawLineEx(carthesian.origin, (Vector2){ carthesian.origin.x, point.y }, size, RED);
    DrawLineEx(point, (Vector2){ point.x, carthesian.origin.y }, size, color);
    DrawLineEx(point, (Vector2){ carthesian.origin.x, point.y }, size, color);
    DrawCircleV(point, radius, color);
}

// increment the counter of the random direction and return the random speed vector
Vector2 rmviRandomSpeed(Vector4 *count) {
    int random = GetRandomValue(1, 6); // Get a random value between -100 and 100 for both x and y components
    if (random <= 3) {
        count->x += 1;
        return (Vector2){ 0, - SPEED };
    }
    else if (random == 4) {
        count->y += 1;
        return (Vector2){ SPEED, 0 };
    }
    else if (random == 5) {
        count->w += 1;
        return (Vector2){ - SPEED , 0 };
    }
    else {
        count->z += 1;
        return (Vector2){ 0, SPEED };
    }
}
// Draw the count of the random directions


// cree un arbre
rmviTree *rmviCreateTree(void) {
    rmviTree *tree = malloc(sizeof(rmviTree));
    tree->n = 0;
    tree->depth = 0;
    tree->frames = NULL;        
    tree->numeros = NULL;
    tree->parents = NULL;           //numero du parent
    tree->nEnfants = NULL;          //nombre d'enfant
    tree->enfants = NULL;           // index des enfants
    return tree;
}

// Donne la profondeur d'un noeud
int getDepth(rmviTree *tree, int index) {
    if (tree->parents[index] == -1) return 1;
    return 1 + getDepth(tree, tree->parents[index]);
}

// Ajoute un frame à l'arbre
rmviTree *rmviAddFrame2Tree(rmviTree *tree, rmviFrame *frame, int parentIndex) {
    if (tree == NULL) return NULL;
    int newIndex = tree->n; // futur index du nouveau frame

    // Agrandir le tableau de pointeur des frames
    rmviFrame **newFrames = realloc(tree->frames, sizeof(rmviFrame*) * (tree->n + 1));
    if (newFrames == NULL) return NULL;
    tree->frames = newFrames;
    tree->frames[newIndex] = frame; // on stocke juste le pointeur, pas une copie


    // Agrandir le tableau des numeros
    int *newNumeros = realloc(tree->numeros, sizeof(int) * (tree->n + 1));
    if (newNumeros == NULL) return NULL;
    tree->numeros = newNumeros;
    tree->numeros[newIndex] = newIndex;  // numéro = index

    // Agrandir le tableau des parents
    int *newParents = realloc(tree->parents, sizeof(int) * (tree->n + 1));
    if (newParents == NULL) return NULL;
    tree->parents = newParents;
    tree->parents[newIndex] = parentIndex;  // -1 si racine

    // Agrandir le tableau des nombres d’enfants
    int *newNEnfants = realloc(tree->nEnfants, sizeof(int) * (tree->n + 1));
    if (newNEnfants == NULL) return NULL;
    tree->nEnfants = newNEnfants;
    tree->nEnfants[newIndex] = 0;

    // Agrandir le tableau des listes d’enfants
    int **newEnfants = realloc(tree->enfants, sizeof(int *) * (tree->n + 1));
    if (newEnfants == NULL) return NULL;
    tree->enfants = newEnfants;
    tree->enfants[newIndex] = NULL;

    // Ajouter ce nouvel enfant à la liste du parent
    if (parentIndex >= 0) {
        int nChild = tree->nEnfants[parentIndex];
        int *newChildList = realloc(tree->enfants[parentIndex], sizeof(int) * (nChild + 1));
        if (newChildList == NULL) return NULL;

        tree->enfants[parentIndex] = newChildList;
        tree->enfants[parentIndex][nChild] = newIndex;
        tree->nEnfants[parentIndex]++;
    }

    // Incrémenter le nombre total de frames
    tree->n++;

    // Mettre à jour la profondeur (approche simple : max profondeur +1 si parent)
    if (parentIndex == -1) {
        tree->depth = 1;
    } else {
        int depthParent = getDepth(tree, parentIndex); // fonction récursive à définir
        if (depthParent + 1 > tree->depth) tree->depth = depthParent + 1;
    }

    return tree;
}

// Positionne le reste de l'arbre par rapport au premier noeud
void rmviPositioningTree(rmviTree *tree,float spaceTreeRatioX, float spaceTreeRatioY){
    if (tree == NULL) return;
    for (int i = 0; i < tree->n; i++) {
        int size = tree->nEnfants[i];
        for(int j = 0; j < size; j++) {
            rmviFrame *childFrame = tree->frames[tree->enfants[i][j]];
            rmviFrame *parentFrame = tree->frames[i];
            float offset = (float)j - (float)(size - 1) / 2.0f;
            // Calculer la position du cadre enfant par rapport au parent
            rmviUpdateFrame(childFrame, parentFrame->outerRect.x + parentFrame->outerRect.width*spaceTreeRatioX,
                            parentFrame->outerRect.y + parentFrame->outerRect.height * spaceTreeRatioY * offset,
                            childFrame->outerRect.width, childFrame->outerRect.height,
                            childFrame->lineThick, childFrame->innerColor, childFrame->outerColor,
                            childFrame->text, childFrame->font);
            
        }
    }
}

// Zoom l'arbre entier en appliquant le zoom à chaque frame
void rmviZoomTree(rmviTree *tree, float zoomFactor){
    if (tree == NULL) return;
    for (int i = 0; i < tree->n; i++) {
        rmviZoomFrame(tree->frames[i], zoomFactor);  // frames[i] est un pointeur
    }
}



// Dessine l'arbre mais ne recalcule pas les positions
void rmviDrawTree(rmviTree *tree, int arrows) {
    if (tree == NULL) return;
    for (int i = 0; i < tree->n; i++) {
        rmviFrame *frame = tree->frames[i];
        // Dessiner le cadre ici
        rmviDrawFrame(*frame, RATIODEFAULT);
        if(arrows ==1) {
            int nChildren = tree->nEnfants[i];
            for (int j = 0; j < nChildren; j++) {
                int childIndex = tree->enfants[i][j];
                rmviFrame *childFrame = tree->frames[childIndex];
                Vector2 start = (Vector2){ frame->outerRect.x + frame->outerRect.width, frame->outerRect.y + frame->outerRect.height / 2.0f };
                Vector2 end = (Vector2){ childFrame->outerRect.x, childFrame->outerRect.y + childFrame->outerRect.height / 2.0f };
                rmviDrawArrow2(start, end, ARROWSIZE, RATIODEFAULT, DRAWCOLOR);
            }
        }
        else if( arrows == 2) rmviDrawTreeSquareWrite(tree, SPLIT_LEFT_ARROWS, NULL, DRAWCOLOR, RATIOWRITEARROWS, SIZETEXT);
    }
}
// Dessine l'arbre avec des flèches en angle droit et du texte sur les flèches
// leftRatio : proportion de la flèche avant le virage
void rmviDrawTreeSquareWrite(rmviTree *tree, float leftRatio, const char **listText, Color color, float ratioWriteArrow, float size) {
    if (tree == NULL) return;
    int count = 0;
    for (int i = 0; i < tree->n; i++) {
        rmviFrame *frame = tree->frames[i];
        // Dessiner le cadre ici
        rmviDrawFrame(*frame, RATIODEFAULT);
        int nChildren = tree->nEnfants[i];
        for (int j = 0; j < nChildren; j++) {
            int childIndex = tree->enfants[i][j];
            rmviFrame *childFrame = tree->frames[childIndex];
            Vector2 start = (Vector2){ frame->outerRect.x + frame->outerRect.width, frame->outerRect.y + frame->outerRect.height / 2.0f };
            Vector2 end = (Vector2){ childFrame->outerRect.x, childFrame->outerRect.y + childFrame->outerRect.height / 2.0f };
            rmviDrawArrowRect2(start, end, ARROWSIZE, RATIODEFAULT, leftRatio, DRAWCOLOR);
            if (listText != NULL && listText[count + j] != NULL) {
                float startRight = start.x + (end.x- start.x)* leftRatio;
                Vector2 position = { startRight + (end.x - startRight) * ratioWriteArrow - rmviCalcTextWidth(listText[count + j], mathFont, size, size / RATIO_SPACE)/2, end.y - rmviCalcTextHeight(listText[count + j], mathFont, size, size / RATIO_SPACE, false)} ;
                rmviMyDrawText(listText[count + j], position, size, color);
            }
        }
        count += nChildren;
    }
}

// done un atom
rmviAtom rmviGetAtom(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter){
    rmviAtom atom;
    atom.frame = frame;
    atom.nature = nature;
    atom.lifespan = lifespan;
    atom.lambda = logf(2.0f) / lifespan;  
    atom.daughter = daughter;
    atom.alive = true;
    frame ? (atom.center = (Vector2){ frame->outerRect.x + frame->outerRect.width / 2.0f, frame->outerRect.y + frame->outerRect.height / 2.0f }) : (atom.center = (Vector2){0.0f, 0.0f});
    atom.speed = (Vector2){0.0f, 0.0f};
    return atom;
}
// get an atom with a speed
rmviAtom rmviGetAtomSpeed(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter, Vector2 speed){
    rmviAtom atom = rmviGetAtom(frame, nature, lifespan, daughter);
    atom.speed = speed;
    return atom;
}
// regarde si il y'a eu une désintégration
bool rmviDesintegration(rmviAtom *atom, float deltaTime) {
    if (atom == NULL || !atom->alive) return false;
    if (atom->lifespan <= 0) return false; // stable
    float p = 1.0f - expf(-atom->lambda * deltaTime); // proba de désintégration pendant dt
    float r = rmviRand();        // tirage aléatoire uniforme [0,1]
    return r < p;
}
// met à jour l'atome si jamais il y'a eu une desintégration
void rmviAtomDecay(rmviAtom *atom){
    // en fonction de la desintégration doit crée les particules qui dégagent
    atom->nature = atom->daughter->nature;
    atom->lifespan = atom->daughter->lifespan;
    atom->lambda = atom->daughter->lambda;
    atom->daughter = atom->daughter->daughter;
    rmviRewriteFrame(atom->frame, atom->nature);
}
bool rmviVector2IsZero(Vector2 vec) {
    return (vec.x == 0.0f && vec.y == 0.0f);
}
void rmviAtomUpdate(rmviAtom *atom){
    if(!rmviVector2IsZero(atom->speed)){
        rmviUpdateFrame(atom->frame, atom->frame->outerRect.x + atom->speed.x , atom->frame->outerRect.y + atom->speed.y, atom->frame->outerRect.width, atom->frame->outerRect.height, atom->frame->lineThick, atom->frame->innerColor, atom->frame->outerColor, atom->frame->text, atom->frame->font);
    }
}

rmviFrame rmviGetElectron(float posX, float posY){
    rmviFrame electron = rmviGetRoundFrameBGCentered(posX, posY, 40, 5, "e-", mathFont);
    return electron;
}
rmviFrame rmviGetNeutrino(float posX, float posY){
    rmviFrame neutrino = rmviGetRoundFrameBGCentered(posX, posY, 50, 5, "/nu", mathFont);
    return neutrino;
}
void rmviAtomEmission(rmviAtom *atom){
    // Crée un électron et l'émet
    rmviFrame electron = rmviGetElectron(atom->frame->outerRect.x, atom->frame->outerRect.y);
    // Crée un neutrino et l'émet
    rmviFrame neutrino = rmviGetNeutrino(atom->frame->outerRect.x, atom->frame->outerRect.y);
    // angle donnée par sampleCosTheta
}
// Dessine l'atom
void rmviDrawAtom(rmviAtom atom) {
    rmviDrawFrame(*(atom.frame), RATIODEFAULT);
}
// Tire un nombre aléatoire uniforme entre 0 et 1
float rmviRand() {
    return (float)rand() / (float)RAND_MAX;
}

// Retourne une loi normale centrée réduite : N(0,1)
float rmviRandomNormalCentered(){
    // Box-Muller: transforme 2 uniformes en un normal
    float u1 = (rand() + 1.0f) / (RAND_MAX + 2.0f);
    float u2 = (rand() + 1.0f) / (RAND_MAX + 2.0f);

    float z = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * PI * u2);
    return z;
}
float rmviRandNormal(float mean, float sigma)
{
    return mean + sigma * rmviRandomNormalCentered();
}

// Tire cosθ distribué selon W(cosθ) ∝ 1 + a·β_e·cosθ
// a : coefficient de corrélation bêta-neutrino
// beta_e : vitesse e-/c (0 < beta_e < 1)
float sampleCosTheta(float a, float beta_e) {
    float cosTheta, w, wmax;
    // wmax = max(1 + a·β·cosθ) sur cosθ ∈ [-1,1]
    wmax = 1 + fabs(a * beta_e);  // maximum en cosθ = +1
    // méthode accept-reject
    while (1) {
        cosTheta = 2.0f * rmviRand() - 1.0f; // uniforme dans [-1,1]
        w = 1.0f + a * beta_e * cosTheta;
        if (rmviRand() * wmax <= w) {
            return cosTheta;
        }
    }
}

rmviPlanet rmviGetPlanet(Vector2 position, float mass, Vector2 velocity, Vector2 force, Texture2D texture, float height){
    rmviPlanet planet = { .features = { position, mass, velocity, force }, .visual = { texture, height, height } };
    return planet;
}

Color GetAverageColor(Texture2D texture) {
    // On charge les pixels en RAM
    Image img = LoadImageFromTexture(texture);
    Color *pixels = LoadImageColors(img);
    unsigned long long r = 0, g = 0, b = 0, a = 0;
    int count = 0;
    for (int i = 0; i < img.width * img.height; i++) {
        // Ignorer les pixels noirs ou quasi-noirs
        if (!(pixels[i].r < 10 && pixels[i].g < 10 && pixels[i].b < 10)) {
            r += pixels[i].r;
            g += pixels[i].g;
            b += pixels[i].b;
            a += pixels[i].a;
            count++;
        }
    }
    Color avg = BLACK;
    if (count > 0) {
        avg.r = r / count;
        avg.g = g / count;
        avg.b = b / count;
        avg.a = a / count; // ou 255 si tu veux forcer l’opacité
    }
    // Nettoyage mémoire
    UnloadImageColors(pixels);
    UnloadImage(img);
    return avg;
}


void rmviDrawFourier(FourierCoeff *coeffs, int n, Vector2 origin, float scale, Color color, float time, Vector2 *figure) {
    if (coeffs == NULL || n <= 0) return;
    Vector2 prevPoint = origin; // Point initial au centre
    for (int k = 0; k < n; k++) {
        // Calcul du point actuel
        DrawCircleLinesV(prevPoint, coeffs[k].amp * scale, color);
        Vector2 currentPoint = {
            prevPoint.x + coeffs[k].amp * scale * cosf(coeffs[k].freq *time + coeffs[k].phase), // Échelle pour la fréquence
            prevPoint.y - coeffs[k].amp * scale* sinf(coeffs[k].freq * time + coeffs[k].phase) // Inversion de l'axe Y pour l'affichage
        };
        // Dessiner la ligne entre le point précédent et le point actuel
        DrawLineV(prevPoint, currentPoint, color);
        // Mettre à jour le point précédent
        prevPoint = currentPoint;
    }
    if (figure != NULL) *figure = prevPoint;
}

void rmviDrawFourierFigure(float countFrame, Vector2 *figure, int timeFourier, int FPS, Color color) {
    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        for(int i = 1; i < min(countFrame, timeFourier * FPS); i++){
            rlVertex2f(figure[i-1].x, figure[i-1].y);
            rlVertex2f(figure[i].x, figure[i].y);
            if (i == (timeFourier * FPS - 1)) {
                rlVertex2f(figure[i].x, figure[i].y);
                rlVertex2f(figure[0].x, figure[0].y);
            }
        }
    rlEnd();
}

// ----------------------------- Jsons ---------------------------------

char* rmviReadFile(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = (char*)malloc(length + 1);
    fread(data, 1, length, f);
    data[length] = '\0';

    fclose(f);
    return data;
}
Anime2* rmviAnime2FromJSON(const char *jsonStr) {
    if (!jsonStr) return NULL;

    cJSON *root = cJSON_Parse(jsonStr);
    if (!root) return NULL;

    int ncontours = cJSON_GetArraySize(root);
    if (ncontours == 0) {
        cJSON_Delete(root);
        return NULL;
    }

    Anime2 *anime = (Anime2*)malloc(sizeof(Anime2));
    anime->ncontours = ncontours;
    anime->contours = (Contour2*)malloc(sizeof(Contour2) * ncontours);

    int i = 0;
    cJSON *contourItem = NULL;
    cJSON_ArrayForEach(contourItem, root) {
        cJSON *pointsArray = contourItem;
        int npoints = cJSON_GetArraySize(pointsArray);
        Vector2 *points = (Vector2*)malloc(sizeof(Vector2) * npoints);

        for (int j = 0; j < npoints; j++) {
            cJSON *pt = cJSON_GetArrayItem(pointsArray, j);
            if (!cJSON_IsArray(pt) || cJSON_GetArraySize(pt) < 2) continue;

            points[j].x = (float)cJSON_GetArrayItem(pt,0)->valuedouble;
            points[j].y = (float)cJSON_GetArrayItem(pt,1)->valuedouble;
        }

        anime->contours[i].points = points;
        anime->contours[i].npoints = npoints;
        i++;
    }

    cJSON_Delete(root);
    return anime;
}
void rmviAnime2Free(Anime2 *anime) {
    if (!anime) return;

    for (int i = 0; i < anime->ncontours; i++) {
        free(anime->contours[i].points);
    }
    free(anime->contours);
    free(anime);
}

void rmvidrawAnime2(Anime2 *anime, Vector2 position, float scale, Color color, bool invertY ) {
    if (!anime) return;

    for (int i = 0; i < anime->ncontours; i++) {
        Contour2 contour = anime->contours[i];
        for (int j = 0; j < contour.npoints - 1; j++) {
            Vector2 p1 = (Vector2){ position.x + contour.points[j].x * scale,  position.y + (invertY ? -1 : 1) * contour.points[j].y * scale };
            Vector2 p2 = (Vector2){ position.x + contour.points[j+1].x * scale,  position.y + (invertY ? -1 : 1) * contour.points[j+1].y * scale};
            DrawLineV(p1, p2, color);
        }
    }
}