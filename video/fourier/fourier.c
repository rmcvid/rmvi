#include "config.h"
// #define FFTW_DLLqqch ne marche pas ici, je propose de faire soit meme les transformée de fourier
//#include <fftw3.h>
#include <stdio.h>
#include "fft_wrapper.h"
#define UA 150e9 // distance moyenne terre soleil en km
#define PLANET_SCALE 10.0f // facteur d'échelle pour les planètes
#define UA2PIXEL 200.0f // facteur d'échelle pour les distances
#define UA2PIXEL_FAR 130.0f
#define CENTER (Vector2) {(float)GetScreenWidth()/2.0f, (float)GetScreenHeight()/2.0f} // centre de l'écran
#define VECTOR20 (Vector2) {0.0f, 0.0f} // vecteur nul
#define FRAME2SEC 100000                 // conversion frame en seconde
#define MEMORY 1000                     // C'est dans des for ca rent le truc pas du tout efficace point qu'on conserve dans la mémoire pour desinner les orbites
#define TRANSITION_TIME 3.0f // en secondes
#define SPACE_DASH 2 // espace entre les dashs en frame
#define DISTANCE_CHANGE 480e9
#define NBPLANETS 7

#define NFOURIER 200 // nombre de coefficient de fourier

Vector2 centerView; // centre initial sur le soleil
Vector2 centerSpeed;
int space_count = 0;
Color colorListPlanet[NBPLANETS];

//  toutes les distances sont mise aux aphélies
rmviPlanet sun, mercury, venus, earth, mars, jupiter, saturn;
rmviPlanet *planets[NBPLANETS] = {&sun, &mercury, &venus, &earth, &mars, &jupiter, &saturn};
rmviDash dashSun[MEMORY], dashMercury[MEMORY], dashVenus[MEMORY], dashEarth[MEMORY], dashMars[MEMORY], dashJupiter[MEMORY], dashSaturn[MEMORY];
rmviDash *dashList[NBPLANETS] = {dashSun, dashMercury, dashVenus, dashEarth, dashMars, dashJupiter, dashSaturn};
bool reset_dash = false;
float frameInitCenterView = 0.0f, countFrame = 0.0f;                  // Début du centrage sur la terre

Image image;    // pour ffmpeg

const char *version;
int bm_visual_initialisation(void) {
    // Initialize raylib
    char audioPath[256];
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int screenWidth = WIDTH;
    int screenHeight = HEIGHT;
    InitWindow(screenWidth, screenHeight, "test example");
    SetTargetFPS(FPS); 
    screen = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    rmviGetCustomFont(FONT_PATH, 80);
    if (RECORDING) ffmpeg = ffmpeg_start_rendering(GetScreenWidth(), GetScreenHeight(), FPS);
    if (AUDIO_RECORDING) {
        InitAudioDevice();                                      // Initialise audio
        snprintf(audioPath, sizeof(audioPath), "%s/%s.wav", OUTPUT_DIR, version);
        printf("Audio will be recorded to: %s\n", audioPath);
        rec = InitAudioRecorder(audioPath);       // faire un chemin avec le cmake
    }
    return 0;  // Success
}
int bm_visual_uninitialisation(void) {
    // De-Initialization
    CloseWindow();  // Close window and OpenGL context
    UnloadFont(mathFont);
    UnloadRenderTexture(screen);
    if (RECORDING) ffmpeg_end_rendering(ffmpeg);
    return 0;  // Success
}

void rmviDraw(){
    BeginDrawing();
        ClearBackground(BG);
        DrawTexturePro(screen.texture, (Rectangle){ 0, 0, (float)screen.texture.width, -(float)screen.texture.height }, (Rectangle){ 0, 0, (float)screen.texture.width, (float)screen.texture.height}, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndDrawing();
}

void addImageToffmpeg(void *ffmpeg) {
    image =  LoadImageFromTexture(screen.texture);
    ffmpeg_send_frame(ffmpeg, image.data, image.width, image.height);
    UnloadImage(image);
}

void rmviDrawPlanet(rmviPlanet planet, Vector2 center, float scale) {
    // On centre la texture par rapport à la position
    Rectangle src = { 0, 0, (float)planet.visual.texture.width, (float)planet.visual.texture.height };
    Rectangle dest;
    dest = (Rectangle){
    CENTER.x + (- center.x + planet.features.position.x) *scale,  // X centre
    CENTER.y + (- center.y + planet.features.position.y) *scale,  // Y centre
    planet.visual.width,         // largeur finale
    planet.visual.height         // hauteur finale
    };
    Vector2 origin = { planet.visual.width/2.0f, planet.visual.height/2.0f };
    DrawTexturePro(planet.visual.texture, src, dest, origin, 0.0f, WHITE);
}

// il faut free
rmviDynamic2D **rmviPlanet2Dynamic(rmviPlanet **planets, int n) {
    rmviDynamic2D **featurePtrs = malloc(n * sizeof(*featurePtrs)); // remplacer par malloc
    if (!featurePtrs) return NULL;
    for (int i = 0; i < n; i++) {featurePtrs[i] = &planets[i]->features; }
    return featurePtrs;
}

void rmviUpdateGravity(rmviPlanet **planets, int n) {
    rmviDynamic2D **featurePtrs = rmviPlanet2Dynamic(planets, n);
    if (!featurePtrs) return;
    rmviGravityRepulsion(featurePtrs, n);
    free(featurePtrs);
}
Vector2 rmviVectorCrossAngle(float norm, float angle){
    return Vector2Scale((Vector2){cos((angle)/360.0f *2* PI),sin(angle/360.0f * 2* PI)},norm);
}

void initialisePlanets(){
    sun = rmviGetPlanet(VECTOR20, 1.989e30, VECTOR20, VECTOR20, LoadTexture("video/fourier/Image/sun.jpg"), PLANET_SCALE*log10(696342));
    mercury = rmviGetPlanet(rmviVectorCrossAngle(69.816e9,-25.49074), 3.285e23,rmviVectorCrossAngle(38.86e3, 90 - 25.49074), VECTOR20, LoadTexture("video/fourier/Image/mercury.jpg"), PLANET_SCALE*log10(2439));
    venus = rmviGetPlanet(rmviVectorCrossAngle(108.943e9,28.58579), 4.867e24, rmviVectorCrossAngle(34.789e3,90 +28.58579), VECTOR20, LoadTexture("video/fourier/Image/venus.jpg"), PLANET_SCALE*log10(60518));
    earth = rmviGetPlanet(rmviVectorCrossAngle(152e9,0.0), 5.972e24,rmviVectorCrossAngle(29.291e3,90), VECTOR20, LoadTexture("video/fourier/Image/earth.jpg"), PLANET_SCALE*log10(63567));
    mars = rmviGetPlanet(rmviVectorCrossAngle(249.230e9,-126.90635), 6.39e23, rmviVectorCrossAngle(21.975e3,90 + -126.90635), VECTOR20, LoadTexture("video/fourier/Image/mars.jpg"), PLANET_SCALE*log10(33762));
    jupiter = rmviGetPlanet(rmviVectorCrossAngle(778e9,-88.19334), 1.898e27, rmviVectorCrossAngle(12.448e3,90 +-88.19334), VECTOR20, LoadTexture("video/fourier/Image/jupyter.jpg"), PLANET_SCALE*log10(69911));
    saturn = rmviGetPlanet(rmviVectorCrossAngle(1503e9,-10.51525), 5.683e26, rmviVectorCrossAngle(9.141e3,90 -10.51525), VECTOR20, LoadTexture("video/fourier/Image/saturn.jpg"), PLANET_SCALE*log10(58232));
    saturn.visual.height = saturn.visual.width/180*95;
}

// ajoute la vitesse du centre de la vue pour avoir l'effet de suivre la terre ou du moins du centre
void rmviUpdatePosition(rmviPlanet *planet, float dt, Vector2 centerSpeed) {
    // Met à jour la position et la vitesse en fonction de la force
    Vector2 acceleration = Vector2Scale(planet->features.force, 1.0f / ((float)planet->features.mass));
    planet->features.velocity = Vector2Add(Vector2Add(planet->features.velocity, Vector2Scale(acceleration, dt)), Vector2Scale(centerSpeed, -1.0f));
    planet->features.position = Vector2Add(planet->features.position, Vector2Scale(planet->features.velocity, dt));
}
void UpdateCenterView( Vector2 *centerView, Vector2 newCenter, Vector2 oldCenter, float timeFrame){
    *centerView = Vector2Add(
    Vector2Scale(newCenter,Clamp((countFrame - frameInitCenterView)/timeFrame,0,1)),
    Vector2Scale(oldCenter,Clamp((1 -(countFrame - frameInitCenterView)/timeFrame),0,1))
    );
}
void rmviAddDash(rmviDash *dashPlanet, rmviDynamic2D *features, Vector2 center, Vector2 speed){
    int i;
    if ((int) countFrame%SPACE_DASH == 0)
    {   
        i = (int)(countFrame/SPACE_DASH)%MEMORY;
        dashPlanet[i] = (rmviDash){Vector2Add(features->position, Vector2Scale(center,-1)), Vector2Add(features->velocity, Vector2Scale(speed,-1))};
    }
}
/*void rmviAdddashList(rmviDash **dashList, rmviPlanet **planets, int n){
    for(int i=0; i<n; i++){rmviAddDash(dashList[i], &planets[i]->features, planets[i]->features.velocity);}
}*/

void rmviDrawDashFast(rmviDash *dashPlanet, Vector2 center, Color color, float scale, int n) {
    rlBegin(RL_LINES);
    rlColor4ub(color.r, color.g, color.b, color.a);
    for (int i = 0; i < n; i++) {
        Vector2 start, end;
        start = Vector2Add(Vector2Scale(center,  scale), Vector2Scale(dashPlanet[i].position, scale));
        end = Vector2Add(start, Vector2Scale(Vector2Normalize(dashPlanet[i].velocity), 10.0f));
        rlVertex2f(CENTER.x + start.x, CENTER.y + start.y);
        rlVertex2f(CENTER.x + end.x, CENTER.y + end.y);
    }

    rlEnd();
}
/*void rmviDrawdashList(rmviDash **dashList, Vector2 center, int n){
    for(int i=0; i<n; i++){rmviDrawDash(dashList[i], center);}
}*/
void slideOne(void){
    if(reset_dash && (int)countFrame%SPACE_DASH == 0){
        for(int i=0; i < sizeof(planets) / sizeof(planets[0]); i++){
            for(int j=0; j<MEMORY; j++){
                dashList[i][j] = (rmviDash){VECTOR20, VECTOR20};
            }
        }
        reset_dash = false;
    }
    if(space_count == 1){
        if(!frameInitCenterView) frameInitCenterView = countFrame;
        UpdateCenterView(&centerView, earth.features.position, sun.features.position, TRANSITION_TIME*FPS); // 10 secondes de transition
        UpdateCenterView(&centerSpeed, earth.features.velocity, sun.features.velocity, TRANSITION_TIME*FPS);
    }
    else{
        centerView = sun.features.position, centerSpeed = sun.features.velocity;
    }
    rmviUpdateGravity(planets, sizeof(planets) / sizeof(planets[0]));
    for (int i = 0; i < sizeof(planets) / sizeof(planets[0]) ; i++) {
        rmviUpdatePosition(planets[i], FRAME2SEC, centerSpeed);
        rmviAddDash(dashList[i], &planets[i]->features, centerView, centerSpeed);
        //rmviDrawDash(dashList[i], centerView, colorListPlanet[i]);
        rmviDrawDashFast(dashList[i], VECTOR20, colorListPlanet[i], i < 5 ? (UA2PIXEL / UA) : (UA2PIXEL_FAR / UA), MEMORY);
        rmviDrawPlanet(*planets[i], centerView, i < 5 ? (UA2PIXEL / UA) : (UA2PIXEL_FAR / UA));
    }
}

rmviPointArray read_csv_points(const char *filename) {
    rmviPointArray points = {NULL, NULL, 0};
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Erreur d'ouverture du fichier");
        return points;
    }
    // Première passe : compter le nombre de lignes
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '#') continue;  // ignorer lignes vides ou commentaires
        count++;
    }
    rewind(fp); // revenir au début du fichier
    points.x = malloc(count * sizeof(double));
    points.y = malloc(count * sizeof(double));
    if (!points.x || !points.y) {
        fprintf(stderr, "Erreur d'allocation mémoire.\n");
        fclose(fp);
        return points;
    }

    // Deuxième passe : lecture des valeurs
    int i = 0;
    while (fgets(line, sizeof(line), fp) && i < count) {
        if (line[0] == '\n' || line[0] == '#') continue;
        if (sscanf(line, "%lf,%lf", &points.x[i], &points.y[i]) == 2) {
            i++;
        }
    }

    points.n = i;
    fclose(fp);
    return points;
}
FourierCoeff *testFFT(rmviPointArray points){
    FourierCoeff *coeffs = malloc(NFOURIER * sizeof(FourierCoeff));
    computeFFT(points.x, points.y, coeffs, NFOURIER, points.n);
    if (!coeffs) {
        fprintf(stderr, "Erreur allocation coeffs\n");
        return NULL;
    }
    return coeffs;
}

int bm_visual_main(void)
{   
    initialisePlanets();
    // initialisation des de la couleur des dashs
    colorListPlanet[sizeof(planets) / sizeof(planets[0])];
    for (int i = 0; i < sizeof(planets) / sizeof(planets[0]); i++){ colorListPlanet[i] = GetAverageColor(planets[i]->visual.texture);}
    centerView = sun.features.position; // centre initial sur le soleil
    centerSpeed = VECTOR20;
    int timeFourier = 10;
    Vector2 *figure = malloc(FPS * timeFourier * sizeof(Vector2));
    //rmviPointArray points = read_csv_points("C:\\Users\\ryanm\\Documents\\Rmvi\\animation\\video\\fourier\\Image\\points_fourier.csv");
    rmviPointArray points = read_csv_points("C:\\Users\\ryanm\\Documents\\Rmvi\\animation\\ryan_jupyter.csv");
    FourierCoeff *coeff = testFFT(points);
    const char *t1 = "Bonjour Quentin! //Ce chère monsieur";
    const char *t2 = "Te souhaite un joyeux anniversaire!";
    float w1 = rmviCalcTextWidth(t1, mathFont,60,60/15);
    float w2 = rmviCalcTextWidth(t2, mathFont,60,60/15);

    while (!WindowShouldClose())
    {
        BeginTextureMode(screen);
            ClearBackground(BG);
            if(IsKeyPressed(KEY_M)) rec.isRecording ? StopAudioRecorder(&rec) : StartAudioRecorder(&rec);
            if(IsKeyPressed(KEY_SPACE)){
                space_count ++;
                countFrame = 0;
            }
            if(IsKeyPressed(KEY_R)) reset_dash = true;
            //slideOne();
            if(space_count == 0)  rmviWriteAnimText(t1 ,(Vector2) {CENTER.x - w1/2, CENTER.y} ,60, WHITE,0,countFrame);
            else if (space_count == 1){
                if (countFrame < FPS*timeFourier) rmviDrawFourier(coeff, NFOURIER, CENTER, 10.00f, WHITE, 2.0f * PI * (float)countFrame/((float)FPS * timeFourier * (coeff[1].freq)), countFrame < FPS*timeFourier ? &figure[(int) countFrame] : NULL);
                rmviDrawFourierFigure(countFrame, figure, timeFourier, FPS, WHITE);
            }
            else if (space_count == 2) rmviWriteAnimText(t2 ,(Vector2) {CENTER.x - w2/2, CENTER.y},60, WHITE,0, countFrame);
            //DrawText(rec.isRecording ? "Recording..." : "Idle", 20, 20, 20, WHITE);
            DrawFPS(10, 10);
        EndTextureMode();
        rmviDraw();
        if (RECORDING) addImageToffmpeg(ffmpeg);
        countFrame++;
    }
    free(figure);
    return 0;
}


int main(int argc, char *argv[]){
    version = (argc > 1) ? argv[1] : "0"; 
    bm_visual_initialisation();
    bm_visual_main();
    bm_visual_uninitialisation();
    return 0;
}
