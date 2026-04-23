#include "visual.h"
#include "ffmpeg.h"
#include "text2Latex.h"
#include "fft_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
    #define popen  _popen
    #define pclose _pclose
#endif


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
#define SPLIT_LEFT_ARROWS 0.5f // gives the left split for arrowsRect
#define RATIOWRITEARROWS 0.5f
#define TOKEN_SIZE 512 // place 512 tokens max pour commencer


//---------------------------------- List INT -------------------------------------------------
void rmviIntListInit(rmviIntList *list) {
    if (!list) return;
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

rmviIntList rmviGetIntList(int num){
    rmviIntList intlist;
    rmviIntListInit(&intlist);
    if(num<0) return intlist;
    rmviIntListAdd(&intlist,num);
    return intlist;
}


void rmviIntListFree(rmviIntList *list) {
    if (!list) return;
    free(list->data);
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool rmviIntListReserve(rmviIntList *list, int newCapacity) {
    if (!list) return false;
    if (newCapacity <= list->capacity) return true;

    int *newData = realloc(list->data, sizeof(int) * newCapacity);
    if (!newData) return false;

    list->data = newData;
    list->capacity = newCapacity;
    return true;
}

bool rmviIntListAdd(rmviIntList *list, int value) {
    if (!list) return false;

    if (list->count >= list->capacity) {
        int newCapacity = (list->capacity == 0) ? 4 : list->capacity * 2;
        if (!rmviIntListReserve(list, newCapacity)) return false;
    }

    list->data[list->count] = value;
    list->count++;
    return true;
}

bool rmviIntListContains(const rmviIntList *list, int value) {
    if (!list) return false;

    for (int i = 0; i < list->count; i++) {
        if (list->data[i] == value) return true;
    }
    return false;
}

// -------------------------------- renderList ---------------------


void rmviRenderBoxListInit(rmviRenderBoxList *list) {
    if (!list) return;
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

rmviRenderBoxList rmviGetRenderBoxList(const char *latex, State *state){
    // ici
    rmviRenderBoxList RenderBoxlist;
    rmviRenderBoxListInit(&RenderBoxlist);
    Token tokens[512];
    int tokenCount = rmviTokenizeLatex(latex,tokens, 512);
    rmviRenderBoxListReserve(&RenderBoxlist,256);
    RenderBoxlist.count = rmviBuildRenderBoxes(tokens, tokenCount, RenderBoxlist.data, state);
    return RenderBoxlist;
}

void rmviRenderBoxListFree(rmviRenderBoxList *list) {
    if (!list) return;
    free(list->data);
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool rmviRenderBoxListReserve(rmviRenderBoxList *list, int newCapacity) {
    if (!list) return false;
    if (newCapacity <= list->capacity) return true;
    RenderBox *newData = realloc(list->data, sizeof(RenderBox) * newCapacity);
    if (!newData) return false;
    list->data = newData;
    list->capacity = newCapacity;
    return true;
}
/*
bool rmviRenderBoxListAdd(rmviRenderBoxList *list, RenderBox *box) {
    if (!list) return false;
    if (list->count >= list->capacity) {
        int newCapacity = (list->capacity == 0) ? 4 : list->capacity * 2;
        if (!rmviRenderBoxListReserve(list, newCapacity)) return false;
    }
    list->data[list->count] = box;
    list->count++;
    return true;
}
*/

// -------------------------------- Float List -------------------------------------------

void rmviFloatListInit(rmviFloatList *list) {
    if (!list) return;
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

rmviFloatList rmviGetFloatList(float num) {
    rmviFloatList floatList;
    rmviFloatListInit(&floatList);
    rmviFloatListAdd(&floatList, num);
    return floatList;
}

void rmviFloatListFree(rmviFloatList *list) {
    if (!list) return;
    free(list->data);
    list->data = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool rmviFloatListReserve(rmviFloatList *list, int newCapacity) {
    if (!list) return false;
    if (newCapacity <= list->capacity) return true;

    float *newData = realloc(list->data, sizeof(float) * newCapacity);
    if (!newData) return false;

    list->data = newData;
    list->capacity = newCapacity;
    return true;
}

bool rmviFloatListAdd(rmviFloatList *list, float value) {
    if (!list) return false;

    if (list->count >= list->capacity) {
        int newCapacity = (list->capacity == 0) ? 4 : list->capacity * 2;
        if (!rmviFloatListReserve(list, newCapacity)) return false;
    }

    list->data[list->count] = value;
    list->count++;
    return true;
}

bool rmviFloatListContains(const rmviFloatList *list, float value) {
    if (!list) return false;
    for (int i = 0; i < list->count; i++) {
        if (fabsf(list->data[i] - value) < 1e-6f) return true;
    }
    return false;
}


//---------------------------------- RECTANGLE AND FRAMES ---------------------------------
void rmviMyDrawText(const char *text, Vector2 position, float size, Color color) {
    if (text == NULL || strlen(text) == 0) return;
    rmviWriteLatexClassic(text, &position);
}

void rmviDrawTextMid(const char *text, Rectangle rec, State *state) {
    Vector2 position = {rec.x + rec.width/2.0f , rec.y + rec.height/2.0f}; 
    rmviWriteLatexLeftCentered(text,&position,state); 
}

void rmviDrawTextMidClassic(const char *text, Rectangle rec){
    Vector2 position = {rec.x + rec.width/2.0f , rec.y + rec.height/2.0f}; 
    rmviWriteLatexLeftCenteredClassic(text,&position); 
}

void rmviDrawTextRec(const char *text, Rectangle rec, float ratio, float ratioWidth, float ratioHeight, State *state){
    printf("Fct rmviDrawTextRec à implémenter");
    exit(EXIT_FAILURE);
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
rmviFrame rmviGetFrame(float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor, const char *text, State *state,float roundness) {
    rmviFrame frame;
    frame.outerRect = (Rectangle){ posX, posY, width, height };
    frame.innerRect = (Rectangle){ posX + lineThick, posY + lineThick, width - 2 * lineThick, height - 2 * lineThick };
    frame.outerColor = outerColor;
    frame.innerColor = innerColor;
    frame.state = state;
    frame.lineThick = lineThick;
    frame.roundness = roundness;  // Default roundness
    rmviRenderBoxListInit(&frame.renderList);
    //ici
    Token tokens[TOKEN_SIZE];
    int tokenCount = rmviTokenizeLatex(text,tokens,TOKEN_SIZE);
    if(!rmviRenderBoxListReserve(&frame.renderList,tokenCount)) exit(0);
    frame.renderList.count =  rmviBuildRenderBoxes(tokens,tokenCount,frame.renderList.data,frame.state); 
    return frame;
}
// Construct a round frame with the given parameters
rmviFrame rmviGetRoundFrame(float posX, float posY, float radius, float lineThick, Color innerColor, Color outerColor, const char *text, State *state) {
    rmviFrame frame = rmviGetFrame(posX, posY, radius * 2, radius * 2, lineThick, innerColor, outerColor, text, state, 1.0f);
    return frame;
}

rmviFrame rmviGetRoundFrameBGCentered(float posX, float posY, float ratio, float lineThick, const char *text, State *state){
    float radius = (float)GetScreenWidth() / ratio;
    Rectangle rec = {
        posX - radius,
        posY - radius,
        radius * 2,
        radius * 2
    };
    rmviFrame frame = rmviGetFrame(rec.x, rec.y, rec.width, rec.height, lineThick, BG, DRAWCOLOR, text, state, 1.0f);
    return frame;
}

// Construct a frame with the given parameters, with a background color
rmviFrame rmviGetFrameBG(float posX, float posY, float width, float height, float lineThick, const char *text, State *state) {
    rmviFrame frame = rmviGetFrame(posX, posY, width, height, lineThick, BG, DRAWCOLOR, text, state, 0.0f);
    return frame;
}
// Construct a frame centered about x, y with a width and height based on the ratio and with the draw and background color
rmviFrame rmviGetFrameBGCentered(float posX, float posY, float ratio_x, float ratio_y, float lineThick, const char *text, State *state) {
    Rectangle rec = rmviGetRectangleCenteredRatio(posX, posY, ratio_x, ratio_y);
    rmviFrame frame = rmviGetFrame(rec.x, rec.y, rec.width, rec.height, lineThick, BG, DRAWCOLOR, text,state, 0.0f);
    return frame;
}
rmviFrame rmviGetFrameFromText(float posX, float posY, const char *text, State *state){
    rmviFrame frame = {0};
    frame.state = state;
    rmviRewriteFrame(&frame,text);
    rmviFloatList widthline;
    rmviCalcWidthLine(frame.renderList.data,frame.renderList.count,&widthline);
    float lineMax = rmviCalcLargestLine(&widthline);
    float width = max(lineMax+ 2*frame.state->sizeText, 3*frame.state->sizeText);
    float height=  max(rmviCalcHeightTotal(frame.renderList.data,frame.renderList.count)+ 2*frame.state->sizeText, 3.5*frame.state->sizeText);
    frame.outerRect = (Rectangle){ posX-width/2.0f, posY-height/2, width, height };
    frame.innerRect = (Rectangle){ posX-width/2.0f + state->spacing, posY-height/2 + state->spacing, width - 2 * state->spacing, height - 2 * state->spacing };
    frame.outerColor = DRAWCOLOR;
    frame.innerColor = BG;
    
    frame.lineThick = state->spacing;
    return frame;
}

// Draw a frame
void rmviDrawFrame(const rmviFrame *frame) {
    DrawRectangleRounded(frame->outerRect, frame->roundness, PRECISIONS, frame->outerColor);
    DrawRectangleRounded(frame->innerRect, frame->roundness, PRECISIONS, frame->innerColor);
    float height = rmviCalcHeightTotal(frame->renderList.data,frame->renderList.count);
    rmviFloatList listWidth;
    rmviCalcWidthLine(frame->renderList.data,frame->renderList.count,&listWidth);
    Vector2 drawPos = {frame->outerRect.x + frame->outerRect.width/2 ,
                    frame->outerRect.y + frame->outerRect.height/2};
    /*Vector2 drawPos = {frame->outerRect.x + frame->outerRect.width/2 - listWidth[0]/2,
                    frame->outerRect.y + frame->outerRect.height/2 - height/2};*/
    if (frame->renderList.count > 0){
        rmviDrawRenderBoxesCentered(&listWidth,height,frame->renderList.data,frame->renderList.count,drawPos);
        //rmviDrawRenderBoxes(frame->renderList.data,frame->renderList.count,drawPos);
    }
}

// fait tourner une frame autour de son centre
void rmviDrawFramePro(rmviFrame frame, float ratio, float rotation, Vector2 origin, Vector2 translation){
    rmviRotateRectangle(frame.outerRect, origin, rotation, frame.outerColor);
    rmviRotateRectangle(frame.innerRect, origin, rotation, frame.innerColor);
    if (frame.renderList.count>0) {
        printf("fonction rmviDrawFramePro à réécrire");
        exit(0);
        /*
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
            translationVector, - rotation, &frame.state);
            */
    }

}

// Update parameters frames
void rmviUpdateFrame(rmviFrame *frame, float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor, State *state) {
    frame->outerRect = (Rectangle){ posX, posY, width, height };
    frame->innerRect = (Rectangle){ posX + lineThick, posY + lineThick, width - 2 * lineThick, height - 2 * lineThick };
    frame->outerColor = outerColor;
    frame->innerColor = innerColor;
    frame->state = state;
    frame->lineThick = lineThick;

}

// Update a frame's zoom
void rmviZoomFrame(rmviFrame *frame, float zoomFactor) {
    float hypothenuse = sqrtf(frame->outerRect.width * frame->outerRect.width + frame->outerRect.height * frame->outerRect.height);
    float rateX = frame->outerRect.width / hypothenuse;
    float rateY = frame->outerRect.height / hypothenuse;
    frame->outerRect.x -= (zoomFactor * rateX)/2.0f;
    frame->outerRect.y -= (zoomFactor * rateY)/2.0f;
}

void rmviRewriteFrame(rmviFrame *frame, const char *text) {
    rmviRenderBoxListInit(&frame->renderList);
    Token tokens[TOKEN_SIZE];
    int tokenCount = rmviTokenizeLatex(text, tokens, TOKEN_SIZE);
    if (!rmviRenderBoxListReserve(&frame->renderList, tokenCount)) {
        printf("rewrite frame has bugged");
        exit(0);
    }
    frame->renderList.count = rmviBuildRenderBoxes( tokens,tokenCount,frame->renderList.data,frame->state);
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
void rmviRotateText(const char *text, Vector2 position, Vector2 origin, float rotation, State *state){
    rlPushMatrix();
        rlTranslatef(position.x  + origin.x, position.y  + origin.y, 0.0f);
        rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
        rlTranslatef(-origin.x, -origin.y, 0.0f);
        Vector2 pos = {0, 0};
        rmviWriteLatex(text, &pos, state);
    rlPopMatrix();
}

// ----------------------------- Carthesian system and legend --------------------------------------------
static Vector2 rmviCartesianVisibleRange(const rmviCartesian *cartesian) {
    Vector2 range = {0};
    if (!cartesian) return range;
    if (cartesian->gridStepPx.x != 0.0f) {
        range.x = cartesian->halfSizePx.x * cartesian->gridStepUnits.x / cartesian->gridStepPx.x;
    }
    if (cartesian->gridStepPx.y != 0.0f) {
        range.y = cartesian->halfSizePx.y * cartesian->gridStepUnits.y / cartesian->gridStepPx.y;
    }
    return range;
}

static bool rmviCartesianPointVisible(const rmviCartesian *cartesian, Vector2 point) {
    Vector2 range = rmviCartesianVisibleRange(cartesian);
    return fabsf(point.x) <= range.x && fabsf(point.y) <= range.y;
}

rmviCartesian rmviGetCartesian(Vector2 origin, Vector2 halfSizePx, Vector2 gridStepUnits) {
    rmviCartesian cartesian = {0};
    cartesian.origin = origin;
    cartesian.halfSizePx = halfSizePx;
    cartesian.gridStepUnits = gridStepUnits;
    cartesian.gridStepPx = (Vector2){50, 50};
    cartesian.thickness = 3;
    cartesian.sizeText = 40;
    cartesian.spacing = 2;
    cartesian.alphaGrid = 0.5f;
    return cartesian;
}

void rmviSetCartesianAxesLabel(rmviCartesian *cartesian,const char *xlabel,const char* ylabel){
    snprintf(cartesian->xlabel, sizeof(cartesian->xlabel), "%s ", xlabel);
    snprintf(cartesian->ylabel, sizeof(cartesian->ylabel), "%s ", ylabel);
}

void rmviSetCartesianTitle(rmviCartesian *cartesian,const char *title){
    snprintf(cartesian->title, sizeof(cartesian->title), "%s", title);
}

void rmviSetCartesianGridStepPx(rmviCartesian *cartesian, Vector2 gridStepPx) {
    cartesian->gridStepPx = gridStepPx;
}

Vector2 rmviCartesianToScreen(const rmviCartesian *cartesian, Vector2 point) {
    Vector2 screenPoint = cartesian->origin;
    if (cartesian->gridStepUnits.x != 0.0f) {
        screenPoint.x += point.x * cartesian->gridStepPx.x / cartesian->gridStepUnits.x;
    }
    if (cartesian->gridStepUnits.y != 0.0f) {
        screenPoint.y -= point.y * cartesian->gridStepPx.y / cartesian->gridStepUnits.y;
    }
    return screenPoint;
}

static float rmviPolarVisibleRange(const rmviPolar *polar) {
    float range = 0.0f;
    if (!polar) return range;
    if (polar->gridStepPx != 0.0f) {
        range = polar->halfLength * polar->gridStepUnits / polar->gridStepPx;
    }
    return range;
}

static bool rmviPolarPointVisible(const rmviPolar *polar, Vector2 point) {
    float range = rmviPolarVisibleRange(polar);
    return point.x >= 0.0f && point.x <= range;
}

rmviPolar rmviGetPolar(Vector2 origin, float halfLength, float gridStepUnits) {
    rmviPolar polar = {0};
    polar.origin = origin;
    polar.halfLength = halfLength;
    polar.gridStepUnits = gridStepUnits;
    polar.gridStepPx = 50.0f;
    polar.thickness = 3;
    polar.sizeText = 40;
    polar.spacing = 2;
    polar.nTheta = 12;
    polar.alphaGrid = 0.5f;
    return polar;
}

void rmviSetPolarAxesLabel(rmviPolar *polar,const char *xlabel,const char* ylabel){
    snprintf(polar->xlabel, sizeof(polar->xlabel), "%s", xlabel);
    snprintf(polar->ylabel, sizeof(polar->ylabel), "%s", ylabel);
}

void rmviSetPolarTitle(rmviPolar *polar,const char *title){
    snprintf(polar->title, sizeof(polar->title), "%s", title);
}

void rmviSetPolarGridStepPx(rmviPolar *polar, float gridStepPx) {
    polar->gridStepPx = gridStepPx;
}

Vector2 rmviPolarToScreen(const rmviPolar *polar, Vector2 point) {
    float radiusPx = 0.0f;
    if (polar->gridStepUnits != 0.0f) {
        radiusPx = point.x * polar->gridStepPx / polar->gridStepUnits;
    }
    return (Vector2){
        polar->origin.x + radiusPx * cosf(point.y),
        polar->origin.y - radiusPx * sinf(point.y)
    };
}

void rmviUpdatePolar(rmviPolar *polar, Vector2 origin, float gridStepUnits, float halfLength) {
    polar->origin = origin;
    polar->gridStepUnits = gridStepUnits;
    polar->halfLength = halfLength;
}

rmviLegend rmviGetLegend(int capacity){
    rmviLegend legend = {0};
    legend.color = malloc(sizeof(Color) * capacity);
    legend.name = malloc(sizeof(char*) * capacity);
    legend.count = 0;
    legend.capacity = capacity;
    legend.sizeText = 30;
    return legend;
}

void rmviAddLegend(rmviLegend *legend, Color color, const char *name){
    // Add an element to the legend
    if (legend->count >= legend->capacity) return; // ici réalloué
    int i = legend->count;
    legend->color[i] = color;
    legend->name[i] = malloc(strlen(name) + 1);
    strcpy(legend->name[i], name);
    legend->count++;
}

void rmviDrawLegend(rmviLegend *legend, Vector2 origin, Vector2 halfSizePx){
    if (legend == NULL) return;
    Vector2 position = Vector2Subtract(origin, Vector2Scale(halfSizePx, 1));
    State state = rmviGetState(legend->sizeText,legend->sizeText/20,1.2f,halfSizePx.x,true,0,mathFont,WHITE);
    for(int i =0; i< legend->count; i++){
        DrawCircleV(Vector2Add(position,(Vector2){-legend->sizeText/2,legend->sizeText/2}), legend->sizeText/4,legend->color[i]);
        rmviWriteLatex(legend->name[i],&position,&state);
        position.y += legend->sizeText*1.2;
    }
}
char *numberToScientific(float nombre){
    static char text[32];
    if (nombre == 0.0f){
        snprintf(text, sizeof(text), "0");
        return text;
    }
    float absNombre = fabsf(nombre);
    int exposant = (int)floorf(log10f(absNombre));
    if (exposant > 3 || exposant < 0){
        float mantisse = nombre / powf(10.0f, (float)exposant);
        if (mantisse == 1.0f) snprintf(text, sizeof(text), "10^{%d}", exposant);
        else snprintf(text, sizeof(text), "%.1f/cross 10^{%d}", mantisse, exposant);
    }
    else{
        if (fabsf(nombre - roundf(nombre)) < 1e-6f)
            snprintf(text, sizeof(text), "%.0f", nombre);
        else
            snprintf(text, sizeof(text), "%.2f", nombre);
    }
    return text;
}

void rmviDrawTickPolar(const rmviPolar *polar, float length, float thickness, Color color) {
    if (!polar || polar->gridStepPx <= 0.0f) return;

    int nRadius = (int)floorf(polar->halfLength / polar->gridStepPx);
    State state = rmviGetStateClassic();
    state.sizeText = min(polar->sizeText, polar->gridStepPx/2);
    state.spacing = state.sizeText/20;

    for (int i = 1; i <= nRadius; i++) {
        Vector2 tickCenter = { polar->origin.x + i * polar->gridStepPx, polar->origin.y };
        Vector2 offset = { 0.0f, length/2.0f };
        DrawLineEx(Vector2Add(tickCenter, offset), Vector2Subtract(tickCenter, offset), thickness, color);

        char *buffer = numberToScientific(i * polar->gridStepUnits);
        Vector2 labelPos = {
            tickCenter.x - MeasureText(buffer, state.sizeText)/2.0f,
            tickCenter.y + state.sizeText/2.0f + length
        };
        rmviWriteLatex(buffer, &labelPos, &state);
    }

    if (polar->nTheta > 0) {
        float outerRadius = polar->halfLength;
        for (int i = 0; i < polar->nTheta; i++) {
            float theta = 2.0f * PI * i / polar->nTheta;
            Vector2 dir = { cosf(theta), -sinf(theta) };
            Vector2 outer = Vector2Add(polar->origin, Vector2Scale(dir, outerRadius));
            Vector2 inner = Vector2Subtract(outer, Vector2Scale(dir, length));
            DrawLineEx(inner, outer, thickness, color);
        }
    }
}

void rmviDrawGridsPolar(const rmviPolar *polar, Color color){
    if (!polar || polar->gridStepPx <= 0.0f) return;

    int nRadius = (int)floorf(polar->halfLength / polar->gridStepPx);
    for (int i = 1; i <= nRadius; i++) {
        DrawCircleLinesV(polar->origin, i * polar->gridStepPx, color);
    }

    if (polar->nTheta > 0) {
        float visibleRange = rmviPolarVisibleRange(polar);
        for (int i = 0; i < polar->nTheta; i++) {
            float theta = 2.0f * PI * i / polar->nTheta;
            Vector2 end = rmviPolarToScreen(polar, (Vector2){visibleRange, theta});
            DrawLineV(polar->origin, end, color);
        }
    }
}
inline Vector2 getPositionXonPlot(Vector2 origin, float halfSize, State state) {
    return Vector2Add(origin, (Vector2) {halfSize*0.8f, state.spaceSize});
}
// get the position of the ylabel on the plot, with a space between the plot and the label, and a space between the label and the title
inline Vector2 getPositionYonPlot(Vector2 origin, float halfSize, State state, float widthY) {
    return Vector2Add(origin,(Vector2) {-widthY - 1.2 * state.spaceSize, - halfSize*0.8f});
}

void rmviWriteAxisPolar(const rmviPolar *polar, float sizeText, float spacing, Font font, Color color){
    State state = rmviGetState(sizeText, spacing, 1.2f, 1.5f*polar->halfLength, true, 0, font, color);
    Token tokensR[512];
    int tokenCountR = rmviTokenizeLatex(polar->xlabel, tokensR, 512);
    RenderBox boxesR[256];
    int boxCountR = rmviBuildRenderBoxes(tokensR, tokenCountR, boxesR, &state);
    Vector2 positionR = getPositionXonPlot(polar->origin, polar->halfLength, state);
    rmviDrawRenderBoxes(boxesR, boxCountR, positionR);
    Token tokensTheta[512];
    int tokenCountTheta = rmviTokenizeLatex(polar->ylabel, tokensTheta, 512);
    RenderBox boxesTheta[256];
    int boxCountTheta = rmviBuildRenderBoxes(tokensTheta, tokenCountTheta, boxesTheta, &state);
    Vector2 positionTheta = getPositionYonPlot(polar->origin, polar->halfLength, state, rmviCalcWidthLatex(polar->ylabel, &state));
    rmviDrawRenderBoxes(boxesTheta, boxCountTheta, positionTheta);
}

void rmviWriteAxisClassicPolar(const rmviPolar *polar){
    rmviWriteAxisPolar(polar, polar->sizeText, polar->spacing, mathFont, WHITE);
}

void rmviWriteTitlePolar(const rmviPolar *polar, float sizeText, float spacing, Font font, Color color){
    Token tokens[512];
    int tokenCount = rmviTokenizeLatex(polar->title, tokens, 512);
    RenderBox boxes[256];
    State state = rmviGetState(sizeText, spacing, 1.2f, 1.5f*polar->halfLength, true, 0, font, color);
    int boxCount = rmviBuildRenderBoxes(tokens, tokenCount, boxes, &state);
    rmviFloatList listWidth;
    rmviCalcWidthLine(boxes, boxCount, &listWidth);
    Vector2 position = {
        polar->origin.x,
        polar->origin.y - polar->halfLength - sizeText - (listWidth.count-1)*(sizeText*1.2f)
    };
    rmviDrawRenderBoxesCentered(&listWidth, sizeText*1.2f*listWidth.count, boxes, boxCount, position);
    rmviFloatListFree(&listWidth);
}

void rmviWriteTitleClassicPolar(const rmviPolar *polar){
    rmviWriteTitlePolar(polar, polar->sizeText, polar->spacing, mathFont, WHITE);
}

// Draw the ticks on the carthesian system, length is the length of the ticks; log coordinate system to add
void rmviDrawTick(const rmviCartesian *cartesian, float length, float thickness, Color color) {
    int nbMax = floorf(max(cartesian->halfSizePx.x / cartesian->gridStepPx.x, cartesian->halfSizePx.y / cartesian->gridStepPx.y));
    State state = rmviGetStateClassic();
    state.sizeText = min(cartesian->sizeText,cartesian->gridStepPx.x/2);
    state.spacing = state.sizeText/20;
    for (int i = - nbMax + 1 ; i < nbMax; i ++) {
        if (abs(i) < floorf(cartesian->halfSizePx.x / cartesian->gridStepPx.x)) {
            Vector2 positionThickX = (Vector2) { cartesian->origin.x + i * cartesian->gridStepPx.x, cartesian->origin.y};
            Vector2 lengthY = (Vector2) {0 , length/2};
            DrawLineEx(Vector2Add(positionThickX, lengthY) , Vector2Subtract(positionThickX, lengthY), thickness, color);
            if(i == 1){
                char *buffer = numberToScientific(cartesian->gridStepUnits.x);
                positionThickX.y += state.sizeText/2.0f;
                positionThickX.x -= MeasureText(buffer,state.sizeText)/2;
                rmviWriteLatex(buffer, &positionThickX,&state);
            }
        }
        if (abs(i) < floorf(cartesian->halfSizePx.y / cartesian->gridStepPx.y)){
            Vector2 positionThickY = (Vector2) { cartesian->origin.x, cartesian->origin.y + i * cartesian->gridStepPx.y};
            Vector2 lengthX = (Vector2) {length/2 , 0};
            DrawLineEx(Vector2Add(positionThickY, lengthX) , Vector2Subtract(positionThickY, lengthX), thickness, color);
            if(i == -1){
                char *buffer = numberToScientific(cartesian->gridStepUnits.y);
                positionThickY.y -= state.sizeText/2;
                positionThickY.x -= rmviCalcWidthLatex(buffer, &state) + state.sizeText/5 ;
                rmviWriteLatex(buffer, &positionThickY,&state);
            }
        }
    }
}

// Draw the grids on the carthesian system, should be modified for the thickness
void rmviDrawGrids(Vector2 origin, Vector2 halfSizePx, Vector2 gridStepPx, Color color){
    int nbMax = floorf(max(halfSizePx.x / gridStepPx.x, halfSizePx.y / gridStepPx.y));
    for (int i = - nbMax ; i <= nbMax; i ++) {
        if (abs(i) <= floorf(halfSizePx.x / gridStepPx.x)) {
            Vector2 positionThickX = (Vector2) { origin.x + i * gridStepPx.x, origin.y};
            Vector2 lengthY = (Vector2) {0 , halfSizePx.y};
            DrawLineV(Vector2Add(positionThickX, lengthY) , Vector2Subtract(positionThickX, lengthY), color);
        }
        if (abs(i) <= floorf(halfSizePx.y / gridStepPx.y)){
            Vector2 positionThickY = (Vector2) { origin.x, origin.y + i * gridStepPx.y};
            Vector2 lengthX = (Vector2) {halfSizePx.x , 0};
            DrawLineV(Vector2Add(positionThickY, lengthX) , Vector2Subtract(positionThickY, lengthX), color);
        }
    }
}

// Draw the carthesian system
void rmviWriteAxisClassic(const rmviCartesian *cartesian){
    rmviWriteAxis(cartesian,cartesian->sizeText,cartesian->spacing,mathFont,WHITE);
}
void rmviDrawCartesianFull(const rmviCartesian *cartesian, float arrowSize, float ratio, Color color,bool drawTicks, bool drawGrids) {
    rmviDrawArrow2(
        (Vector2){ cartesian->origin.x - cartesian->halfSizePx.x, cartesian->origin.y },
        (Vector2){ cartesian->origin.x + cartesian->halfSizePx.x , cartesian->origin.y },
        arrowSize, ratio, color);
    rmviDrawArrow2(
        (Vector2){ cartesian->origin.x, cartesian->origin.y + cartesian->halfSizePx.y },
        (Vector2){ cartesian->origin.x, cartesian->origin.y - cartesian->halfSizePx.y },
        arrowSize, ratio, color);
    if (drawGrids) rmviDrawGrids(cartesian->origin, cartesian->halfSizePx, cartesian->gridStepPx, (Color) { color.r, color.g, color.b, cartesian->alphaGrid * color.a });
    if (drawTicks) rmviDrawTick(cartesian, arrowSize/2,2.0f, color);
    if (cartesian->title[0] != '\0') rmviWriteTitleClassic(cartesian);
    if(cartesian->xlabel[0] != '\0' && cartesian->ylabel[0] != '\0') rmviWriteAxisClassic(cartesian);
    if (cartesian->legendData) rmviDrawLegend(cartesian->legendData, cartesian->origin, cartesian->halfSizePx);
}

void rmviDrawPolarFull(const rmviPolar *polar, float arrowSize, float ratio, Color color, bool drawTicks, bool drawGrids) {
    Vector2 halfSizePx = { polar->halfLength, polar->halfLength };
    rmviDrawArrow2(
        (Vector2){ polar->origin.x - polar->halfLength, polar->origin.y },
        (Vector2){ polar->origin.x + polar->halfLength , polar->origin.y },
        arrowSize, ratio, color);
    rmviDrawArrow2(
        (Vector2){ polar->origin.x, polar->origin.y + polar->halfLength },
        (Vector2){ polar->origin.x, polar->origin.y - polar->halfLength },
        arrowSize, ratio, color);
    if (drawGrids) rmviDrawGridsPolar(polar, (Color) { color.r, color.g, color.b, polar->alphaGrid * color.a });
    if (drawTicks) rmviDrawTickPolar(polar, arrowSize/2, 2.0f, (Color) { color.r, color.g, color.b, polar->alphaGrid * color.a });
    if (polar->title[0] != '\0') rmviWriteTitleClassicPolar(polar);
    if(polar->xlabel[0] != '\0' && polar->ylabel[0] != '\0')  rmviWriteAxisClassicPolar(polar);
    if (polar->legendData) rmviDrawLegend(polar->legendData, polar->origin, halfSizePx);
}

void rmviDrawListPolar(const rmviPolar *polar, const float *listR, const float *listTheta, int count, bool line, Color color){
    if (!listR || !listTheta || count <= 0) return;

    Vector2 pPrev = {0};
    bool hasPrev = false;
    for (int i = 0; i < count; i++) {
        Vector2 polarPoint = { listR[i], listTheta[i] };
        if (!rmviPolarPointVisible(polar, polarPoint)) {
            hasPrev = false;
            continue;
        }

        Vector2 p = rmviPolarToScreen(polar, polarPoint);
        if (line) {
            if (hasPrev) DrawLineEx(pPrev, p, polar->thickness, color);
            pPrev = p;
            hasPrev = true;
        }
        else {
            DrawPixelV(p, color);
        }
    }
}


void rmviWriteAxis(const rmviCartesian *cartesian,float sizeText, float spacing,Font font,Color color){
    State state = rmviGetState(sizeText,spacing,1.2f,1.5f*cartesian->halfSizePx.x,true,0,font,color);
    Token tokensX[512];
    int tokenCountX = rmviTokenizeLatex(cartesian->xlabel,tokensX, 512);
    RenderBox boxesX[256];
    int boxCountX = rmviBuildRenderBoxes(tokensX, tokenCountX, boxesX, &state);
    Vector2 positionX = getPositionXonPlot(cartesian->origin, cartesian->halfSizePx.x, state);
    rmviDrawRenderBoxes(boxesX,boxCountX,positionX);
    Token tokensY[512];
    int tokenCountY = rmviTokenizeLatex(cartesian->ylabel,tokensY, 512);
    RenderBox boxesY[256];
    int boxCountY = rmviBuildRenderBoxes(tokensY, tokenCountY, boxesY, &state);
    Vector2 positionY = getPositionYonPlot(cartesian->origin, cartesian->halfSizePx.y, state, rmviCalcWidthLatex(cartesian->ylabel, &state));
    rmviDrawRenderBoxes(boxesY,boxCountY,positionY);
}
void rmviWriteTitle(const rmviCartesian *cartesian,float sizeText, float spacing,Font font,Color color){
    Token tokens[512];
    int tokenCount = rmviTokenizeLatex(cartesian->title,tokens, 512);
    RenderBox boxes[256];
    State state = rmviGetState(sizeText,spacing,1.2f,1.5f*cartesian->halfSizePx.x,true,0,font,color);
    int boxCount = rmviBuildRenderBoxes(tokens, tokenCount, boxes, &state);
    rmviFloatList listWidth;
    rmviCalcWidthLine(boxes,boxCount,&listWidth);
    Vector2 position = Vector2Add(cartesian->origin, (Vector2) {0,- cartesian->halfSizePx.y - sizeText - (listWidth.count-1)*(sizeText*1.2f)});
    rmviDrawRenderBoxesCentered(&listWidth,sizeText*1.2* listWidth.count,boxes, boxCount, position);
    rmviFloatListFree(&listWidth);
}
void rmviWriteTitleClassic(const rmviCartesian *cartesian){
    rmviWriteTitle(cartesian,cartesian->sizeText,cartesian->spacing,mathFont,WHITE);
}

// Update the carthesian system
void rmviUpdateCartesian(rmviCartesian *cartesian, Vector2 origin, Vector2 gridStepUnits , Vector2 halfSizePx) {
    cartesian->origin = origin;
    cartesian->gridStepUnits = gridStepUnits;
    cartesian->halfSizePx = halfSizePx;
}

// Draw a function on the carthesian system
void rmviDrawFunction(const rmviCartesian *cartesian, MathFunction fct, Color color) {
    int nbPoints = NBPOINTS;
    float xRange = rmviCartesianVisibleRange(cartesian).x;
    float step = (xRange * 2.0f) / nbPoints;

    float x2 = 0.0f;
    float y2 = 0.0f;
    Vector2 p2 = {0};
    for (int i = 0; i < nbPoints; i++) {
        if (i != 0) {
            Vector2 p1 = p2;
            x2 = -xRange + i * step;
            y2 = fct(x2);
            p2 = rmviCartesianToScreen(cartesian, (Vector2){x2, y2});
            if (rmviCartesianPointVisible(cartesian, (Vector2){x2, y2})) {
                DrawLineV(p1, p2, color);
            }
        }
        else {
            x2 = -xRange + i * step;
            y2 = fct(x2);
            p2 = rmviCartesianToScreen(cartesian, (Vector2){x2, y2});
        }
    }
}
// listX/listY : valeurs en "unités" (pas en pixels)
// count       : nb de points valides
// line        : si true => relie les points, sinon dessine juste des points
void rmviDrawList2(const rmviCartesian *cartesian, const float *listX, const float *listY, int count, bool line, Color color){
    if (!listX || !listY || count <= 0) return;
    Vector2 pPrev = {0};
    bool hasPrev = false;
    for (int i = 0; i < count; i++){
        float x = listX[i];
        float y = listY[i];
        if (!rmviCartesianPointVisible(cartesian, (Vector2){x, y})) {
            hasPrev = false; // casse la ligne si on sort du cadre
            continue;
        }
        Vector2 p = rmviCartesianToScreen(cartesian, (Vector2){x, y});
        if (line){
            if (hasPrev) DrawLineEx(pPrev, p, cartesian->thickness, color);
            pPrev = p;
            hasPrev = true;
        }
        else{
            DrawPixelV(p, color);
        }
    }
}

void rmviLegendFree(rmviLegend *legend){
    for(int i = 0; i < legend->count; i++)
        free(legend->name[i]);
    free(legend->name);
    free(legend->color);
}

// circles axes projections
float circleX(float t, float radius) {
    return radius * cosf(t);
}
float circleY(float t, float radius) {
    return radius * sinf(t);
}

// Draw a parametric curve defined by two functions which takes 2 args
void rmviDraw2Parametric(const rmviCartesian *cartesian, float fx(float,float), float fy(float,float), float radius, float tMin, float tMax, int n, Color color) {
    float dt = (tMax - tMin) / n;
    for (int i = 0; i < n; i++) {
        float t1 = tMin + i * dt;
        float t2 = t1 + dt;
        float x1 = fx(t1, radius);
        float y1 = fy(t1, radius);
        float x2 = fx(t2, radius);
        float y2 = fy(t2, radius);
        Vector2 p1 = rmviCartesianToScreen(cartesian, (Vector2){x1, y1});
        Vector2 p2 = rmviCartesianToScreen(cartesian, (Vector2){x2, y2});
        DrawLineV(p1, p2, color);
    }
}



// ----------------- Fonctions pour vidéos, à mettre dans un fichier séparé -----------------
// Draw a point a trigonometric circle and its projections on the axes 
void rmviDrawTrigo(const rmviCartesian *cartesian, float x, float y, Color color, float radius) {
    float size = 7.0f;
    Vector2 point = rmviCartesianToScreen(cartesian, (Vector2){x, y});
    DrawLineEx(cartesian->origin, point, size ,color);
    DrawLineEx(cartesian->origin, (Vector2){ point.x, cartesian->origin.y },size, GREEN);
    DrawLineEx(cartesian->origin, (Vector2){ cartesian->origin.x, point.y }, size, RED);
    DrawLineEx(point, (Vector2){ point.x, cartesian->origin.y }, size, color);
    DrawLineEx(point, (Vector2){ cartesian->origin.x, point.y }, size, color);
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
                            childFrame->lineThick, childFrame->innerColor, childFrame->outerColor, childFrame->state);
            
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
        rmviDrawFrame(frame);
        if(arrows == ARROWDIRECT) {
            int nChildren = tree->nEnfants[i];
            for (int j = 0; j < nChildren; j++) {
                int childIndex = tree->enfants[i][j];
                rmviFrame *childFrame = tree->frames[childIndex];
                Vector2 start = (Vector2){ frame->outerRect.x + frame->outerRect.width, frame->outerRect.y + frame->outerRect.height / 2.0f };
                Vector2 end = (Vector2){ childFrame->outerRect.x, childFrame->outerRect.y + childFrame->outerRect.height / 2.0f };
                rmviDrawArrow2(start, end, ARROWSIZE, RATIODEFAULT, DRAWCOLOR);
            }
        }
        else if( arrows == ARROWSQUARE) rmviDrawTreeSquareWrite(tree, SPLIT_LEFT_ARROWS, NULL, DRAWCOLOR, RATIOWRITEARROWS, SIZETEXT);
        else rmviDrawTreeSquareWrite(tree, SPLIT_LEFT_ARROWS, NULL, DRAWCOLOR, RATIOWRITEARROWS, SIZETEXT);
    }
}

rmviGraph *rmviGetGraph(void){
    rmviGraph *graph = malloc(sizeof(rmviGraph));
    if(graph == NULL) return NULL;
    graph->n = 0;
    graph->depth = 0;
    graph->frames = NULL;
    graph->numeros = NULL;
    graph->parents = NULL;
    graph->enfants = NULL;
    return graph;
}
// suppose pas de cycle
int rmviGetDepth(const rmviGraph *graph, int index){
    if(graph == NULL) return 0;
    if(index < 0 || index >= graph->n) return 0;
    if(graph->parents[index].count == 0) return 1;
    int maxDepth = 0;
    for(int i = 0; i < graph->parents[index].count; i++){
        int parentIndex = graph->parents[index].data[i];
        int parentDepth = rmviGetDepth(graph, parentIndex);
        if(parentDepth > maxDepth) maxDepth = parentDepth;
    }
    return maxDepth + 1;
}

// ajoute un lien parent enfant
bool rmviAddEdge2Graph(rmviGraph *graph, int parentIndex, int childIndex){
    if(graph == NULL) return false;
    if(parentIndex < 0 || parentIndex >= graph->n) return false;
    if(childIndex < 0 || childIndex >= graph->n) return false;
    if(parentIndex == childIndex) return false;
    if(!rmviIntListContains(&graph->enfants[parentIndex], childIndex)){
        if(!rmviIntListAdd(&graph->enfants[parentIndex], childIndex)) return false;
    }
    if(!rmviIntListContains(&graph->parents[childIndex], parentIndex)){
        if(!rmviIntListAdd(&graph->parents[childIndex], parentIndex)) return false;
    }
    return true;
}

void rmviAddFrame2Graph(rmviGraph *graph, rmviFrame *frame, const rmviIntList *parents){
    if(graph == NULL || frame == NULL) return;

    int newIndex = graph->n;

    rmviFrame **newFrames = realloc(graph->frames, sizeof(rmviFrame *) * (graph->n + 1));
    if(newFrames == NULL) return;
    graph->frames = newFrames;

    int *newNumeros = realloc(graph->numeros, sizeof(int) * (graph->n + 1));
    if(newNumeros == NULL) return;
    graph->numeros = newNumeros;

    rmviIntList *newParents = realloc(graph->parents, sizeof(rmviIntList) * (graph->n + 1));
    if(newParents == NULL) return;
    graph->parents = newParents;

    rmviIntList *newEnfants = realloc(graph->enfants, sizeof(rmviIntList) * (graph->n + 1));
    if(newEnfants == NULL) return;
    graph->enfants = newEnfants;

    graph->frames[newIndex] = frame;
    graph->numeros[newIndex] = newIndex;
    rmviIntListInit(&graph->parents[newIndex]);
    rmviIntListInit(&graph->enfants[newIndex]);

    frame->num = newIndex;
    graph->n++;

    if(parents != NULL){
        for(int i = 0; i < parents->count; i++){
            int parentIndex = parents->data[i];
            if(parentIndex < 0 || parentIndex >= newIndex) continue;
            rmviAddEdge2Graph(graph, parentIndex, newIndex);
        }
    }

    int newDepth = rmviGetDepth(graph, newIndex);
    if(newDepth > graph->depth) graph->depth = newDepth;
    if(graph->depth == 0) graph->depth = 1;
}

void rmviAddFrame2GraphSingleParent(rmviGraph *graph, rmviFrame *frame, int parentIndex){
    rmviIntList intList; 
    rmviIntListInit(&intList);
    rmviIntListAdd(&intList,parentIndex);
    rmviAddFrame2Graph(graph, frame, &intList);
}

void rmviPositioningGraph(rmviGraph *graph, float spaceTreeRatioX, float spaceTreeRatioY){
    if(graph == NULL) return;
    for(int i = 0; i < graph->n; i++) {
        // Si plusieurs parents, on place le noeud à la moyenne des positions des parents
        if(graph->parents[i].count > 1){
            float avgX = 0.0f;
            float avgY = 0.0f;
            for(int j = 0; j < graph->parents[i].count; j++){
                int parentIndex = graph->parents[i].data[j];
                rmviFrame *parentFrame = graph->frames[parentIndex];
                avgX += parentFrame->outerRect.x + parentFrame->outerRect.width;
                avgY += parentFrame->outerRect.y + parentFrame->outerRect.height * 0.5f;
            }
            avgX  = avgX /(float)graph->parents[i].count + spaceTreeRatioX;
            avgY  = avgY / (float)graph->parents[i].count + spaceTreeRatioY;
            rmviFrame *frame = graph->frames[i];
            frame->outerRect = (Rectangle){
                avgX,
                avgY - frame->outerRect.height * 0.5f,
                frame->outerRect.width,
                frame->outerRect.height
            };

            frame->innerRect = (Rectangle){
                frame->outerRect.x + frame->lineThick,
                frame->outerRect.y + frame->lineThick,
                frame->outerRect.width - 2.0f * frame->lineThick,
                frame->outerRect.height - 2.0f * frame->lineThick
            };
        }

        int size = graph->enfants[i].count;

        for(int j = 0; j < size; j++) {
            int childIndex = graph->enfants[i].data[j];
            rmviFrame *childFrame = graph->frames[childIndex];
            rmviFrame *parentFrame = graph->frames[i];
            float offset = (float)j - (float)(size - 1) / 2.0f;
            rmviUpdateFrame(childFrame,
                            parentFrame->outerRect.x + parentFrame->outerRect.width + spaceTreeRatioX,
                            parentFrame->outerRect.y + parentFrame->outerRect.height * spaceTreeRatioY * offset,
                            childFrame->outerRect.width,
                            childFrame->outerRect.height,
                            childFrame->lineThick,
                            childFrame->innerColor,
                            childFrame->outerColor,
                            childFrame->state);
        }
    }
}

void rmviPositioningGraphAverageParents(rmviGraph *graph, float spaceTreeRatioX, float spaceTreeRatioY){
    // on peut ajouter un paramettre pour bot ou top non ?
    if(graph == NULL) return;
    for(int i = 0; i < graph->n; i++){
        rmviFrame *frame = graph->frames[i];
        if(graph->parents[i].count == 0) continue;
        float avgX = 0.0f;
        float avgY = 0.0f;
        for(int j = 0; j < graph->parents[i].count; j++){
            int parentIndex = graph->parents[i].data[j];
            rmviFrame *parentFrame = graph->frames[parentIndex];
            avgX += parentFrame->outerRect.x + parentFrame->outerRect.width * spaceTreeRatioX;
            avgY += parentFrame->outerRect.y + parentFrame->outerRect.height * 0.5f;
        }
        avgX /= (float)graph->parents[i].count;
        avgY /= (float)graph->parents[i].count;
        frame->outerRect = (Rectangle){ avgX, - frame->outerRect.height * 0.5f , frame->outerRect.width, frame->outerRect.height };
        frame->innerRect = (Rectangle){ avgX + frame->lineThick, avgY + frame->lineThick, frame->outerRect.width - 2 * frame->lineThick, frame->outerRect.height - 2 * frame->lineThick };
    }
}

void rmviZoomGraph(rmviGraph *graph, float zoomFactor){
    if(graph == NULL) return;

    for(int i = 0; i < graph->n; i++){
        rmviZoomFrame(graph->frames[i], zoomFactor);
    }
}

void rmviDrawGraph(rmviGraph *graph, int arrows){
    if(graph == NULL) return;
    for(int i = 0; i < graph->n; i++){
        rmviFrame *frame = graph->frames[i];
        rmviDrawFrame(frame);
        if(arrows == ARROWDIRECT){
            int nChildren = graph->enfants[i].count;
            for(int j = 0; j < nChildren; j++){
                int childIndex = graph->enfants[i].data[j];
                rmviFrame *childFrame = graph->frames[childIndex];
                Vector2 start = (Vector2){
                    frame->outerRect.x + frame->outerRect.width,
                    frame->outerRect.y + frame->outerRect.height / 2.0f
                };
                Vector2 end = (Vector2){
                    childFrame->outerRect.x,
                    childFrame->outerRect.y + childFrame->outerRect.height / 2.0f
                };
                rmviDrawArrow2(start, end, ARROWSIZE, RATIODEFAULT, DRAWCOLOR);
            }
        }
    }
    if(arrows == ARROWSQUARE){
        rmviDrawGraphSquareWrite(graph, SPLIT_LEFT_ARROWS, NULL, DRAWCOLOR, RATIOWRITEARROWS, SIZETEXT);
    }
    else if(arrows != ARROWDIRECT){
        rmviDrawGraphSquareWrite(graph, SPLIT_LEFT_ARROWS, NULL, DRAWCOLOR, RATIOWRITEARROWS, SIZETEXT);
    }
}

void rmviDrawGraphSquareWrite(rmviGraph *graph, float leftRatio, const char **listText, Color color, float ratioWriteArrow, float size) {
    if (graph == NULL) return;
    int count = 0;

    for (int i = 0; i < graph->n; i++) {
        rmviFrame *frame = graph->frames[i];
        rmviDrawFrame(frame);
        int nChildren = graph->enfants[i].count;
        for (int j = 0; j < nChildren; j++) {
            int childIndex = graph->enfants[i].data[j];
            rmviFrame *childFrame = graph->frames[childIndex];

            Vector2 start = {
                frame->outerRect.x + frame->outerRect.width,
                frame->outerRect.y + frame->outerRect.height / 2.0f
            };

            Vector2 end = {
                childFrame->outerRect.x,
                childFrame->outerRect.y + childFrame->outerRect.height / 2.0f
            };

            rmviDrawArrowRect2(start, end, ARROWSIZE, RATIODEFAULT, leftRatio, color);
        }

        count += nChildren;
    }
}

void rmviFreeGraph(rmviGraph *graph){
    if(graph == NULL) return;

    for(int i = 0; i < graph->n; i++){
        rmviIntListFree(&graph->parents[i]);
        rmviIntListFree(&graph->enfants[i]);
    }

    free(graph->frames);
    free(graph->numeros);
    free(graph->parents);
    free(graph->enfants);
    free(graph);
}

// Dessine l'arbre avec des flèches en angle droit et du texte sur les flèches
// leftRatio : proportion de la flèche avant le virage
void rmviDrawTreeSquareWrite(rmviTree *tree, float leftRatio, const char **listText, Color color, float ratioWriteArrow, float size) {
    if (tree == NULL) return;
    int count = 0;
    for (int i = 0; i < tree->n; i++) {
        rmviFrame *frame = tree->frames[i];
        // Dessiner le cadre ici
        rmviDrawFrame(frame);
        int nChildren = tree->nEnfants[i];
        for (int j = 0; j < nChildren; j++) {
            int childIndex = tree->enfants[i][j];
            rmviFrame *childFrame = tree->frames[childIndex];
            Vector2 start = (Vector2){ frame->outerRect.x + frame->outerRect.width, frame->outerRect.y + frame->outerRect.height / 2.0f };
            Vector2 end = (Vector2){ childFrame->outerRect.x, childFrame->outerRect.y + childFrame->outerRect.height / 2.0f };
            rmviDrawArrowRect2(start, end, ARROWSIZE, RATIODEFAULT, leftRatio, DRAWCOLOR);
            if (listText != NULL && listText[count + j] != NULL) {
                rmviDrawTextMidClassic(listText[count + j], childFrame->innerRect);
            }
        }
        count += nChildren; 
    }
}





rmviAtom rmviGetAtom(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter){
    rmviAtom atom = {0};
    atom.frame = frame;
    atom.nature = nature;
    atom.lifespan = lifespan;
    atom.lambda = logf(2.0f) / lifespan;  
    atom.daughter = daughter;
    atom.alive = true;
    frame ? (atom.center = (Vector2d){ frame->outerRect.x + frame->outerRect.width / 2.0, frame->outerRect.y + frame->outerRect.height / 2.0 }) : 
            (atom.center = (Vector2d){0});
    return atom;
}
// get an atom with a speed
rmviAtom rmviGetAtomSpeed(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter, Vector2d speed){
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
bool rmviVector2dIsZero(Vector2d vec) {
    return (vec.x == 0.0 && vec.y == 0.0);
}
void rmviAtomUpdate(rmviAtom *atom){
    if(!rmviVector2dIsZero(atom->speed)){
        rmviUpdateFrame(atom->frame, atom->frame->outerRect.x + atom->speed.x , atom->frame->outerRect.y + atom->speed.y, atom->frame->outerRect.width, atom->frame->outerRect.height, atom->frame->lineThick, atom->frame->innerColor, atom->frame->outerColor, atom->frame->state);
        //rmviUpdateFrameText(atom->frame->text);
    }
}

rmviFrame rmviGetElectron(float posX, float posY, State *state){
    rmviFrame electron = rmviGetRoundFrameBGCentered(posX, posY, 40, 5, "e-", state);
    return electron;
}
rmviFrame rmviGetNeutrino(float posX, float posY, State *state){
    rmviFrame neutrino = rmviGetRoundFrameBGCentered(posX, posY, 50, 5, "/nu", state);
    return neutrino;
}
void rmviAtomEmission(rmviAtom *atom){
    // Crée un électron et l'émet
    rmviFrame electron = rmviGetElectron(atom->frame->outerRect.x, atom->frame->outerRect.y, atom->frame->state);
    // Crée un neutrino et l'émet
    rmviFrame neutrino = rmviGetNeutrino(atom->frame->outerRect.x, atom->frame->outerRect.y,atom->frame->state);
    // angle donnée par sampleCosTheta
}
// Dessine l'atom
void rmviDrawAtom(rmviAtom atom) {
    rmviDrawFrame((atom.frame));
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

rmviPlanet rmviGetPlanet(Vector2d position, double mass, Vector2d velocity, Vector2d force, Texture2D texture, float height){
    rmviPlanet planet = { .features = { position, mass, velocity, force }, .visual = { texture, height, height } };
    return planet;
}

void rmviAddDash2(rmviDash2 *dashPlanet,int count, int countFrame, int step, rmviDynamic2D *features, Vector2 center, Vector2 speed){
    int i;
    if ((int) countFrame%step == 0){   
        i = (int)(countFrame/step)%count;
        dashPlanet[i] = (rmviDash2){Vector2Add(Vector2d2Vector2(features->position), Vector2Scale(center,-1)), Vector2Add(Vector2d2Vector2(features->velocity), Vector2Scale(speed,-1))};
    }
}
void rmviDrawDash2Fast(rmviDash2 *dashPlanet, Vector2 center, Color color, float scale, int n) {
    rlBegin(RL_LINES);
    rlColor4ub(color.r, color.g, color.b, color.a);
    for (int i = 0; i < n; i++) {
        Vector2 start, end;
        start = Vector2Add(Vector2Scale(center,  scale), Vector2Scale(dashPlanet[i].position, scale));
        end = Vector2Add(start, Vector2Scale(Vector2Normalize(dashPlanet[i].velocity), 10.0f));
        rlVertex2f(GetScreenWidth()/2.0f + start.x, GetScreenHeight()/2.0f + start.y);
        rlVertex2f(GetScreenWidth()/2.0f + end.x, GetScreenHeight()/2.0f + end.y);
    }
    rlEnd();
}

rmviPlanet3D rmviGetPlanet3D(Vector3d position, float mass, Vector3d velocity, Vector3d force, const char* modelPath){
    rmviPlanet3D planet = {.features = { position, mass, velocity, force }, .model = LoadModel(modelPath) };
    return planet;
}

void rmviAddDash3(rmviDash3 *dashPlanet,int count, int countFrame, int step, rmviDynamic3D *features, Vector3 center, Vector3 speed){
    int i;
    if ((int) countFrame%step == 0){   
        i = (int)(countFrame/step)%count;
        dashPlanet[i] = (rmviDash3){Vector3Add(Vector3d2Vector3(features->position), Vector3Scale(center,-1)), Vector3Add(Vector3d2Vector3(features->velocity), Vector3Scale(speed,-1))};
    }
}

void rmviDrawDash3Fast(const rmviDash3 *dashList, int count,float dashLen, float scale,Color color)
{
    if (!dashList || count <= 0) return;

    rlBegin(RL_LINES);
    rlColor4ub(color.r, color.g, color.b, color.a);

    for (int i = 0; i < count; i++){
        Vector3 start = Vector3Scale(dashList[i].position, scale);
        Vector3 v = dashList[i].velocity;
        float vlen = Vector3Length(v);
        if (vlen < 1e-8f) continue;

        Vector3 dir = Vector3Scale(v, 1.0f / vlen);
        Vector3 end = Vector3Add(start, Vector3Scale(dir, -dashLen));

        rlVertex3f(start.x, start.y, start.z);
        rlVertex3f(end.x,   end.y,   end.z);
    }

    rlEnd();
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

static float GetVideoFPS(const char *mp4Path)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "ffprobe -v error -select_streams v:0 "
        "-show_entries stream=r_frame_rate "
        "-of default=noprint_wrappers=1:nokey=1 "
        "\"%s\"", mp4Path);
    FILE *pipe = popen(cmd, "r");
    if (!pipe) return 0.0f;
    char buffer[128] = {0};
    fgets(buffer, sizeof(buffer), pipe);
    pclose(pipe);
    int num = 0, den = 1;
    if (sscanf(buffer, "%d/%d", &num, &den) == 2 && den != 0)
        return (float)num / (float)den;
    return 0.0f;
}


void mp4ToTexture(const char *pathFile, const char *outDir, const char *name){
    char cmd_buffer[2048];
    snprintf(cmd_buffer, sizeof(cmd_buffer),
        "ffmpeg -y -i \"%s\" "
        "\"%s/%s_%%04d.jpg\" "
        "-vn -acodec pcm_s16le -ar 44100 -ac 2 "
        "\"%s/%s.wav\"",
        pathFile, outDir, name,
        outDir, name);
    printf("Running ffmpeg:\n%s\n", cmd_buffer);
    system(cmd_buffer);
}
static bool FileExistsSimple(const char *path){
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static int CountVideoFrames(const char *dir, const char *name){
    char path[512];
    int count = 0;
    for (int i = 1; ; i++)
    {
        snprintf(path, sizeof(path),
                 "%s/%s_%04d.jpg", dir, name, i);
        if (!FileExistsSimple(path))
            break;
        count++;
    }
    return count;
}

Video LoadVideo(const char *mp4Path, const char *outDir, const char *name, float fallbackFPS){
    Video v = {0};
    v.current = 0;
    v.timer = 0.0f;
    v.started = false;
    v.finished = false;
    float fps = fallbackFPS;
    if (fallbackFPS == 0) fps = GetVideoFPS(mp4Path);
    v.fps = fps; 
    // ---- fichiers existants ? ----
    int frameCount = CountVideoFrames(outDir, name);
    char wavPath[512];
    snprintf(wavPath, sizeof(wavPath),
             "%s/%s.wav", outDir, name);
    bool hasAudioFile = FileExistsSimple(wavPath);
    // ---- sinon on génère ----

    if (frameCount == 0 || !hasAudioFile){
        mp4ToTexture(mp4Path, outDir, name);
        frameCount = CountVideoFrames(outDir, name);
    }
    // ---- charger les textures ----
    v.frameCount = frameCount;
    v.frames = (Texture2D *)malloc(sizeof(Texture2D) * frameCount);
    char imgPath[512];
    SetTraceLogLevel(LOG_NONE);
    for (int i = 0; i < frameCount; i++)
    {
        snprintf(imgPath, sizeof(imgPath),
                 "%s/%s_%04d.jpg", outDir, name, i + 1);

        v.frames[i] = LoadTexture(imgPath);
    }
    SetTraceLogLevel(LOG_INFO);
    // ---- charger l'audio ----
    if (FileExistsSimple(wavPath)){
        v.audio = LoadMusicStream(wavPath);
        v.hasAudio = true;
    }
    else{
        v.hasAudio = false;
    }
    return v;
}

void PlayVideo(Video *video)
{
    // Déjà terminée → rien à faire
    if (video->finished)
        return;
    if (video->hasAudio && !video->started)
    {
        PlayMusicStream(video->audio);
        video->started = true;
    }
    if (video->hasAudio)
        UpdateMusicStream(video->audio);
    float t = video->hasAudio ? GetMusicTimePlayed(video->audio) : 0.0f;
    int frame = (int)(t * video->fps);
    if (frame >= video->frameCount)
    {
        if (video->hasAudio)
        {
            StopMusicStream(video->audio);
            UnloadMusicStream(video->audio);
            video->hasAudio = false;
        }
        video->finished = true;
        return; // plus rien n’est dessiné
    }
    if (frame < 0)
        frame = 0;
    DrawTexture(video->frames[frame], 0, 0, WHITE);
}

void UpdateCursorToggle(){
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (IsCursorHidden())
            EnableCursor();
        else
            DisableCursor();
    }
}

