#include "config.h"
#include <stdio.h>
#include "fft_wrapper.h"
#define UA 150e9 // distance moyenne terre soleil en km
#define UA2PIXEL 1000.0f // facteur d'échelle pour les distances
#define UA2PIXEL_FAR 180.0f
#define TIME_DAY 24*60*60
#define FRAME2SEC 1000                // conversion frame en seconde
#define EARTH_APPARENT_SIZE 30.0f
#define MAX_LINE 1024
#define MAX_PARAGRAPH 8192
#define SIZE_TEXT 40.0f
#define SIZE_SPACING SIZE_TEXT/5.0f
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

typedef struct{
    float   frame2Sec;
    bool    rotation;
    int     reference;
    float   ua2Pixel;
    bool    asteroid;
    bool    helpActive;
    int     countRight;
    bool    draw;
    bool    ecrit;
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
void drawPlanet3D(rmviPlanet3D planet,Matrix modelMat,Matrix viewMat,Matrix projMat);
float getModelNormalizationScale(Model model);
void rmviUpdatePosition3D(rmviDynamic3D *features, float dt, Vector3 centerSpeed);
void CartesianToSpherical(Vector3 p, float *r, float *theta, float *phi);
void help(rmviDynamic3D **features, int n);
float apparentRadiusRatio(int i); 
Vector3 GetModelCenter(Model model);
void SetShaderLight(Shader shader, lightLoc matLoc, Light light);
lightLoc rlGetLocationLight(unsigned int shader_id, const char *name);
char* readScenario(const char *path, int countRight);

static bool useOrtho = false;


Vector3 cameraPos   = {0.0f, 0.0f,  +100.0f};
Vector3 cameraFront = {0.0f, 0.0f, -1.0f};
Vector3 cameraUp    = {0.0f, 1.0f,  0.0f};
float yaw = -90, pitch = 0;
float cameraSpeed;
float lastX, lastY;
float fov = 45;
float shininess = 64;
bool firstMouse = true;
rmviPlanet3D* planets;
rmviDynamic3D **features;

int space_count = 0;
Representation actualRep;
Representation repEarth = {
    .frame2Sec = 1000,
    .reference = 3,
    .rotation = true,
    .ua2Pixel = 800,
    .asteroid = false,
    .countRight = 0,
    .draw = false,
    .ecrit = false
};

static bool strongAmbient = false;
float ambientSpace = 0.02f, ambientVisible = 0.4f;
Light light ={
    {0.0f, 0.0f, 0.0f},
    {.02f, .02f, .02f},
    {0.6f, 0.6f, 0.6f},
    {.40f, .40f, .40f}
};



int bm_visual_main(void)
{   
    char *paragraph;
    Vector2 paragraphPos = CENTER;
    float paragraphHeight;
    Matrix modelMat, viewMat, projMat;
    float deltatime = 0;
    float curentFrame,oldFrame = 0;
    int countFrame = 0;
    int planetCount = 0;
    planetsData = LoadPlanetsFromJSON(HERE_PATH "planets.json", &planetCount);
    planets = malloc(sizeof(rmviPlanet3D)*planetCount);
    features = malloc(sizeof(rmviDynamic3D*) * planetCount);
    for (int i = 0; i < planetCount; i++){
        planets[i] = getPlanet3D(i);
        features[i] = &planets[i].features;
    }
    Texture2D nightTex = LoadTexture(IMAGE_PATH"earth/Textures/Night_lights_2K.png");
    int nightLoc = GetShaderLocation(planets[3].shader, "nightMap");
    SetShaderValueTexture(planets[3].shader,nightLoc,nightTex);
    float playerSpeed = 500;
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float sizeOrtho = 10.0f;
    actualRep = repEarth;
    int rotationFrame = 0;
    int oldCountRight = -1;
    //mp4ToTexture(IMAGE_PATH "decompte/decompte.mp4", IMAGE_PATH "decompte", "decompte");
    //Video decompte = LoadVideo(IMAGE_PATH "decompte/decompte.mp4", IMAGE_PATH "decompte", "decompte", 0);
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
    
    rlDisableBackfaceCulling();
    DisableCursor();
    float startTime = GetTime();
    while (!WindowShouldClose())
    {
        UpdateCursorToggle();
        curentFrame = GetTime();
        deltatime = curentFrame - oldFrame;
        oldFrame = curentFrame;
        cameraSpeed = playerSpeed*deltatime;
        BeginTextureMode(screen);
            ClearBackground(BG);
            rlClearScreenBuffers();  // clear color + depth (et stencil si présent)
            rlEnableDepthTest();
            // Player movement and camera
            processInput();
            updateMouse();
            //PlayVideo(&decompte);
            rmviGravityRepulsion3D(features,planetCount);
            updateZoom(GetMouseWheelMoveV());
            viewMat = MatrixLookAt(
                cameraPos, 
                Vector3Add(cameraPos,cameraFront), 
                cameraUp);
            
            useOrtho ? 
                (projMat = MatrixOrtho( -sizeOrtho * aspect,sizeOrtho * aspect,-sizeOrtho, sizeOrtho, 0.1f, 3000.0f)):
                (projMat = MatrixPerspective(DEG2RAD * fov,aspect,0.1f,30000.0f));
            // updatepos
            if (actualRep.rotation) rotationFrame++;
            for(int i = 0; i< planetCount; i++){
                // update model
                rmviUpdatePosition3D(features[i],actualRep.frame2Sec,Vector3Zero());
                if(actualRep.draw){
                    if(i == 0) light.position = Vector3Scale(features[i]->position, 1.0f /UA * actualRep.ua2Pixel);
                modelMat = MatrixIdentity();
                modelMat = MatrixMultiply(modelMat, planets[i].model.transform);
                modelMat = MatrixMultiply( 
                    modelMat,
                    MatrixScale(apparentRadiusRatio(i)*EARTH_APPARENT_SIZE,
                                apparentRadiusRatio(i)*EARTH_APPARENT_SIZE,
                                apparentRadiusRatio(i)*EARTH_APPARENT_SIZE)
                    );
                modelMat = MatrixMultiply(modelMat,
                                MatrixRotateY(2*PI*actualRep.frame2Sec*rotationFrame/(planetsData[i].rotation*TIME_DAY) )
                );
                modelMat = MatrixMultiply(modelMat, MatrixRotateZ(planetsData[i].inclinaison * DEG2RAD));
                modelMat = MatrixMultiply(modelMat,
                    MatrixTranslate(
                        planets[i].features.position.x /UA * actualRep.ua2Pixel,
                        planets[i].features.position.y /UA * actualRep.ua2Pixel,
                        planets[i].features.position.z /UA * actualRep.ua2Pixel
                    ));
                SetShaderValue(planets[i].shader, planets[i].locUniform.lightLoc.position, &light.position,RL_SHADER_UNIFORM_VEC3);
                SetShaderValue(planets[i].shader, planets[i].locUniform.lightLoc.ambient, &light.ambient,RL_SHADER_UNIFORM_VEC3);
                SetShaderValue(planets[i].shader, planets[i].locUniform.viewPosLoc, &cameraPos, RL_SHADER_UNIFORM_VEC3);
                if (planets[i].locUniform.timeLoc != -1){
                    float time = (float)GetTime();
                    SetShaderValue(planets[i].shader, planets[i].locUniform.timeLoc, &time, RL_SHADER_UNIFORM_FLOAT);
                }
                SetShaderValueMatrix(planets[i].shader,planets[i].locUniform.modelLoc,modelMat);
                SetShaderValueMatrix(planets[i].shader,planets[i].locUniform.viewLoc,viewMat);
                SetShaderValueMatrix(planets[i].shader,planets[i].locUniform.projLoc,projMat);
                rlEnableShader(planets[i].shader.id);
                rlActiveTextureSlot(0);
                rlEnableTexture(planets[i].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.id);
                rlActiveTextureSlot(1);
                rlEnableTexture(nightTex.id);
                drawPlanet3D(planets[i],modelMat,viewMat,projMat);
                if(i==6) astModelMat = modelMat;
                rlDisableShader();
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
            DrawFPS(10, 10);
            if (actualRep.helpActive) help(features,3);
            if(actualRep.ecrit){
                if(oldCountRight != actualRep.countRight){
                oldCountRight = actualRep.countRight;
                paragraph = readScenario(HERE_PATH "presentation.txt", actualRep.countRight);
                if( paragraph != NULL){
                    paragraphHeight = rmviCalcTextHeight(paragraph, mathFont, SIZE_TEXT, SIZE_SPACING, true);
                    paragraphPos = (Vector2) { CENTER.x, CENTER.y - paragraphHeight/2.0f };
                } 
            }
            
            Vector2 drawPos = paragraphPos;
            rmviWriteLatexLeftCentered(paragraph, &drawPos, SIZE_TEXT, SIZE_SPACING, WHITE, mathFont);    
            }
        EndTextureMode();
        rmviDraw();
        if (RECORDING) addImageToffmpeg(ffmpeg);
        countFrame++;
    }
    free(planets);
    return 0;
}

int isBlankLine(const char *line){
    while (*line)
    {
        if (*line != ' ' && *line != '\t' &&
            *line != '\n' && *line != '\r')
            return 0;
        line++;
    }
    return 1;
}
char* readScenario(const char *path, int countRight)
{
    FILE *file = fopen(path, "r");
    if (!file) return NULL;

    char line[MAX_LINE];
    char temp[MAX_PARAGRAPH] = {0};

    int currentParagraph = 0;
    int hasContent = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (isBlankLine(line))
        {
            if (hasContent)
            {
                if (currentParagraph == countRight)
                {
                    fclose(file);
                    char *result = malloc(strlen(temp) + 1);
                    strcpy(result, temp);
                    return result;
                }

                temp[0] = '\0';
                hasContent = 0;
                currentParagraph++;
            }
        }
        else
        {
            hasContent = 1;
            if (strlen(temp) + strlen(line) < MAX_PARAGRAPH)
                strcat(temp, line);
        }
    }

    // Fin de fichier
    if (hasContent && currentParagraph == countRight)
    {
        char *result = malloc(strlen(temp) + 1);
        strcpy(result, temp);
        fclose(file);
        return result;
    }

    fclose(file);
    return NULL;
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
 
Vector3 getPositionFromJson(int num){
    return (Vector3) {
        planetsData[num].aphelion*cosf(planetsData[num].theta*DEG2RAD)*cosf(planetsData[num].phi*DEG2RAD),
        planetsData[num].aphelion*sinf(planetsData[num].theta*DEG2RAD)*sinf(planetsData[num].phi*DEG2RAD),
        planetsData[num].aphelion*sinf(planetsData[num].theta*DEG2RAD)*cosf(planetsData[num].phi*DEG2RAD)
    };
}

Vector3 getVelocityFromJson(int num, Vector3 position){
    Vector3 up = { 0, 1, 0 };
    Vector3 normal = Vector3RotateByAxisAngle(
        up,
        Vector3Normalize(position),
        planetsData[num].phi * DEG2RAD
    );
    Vector3 tangent = Vector3CrossProduct(normal, position);
    return Vector3Normalize(tangent);
}

rmviDynamic3D getDynamicPlanet3D(int num){
    Vector3 position = getPositionFromJson(num);
    Vector3 velocityDir = getVelocityFromJson(num, position); // DOIT être normalisée
    rmviDynamic3D feature = {
        .position = position,
        .mass     = planetsData[num].mass,
        .velocity = Vector3Scale(velocityDir, planetsData[num].velocity_min),
        .force    = Vector3Zero(),
        .radius   = planetsData[num].radius_e
    };
    return feature;
}
rmviPlanet3D getPlanet3D(int num){
    rmviPlanet3D planet = getModelPlanet3D(num);
    planet.features = getDynamicPlanet3D(num);
    return planet;
}

void rmviUpdatePosition3D(rmviDynamic3D *features, float dt, Vector3 centerSpeed){
    // Met à jour la position et la vitesse en fonction de la force
    Vector3 acceleration = Vector3Scale(features->force, 1.0f / ((float)features->mass));
    features->position = Vector3Add(Vector3Add(features->position, Vector3Scale(features->velocity, dt)), Vector3Scale(acceleration, dt*dt/2.0f));
    features->velocity = Vector3Add(Vector3Add(features->velocity, Vector3Scale(acceleration, dt)), Vector3Scale(centerSpeed, -1.0f));
    
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
void help(rmviDynamic3D **features, int n)
{
    float r, theta, phi;
    CartesianToSpherical(features[n]->position, &r, &theta, &phi);
    char text[256];
    char text2[256];
    snprintf(
        text,
        sizeof(text),
        //" j'écris en 0 /begin(itemize) /item Planet: %s /item Distance: %.2f UA /item /theta : %.2f /item /phi : %.2f /end(itemize) // ",
        "j'écris en centrée // et pour la suite ?",
        planetsData[n].name,
        r,
        theta * RAD2DEG,
        phi   * RAD2DEG
    );
    snprintf(
        text2,
        sizeof(text2),
        //" j'écris en 0 /begin(itemize) /item Planet: %s /item Distance: %.2f UA /item /theta : %.2f /item /phi : %.2f /end(itemize)",
        "position suivante ?",
        planetsData[n].name,
        r,
        theta * RAD2DEG,
        phi   * RAD2DEG
    );
    Vector2 textPos = {CENTER.x , CENTER.y};
    rmviWriteLatexLeftCenteredClassic(text, &textPos);
    rmviWriteLatexLeftCenteredClassic(text2, &textPos);

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
    Vector2 delta = GetMouseDelta(); // dx et dy depuis la dernière frame
    float sensitivity = 0.1f;
    yaw   += delta.x * sensitivity;
    pitch -= delta.y * sensitivity;

    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;

    Vector3 direction = {
        cosf(DEG2RAD*yaw) * cosf(DEG2RAD*pitch),
        sinf(DEG2RAD*pitch),
        sinf(DEG2RAD*yaw) * cosf(DEG2RAD*pitch)
    };
    cameraFront = Vector3Normalize(direction);
}

void updateZoom(Vector2 wheel)
{
    fov -= (float)wheel.y;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f; 
}
void processInput(){
    if (IsKeyDown(KEY_SPACE)) cameraPos = Vector3Add(cameraPos , Vector3Scale(cameraFront,cameraSpeed));
    if (IsKeyDown(KEY_S)) cameraPos = Vector3Subtract(cameraPos , Vector3Scale(cameraFront,cameraSpeed));
    if (IsKeyDown(KEY_A)) cameraPos = Vector3Subtract(cameraPos , 
        Vector3Scale(Vector3Normalize (Vector3CrossProduct(cameraFront,cameraUp)),cameraSpeed));
    if (IsKeyDown(KEY_D)) cameraPos = Vector3Add(cameraPos , 
        Vector3Scale(Vector3Normalize (Vector3CrossProduct(cameraFront,cameraUp)),cameraSpeed));
    if (IsKeyPressed(KEY_P)) useOrtho = !useOrtho;
    if(IsKeyPressed(KEY_M)) rec.isRecording ? StopAudioRecorder(&rec) : StartAudioRecorder(&rec);
    if(IsKeyPressed(KEY_R)) actualRep.rotation = !actualRep.rotation;
    if (IsKeyPressed(KEY_ZERO))  cameraPos = Vector3Scale(planets[0].features.position,  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_ONE))   cameraPos = Vector3Scale(planets[1].features.position,  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_TWO))   cameraPos = Vector3Scale(planets[2].features.position,  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_THREE)) cameraPos = Vector3Scale(planets[3].features.position,  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_FOUR))  cameraPos = Vector3Scale(planets[4].features.position,  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_FIVE))  cameraPos = Vector3Scale(planets[5].features.position,  1.0f /UA * actualRep.ua2Pixel);
    if (IsKeyPressed(KEY_SIX))   cameraPos = Vector3Scale(planets[6].features.position,  1.0f /UA * actualRep.ua2Pixel);
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