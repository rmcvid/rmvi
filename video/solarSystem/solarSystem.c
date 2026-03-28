#include "config.h"
#include <stdio.h>
#include "fft_wrapper.h"
#define UA 150e9 // distance moyenne terre soleil en km
#define UA2PIXEL 1000.0f // facteur d'échelle pour les distances
#define UA2PIXEL_FAR 180.0f
#define TIME_DAY 24*60*60
#define FRAME2SEC 100                // conversion frame en seconde
#define EARTH_APPARENT_SIZE 30.0f
#define SIZE_TEXT 40.0f
#define SIZE_SPACING SIZE_TEXT/20.0f
#define TIME_TO_TRANSITION 10.0f
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
    bool    run;
    int     calculByFrame;          // on fait pow(2, calculByFrame) par frame pour faire avancer la simulation plus vite
    bool    isTransition;           // si on est en train de faire un changement de référentiel ou pas
    bool    resetDash;              // reset de dash
} Representation; 
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
void drawPlanet3D(rmviPlanet3D planet);
void updatePlanet3D(rmviPlanet3D planet,Matrix modelMat,Matrix viewMat,Matrix projMat);
float getModelNormalizationScale(Model model);
void rmviUpdatePosition3D(rmviDynamic3D *features, double dt, Vector3d centerSpeed, Vector3d centerPos);
void CartesianToSpherical(Vector3 p, float *r, float *theta, float *phi);
void help();
float apparentRadiusRatio(int i); 
Vector3 GetModelCenter(Model model);
void SetShaderLight(Shader shader, lightLoc matLoc, Light light);
lightLoc rlGetLocationLight(unsigned int shader_id, const char *name);
static bool useOrtho = false;
#define MEMORY 500
//ici
rmviDash3 dashSun[MEMORY], dashMercury[MEMORY], dashVenus[MEMORY], dashEarth[MEMORY], dashMars[MEMORY], dashJupiter[MEMORY], dashSaturn[MEMORY];
rmviDash3 *dashList[7] = {dashSun, dashMercury, dashVenus, dashEarth, dashMars, dashJupiter, dashSaturn};

float skyboxVertices[] = {
    // positions          
    -1.0f,  0.4, -1.0f,
    -1.0f, -0.4, -1.0f,
     1.0f, -0.4, -1.0f,
     1.0f, -0.4, -1.0f,
     1.0f,  0.4, -1.0f,
    -1.0f,  0.4, -1.0f,

    -1.0f, -0.4,  1.0f,
    -1.0f, -0.4, -1.0f,
    -1.0f,  0.4, -1.0f,
    -1.0f,  0.4, -1.0f,
    -1.0f,  0.4,  1.0f,
    -1.0f, -0.4,  1.0f,

     1.0f, -0.4, -1.0f,
     1.0f, -0.4,  1.0f,
     1.0f,  0.4,  1.0f,
     1.0f,  0.4,  1.0f,
     1.0f,  0.4, -1.0f,
     1.0f, -0.4, -1.0f,

    -1.0f, -0.4,  1.0f,
    -1.0f,  0.4,  1.0f,
     1.0f,  0.4,  1.0f,
     1.0f,  0.4,  1.0f,
     1.0f, -0.4,  1.0f,
    -1.0f, -0.4,  1.0f
};

static Quaternion camRot = {0.0f,0.0f,0.0f,1.0f};
Vector3 initUp = {0.0f, 1.0f,  0.0f};                   // vecteur initialement en haut de la caméra
Vector3 initRight = { 1.0f, 0.0f, 0.0f };               // vecteur initialement à droite de la caméra
Vector3 initTarget;                                     // j'ai l'impressions qu'il sert à rien de le définir
Vector3 initPos = {0.0f, 400.0f,  0.0f};                // position initiale de la camera
float initAngle = -90.0*DEG2RAD;                        // angle de rotation initiale de la camera
Vector3 initVector = {1.0f,0.0f,0.0f};                  // vecteur de rotation initiale de la camera
Camera3D camera;

Player player;
float lastX, lastY;                // zoom
float shininess = 64;
bool firstMouse = true;
rmviPlanet3D* planets;
rmviDynamic3D **features;
State helpState;
int space_count = 0;
float deltatime = 0;
Representation actualRep;
Representation repEarth = {
    .frame2Sec = FRAME2SEC,
    .reference = 3,
    .rotation = true,
    .ua2Pixel = 800,
    .asteroid = false,
    .draw = true,
    .run = true,
    .calculByFrame = 4,
    .helpActive = true
};

static bool strongAmbient = false;
float ambientSpace = 0.02f, ambientVisible = 0.4f;
Light light ={
    {0.0f, 0.0f, 0.0f},
    {.02f, .02f, .02f},
    {0.6f, 0.6f, 0.6f},
    {.40f, .40f, .40f}
};

Matrix getModelMatPlanet(int i, rmviPlanet3D planet, int rotationFrame, Representation rep, PlanetData planetData){
    Matrix modelMat = MatrixIdentity();
    modelMat = MatrixMultiply(modelMat, planet.model.transform);
    modelMat = MatrixMultiply( 
        modelMat,
        MatrixScale(apparentRadiusRatio(i)*EARTH_APPARENT_SIZE,
                    apparentRadiusRatio(i)*EARTH_APPARENT_SIZE,
                    apparentRadiusRatio(i)*EARTH_APPARENT_SIZE)
        );
    modelMat = MatrixMultiply(modelMat, MatrixRotateY(2*PI*actualRep.frame2Sec*rotationFrame*pow(2,actualRep.calculByFrame)/(planetData.rotation*TIME_DAY) )
    );
    modelMat = MatrixMultiply(modelMat, MatrixRotateZ(planetData.inclinaison * DEG2RAD));
    modelMat = MatrixMultiply(modelMat,
        MatrixTranslate(
            planet.features.position.x /UA * actualRep.ua2Pixel,
            planet.features.position.y /UA * actualRep.ua2Pixel,
            planet.features.position.z /UA * actualRep.ua2Pixel
        ));
    return modelMat;
}



int bm_visual_main(void){   
    Vector2 paragraphPos = CENTER;
    float paragraphHeight;
    Matrix modelMat, viewMat, projMat;
    Token tokens[256];
    RenderBox boxes[256];
    AnimText *animText = initAnimText();
    State state = rmviGetStateClassic();
    int boxCount;
    float curentFrame,oldFrame = 0;
    int countFrame = 0;
    int planetCount = 0;
    planetsData = LoadPlanetsFromJSON(HERE_PATH "planets.json", &planetCount);
    planetCount = 7; 
    planets = malloc(sizeof(rmviPlanet3D)*planetCount);
    features = malloc(sizeof(rmviDynamic3D*) * planetCount);
    //ici
    for (int i = 0; i < planetCount; i++){
        planets[i] = getPlanet3D(i);
        features[i] = &planets[i].features;
    }
    Texture2D nightTex = LoadTexture(IMAGE_PATH"earth/Textures/Night_lights_2K.png");
    int nightLoc = GetShaderLocation(planets[3].shader, "nightMap");
    SetShaderValueTexture(planets[3].shader,nightLoc,nightTex);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float sizeOrtho = 10.0f;
    actualRep = repEarth;
    int rotationFrame = 0;
    rmviFloatList listWidth;
    int lineCount;
    float heighTotal;
    float transitionFactor = 0.0f;
    Vector3 sunDir;
    Vector3d  centerSpeed = (Vector3d) {0};
    Vector3d centerPos = (Vector3d) {0};
    rmviCarthesian carthesian = rmviGetCarthesian((Vector2) {CENTER.x/2, CENTER.y},(Vector2){490, 420},(Vector2){70, 70});
    Lecture lecture = rmviGetLecture(HERE_PATH "presentation.txt");
    //mp4ToTexture(IMAGE_PATH "decompte/decompte.mp4", IMAGE_PATH "decompte", "decompte");
    //Video decompte = LoadVideo(IMAGE_PATH "decompte/decompte.mp4", IMAGE_PATH "decompte", "decompte", 0);
    camera =(Camera3D) {
        .position = initPos,
        .target = initTarget,
        .up = initUp,
        .fovy = 45,
        .projection = CAMERA_PERSPECTIVE
    };
    player = (Player){
        .camera = camera,
        .velocity = {0,0,0},
        .speed = 500.0f
    };
    camRot = QuaternionFromAxisAngle(initVector, initAngle);

    // asteroid
    Shader asteroidShader = LoadShader(SHADER_PATH "asteroid.vs", SHADER_PATH "asteroid.fs");
    unsigned int astViewLoc = rlGetLocationUniform(asteroidShader.id, "view");
    unsigned int astProjLoc = rlGetLocationUniform(asteroidShader.id, "projection");
    unsigned int astModelLoc = rlGetLocationUniform(asteroidShader.id,"model");
    unsigned int astParentLoc = rlGetLocationUniform(asteroidShader.id,"parentModel");
    Model asteroid = LoadModel(IMAGE_PATH "rock/rock.obj");
    unsigned int amount = 1000;
    Matrix *modelMatrices = MemAlloc(sizeof(Matrix) * amount);
    srand((unsigned int)time(NULL));
    float radius = apparentRadiusRatio(6)*EARTH_APPARENT_SIZE;
    float offset = 50.0f;
    Matrix astModelMat;
    Color colorListPlanet[10];
    for (int i = 0; i < planetCount; i++){ 
        colorListPlanet[i] = GetAverageColor(planets[i].model.materials[0].maps[0].texture);
    }
    for (unsigned int i = 0; i < amount; i++){
        Matrix model = MatrixIdentity();
        // angle initial réparti uniformément
        float angle = ((float)i / (float)amount) * 2.0f * PI;
        // position SUR le cercle orbital
        float x = cosf(angle) *radius*radius*1.2f;
        float y = sinf(angle) *radius*radius*1.2f;
        float z = 0.0f;
        // scale de l’astéroïde (indépendant du rayon)
        float scale = Lerp(2.0f, 15.0f, rmviRand());
        model = MatrixMultiply(model, MatrixScale(scale, scale, scale));
        model = MatrixMultiply(model, MatrixTranslate(x, y, z));
        // rotation propre de l’astéroïde
        Vector3 axis = Vector3Normalize((Vector3){
            rmviRand(), rmviRand(), rmviRand()
        });
        float rotAngle = rmviRand() * 2.0f * PI;
        model = MatrixMultiply(MatrixRotate(axis, rotAngle), model);
        modelMatrices[i] = MatrixTranspose(model);
    }
    unsigned int instanceVBO = rlLoadVertexBuffer(modelMatrices, amount * sizeof(Matrix), false );
    for ( int i = 0; i < asteroid.meshCount; i++){
        asteroid.materials[i].shader = asteroidShader;
        rlEnableVertexArray(asteroid.meshes[i].vaoId);
        rlEnableVertexBuffer(instanceVBO);
        int matSize = sizeof(Matrix);
        int vec4Size = sizeof(Vector4);
        // mat4 = 4 x vec4
        rlSetVertexAttribute(4, 4, RL_FLOAT, false, matSize, 0);
        rlEnableVertexAttribute(4);
        rlSetVertexAttributeDivisor(4, 1);
        rlSetVertexAttribute(5, 4, RL_FLOAT, false, matSize, vec4Size);
        rlEnableVertexAttribute(5);
        rlSetVertexAttributeDivisor(5, 1);
        rlSetVertexAttribute(6, 4, RL_FLOAT, false, matSize, vec4Size * 2);
        rlEnableVertexAttribute(6);
        rlSetVertexAttributeDivisor(6, 1);
        rlSetVertexAttribute(7, 4, RL_FLOAT, false, matSize, vec4Size * 3);
        rlEnableVertexAttribute(7);
        rlSetVertexAttributeDivisor(7, 1);
        rlDisableVertexArray();
    }
    
    // sky box
    Shader skyshader = LoadShader(SHADER_PATH "skybox.vs",SHADER_PATH "skybox.fs");
    //unsigned int cubemapId = LoadCubemapFromCross4x3(IMAGE_PATH "skybox.png");
    unsigned int skyVao = rlLoadVertexArray();
    rlEnableVertexArray(skyVao);
    unsigned int skyVbo = rlLoadVertexBuffer(skyboxVertices, sizeof(skyboxVertices), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 3 * sizeof(float), 0);
    rlEnableVertexAttribute(0);
    //int skyboxLoc = GetShaderLocation(skyshader, "skybox");
    unsigned int skyViewLoc = rlGetLocationUniform(skyshader.id,"view");
    unsigned int skyProjLoc = rlGetLocationUniform(skyshader.id,"projection");
    unsigned int sunDirLoc = rlGetLocationUniform(skyshader.id,"sunDir");
    unsigned int cameraFrontLoc = rlGetLocationUniform(skyshader.id,"cameraFront");
    rlEnableShader(skyshader.id);
    //rlSetUniformSampler(skyboxLoc, 0); // texture unit 0
    helpState = rmviGetStateClassic();
    helpState.widthMax = 0.4*GetScreenWidth();
    rlDisableShader();
    rlDisableBackfaceCulling();
    DisableCursor();

    Model sunModel = LoadModelFromMesh(GenMeshSphere(1.0f, 36, 36));
    //Model coronaModel = LoadModelFromMesh(GenMeshSphere(120.0f, 36, 36));
    // on peut utiliser notre shader ou pas, il faudrait faire un test pour permettre
    Shader myShader = LoadShader(HERE_PATH"shader/sun2.vs", HERE_PATH"shader/sun2.fs"); 
    unsigned int locCam  = GetShaderLocation(myShader, "uCamPos");
    unsigned int centerSunLoc = GetShaderLocation(myShader, "centerSun");
    Vector3 sunPos = {0};
    Texture2D noiseTex = LoadTexture(IMAGE_PATH "sun/textures/sun2.jpg");
    sunModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = noiseTex;
    sunModel.materials[0].shader = myShader;
    planets[0].model = sunModel;
    planets[0].shader = myShader;

    float startTime = GetTime();
    while (!WindowShouldClose()){
        UpdateCursorToggle();
        curentFrame = GetTime();
        deltatime = curentFrame - oldFrame;
        oldFrame = curentFrame;
        BeginTextureMode(screen);
            processInput();
            if(actualRep.run){
                ClearBackground(BG);
                rlClearScreenBuffers();  // clear color + depth (et stencil si présent)
                rlEnableDepthTest();
                // Player movement and camera
                if(IsKeyPressed(KEY_RIGHT)) resetAnimText(animText);
                updateMouse();
                //PlayVideo(&decompte);
                updateZoom(GetMouseWheelMoveV());
                // updatepos
                if (actualRep.rotation) rotationFrame++;
                if (actualRep.isTransition){
                    transitionFactor += 1/(TIME_TO_TRANSITION*FPS);
                    if(transitionFactor >=1){
                        transitionFactor = 1;
                    } 
                }
                else{
                    transitionFactor -= 1/(TIME_TO_TRANSITION*FPS);
                    if(transitionFactor <=0) transitionFactor = 0;
                }
                
                if(actualRep.resetDash){
                    actualRep.resetDash = false;
                    for (int i = 0; i < 7; i++) {
                        memset(dashList[i], 0, sizeof(rmviDash3) * MEMORY);
                    }
                }
                for(int j = 0; j< pow(2,actualRep.calculByFrame); j++){
                    rmviGravityRepulsion3D(features,planetCount);
                    centerSpeed = Vector3dScale(features[3]->velocity,transitionFactor);
                    centerPos = Vector3dScale(features[3]->position,transitionFactor);
                    for(int i = 0; i< planetCount; i++){
                        rmviUpdatePosition3D(features[i],actualRep.frame2Sec,centerSpeed, (Vector3d) {0});
                        rmviAddDash3(dashList[i], MEMORY, countFrame, 2, features[i], camera.position,Vector3d2Vector3(centerSpeed));
                    }
                }
                for(int i = 0; i< planetCount; i++){
                    features[i]->position = Vector3dSubtract(features[i]->position, Vector3dScale(centerPos,transitionFactor));
                }//,
                viewMat = MatrixLookAt(camera.position, 
                    Vector3Add(camera.position,camera.target),
                    camera.up);
                useOrtho ? 
                    (projMat = MatrixOrtho( -sizeOrtho * aspect,sizeOrtho * aspect,-sizeOrtho, sizeOrtho, 0.1f, 3000.0f)):
                    (projMat = MatrixPerspective(DEG2RAD * camera.fovy,aspect,0.1f,30000.0f));
                rlSetMatrixProjection(projMat);
                rlSetMatrixModelview(viewMat);
                light.position = Vector3Scale(Vector3d2Vector3(features[0]->position), 1.0 /UA * actualRep.ua2Pixel);
                for(int i = 0; i< planetCount; i++){
                    if(actualRep.draw){
                        modelMat = getModelMatPlanet(i, planets[i], rotationFrame, actualRep, planetsData[i]);
                        SetShaderValue(planets[i].shader, planets[i].locUniform.lightLoc.position, &light.position,RL_SHADER_UNIFORM_VEC3);
                        SetShaderValue(planets[i].shader, planets[i].locUniform.lightLoc.ambient, &light.ambient,RL_SHADER_UNIFORM_VEC3);
                        SetShaderValue(planets[i].shader, planets[i].locUniform.viewPosLoc, &camera.position, RL_SHADER_UNIFORM_VEC3);
                        if (planets[i].locUniform.timeLoc != -1){
                            float time = (float)GetTime();
                            SetShaderValue(planets[i].shader, planets[i].locUniform.timeLoc, &time, RL_SHADER_UNIFORM_FLOAT);
                            //SetShaderValue(planets[i].shader, locCam, &camera.position, SHADER_UNIFORM_VEC3);
                            if(i == 0){
                                sunPos = Vector3Scale(Vector3d2Vector3(features[i]->position), UA2PIXEL/UA) ; 
                                SetShaderValue(planets[i].shader, centerSunLoc, &sunPos, SHADER_UNIFORM_VEC3);
                                SetShaderValue(myShader, locCam, &camera.position, SHADER_UNIFORM_VEC3);
                            }
                        }
                        updatePlanet3D(planets[i],modelMat,viewMat,projMat);
                        if( i == 3) {
                            rlActiveTextureSlot(1);
                            rlEnableTexture(nightTex.id);
                        }

                        drawPlanet3D(planets[i]);
                        if(i==6) astModelMat = modelMat;
                        rlDisableShader();
                        // modelview
                        rmviDrawDash3Fast(dashList[i], MEMORY , 10.0f,  1.0f /UA * actualRep.ua2Pixel,colorListPlanet[i]);
                    }
                }
                // draw asteroid field
                if (actualRep.asteroid && actualRep.draw){
                    SetShaderValueMatrix(asteroidShader, astParentLoc, astModelMat);
                    for (int m = 0; m < asteroid.meshCount; m++){
                        int matIndex = asteroid.meshMaterial[m];
                        Material *mat = &asteroid.materials[matIndex];
                        rlEnableShader(mat->shader.id);
                        rlSetUniformMatrices(astViewLoc, &viewMat,1);
                        rlSetUniformMatrices(astProjLoc, &projMat,1);
                        rlActiveTextureSlot(0);
                        rlEnableTexture(mat->maps[MATERIAL_MAP_DIFFUSE].texture.id);
                        rlEnableVertexArray(asteroid.meshes[m].vaoId);
                        rlDrawVertexArrayInstanced(0, asteroid.meshes[m].vertexCount,amount );
                        rlDisableVertexArray();
                        
                    }
                }
                rlDisableShader();
                lecture.currentParagraph = actualRep.countRight;
                //skybox
                sunDir = Vector3Normalize(Vector3Subtract(light.position, camera.position));
                rlEnableVertexArray(skyVao);
                rlEnableShader(skyshader.id);
                rlSetUniformMatrices(skyViewLoc, &viewMat,1);
                rlSetUniformMatrices(skyProjLoc, &projMat,1);
                rlSetUniform(sunDirLoc, &sunDir, RL_SHADER_UNIFORM_VEC3, 1);
                rlSetUniform(cameraFrontLoc, &camera.target, RL_SHADER_UNIFORM_VEC3, 1);
                //rlActiveTextureSlot(0);
                //rlEnableTextureCubemap(cubemapId);
                rlDrawVertexArray(0,36);
                rlEnableDepthMask();
            }
        EndTextureMode();
        // ecrans 2d
        BeginTextureMode(screen);
            DrawFPS(10, 10);
            if (actualRep.helpActive) help();
            if(actualRep.ecrit){
                if(readScenario(&lecture)){
                    animText->animTime = 0;
                    int tokenCount = rmviTokenizeLatex(lecture.content,tokens, 256);
                    boxCount = rmviBuildRenderBoxes(tokens,tokenCount,boxes,&state);
                    if( lecture.content != NULL) paragraphPos = (Vector2) { CENTER.x, CENTER.y}; 
                }
                Vector2 drawPos = paragraphPos;
                rmviCalcWidthLine(boxes, boxCount,&listWidth);
                heighTotal = rmviCalcHeightTotal(boxes, boxCount);
                //rmviDrawRenderBoxesCentered(listWidth, heighTotal,boxes, boxCount,drawPos,mathFont,SIZE_TEXT, SIZE_SPACING,WHITE);
                rmviDrawRenderBoxesCenteredAnimed(boxes, boxCount,drawPos,animText);
                rmviAudioRenderBoxesAnimed(animText);
                //rmviDrawCarthesianFull(carthesian, 10, 3, WHITE,true,false);
                //rmviWriteLatex(paragraph, &drawPos, SIZE_TEXT, SIZE_SPACING, WHITE, mathFont);    
            }
        EndTextureMode();
        rmviDraw();
        if (RECORDING) addImageToffmpeg(ffmpeg);
        countFrame++;
    }
    free(planets);
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
        .force    = (Vector3d){0},
        .radius   = planetsData[num].radius_e
    };
    return feature;
}
rmviPlanet3D getPlanet3D(int num){
    rmviPlanet3D planet = getModelPlanet3D(num);
    planet.features = getDynamicPlanet3D(num);
    return planet;
}

void rmviUpdatePosition3D(rmviDynamic3D *features, double dt, Vector3d centerSpeed, Vector3d centerPos){
    // Met à jour la position et la vitesse en fonction de la force
    Vector3d acceleration = Vector3dScale(features->force, 1.0 / (features->mass));
    features->position = Vector3dAdd(Vector3dAdd(Vector3dAdd(features->position, Vector3dScale(features->velocity, dt)),Vector3dAdd(Vector3dScale(acceleration, dt*dt/2.0f),Vector3dScale(centerSpeed, -dt))), Vector3dScale(centerPos, -1.0f));
    features->velocity = Vector3dAdd(features->velocity, Vector3dScale(acceleration, dt));
    
}
void updatePlanet3D(rmviPlanet3D planet,Matrix modelMat,Matrix viewMat,Matrix projMat){
    SetShaderValueMatrix(planet.shader,planet.locUniform.modelLoc,modelMat);
    SetShaderValueMatrix(planet.shader,planet.locUniform.viewLoc,viewMat);
    SetShaderValueMatrix(planet.shader,planet.locUniform.projLoc,projMat);
    rlEnableShader(planet.shader.id);
    rlActiveTextureSlot(0);
    rlEnableTexture(planet.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.id);
}
void drawPlanet3D(rmviPlanet3D planet){
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
    rmviWriteLatex(text,&helpPos , &helpState);
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
    if (!cJSON_IsArray(root))
    {
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
    // En gros on un quaternion, camRot, qui représente toutes les modifications d'orientation éffectué
    // à chaque rotation additionnel, camRot est est modifié pour ajouter cette rotation.
    // L'oriantation de la camera à tout moment est l'orientation initiale multiplié par cam rot
    //
    Vector2 delta = GetMouseDelta();
    float sensitivity = 0.0025f;
    float yawDelta   =  -delta.x * sensitivity;
    float pitchDelta =  -delta.y * sensitivity;
    // il faut ajouter un qroll
    // ici on pourrait définir directement l'axe perpendiculaire et définir seulement un q non
    Vector3 worldUp = Vector3RotateByQuaternion(initUp, camRot);
    Vector3 right = Vector3RotateByQuaternion(initRight, camRot);
    
    Quaternion qYaw   = QuaternionFromAxisAngle(worldUp, yawDelta);
    Quaternion qPitch = QuaternionFromAxisAngle(right,  pitchDelta);
    camRot = QuaternionMultiply(qYaw, camRot);
    camRot = QuaternionMultiply(qPitch, camRot);
    camRot = QuaternionNormalize(camRot);

    // Déduire les axes caméra depuis l'orientation
    Vector3 front = Vector3Normalize(Vector3RotateByQuaternion((Vector3){ 0, 0, -1 }, camRot));
    Vector3 up    = Vector3Normalize(Vector3RotateByQuaternion((Vector3){ 0, 1,  0 }, camRot));
    camera.target = front;
    camera.up = up;
}

void updateZoom(Vector2 wheel){
    camera.fovy -= (float)wheel.y;
    if (camera.fovy < 1.0f)
        camera.fovy = 1.0f;
    if (camera.fovy > 45.0f)
        camera.fovy = 45.0f; 
}

void processInput(){
    if (IsKeyDown(KEY_SPACE)) camera.position = Vector3Add(camera.position , Vector3Scale(camera.target,player.speed*deltatime));
    if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position , Vector3Scale(camera.target,player.speed*deltatime));
    if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position , 
        Vector3Scale(Vector3Normalize (Vector3CrossProduct(camera.target,camera.up)),player.speed*deltatime));
    if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position , 
        Vector3Scale(Vector3Normalize (Vector3CrossProduct(camera.target,camera.up)),player.speed*deltatime));
    if (IsKeyPressed(KEY_P)){ actualRep.run = !actualRep.run;}
    if(IsKeyPressed(KEY_M)) rec.isRecording ? StopAudioRecorder(&rec) : StartAudioRecorder(&rec);
    if(IsKeyPressed(KEY_R)) actualRep.rotation = !actualRep.rotation;
    if (IsKeyPressed(KEY_ZERO))  camera.position = Vector3Scale( Vector3d2Vector3(planets[0].features.position),  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_ONE))   camera.position = Vector3Scale( Vector3d2Vector3(planets[1].features.position),  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_TWO))   camera.position = Vector3Scale( Vector3d2Vector3(planets[2].features.position),  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_THREE)) camera.position = Vector3Scale( Vector3d2Vector3(planets[3].features.position),  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_FOUR))  camera.position = Vector3Scale( Vector3d2Vector3(planets[4].features.position),  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_FIVE))  camera.position = Vector3Scale( Vector3d2Vector3(planets[5].features.position),  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_SIX))   camera.position = Vector3Scale( Vector3d2Vector3(planets[6].features.position),  1.0f /UA * actualRep.ua2Pixel);
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
    if (IsKeyPressed(KEY_Y)) actualRep.isTransition = !actualRep.isTransition;
    if (IsKeyPressed(KEY_X)) actualRep.isTransition = true ;
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