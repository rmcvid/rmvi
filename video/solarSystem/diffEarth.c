#include "config.h"
#include <stdio.h>
#include "fft_wrapper.h"
#define UA 150e9 // distance moyenne terre soleil en km
#define UA2PIXEL 1000.0f // facteur d'échelle pour les distances
#define UA2PIXEL_FAR 180.0f
#define TIME_DAY 24*60*60
#define FRAME2SEC 5                // conversion frame en seconde
#define EARTH_APPARENT_SIZE 30.0f
#define SIZE_TEXT 40.0f
#define SIZE_SPACING SIZE_TEXT/20.0f
#define TIME_TO_TRANSITION 10.0f
#define POLARCASE true
// /loadvideo{C:\Users\ryanm\Documents\Rmvi\animation\video\solarSystem\Image\decompte}{decompte}


typedef struct{
    int    position;
    char   name[32];
    double mass;
    double radius_e;
    double phi;
    double theta;
    double perihelion;
    double aphelion;
    double velocity_max;
    double velocity_min;
    double rotation;
    double inclinaison;
    double a;
    double e;
    bool   zUp;
    char   model_path[256];
    char   shader_path[256];
    char   description[256];
} PlanetData;

typedef struct Player{
    Camera3D camera;
    Vector3 velocity;
    float speed;
} Player;

typedef struct{
    float   frame2Sec;              // conversion frame en seconde
    bool    rotation;               // si les planètes tournent sur elles même
    int     reference;              // la planete sur laquelle est placée l'échelle
    float   ua2Pixel;               // conversion unité astronomique en pixel
    bool    asteroid;               // si on affiche lees anneaux de saturne
    bool    helpActive;             // si on affiche l'aide ou pas
    int     countRight;             // nombre de fois ou on a appuyé sur la touche pour faire avancer la présentation
    bool    draw;                   // si on affiche les planètes ou pas    
    bool    ecrit;                  // si on affiche le texte ou pas
    int     calculByFrame;          // on fait pow(2, calculByFrame) par frame pour faire avancer la simulation plus vite
    bool    isTransition;           // si on est en train de faire un changement de référentiel ou pas
    bool    resetDash;              // reset de dash
} Representation;

static Quaternion camRot = { 0, 0, 0, 1 }; 
//  toutes les distances sont mise aux aphélies
float frameInitCenterView = 0.0f, countFrame = 0.0f;                  // Début du centrage sur la terre
PlanetData *planetsData;

Image image;    // pour ffmpeg

const char *version;
void processInput();
void updateMouse();
void updateZoom(Vector2 wheel);
void fillLocUniform(Shader shader,rmviPlanet3D *planet);
void rmviDraw();
void addImageToffmpeg(void *ffmpeg);
rmviPlanet3D getPlanet3D(int num);
char *LoadTextFile(const char *path);
PlanetData *LoadPlanetsFromJSON(const char *path, int *outCount);
void drawPlanet3D(rmviPlanet3D planet,Matrix modelMat,Matrix viewMat,Matrix projMat);
float getModelNormalizationScale(Model model);
void rmviUpdatePosition3D(rmviDynamic3D *features, float dt, Vector3d centerSpeed, Vector3d centerPos);
void CartesianToSpherical(Vector3 p, float *r, float *theta, float *phi);
void help();
float apparentRadiusRatio(int i); 
Vector3 GetModelCenter(Model model);
void SetShaderLight(Shader shader, lightLoc matLoc, Light light);
lightLoc rlGetLocationLight(unsigned int shader_id, const char *name);
static bool useOrtho = false;
#define MEMORY 500

Player player;
float lastX, lastY;                // zoom
float shininess = 64;
bool firstMouse = true;


int space_count = 0;
float deltatime = 0;
Representation actualRep;
State helpState;
Representation repEarth = {
    .frame2Sec = FRAME2SEC,
    .reference = 3,
    .rotation = true,
    .ua2Pixel = 800,
    .asteroid = false,
    .countRight = 0,
    .draw = false,
    .ecrit = false,
    .calculByFrame = 16
};

static bool strongAmbient = false;
float ambientSpace = 0.02f, ambientVisible = 0.4f;
Light light ={
    {0.0f, 0.0f, 0.0f},
    {.02f, .02f, .02f},
    {0.6f, 0.6f, 0.6f},
    {.40f, .40f, .40f}
};

bool countYear(rmviDynamic3D *features, bool *changeAnnee, int *annee, float *r, float *theta){
    *r = Vector3dDistance(features->position, (Vector3d){0});
    bool res = false;
    if (*r == 0) return false;
    if (features->position.x >= 0 && features->position.z > 0) { // Q1
        *theta = acos(features->position.x / *r);
        *changeAnnee = true;
    }
    else if (features->position.x <= 0 && features->position.z >= 0) { // Q2
        *theta = acos(features->position.x / *r);
    }
    else if (features->position.x < 0 && features->position.z < 0) { // Q3
        *theta = acos(features->position.x / *r);
    }
    else { // Q4
        if (*changeAnnee) {
            (*annee)++;
            *changeAnnee = false;
            res = true;
        }
        *theta = acos(features->position.x / *r);
    }
    return res;
}

float calculH2(float mass, float a, float e){
    // return de H^2 which is the constt of the movement
    // mass est la masse du soleil, ou du corps auquel le petit tourne autour
    return 6.67430e-11*mass*a*(1-e*e);
}

float calculr(float H2, float mass, float e, float theta){
    // calcul r en fonction de theta dans un settting à deux corps
    float g = 6.67430e-11;
    return (H2/(g*mass))/(1+e*cos(theta-PI));
}

double gravityOptimised(rmviDynamic3D **features){
    double acc = G * (features[0]->mass)/pow(features[1]->position.x,2);
    return acc;
}

void positionOptimised(rmviDynamic3D **features, float dt, double acc){
    double rpp =  (features[1]->position.x*pow(features[1]->velocity.y,2) - acc);
    double epsilon = (-2.0f * features[1]->velocity.x * features[1]->velocity.y / features[1]->position.x);
    features[1]->position.x += features[1]->velocity.x*dt + rpp *dt*dt/2.0;
    features[1]->position.y += features[1]->velocity.y*dt + epsilon*dt*dt/2.0;
    features[1]->velocity.x += rpp * dt;
    features[1]->velocity.y += epsilon* dt;
    
    
}

int bm_visual_main(void){   
    Vector2 paragraphPos = CENTER;
    float paragraphHeight;
    Token tokens[256];
    RenderBox boxes[256];
    AnimText *animText = initAnimText();
    int boxCount;
    float curentFrame,oldFrame = 0;
    int countFrame = 0;
    int planetCount = 0;
    planetsData = LoadPlanetsFromJSON(HERE_PATH "sunEarth.json", &planetCount);
    planetCount = 2;
    
    //ici
    int factDeltat[4] = {1,4,16,64};
    Color colors[4] = {BLUE,RED,GREEN,YELLOW};
    rmviPlanet3D planets[4][2];
    rmviDynamic3D *features[4][2];
    for(int i=0; i <4; i++){
        for(int j=0; j<planetCount; j++){
            planets[i][j] = getPlanet3D(j);
            // passé en polaire
            if(POLARCASE){
                Vector3d pos = planets[i][j].features.position;
                Vector3d vel = planets[i][j].features.velocity;
                double r = Vector3dLength(pos);
                float vr = (pos.x*vel.x + pos.z*vel.z) / r;
                float w  = (pos.x*vel.z - pos.z*vel.x) / (r*r);
                planets[i][j].features.position = (Vector3d){r, 0.0f, 0.0f};
                planets[i][j].features.velocity = (Vector3d){vr, w, 0.0f};
            }
            features[i][j] = &planets[i][j].features;
        }
    }

    Vector3d initPos = planets[0][1].features.position;
    float H2Mercure = calculH2(planets[0]->features.mass,planetsData[1].a,planetsData[1].e);
    int t2 = 2;
    float sizeOrtho = 10.0f;
    actualRep = repEarth;
    float *listWidth;
    int lineCount;
    float heighTotal;
    float transitionFactor = 0.0f;
    float distList[4][MEMORY]= {0};
    float yearInD[4][MEMORY] = {0};
    float years[MEMORY];
    for(int j = 0; j < MEMORY; j++){
        years[j] = j + 1;
    }
    float dist = 0, lastJour = 0;
    int annee[4] = {0,0,0,0};
    bool changeAnnee[4] = {false,false,false,false};
    Vector3d  centerSpeed = (Vector3d) {0};
    rmviCarthesian carthesian = rmviGetCarthesian((Vector2) {CENTER.x, CENTER.y},(Vector2){CENTER.x/2, CENTER.y/2},(Vector2){20, 0.01});
    rmviSetCarthesianTitle(&carthesian,"Ecart relatif de l'aphélie mesuré");
    rmviSetCarthesianAxesLabel(&carthesian,"Tours", "/frac{r-r_e}{r_e}");
    rmviLegend legend = rmviGetLegend(4);
    for(int i=0; i<4;i++){
        char name[64];
        snprintf(name, sizeof(name), "/Delta t =/ %d s", factDeltat[i]*(int)actualRep.frame2Sec);
        rmviAddLegend(&legend, colors[i], name);
    }
    float r[4];
    float theta[4];
    float exactr = calculr(H2Mercure,features[0][0]->mass,planetsData[1].e,0.0);
    DisableCursor();
    float startTime = GetTime();
    float acc;
    helpState = rmviGetStateClassic();
    helpState.widthMax = 0.4*GetScreenWidth();
    float res = 0;
    while (!WindowShouldClose()){
        UpdateCursorToggle();
        BeginTextureMode(screen);
            ClearBackground(BG);
            rlEnableDepthTest();
            // Player movement and camera
            processInput();
            // ici optimisation à mettre en place en faisant les calculs en polaire
            // deuxième maj à faire avant de sauvegarder les données, placé les données calculer en CI.
            for(int i = 0; i< pow(2,actualRep.calculByFrame); i++){
                // c'est un peu compliqué mais en gros on vérifie si le pas est commum à plusieurs si oui alors on calcule
                for(int j = 3; j>=0; j--){
                    if(i%factDeltat[j] == 0){
                        for(int k = j; k>=0; k--){
                            if(POLARCASE){
                                // ajouter un graphique en fonction de la variation de la vitesse radiale
                                acc = gravityOptimised(features[k]); 
                                positionOptimised(features[k],factDeltat[k]*actualRep.frame2Sec,acc);
                                if((int) floor(-features[k][1]->position.y / (2*PI)) > annee[k]){
                                    annee[k] += 1 ;
                                    //-somme de  yearInD[k][annee[k]-1]
                                    if(annee[k]<MEMORY){
                                        for(int l =0; l < annee[k]; l++){
                                            res += yearInD[k][l];
                                        } 
                                        yearInD[k][annee[k]] = actualRep.frame2Sec*(i+countFrame*pow(2,actualRep.calculByFrame)) /(24*3600) - res;
                                        distList[k][annee[k]-1] = (features[k][1]->position.x - exactr)/exactr;
                                        res = 0;
                                    }
                                }
                            }
                            else{
                                rmviGravityRepulsion3D(features[k],planetCount);
                                rmviUpdatePosition3D(features[k][1],factDeltat[k]*actualRep.frame2Sec,centerSpeed, (Vector3d) {0});
                                if(countYear(features[k][1],&changeAnnee[k], &annee[k], &r[k], &theta[k])){
                                    if(annee[k]<MEMORY){
                                        yearInD[k][annee[k]-1] = annee[k];
                                        //lastJour = actualRep.frame2Sec*(i+countFrame*pow(2,actualRep.calculByFrame)) /(24*3600);
                                        distList[k][annee[k]-1] = (r[k] - exactr)/exactr;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            } 
            rlEnableDepthMask();
        EndTextureMode();
        // ecrans 2d
        BeginTextureMode(screen);
            DrawFPS(10, 10);
            if (actualRep.helpActive) help();
            rmviDrawCarthesianFull(carthesian,20,5,WHITE,true,false);
            rmviDrawLegend(carthesian,&legend);
            for(int i=0; i<4; i++){
                rmviDrawList2(carthesian , years, distList[i], min(annee[i],MEMORY), true, colors[i]);
            }
        EndTextureMode();
        rmviDraw();
        if (RECORDING) addImageToffmpeg(ffmpeg);
        countFrame++;
    }
    return 0;
}

int bm_visual_initialisation(void) {
    // Initialize raylib and audio
    char audioPath[256];
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int screenWidth = WIDTH;
    int screenHeight = HEIGHT;
    InitWindow(screenWidth, screenHeight, "test example");
    SetTargetFPS(FPS); 
    rlEnableDepthTest();
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
int main(int argc, char *argv[]){
    version = (argc > 1) ? argv[1] : "0"; 
    bm_visual_initialisation();
    printf("Audio ready: %d\n", IsAudioDeviceReady());
    bm_visual_main();
    bm_visual_uninitialisation();
    return 0;
}


void fillLocUniform(Shader shader,rmviPlanet3D *planet){
    planet->locUniform.viewLoc = rlGetLocationUniform(shader.id, "view");
    planet->locUniform.projLoc = rlGetLocationUniform(shader.id, "projection");
    planet->locUniform.modelLoc = rlGetLocationUniform(shader.id, "model");
    planet->locUniform.timeLoc = GetShaderLocation(shader,"time");
    planet->locUniform.lightLoc = rlGetLocationLight(shader.id, "light");
    planet->locUniform.viewPosLoc = GetShaderLocation(shader,"viewPos");
}
float getModelNormalizationScale(Model model)
{
    BoundingBox box = GetModelBoundingBox(model);
    Vector3 size = Vector3Subtract(box.max, box.min);
    float maxExtent = fmaxf(size.x, fmaxf(size.y, size.z));
    // rayon = 1.0
    return 2.0f / maxExtent;
}
Vector3 GetModelCenter(Model model)
{
    BoundingBox box = GetModelBoundingBox(model);
    return (Vector3){
        (box.min.x + box.max.x) * 0.5f,
        (box.min.y + box.max.y) * 0.5f,
        (box.min.z + box.max.z) * 0.5f
    };
}

rmviPlanet3D getModelPlanet3D(int num){
    char shaderPath[256];
    char modelPath[256];
    snprintf(shaderPath, sizeof(shaderPath), "%s%s", SHADER_PATH, planetsData[num].shader_path);
    snprintf(modelPath, sizeof(modelPath), "%s%s", IMAGE_PATH, planetsData[num].model_path);
    Shader planetShader = LoadShader(TextFormat("%s.vs", shaderPath),TextFormat("%s.fs", shaderPath));
    rmviPlanet3D planet = {0};
    planet.model = LoadModel(TextFormat("%s", modelPath));
    lightLoc lightLoc = rlGetLocationLight(planetShader.id, "light");
    fillLocUniform(planetShader, &planet);
    SetShaderLight(planetShader,lightLoc,light);
    for ( int i = 0; i < planet.model.materialCount; i++){
        planet.model.materials[i].shader = planetShader;
    }
    Vector3 modelCenter = GetModelCenter(planet.model);
    Matrix translate = MatrixTranslate(
        -modelCenter.x,
        -modelCenter.y,
        -modelCenter.z
    );

    planet.model.transform = MatrixMultiply(
        planet.model.transform,
        translate
    );
    float normalizeScale = getModelNormalizationScale(planet.model);
    Matrix norm = MatrixScale(
        normalizeScale,
        normalizeScale,
        normalizeScale
    );
    planetsData[num].zUp ? 
        ( planet.model.transform = MatrixMultiply(
            MatrixMultiply(planet.model.transform, norm),
            MatrixRotateX(-90*DEG2RAD))
        )
            :
        ( planet.model.transform =
            MatrixMultiply(planet.model.transform, norm)
        );
            
    planet.shader = planetShader;
    return planet;
}

// retourne un ratio linéaire pour les petites planète et
float apparentRadiusRatio(int num){ //
    return planetsData[num].radius_e < 3*planetsData[actualRep.reference].radius_e ? 
        planetsData[num].radius_e/planetsData[actualRep.reference].radius_e :
        log(planetsData[num].radius_e/planetsData[actualRep.reference].radius_e);   
}
 
Vector3d getPositionFromJson(int num){
    return (Vector3d) {
        (double) planetsData[num].aphelion*cosf(planetsData[num].theta*DEG2RAD)*cosf(planetsData[num].phi*DEG2RAD),
        (double) planetsData[num].aphelion*sinf(planetsData[num].theta*DEG2RAD)*sinf(planetsData[num].phi*DEG2RAD),
        (double) planetsData[num].aphelion*sinf(planetsData[num].theta*DEG2RAD)*cosf(planetsData[num].phi*DEG2RAD)
    };
}

Vector3d getVelocityFromJson(int num, Vector3d position){
    Vector3d up = { 0, 1, 0 };
    Vector3d normal = Vector3dRotateByAxisAngle(
        up,
        Vector3dNormalize(position),
        planetsData[num].phi * DEG2RAD
    );
    Vector3d tangent = Vector3dCrossProduct(normal, position);
    return Vector3dNormalize(tangent);
}

rmviDynamic3D getDynamicPlanet3D(int num){
    Vector3d position = getPositionFromJson(num);
    Vector3d velocityDir = getVelocityFromJson(num, position); // DOIT être normalisée
    rmviDynamic3D feature = {
        .position = position,
        .mass     = planetsData[num].mass,
        .velocity = Vector3dScale(velocityDir, planetsData[num].velocity_min),
        .force    = (Vector3d) {0},
        .radius   = planetsData[num].radius_e
    };
    return feature;
}
rmviPlanet3D getPlanet3D(int num){
    rmviPlanet3D planet = getModelPlanet3D(num);
    planet.features = getDynamicPlanet3D(num);
    return planet;
}

void rmviUpdatePosition3D(rmviDynamic3D *features, float dt, Vector3d centerSpeed, Vector3d centerPos){
    // Met à jour la position et la vitesse en fonction de la force
    Vector3d acceleration = Vector3dScale(features->force, 1.0 /features->mass);
    features->position = Vector3dAdd(Vector3dAdd(Vector3dAdd(features->position, Vector3dScale(features->velocity, dt)),Vector3dAdd(Vector3dScale(acceleration, dt*dt/2),Vector3dScale(centerSpeed, -dt))), Vector3dScale(centerPos, -1.0));
    features->velocity = Vector3dAdd(features->velocity, Vector3dScale(acceleration, dt));
    
}
void drawPlanet3D(rmviPlanet3D planet,Matrix modelMat,Matrix viewMat,Matrix projMat){
    for (int m = 0; m < planet.model.meshCount; m++){
        rlEnableVertexArray(planet.model.meshes[m].vaoId);
        (planet.model.meshes->indices != NULL) ? rlDrawVertexArrayElements(0, planet.model.meshes->triangleCount * 3, NULL) : rlDrawVertexArray(0, planet.model.meshes->vertexCount);
        rlDisableVertexArray();
    }
}
void CartesianToSpherical(Vector3 p, float *r, float *theta, float *phi){
    *r = Vector3Length(p);
    if (*r > 0.0f){
        *theta = atan2f(p.z, p.x);            // azimut
        *phi   = asinf(p.y / (*r));           // élévation
    }
    else{
        *theta = 0.0f;
        *phi   = 0.0f;
    }
}
// à modifier
void help(){
    //float r, theta, phi;
    //CartesianToSpherical(features[n]->position, &r, &theta, &phi);
    char text[256];
    float value = FRAME2SEC*pow(2.0f, actualRep.calculByFrame)*GetFPS();
    if (value > 2*TIME_DAY)snprintf(text, sizeof(text), "1s = %.0f jour%s" ,(value/(TIME_DAY)),(value/(TIME_DAY) > 1.0f) ? "s" : "");
    else if (value > 10*60*60)snprintf(text, sizeof(text), "1s = %.0f heure%s", value/3600,(value/3600 > 1.0f) ? "s" : "");
    else snprintf(text, sizeof(text),"1s = %.0f seconde%s",value,(value > 1.0f) ? "s" : "");
    Vector2 helpPos = (Vector2){GetScreenWidth()*0.8, GetScreenHeight()*0.2};
    rmviWriteLatex(text,&helpPos, &helpState);
}


char *LoadTextFile(const char *path){
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *data = malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);
    return data;
}
PlanetData *LoadPlanetsFromJSON(const char *path, int *outCount){
    char *text = LoadTextFile(path);
    if (!text) return NULL;
    cJSON *root = cJSON_Parse(text);
    if (!cJSON_IsArray(root)){
        cJSON_Delete(root);
        free(text);
        return NULL;
    }
    int count = cJSON_GetArraySize(root);
    PlanetData *planets = calloc(count, sizeof(PlanetData));
    for (int i = 0; i < count; i++)
    {
        cJSON *p = cJSON_GetArrayItem(root, i);
        PlanetData *dst = &planets[i];
        dst->position      = atoi(cJSON_GetObjectItem(p, "position")->valuestring);
        strncpy(dst->name, cJSON_GetObjectItem(p, "name")->valuestring, 31);
        dst->mass          = cJSON_GetObjectItem(p, "mass")->valuedouble;
        dst->radius_e      = cJSON_GetObjectItem(p, "radius_e")->valuedouble;
        dst->phi           = cJSON_GetObjectItem(p, "phi")->valuedouble;
        dst->theta         = cJSON_GetObjectItem(p, "theta")->valuedouble;
        dst->perihelion    = cJSON_GetObjectItem(p, "Perihelion")->valuedouble;
        dst->aphelion      = cJSON_GetObjectItem(p, "Aphelion")->valuedouble;
        dst->velocity_max  = cJSON_GetObjectItem(p, "velocity_max")->valuedouble;
        dst->velocity_min  = cJSON_GetObjectItem(p, "velocity_min")->valuedouble;
        dst->rotation      = cJSON_GetObjectItem(p, "rotation")->valuedouble;
        dst->inclinaison   = cJSON_GetObjectItem(p, "inclinaison")->valuedouble;
        dst-> zUp          = cJSON_GetObjectItem(p, "zUp")->valueint;
        dst->a             = cJSON_GetObjectItem(p, "a")->valuedouble;
        dst->e             = cJSON_GetObjectItem(p, "e")->valuedouble;
        cJSON *model_path  = cJSON_GetObjectItem(p, "model_path");
        if (cJSON_IsString(model_path))
            snprintf(dst->model_path, sizeof(dst->model_path), "%s",model_path->valuestring);
        cJSON *shader_path = cJSON_GetObjectItem(p, "shader_path");
        if (cJSON_IsString(shader_path))
            snprintf(dst->shader_path, sizeof(dst->shader_path), "%s",shader_path->valuestring);
        cJSON *desc = cJSON_GetObjectItem(p, "description");
        if (cJSON_IsString(desc))
            strncpy(dst->description, desc->valuestring, 255);
    }
    cJSON_Delete(root);
    free(text);
    *outCount = count;
    return planets;
}

void updateMouse(){
}

void updateZoom(Vector2 wheel){ 
}

void processInput(){
    if(IsKeyPressed(KEY_M)) rec.isRecording ? StopAudioRecorder(&rec) : StartAudioRecorder(&rec);
    if(IsKeyPressed(KEY_R)) actualRep.rotation = !actualRep.rotation;
    if (IsKeyPressed(KEY_L)){
        strongAmbient = !strongAmbient;
        if (strongAmbient){
            light.ambient = (Vector3){ambientVisible, ambientVisible, ambientVisible};
        } else {
            light.ambient = (Vector3){ambientSpace, ambientSpace, ambientSpace};
        }
    }
    if (IsKeyPressed(KEY_Q)) actualRep.asteroid = !actualRep.asteroid;
    if (IsKeyPressed(KEY_H)) actualRep.helpActive = !actualRep.helpActive;
    if (IsKeyPressed(KEY_F)) actualRep.draw = !actualRep.draw;
    if (IsKeyPressed(KEY_E)) actualRep.ecrit = !actualRep.ecrit;
    if (IsKeyPressed(KEY_RIGHT) && actualRep.ecrit) actualRep.countRight++;
    if (IsKeyPressed(KEY_LEFT) && actualRep.countRight > 0) actualRep.countRight--;
    if (IsKeyPressed(KEY_J)) actualRep.reference = 5;
    if (IsKeyPressed(KEY_T)) actualRep.reference = 3;
    if (IsKeyPressed(KEY_Y)) actualRep.isTransition = !actualRep.isTransition ;
    if (IsKeyPressed(KEY_DOWN)) actualRep.calculByFrame --;
    if (IsKeyPressed(KEY_UP)) actualRep.calculByFrame ++;
    if (IsKeyPressed(KEY_B)) actualRep.resetDash = true;
    
}

lightLoc rlGetLocationLight(unsigned int shader_id, const char *name) {
    lightLoc loc;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s.position", name);
    loc.position = rlGetLocationUniform(shader_id, buffer);
    snprintf(buffer, sizeof(buffer), "%s.ambient", name);
    loc.ambient = rlGetLocationUniform(shader_id, buffer);
    snprintf(buffer, sizeof(buffer), "%s.diffuse", name);
    loc.diffuse = rlGetLocationUniform(shader_id, buffer);
    snprintf(buffer, sizeof(buffer), "%s.specular", name);
    loc.specular = rlGetLocationUniform(shader_id, buffer);
    return loc;
}

void SetShaderLight(Shader shader, lightLoc matLoc, Light light){
    SetShaderValue(shader, matLoc.position, &light.position,RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, matLoc.ambient,(float*) &light.ambient,RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, matLoc.diffuse,(float*) &light.diffuse,RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, matLoc.specular,(float*) &light.specular,RL_SHADER_UNIFORM_VEC3);
} 