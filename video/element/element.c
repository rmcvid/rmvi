#include "config.h"
#define TSTHICK 2.0f
#define RATIOTICKTHICK 2.0f
#define RATIOTICKSIZE 5.0f
#define TEXTSIZE 30.0f
#define TEXTSPACE 5.0f
#define TSSCALE 300.0f
#define TSSTEP 100
#define TSSPEED 1 // nombre d'année par frame
#define TEMPTRESHOLD 704.0f
#define TEMPERATURE 3000.0f
#define TEMPDECREASE 60.0f

#include <math.h>
#include <sys/types.h>
#include <time.h>

Image image;

typedef struct {
    int actualyear;
    float scale;
    int step;
} Timescale;

typedef struct{
    Vector2 position;
    Vector2 velocity;
    float temperature; // -1 = white
} particle;

typedef struct {
    particle *data;   // tableau dynamique
    int count;        // nombre de particules actives
    int capacity;     // taille max du tableau
} ParticleSystem;


void DrawRasenganCore(Vector2 center, float radius, float time){
    int layers = 80;
    for (int i = layers - 1; i >= 0; i--) {
        float t = (float)i / (float)(layers - 1);
        // Couleur du gradient
        unsigned char r = Lerp(255, 0, t);
        unsigned char g = Lerp(255, 160, t);
        unsigned char b = Lerp(255, 255, t);
        Color col = (Color){r, g, b, 255};
        // Rayon du layer
        float rLayer = radius * t;
        float swirl = sinf(time * 2.0f + t * 8.0f) * 4.0f;
        // on "décale" le cercle avec une ellipse déformée
        DrawEllipse(center.x + swirl, center.y, rLayer, rLayer, col);
    }
}

void initBriques(Texture2D text, Rectangle* briques){
    // Ligne 1
    float f11 = 2.1f / 3.0f;
    float f21 = 0.77f / 4.0f;
    float f22 = 2.0f / 4.0f;
    float f31 = 2.0f / 3.0f;
    briques[0] = (Rectangle){0.0f, 0.0f, f11* text.width, text.height / 3.0f};
    briques[1] = (Rectangle){2.1f * text.width / 3.0f, 0.0f, text.width*(1 - f11), text.height / 3.0f};
    // Ligne 2
    briques[2] = (Rectangle){0.0f, text.height / 3.0f, f21 * text.width, text.height / 3.0f};
    briques[3] = (Rectangle){f21 * text.width, text.height / 3.0f, text.width * (f22 - f21), text.height / 3.0f};
    briques[4] = (Rectangle){f22 * text.width, text.height / 3.0f, text.width* (1 - f22), text.height / 3.0f};

    // Ligne 3
    briques[5] = (Rectangle){0.0f, 2 * text.height / 3.0f, f31 * text.width, text.height / 3.0f};
    briques[6] = (Rectangle){f31 * text.width, 2 * text.height / 3.0f, text.width* (1 - f31), text.height / 3.0f};
}
void initBriquePositions(Vector2* positions, Vector2 initialpos, Rectangle* briques, float scale){
    // Ligne 1
    positions[0] = initialpos;
    positions[1] = (Vector2){initialpos.x + briques[0].width * scale, initialpos.y};
    // Ligne 2
    positions[2] = (Vector2){initialpos.x, initialpos.y + briques[0].height * scale};
    positions[3] = (Vector2){initialpos.x + briques[2].width * scale, initialpos.y + briques[0].height * scale};
    positions[4] = (Vector2){initialpos.x + (briques[2].width + briques[3].width) * scale, initialpos.y + briques[0].height * scale};
    // Ligne 3
    positions[5] = (Vector2){initialpos.x, initialpos.y + 2 * briques[0].height * scale};
    positions[6] = (Vector2){initialpos.x + briques[5].width * scale, initialpos.y + 2 * briques[0].height * scale};
}
void drawBrique(Rectangle brique, Texture2D text, Vector2 position, float scale, float t){
    Rectangle dest = {
        position.x,
        position.y,
        brique.width * scale,
        brique.height * scale
    };
    rmviRotateTexture(text,brique,dest,(Vector2){0, 0},t,WHITE);
}
void drawBriques(Rectangle* briques, Texture2D text, Vector2* positions, float scale, float countFrame){
    float eloignement = 200.0f;
    float temps = 6*FPS;
    float t = eloignement*powf(cosf((float)countFrame *2.0f*PI/temps),8.0f);
    // Directions 6 branches
    Vector2 dir[] = {
        {cos(4*PI/3), sin(4*PI/3)},
        {cos(5*PI/3), sin(5*PI/3)},
        {cos(3*PI/3), sin(3*PI/3)},
        {0,0},
        {1,0},
        {cos(2*PI/3), sin(2*PI/3)},
        {cos(1*PI/3), sin(1*PI/3)}
    };
    for (int i = 0; i < 7; i++){
        Vector2 shifted = Vector2Add(positions[i], Vector2Scale(dir[i], t));
        i == 3 ? drawBrique(briques[i], text, shifted, scale,t/eloignement*370.0f) : drawBrique(briques[i], text, shifted, scale,-t/eloignement*370.0f);
        
        
    }
}


ParticleSystem initParticleSystem(int initialCapacity) {
    ParticleSystem ps;
    ps.data = malloc(sizeof(particle) * initialCapacity);
    ps.count = 0;
    ps.capacity = initialCapacity;
    return ps;
}
void particleSystem_grow(ParticleSystem *ps) {
    ps->capacity *= 2;
    ps->data = realloc(ps->data, ps->capacity * sizeof(particle));
}

void rasenganSource(Vector2 center, ParticleSystem *ps, int num, float radius) {
    for (int i = 0; i < num; i++) {
        if (ps->count >= ps->capacity)
            particleSystem_grow(ps);
        // Distance max du Rasengan
        // Position aléatoire dans la boule
        Vector2 offset = {
            (rmviRand()-0.5)*2*radius,
            (rmviRand()-0.5)*2*radius
        };
        // Tangente perpendiculaire au rayon
        Vector2 tangent = (Vector2){ -offset.y, offset.x };
        tangent = Vector2Normalize(tangent);
        // Bruit aléatoire sur la vitesse
        float speed = 0.8f + rmviRandNormal(0, 0.2f);
        particle p = {
            .position = Vector2Add(center, offset),
            .velocity = Vector2Scale(tangent, speed),
            .temperature = -1.0f
        };
        ps->data[ps->count++] = p;
    }
}
void rasenganParticlesUpdate(ParticleSystem *ps, Vector2 center, float R) {
    for (int i = 0; i < ps->count; i++) {
        Vector2 offset = Vector2Subtract(ps->data[i].position, center);
        float dist = Vector2Length(offset);
        Vector2 tangent = Vector2Normalize((Vector2){ -offset.y, offset.x });
        Vector2 tangentialPush = Vector2Scale(tangent, 0.07f);
        Vector2 centerPull = Vector2Scale(offset, -0.005f);
        Vector2 noise = { rmviRandNormal(0, 0.1f), rmviRandNormal(0, 0.1f) };
        ps->data[i].velocity = Vector2Add(ps->data[i].velocity, tangentialPush);
        ps->data[i].velocity = Vector2Add(ps->data[i].velocity, centerPull);
        ps->data[i].velocity = Vector2Add(ps->data[i].velocity, noise);
        ps->data[i].position = Vector2Add(ps->data[i].position, ps->data[i].velocity);
        // --- 1. Si dehors -> respawn
        if (Vector2Length(Vector2Subtract(ps->data[i].position, center)) > R) {
            ps->data[i].position = Vector2Add(center,
                (Vector2){ rmviRandNormal(0, R*0.3f), rmviRandNormal(0, R*0.3f) }
            );
            continue;
        }
    }
}


void drawRasenganParticles(ParticleSystem *ps){
    for (int i = 0; i < ps->count; i++){
        DrawCircleV(ps->data[i].position, 1.0f, (Color) {255, 255, 255, 235});
    }
}

void fireSource(Vector2 position, float temperature, ParticleSystem *ps, int num) {
    for (int i = 0; i < num; i++) {
        if (ps->count >= ps->capacity)
            particleSystem_grow(ps);
        particle p = {
            .position = Vector2Add(position, (Vector2){ rmviRandNormal(0,7), rmviRandNormal(0,3) }),
            .velocity = (Vector2){ rmviRandNormal(0,4), Clamp(rmviRandNormal(-0.25,1.5),-3,0.5) },
            .temperature = rmviRandNormal(temperature, 0.3 * temperature)
        };
        ps->data[ps->count++] = p;
    }
}

void fireParticlesUpdate(ParticleSystem *ps,Vector2 center) {
    for (int i = 0; i < ps->count; ) {
        ps->data[i].temperature -= rmviRandNormal(TEMPDECREASE,TEMPDECREASE/5);
        if (ps->data[i].temperature < TEMPTRESHOLD) {
            // enlever particule : remplace par la dernière
            ps->data[i] = ps->data[ps->count - 1];
            ps->count--;
            continue; // ne pas incrementer i, car on doit traiter le nouvel élément
        }
        // update normal
        ps->data[i].velocity.y -= 0.02f;
        ps->data[i].velocity.x += (center.x - ps->data[i].position.x) * 0.01f;
        ps->data[i].position.x += ps->data[i].velocity.x;
        ps->data[i].position.y += ps->data[i].velocity.y;
        i++;
    }
}


Color colorTemperatureToRGB(float kelvin, float alpha) {
    float temp = kelvin / 100;
    float red, green, blue;
    if( temp <= 66 ){ 
        red = 255; 
        green = temp;
        green = 99.4708025861 * logf(green) - 161.1195681661;
        if( temp <= 19){
            blue = 0;
        } else {
            blue = temp-10;
            blue = 138.5177312231 * logf(blue) - 305.0447927307;
        }
    } else {
        red = temp - 60;
        red = 329.698727446 * powf(red, -0.1332047592);
        green = temp - 60;
        green = 288.1221695283 * powf(green, -0.0755148492 );
        blue = 255;
    }
    return (Color){
        Clamp(red,   0, 255),
        Clamp(green, 0, 255),
        Clamp(blue,  0, 255),
        alpha};
}
void drawTemperatureParticles(ParticleSystem *ps){
    for (int i = 0; i < ps->count; i++){
        DrawCircleV(ps->data[i].position, 1.0f, colorTemperatureToRGB(ps->data[i].temperature,255));
        DrawLineEx(ps->data[i].position, Vector2Subtract(ps->data[i].position, Vector2Scale(ps->data[i].velocity, Clamp(rmviRandNormal(1,1),0,3))), 0.5f, colorTemperatureToRGB(ps->data[i].temperature,200));
    }
}





void DrawTickMarks(float posX, float posY, int year, Color color){
    char buffer[16];
    (year == -1) ? (void)strcpy(buffer, "") : (void)snprintf(buffer, sizeof(buffer), "%d", year);
    DrawLineEx((Vector2){posX, posY}, (Vector2){posX, posY - TSTHICK*RATIOTICKSIZE}, TSTHICK*RATIOTICKTHICK, color);
    Vector2 textSize = MeasureTextEx(mathFont, buffer, TEXTSIZE, TEXTSPACE);
    Vector2 posWrite = (Vector2){posX - textSize.x / 2, posY + TSTHICK*RATIOTICKSIZE};
    rmviWriteLatex(buffer, &posWrite, TEXTSIZE, TEXTSPACE, color, mathFont);
}

void drawTimescale(Timescale ts, float posY, Color color) {
    DrawLineEx((Vector2){0, posY}, (Vector2){GetScreenWidth(), posY}, TSTHICK, color);
    int num = GetScreenWidth()/ts.scale ;
    float rest = ts.actualyear%ts.step;
    for (int i = 1; i <= (num/2+1); i++) {
        int yearRight = ts.actualyear + i * ts.step - rest;
        int yearLeft = ts.actualyear - (i-1) * ts.step - rest;
        float posXRight = GetScreenWidth()/2 + (i - rest/(float)ts.step) * ts.scale;
        float posXLeft = GetScreenWidth()/2 - (i-1 + rest/(float)ts.step) * ts.scale;
        if (posXRight <= GetScreenWidth()) {
            yearRight-ts.actualyear > ts.step/10 ? DrawTickMarks(posXRight, posY, yearRight, color) : DrawTickMarks(posXRight, posY, -1, color);
        }
        if (posXLeft >= 0) {
            DrawTickMarks(posXLeft, posY, yearLeft, color);
        }
    }
    DrawTickMarks(GetScreenWidth()/2, posY, ts.actualyear, color);
}

void tsYearUpdateOne(Timescale *ts, int year){
    if (year > ts->actualyear){
        ts->actualyear += 1;
    }
}

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


int bm_visual_main(void)
{   
    Timescale ts = { 1764, TSSCALE, TSSTEP };
    float rasenganRadius = 60.0f;
    int space_count = 0;
    char *jsonData = rmviReadFile(IMAGE_PATH "pilone_grec.json");
    Anime2 *grec = rmviAnime2FromJSON(jsonData);
    Vector2 piloneGrecPos = {GetScreenWidth()/4.0f, GetScreenHeight()/2.0f};
    printf("Anime2 loaded with %d contours.\n", grec->ncontours);
    ParticleSystem fireps = initParticleSystem(1000);
    ParticleSystem rasenganps = initParticleSystem(200);
    Vector2 firePosition = {GetScreenWidth()/2.0f, GetScreenHeight()/2.0f};
    Vector2 rasenganPosition = {3*GetScreenWidth()/4.0f, GetScreenHeight()/2.0f};
    rasenganSource(rasenganPosition, &rasenganps, 800,rasenganRadius);
    Texture2D briquesTexture = LoadTexture(IMAGE_PATH "brique.jpg");
    Vector2 briquePositions[7];
    float scaleBrique = 0.15f;
    Vector2 briqueInitialPos = {GetScreenWidth()/2.0f - (briquesTexture.width/3.0f)*scaleBrique, GetScreenHeight()/4.0f - (briquesTexture.height/3.0f)*scaleBrique};
    
    Rectangle briques[7];
    initBriques(briquesTexture, briques);
    initBriquePositions(briquePositions, briqueInitialPos, briques, scaleBrique);
    int framecount=0;
    while (!WindowShouldClose())
    {   
        if (IsKeyPressed(KEY_SPACE)) space_count ++;
        if (space_count > 0) tsYearUpdateOne(&ts, 2024);
        if (IsKeyPressed(KEY_B)){
            space_count = 0;
            ts.actualyear = 1764;
        } 
        BeginTextureMode(screen);
            ClearBackground(BG);
            drawTimescale(ts, 3*GetScreenHeight()/4.0f, DRAWCOLOR);
            
            /*
            // ----------- BRIQUES -------------
            drawBriques(briques, briquesTexture, briquePositions, 0.15f, framecount);

            // ----------- FEU -------------
            fireSource(firePosition, TEMPERATURE, &fireps, 50);
            fireParticlesUpdate(&fireps,firePosition);
            drawTemperatureParticles(&fireps);

            // ----------- RASENGAN -------------
            DrawRasenganCore(rasenganPosition, rasenganRadius,0);
            rasenganParticlesUpdate(&rasenganps,rasenganPosition, 0.97*rasenganRadius);
            drawRasenganParticles(&rasenganps);

            // ----------- PILONE GREC -------------
            rmvidrawAnime2(grec, piloneGrecPos, 1.0f, WHITE, true);
            
            DrawFPS(10, 10);
            */
        EndTextureMode();
        rmviDraw();
        if (RECORDING) addImageToffmpeg(ffmpeg);
        framecount++;
    }
    rmviAnime2Free(grec);
    free(fireps.data);
    free(rasenganps.data);
    UnloadTexture(briquesTexture);
    free(jsonData);
    return 0;
}


int main(int argc, char *argv[]){
    version = (argc > 1) ? argv[1] : "0"; 
    bm_visual_initialisation();
    bm_visual_main();
    bm_visual_uninitialisation();
    return 0;
}
