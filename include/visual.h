#ifndef VISUAL_H
#define VISUAL_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rmviMath.h"
#include "text2Latex.h"
#include "fft_wrapper.h"
#include "cJSON.h"
#include "audio.h"


enum {
    ARROWDIRECT,
    ARROWSQUARE
};

enum {
    RIGHT,
    BOT,
    LEFT,
    TOP,
};

typedef struct{
    RenderBox *data;
    int count;
    int capacity;
} rmviRenderBoxList;

typedef struct{
    Token *data;
    int count;
    int capacity;
}rmviTokenList;

// origin: center of the plot in screen pixels
// halfSizePx: half-size of the visible plot in screen pixels
// gridStepUnits: value represented by one grid step on each axis
// gridStepPx: distance between two grid lines in screen pixels

typedef struct{
    Color *color;
    char **name;
    int count;
    int capacity;
    Vector2 position;
    float sizeText;
} rmviLegend;

typedef struct {
    Vector2 origin;
    Vector2 halfSizePx;
    Vector2 gridStepUnits;
    Vector2 gridStepPx;
    char xlabel[32];
    char ylabel[32];
    char title[512];
    float thickness;
    float sizeText;                     // ç suprimer 
    float spacing;
    float alphaGrid;                    // alpha value for grid lines
    rmviLegend *legendData;
} rmviCartesian;

typedef struct {
    Vector2 origin;
    float halfLength;                   // in pixels, radius of the polar plot
    float gridStepUnits;                // value represented by one grid step on each axis (radius and angle)
    float gridStepPx;                   // distance between two grid lines in screen pixels (radius and angle)  
    int nTheta;                         // number of angular grid lines
    char xlabel[32];
    char ylabel[32];
    char title[512];
    float thickness;
    float sizeText;
    float spacing;
    float alphaGrid;                    // alpha value for grid lines
    rmviLegend *legendData;
} rmviPolar;



typedef struct {
    // ici la question de la mémoire est à regarder
    Rectangle outerRect;
    Rectangle innerRect;
    float lineThick;
    Color outerColor;
    Color innerColor;
    rmviRenderBoxList renderList;
    State *state;
    float roundness;
    int num;    // utilisé pour chainer facilement
} rmviFrame;

typedef struct {
    int n;                // nombre total de frames
    int depth;            // profondeur de l’arbre
    rmviFrame **frames;     // tableau de frames
    int *numeros;         // numéro associé à chaque frame
    int *parents;         // parent[i] = index du parent du frame i
    int *nEnfants;        // nombre d'enfants pour chaque frame
    int **enfants;        // tableau de tableaux : indices des enfants
} rmviTree;

typedef struct rmviAtom {
    rmviFrame *frame;
    const char *nature;
    float lifespan;            // à transformer en list pour le type de désintégration ? ; égale à -1 si stable
    float lambda;
    struct rmviAtom *daughter; // isotope fils (NULL si stable)
    bool alive;
    Vector2d center;
    Vector2d speed;
} rmviAtom;

typedef struct rmviParticle {
    rmviFrame *frame;
    Vector2d center;
    Vector2d speed;
    float size;
    float lifespan;
} rmviParticle;

typedef struct rmviDynamic2D{
    Vector2d position;
    double mass;
    Vector2d velocity;
    Vector2d force;
} rmviDynamic2D;

typedef struct rmviDynamic3D{
    Vector3d position;
    double mass;
    Vector3d velocity;
    Vector3d force;
    float radius;
} rmviDynamic3D;

typedef struct rmviVisual{
    Texture2D texture;
    float width;
    float height;
} rmviVisual;

typedef struct rmviPlanet{
    rmviDynamic2D features ;
    rmviVisual visual;
} rmviPlanet;

typedef struct rmviModel{
    Model model;
}rmviModel;

typedef struct Light {
    Vector3 position;
    Vector3 ambient;
    Vector3 diffuse;
    Vector3 specular;
} Light;

typedef struct lightLoc {
    unsigned int position;
    unsigned int ambient;
    unsigned int diffuse;
    unsigned int specular;
} lightLoc;

typedef struct rmviLocUniform{
    unsigned int viewLoc;
    unsigned int projLoc;
    unsigned int modelLoc;
    unsigned int timeLoc;
    lightLoc lightLoc;
    unsigned int viewPosLoc;
}rmviLocUniform;

typedef struct rmviPlanet3D{
    rmviDynamic3D features;
    Shader shader;
    Model model;
    rmviLocUniform locUniform;
}rmviPlanet3D;


typedef struct rmviDash2{
    Vector2 position;
    Vector2 velocity;
} rmviDash2;

typedef struct rmviDash3{
    Vector3 position;
    Vector3 velocity;
} rmviDash3;

typedef struct {
    double *x;
    double *y;
    int n;  // nombre de points lus
} rmviPointArray;

typedef struct {
    Vector2 *points;   // tableau de points
    int npoints;       // nombre de points
} Contour2;

typedef struct {
    Contour2 *contours;  // tableau de contours
    int ncontours;       // nombre de contours
} Anime2;

typedef struct {
    Texture2D *frames;
    int frameCount;
    int current;
    float fps;
    float timer;
    Music audio;
    bool hasAudio;
    bool started;
    bool finished;
} Video;


typedef struct{
    int n;                  // nombre total de frames
    int depth;              // profondeur max approximative
    rmviFrame **frames;     // tableau de frames
    int *numeros;           // numéro associé à chaque frame
    rmviIntList *parents;   // parents[i] = liste des parents du frame i
    rmviIntList *enfants;   // enfants[i] = liste des enfants du frame i
} rmviGraph;

#define G 6.6743015e-11


typedef float (*MathFunction)(float);

void rmviDrawTextMid(const char *text, Rectangle rec, State *state);

void rmviRenderBoxListInit(rmviRenderBoxList *list);
rmviRenderBoxList rmviGetRenderBoxList(const char *latex, State *state);
void rmviRenderBoxListFree(rmviRenderBoxList *list);
bool rmviRenderBoxListReserve(rmviRenderBoxList *list, int newCapacity);

// ----------------------------- RECTANGLES -----------------------------
Rectangle rmviGetRectangleCenteredRatio(float x, float y, float ratio_x, float ratio_y);
void rmviRotateRectangle(Rectangle rec, Vector2 origin, float rotation, Color color);                                   // fait tourner un rectangle autour de son centre

// ----------------------------- TEXT AND FRAMES -----------------------------


void rmviRotateText(const char *text, Vector2 position, Vector2 origin, float rotation, State *state);
void rmviDrawTextRec(const char *text, Rectangle rec, float ratio, float ratioWidth, float ratioHeight,State *state);
void rmviMyDrawText(const char *text, Vector2 position, float size, Color color);

rmviFrame rmviGetFrame(float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor, const char *text, State *state, float roundness);
rmviFrame rmviGetFrameBG(float posX, float posY, float width, float height, float lineThick, const char *text, State *state);
rmviFrame rmviGetFrameBGCentered(float posX, float posY, float ratio_x, float ratio_y, float lineThick, const char *text, State *state);
rmviFrame rmviGetFrameBGCentered(float posX, float posY, float ratio_x, float ratio_y, float lineThick, const char *text, State *state);
rmviFrame rmviGetRoundFrameBGCentered(float posX, float posY, float ratio, float lineThick, const char *text, State *state);
rmviFrame rmviGetFrameFromText(float posX, float posY, const char *text, State *state);
void rmviDrawFrame(const rmviFrame *frame);
void rmviDrawFramePro(rmviFrame frame, float ratio, float rotation, Vector2 origin, Vector2 translationVector);
void rmviUpdateFrame(rmviFrame *frame, float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor,State *state);
void rmviZoomFrame(rmviFrame *frame, float rate);
void rmviRewriteFrame(rmviFrame *frame, const char *text); 

// ----------------------------- Tree -----------------------------
rmviTree *rmviCreateTree(void);
int getDepth(rmviTree *tree, int index);

// ----------------------------- rmviGraph ---------------------------
rmviGraph *rmviGetGraph(void);
int rmviGetDepth(const rmviGraph *graph, int index);
bool rmviAddEdge2Graph(rmviGraph *graph, int parentIndex, int childIndex);
void rmviAddFrame2Graph(rmviGraph *graph, rmviFrame *frame, const rmviIntList *parents);
void rmviAddFrame2GraphSingleParent(rmviGraph *graph, rmviFrame *frame, int parentIndex);
void rmviPositioningGraph(rmviGraph *graph, float spaceTreeRatioX, float spaceTreeRatioY);
void rmviPositioningGraphAverageParents(rmviGraph *graph, float spaceTreeRatioX, float spaceTreeRatioY);
void rmviZoomGraph(rmviGraph *graph, float zoomFactor);
void rmviDrawGraph(rmviGraph *graph, int arrows);
void rmviFreeGraph(rmviGraph *graph);
void rmviDrawGraphSquareWrite(rmviGraph *graph, float leftRatio, const char **listText, Color color, float ratioWriteArrow, float size) ;

// ----------------------------- CARTHESIAN AND ARROWS -----------------------------
void rmviDrawArrow2(Vector2 start, Vector2 end, float arrowSize, float ratio, Color color);
rmviCartesian rmviGetCartesian(Vector2 origin, Vector2 halfSizePx, Vector2 gridStepUnits);
void rmviSetCartesianAxesLabel(rmviCartesian *cartesian, const char *xlabel, const char *ylabel);
void rmviSetCartesianTitle(rmviCartesian *cartesian, const char *title);
void rmviSetCartesianGridStepPx(rmviCartesian *cartesian, Vector2 gridStepPx);
Vector2 rmviCartesianToScreen(const rmviCartesian *cartesian, Vector2 point);
rmviLegend rmviGetLegend(int capacity);
void rmviAddLegend(rmviLegend *legend, Color color, const char *name);
void rmviWriteTitle(const rmviCartesian *cartesian, float sizeText, float spacing, Font font, Color color);
void rmviWriteTitleClassic(const rmviCartesian *cartesian);
void rmviDrawLegend(rmviLegend *legend, Vector2 origin, Vector2 halfSizePx);
void rmviDrawCartesianFull(const rmviCartesian *cartesian, float arrowSize, float ratio, Color color, bool drawTicks, bool drawLines);
void rmviUpdateCartesian(rmviCartesian *cartesian, Vector2 origin, Vector2 gridStepUnits, Vector2 halfSizePx);
void rmviWriteAxis(const rmviCartesian *cartesian, float sizeText, float spacing, Font font, Color color);
void rmviDrawTick(const rmviCartesian *cartesian, float length, float thickness, Color color);
void rmviDrawGrids(Vector2 origin, Vector2 halfSizePx, Vector2 gridStepPx, Color color);
void rmviDrawFunction(const rmviCartesian *cartesian, MathFunction fct, Color color);
void rmviDraw2Parametric(const rmviCartesian *cartesian, float fx(float,float), float fy(float,float), float radius, float tMin, float tMax, int n, Color color);
void rmviDrawTrigo(const rmviCartesian *cartesian, float x, float y, Color color, float radius);
void rmviDrawList2(const rmviCartesian *cartesian, const float *listX, const float *listY, int count, bool line, Color color);
rmviPolar rmviGetPolar(Vector2 origin, float halfLength, float gridStepUnits);
void rmviSetPolarAxesLabel(rmviPolar *polar, const char *xlabel, const char *ylabel);
void rmviSetPolarTitle(rmviPolar *polar, const char *title);
void rmviSetPolarGridStepPx(rmviPolar *polar, float gridStepPx);
Vector2 rmviPolarToScreen(const rmviPolar *polar, Vector2 point);
void rmviWriteTitlePolar(const rmviPolar *polar, float sizeText, float spacing, Font font, Color color);
void rmviWriteTitleClassicPolar(const rmviPolar *polar);
void rmviWriteAxisPolar(const rmviPolar *polar, float sizeText, float spacing, Font font, Color color);
void rmviWriteAxisClassicPolar(const rmviPolar *polar);
void rmviDrawTickPolar(const rmviPolar *polar, float length, float thickness, Color color);
void rmviDrawGridsPolar(const rmviPolar *polar, Color color);
void rmviDrawPolarFull(const rmviPolar *polar, float arrowSize, float ratio, Color color, bool drawTicks, bool drawGrids);
void rmviUpdatePolar(rmviPolar *polar, Vector2 origin, float gridStepUnits, float halfLength);
void rmviDrawListPolar(const rmviPolar *polar, const float *listR, const float *listTheta, int count, bool line, Color color);
void rmviPositioningTree(rmviTree *tree,float spaceTreeRatioX, float spaceTreeRatioY);


float rmviRand();
float rmviRandNormalCentered();
float rmviRandNormal(float mean, float sigma);

Vector2 rmviRandomSpeed(Vector4 *count);
bool rmviVector2IsZero(Vector2 vec);
// ----------------------------- ATOMS AND TREE -----------------------------
rmviAtom rmviGetAtom(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter);
void rmviDrawTree(rmviTree *tree, int arrows);
void rmviDrawTreeSquareWrite(rmviTree *tree, float leftRatio, const char **listText, Color color, float ratioWriteArrow, float size);
void rmviAtomDecay(rmviAtom *atom);
bool rmviDesintegration(rmviAtom *atom, float deltaTime);
void rmviAtomUpdate(rmviAtom *atom);
rmviAtom rmviGetAtomSpeed(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter, Vector2d speed);
rmviFrame rmviGetElectron(float posX, float posY, State *state);

// ----------------------------- Texture -----------------------------
void rmviRotateTexture(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);

// ----------------------------- Color -----------------------------
Color GetAverageColor(Texture2D texture);

// ----------------------------- DYNAMICS 2D AND PLANETS -----------------------------
rmviPlanet rmviGetPlanet(Vector2d position, double mass, Vector2d velocity, Vector2d force, Texture2D texture, float height);
void rmviGravityRepulsion(rmviDynamic2D **features, int n);
void rmviAddDash2(rmviDash2 *dashPlanet,int count, int countFrame, int step, rmviDynamic2D *features, Vector2 center, Vector2 speed);
void rmviDrawDash2Fast(rmviDash2 *dashPlanet, Vector2 center, Color color, float scale, int n);

// ----------------------------- DYNAMICS 3D AND PLANETS -----------------------------
rmviPlanet3D rmviGetPlanet3D(Vector3d position, float mass, Vector3d velocity, Vector3d force, const char* modelPath);
void rmviGravityRepulsion3D(rmviDynamic3D **features, int n);
void rmviAddDash3(rmviDash3 *dashPlanet,int count, int countFrame, int step, rmviDynamic3D *features, Vector3 center, Vector3 speed);
void rmviDrawDash3Fast(const rmviDash3 *dashList, int count,float dashLen, float scale,Color color);
// ----------------------------- Fourrier -----------------------------
void rmviDrawFourier(FourierCoeff *coeffs, int n, Vector2 origin, float scale, Color color, float time, Vector2 *figure);  // dessine les cercles et le tracé
void rmviDrawFourierFigure(float countFrame, Vector2 *figure, int timeFourier, int FPS, Color color);
// fait tourner un rectangle autour de son centre
// ------------------------------ JSON ------------------------------
char* rmviReadFile(const char *filename);
Anime2* rmviAnime2FromJSON(const char *jsonStr);
void rmviAnime2Free(Anime2 *anime);
void rmvidrawAnime2(Anime2 *anime, Vector2 position, float scale, Color color, bool invertY );


void mp4ToTexture(const char *pathFile, const char *outDir, const char *name);
Video LoadVideo(const char *mp4Path, const char *outDir, const char *name, float fallbackFPS);
void PlayVideo(Video *video);

// fonction pour les système solaire

void UpdateCursorToggle();
#endif // VISUAL_H
