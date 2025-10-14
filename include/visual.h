#ifndef VISUAL_H
#define VISUAL_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "text2Latex.h"
#include "fft_wrapper.h"

typedef struct {
    Rectangle outerRect;
    Rectangle innerRect;
    float lineThick;
    Color outerColor;
    Color innerColor;
    const char *text;
    Font font;
    float roundness;
} rmviFrame;

// the origin defines the center of the plot; the scale defines the unit scale and the size defines arrows length
typedef struct {
    Vector2 origin;
    Vector2 unit;
    Vector2 size;
} rmviCarthesian;

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
    Vector2 center;
    Vector2 speed;
} rmviAtom;

typedef struct rmviParticle {
    rmviFrame *frame;
    Vector2 center;
    Vector2 speed;
    float size;
    float lifespan;
} rmviParticle;

typedef struct rmviDynamic2D{
    Vector2 position;
    double mass;
    Vector2 velocity;
    Vector2 force;
} rmviDynamic2D;

typedef struct rmviVisual{
    Texture2D texture;
    float width;
    float height;
} rmviVisual;

typedef struct rmviPlanet{
    rmviDynamic2D features ;
    rmviVisual visual;
} rmviPlanet;

typedef struct rmviDash{
    Vector2 position;
    Vector2 velocity;
} rmviDash;


typedef float (*MathFunction)(float);


void rmviDrawTextMid(const char *text, Rectangle rec, Color color, float ratio, Font font);

// ----------------------------- RECTANGLES -----------------------------
Rectangle rmviGetRectangleCenteredRatio(float x, float y, float ratio_x, float ratio_y);
void rmviRotateRectangle(Rectangle rec, Vector2 origin, float rotation, Color color);                                   // fait tourner un rectangle autour de son centre

// ----------------------------- TEXT AND FRAMES -----------------------------


void rmviRotateText(const char *text, Vector2 position, Vector2 origin, float rotation, float fontSize, Color color, Font font);
void rmviDrawTextRec(const char *text, Rectangle rec, Color color, float ratio, float ratioWidth, float ratioHeight, Font font);
void rmviDrawTextPro(const char *text, Rectangle rec, Color color, float ratio, float ratioWidth, float ratioHeight, Font font);
void rmviMyDrawText(const char *text, Vector2 position, float size, Color color);

rmviFrame rmviGetFrame(float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor, const char *text, Font font, float roundness);
rmviFrame rmviGetFrameBG(float posX, float posY, float width, float height, float lineThick, const char *text, Font font);
rmviFrame rmviGetFrameBGCentered(float posX, float posY, float ratio_x, float ratio_y, float lineThick, const char *text, Font font);
rmviFrame rmviGetFrameBGCentered(float posX, float posY, float ratio_x, float ratio_y, float lineThick, const char *text, Font font);
rmviFrame rmviGetRoundFrameBGCentered(float posX, float posY, float ratio, float lineThick, const char *text, Font font);
void rmviDrawFrame(rmviFrame frame, float ratio);
void rmviDrawFramePro(rmviFrame frame, float ratio, float rotation, Vector2 origin, Vector2 translationVector);
void rmviUpdateFrame(rmviFrame *frame, float posX, float posY, float width, float height, float lineThick, Color innerColor, Color outerColor, const char *text, Font font);
void rmviZoomFrame(rmviFrame *frame, float rate);
void rmviRewriteFrame(rmviFrame *frame, const char *text); 

// ----------------------------- Tree -----------------------------
rmviTree *rmviCreateTree(void);
int getDepth(rmviTree *tree, int index);

// ----------------------------- CARTHESIAN AND ARROWS -----------------------------
void rmviDrawArrow2(Vector2 start, Vector2 end, float arrowSize, float ratio, Color color);
rmviCarthesian rmviGetCarthesian(Vector2 origin, Vector2 size, Vector2 unit);
void rmviDrawCarthesianFull(rmviCarthesian carthesian, float arrowSize, float ratio, Color color, bool drawTicks, bool drawLines);
void rmviUpdateCarthesian(rmviCarthesian *carthesian, Vector2 origin, Vector2 unit, Vector2 size);
void rmviDrawTick(rmviCarthesian carthesian, float length, Color color);
void rmviDrawGrids(rmviCarthesian carthesian, Color color);
void rmviDrawFunction(rmviCarthesian carthesian, MathFunction fct, Color color);
void rmviDraw2Parametric(rmviCarthesian carthesian,float fx(float,float),float fy(float,float),float radius, float tMin, float tMax, int n, Color color);
void rmviDrawTrigo(rmviCarthesian carthesian, float x, float y, Color color, float radius);
void rmviPositioningTree(rmviTree *tree,float spaceTreeRatioX, float spaceTreeRatioY);


float rmviRand();
Vector2 rmviRandomSpeed(Vector4 *count);
bool rmviVector2IsZero(Vector2 vec);
// ----------------------------- ATOMS AND TREE -----------------------------
rmviAtom rmviGetAtom(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter);
void rmviDrawTree(rmviTree *tree, int arrows);
void rmviDrawTreeSquareWrite(rmviTree *tree, float leftRatio, const char **listText, Color color, float ratioWriteArrow, float size);
void rmviAtomDecay(rmviAtom *atom);
bool rmviDesintegration(rmviAtom *atom, float deltaTime);
void rmviAtomUpdate(rmviAtom *atom);
rmviAtom rmviGetAtomSpeed(rmviFrame *frame, const char *nature, float lifespan, rmviAtom *daughter, Vector2 speed);
rmviFrame rmviGetElectron(float posX, float posY);

// ----------------------------- Color -----------------------------
Color GetAverageColor(Texture2D texture);

// ----------------------------- DYNAMICS 2D AND PLANETS -----------------------------
rmviPlanet rmviGetPlanet(Vector2 position, float mass, Vector2 velocity, Vector2 force, Texture2D texture, float height);
void rmviGravityRepulsion(rmviDynamic2D **features, int n);


// ----------------------------- Fourrier -----------------------------
void rmviDrawFourier(FourierCoeff *coeffs, int n, Vector2 origin, float scale, Color color, float time, Vector2 *figure);  // dessine les cercles et le tracé

// fait tourner un rectangle autour de son centre
#endif // VISUAL_H