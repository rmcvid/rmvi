#include "config.h"
#include <stdio.h>

Image image;    // pour ffmpeg
bool recording;
const char *version;
Lecture lecture;
bool ecrit = false;
FILE *buildingFile = NULL;
int currentFrame = 0;
int micStartFrame = -1;
double sessionStartTime = 0.0;
double micStartTime = -1.0;
char buildingDocumentPath[512] = {0};
int spaceCount = 0;
void processInput();
void updateMouse();
void updateZoom(Vector2 wheel);
void rmviDraw();
void addImageToffmpeg(void *ffmpeg);
void drawText(Vector2 pos, State *state);
void openBuildingFile(const char *path, const char *audioPath, const char *videoPath);
void closeBuildingFile(void);
void logBuildEvent(const char *eventName, int frame, double timeSeconds, double durationSeconds);
void toggleMicrophoneTrace(void);
#define DRAWCOLORJ BLACK

int bm_visual_main(void){   
    sessionStartTime = GetTime();
    currentFrame = 0;
    lecture = rmviGetLecture(HERE_PATH "/presentation.txt");
    //Vector2 paragraphPos = CENTER;

    // -------- setup Text ---------
    State state = rmviGetStateClassic();
    state.widthMax = CENTER.x*0.8;
    state.color = BLACK;
    Vector2 textPos = (Vector2){3.0f/2.0f*CENTER.x, 1.0f/2.0f*CENTER.y};

    // -------- setup Grid ---------
    Vector2 gridCenter = (Vector2){CENTER.x*3.0f/5.0f, CENTER.y};
    Vector2 CenterLeft = (Vector2){CENTER.x*3.0f/5.0f, CENTER.y};
    Vector2 halfSizePx;
    float rightLimit = 0.6f * GetScreenWidth();
    Color gridColor = (Color) { 0, 0, 0, 125 };
   
    
    // -------- setup Polar ---------
    rmviPolar polar = rmviGetPolar(CENTER, 360.0f, 1.0f);
    polar.alphaGrid = 0.0f;
    rmviSetPolarGridStepPx(&polar, 80.0f);
    //rmviSetPolarAxesLabel(&polar, "r", "/theta");
    //rmviSetPolarTitle(&polar, "Polar Plot");

    // -------- setup Particle ---------
    float sizeParticle = 30.0f;
    Vector2 dir= {1.0f,0.0f};
    float speedIntensity = 2.0f;
    rmviDynamic2D particle = {Vector22Vector2d(gridCenter), 1.0, Vector22Vector2d(Vector2Scale(dir, speedIntensity)), (Vector2d){0.0, 0.0}};
    float arrowLengthFactor = 100.0f;
    float arrowSize = 20.0f;
    Vector2 centerRotation = {CENTER.x, CENTER.y};
    float distance2Center = 200;
    float thickLine = 5.0f;
    float radiusCenter = 5.0f;
    float theta=0.0f;
    double CONSTANT = 360.0f;

    // -------- setup Cartesian ---------
    rmviCartesian cartesian = rmviGetCartesian(centerRotation, (Vector2){CENTER.x, CENTER.y}, (Vector2){5, 5});
    rmviSetCartesianGridStepPx(&cartesian, (Vector2){100, 100});
    //rmviSetCartesianAxesLabel(&cartesian, "x", "y");
    //rmviSetCartesianTitle(&cartesian, "Repere");

    char coordinatesCartesian[64];
    char coordinatesPolar[64];
    Vector2 posText = (Vector2){CENTER.x*3/2, CENTER.y/2};
    Vector2 posTextPolar = (Vector2){CENTER.x*3/2, CENTER.y/2 + 50};
    while (!WindowShouldClose()){
        UpdateCursorToggle();
        processInput();
        BeginTextureMode(screen);
            ClearBackground(BG);
            //rmviDrawPolarFull(&polar, 12.0f, 4.0f, WHITE, true, true);
            //rmviDrawCartesianFull(&cartesian, 12.0f, 4.0f, WHITE, true, true);
            if(spaceCount == 0){
                //DrawCircleLines(centerRotation.x, centerRotation.y, distance2Center, DRAWCOLORJ);

                theta = -GetTime();

                particle.position = (Vector2d){centerRotation.x+ distance2Center*cosf(theta), centerRotation.y + distance2Center*sinf(theta)};
                DrawCircleV(centerRotation, distance2Center+thickLine/2.0f, DRAWCOLORJ);
                DrawCircleV(centerRotation, distance2Center-thickLine/2.0f, BG) ;

                DrawCircleSector(centerRotation, distance2Center/3, 0, fmod(theta, 2*PI) * 180 / PI, 100, GREEN);
                DrawCircleV(centerRotation, radiusCenter, DRAWCOLORJ);
                DrawCircleV(Vector2d2Vector2(particle.position), sizeParticle, DRAWCOLORJ);
                DrawLineEx(( Vector2) {particle.position.x, centerRotation.y}, Vector2d2Vector2(particle.position), thickLine, DRAWCOLORJ);
                DrawLineEx(( Vector2) {centerRotation.x, particle.position.y}, Vector2d2Vector2(particle.position), thickLine, DRAWCOLORJ);
                DrawLineEx(centerRotation, Vector2d2Vector2(particle.position), thickLine, DRAWCOLORJ);
                rmviDrawCartesianFull(&cartesian, 12.0f, 4.0f, DRAWCOLORJ, false, true);
                snprintf(coordinatesCartesian, sizeof(coordinatesCartesian), "x =/ %.2f, y =/ %.2f", (particle.position.x - centerRotation.x)/cartesian.gridStepPx.x, - (particle.position.y - centerRotation.y)/cartesian.gridStepPx.y);
                snprintf(coordinatesPolar, sizeof(coordinatesPolar), "r =/ %.2f, /theta =/ %.2f", distance2Center/cartesian.gridStepPx.x, - fmod(theta, 2*PI) * 180 / PI);
                rmviWriteLatexLeftCentered(coordinatesCartesian, &posText, &state);
                rmviWriteLatexLeftCentered(coordinatesPolar, &posTextPolar, &state);
                polar.alphaGrid = 0.0f;
                rmviDrawPolarFull(&polar, 12.0f, 4.0f, DRAWCOLORJ, false, true);
            }

            if(spaceCount == 1){
                polar.alphaGrid = min(0.5f, polar.alphaGrid + 0.002f);
                cartesian.alphaGrid = max(0.0f, cartesian.alphaGrid - 0.002f);
                theta = -GetTime();
                particle.position = (Vector2d){centerRotation.x+ distance2Center*cosf(theta), centerRotation.y + distance2Center*sinf(theta)};
                DrawCircleV(centerRotation, distance2Center+thickLine/2.0f, DRAWCOLORJ);
                DrawCircleV(centerRotation, distance2Center-thickLine/2.0f, BG) ;

                DrawCircleSector(centerRotation, distance2Center/3, 0, fmod(theta, 2*PI) * 180 / PI, 100, GREEN);
                DrawCircleV(centerRotation, radiusCenter, DRAWCOLORJ);
                DrawCircleV(Vector2d2Vector2(particle.position), sizeParticle, DRAWCOLORJ);
                DrawLineEx(( Vector2) {particle.position.x, centerRotation.y}, Vector2d2Vector2(particle.position), thickLine, DRAWCOLORJ);
                DrawLineEx(( Vector2) {centerRotation.x, particle.position.y}, Vector2d2Vector2(particle.position), thickLine, DRAWCOLORJ);
                DrawLineEx(centerRotation, Vector2d2Vector2(particle.position), thickLine, DRAWCOLORJ);
                rmviDrawCartesianFull(&cartesian, 12.0f, 4.0f, DRAWCOLORJ, false, true);
                snprintf(coordinatesCartesian, sizeof(coordinatesCartesian), "x =/ %.2f, y =/ %.2f", (particle.position.x - centerRotation.x)/cartesian.gridStepPx.x, - (particle.position.y - centerRotation.y)/cartesian.gridStepPx.y);
                snprintf(coordinatesPolar, sizeof(coordinatesPolar), "r =/ %.2f, /theta =/ %.2f", distance2Center/cartesian.gridStepPx.x, - fmod(theta, 2*PI) * 180 / PI);
                rmviWriteLatexLeftCentered(coordinatesCartesian, &posText, &state);
                rmviWriteLatexLeftCentered(coordinatesPolar, &posTextPolar, &state);
                rmviDrawPolarFull(&polar, 12.0f, 4.0f, DRAWCOLORJ, true, true);
            }

            if( spaceCount == 2 ){
                dir = Vector2Normalize(Vector2Subtract(GetMousePosition(),Vector2d2Vector2(particle.position)));
                particle.velocity = Vector22Vector2d(Vector2Scale(dir, speedIntensity));
            }
            if( spaceCount == 3 ){
                rmviDrawPolarFull(&polar, 12.0f, 4.0f, DRAWCOLORJ, true, true);
                gridCenter.x -= particle.velocity.x;
                gridCenter.y -= particle.velocity.y;
                halfSizePx.x = rightLimit - gridCenter.x;
                halfSizePx.y = gridCenter.y;
                rmviDrawGrids(gridCenter, halfSizePx, cartesian.gridStepPx, gridColor);
                DrawCircle(particle.position.x, particle.position.y, sizeParticle, DRAWCOLORJ);
                rmviDrawCartesianFull(&cartesian, 12.0f, 4.0f, DRAWCOLORJ, false, false);
                rmviDrawArrow2(
                (Vector2){ particle.position.x - particle.velocity.x, particle.position.y - particle.velocity.y },
                (Vector2){ particle.position.x + arrowLengthFactor*particle.velocity.x, particle.position.y + arrowLengthFactor*particle.velocity.y },
                arrowSize, 4.0f, DRAWCOLORJ);
            }
            drawText(textPos, &state);
        EndTextureMode();
        BeginTextureMode(screen);
            DrawFPS(10, 10);
        EndTextureMode();
        rmviDraw();
        if (recording) addImageToffmpeg(ffmpeg);
        currentFrame++;
    }
    return 0;
}

int bm_visual_initialisation(void) {
    // Initialize raylib and audio
    char audioPath[256];
    char videoPath[256];
    char buildingPath[256];
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int screenWidth = WIDTH;
    int screenHeight = HEIGHT;
    InitWindow(screenWidth, screenHeight, "test example");
    SetTargetFPS(FPS); 
    rlEnableDepthTest();
    screen = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    rmviGetCustomFont(FONT_PATH, 80);
    if(recording){
        snprintf(audioPath, sizeof(audioPath), "%s%s.wav", OUTPUT_DIR, version);
        snprintf(videoPath, sizeof(videoPath), "%s%s.mp4", OUTPUT_DIR, version);
        snprintf(buildingPath, sizeof(buildingPath), "%s%s.txt", OUTPUT_DIR, version);
        snprintf(buildingDocumentPath, sizeof(buildingDocumentPath), "%s", buildingPath);
        ffmpeg = ffmpeg_start_rendering(GetScreenWidth(), GetScreenHeight(), FPS, videoPath);
        openBuildingFile(buildingPath, audioPath, videoPath);
        InitAudioDevice();
        printf("Audio will be recorded to: %s\n", audioPath);
        rec = InitAudioRecorder(audioPath);       // faire un chemin avec le cmake
        
    }
    return 0;  // Success
}
int bm_visual_uninitialisation(void) {
    // De-Initialization
    if (rec.isRecording){
        toggleMicrophoneTrace();
    }
    closeBuildingFile();
    if (rec.ctx){
        CloseAudioRecorder(&rec);
    }
    if (IsAudioDeviceReady()){
        CloseAudioDevice();
    }
    if (recording) ffmpeg_end_rendering(ffmpeg);
    CloseWindow();  // Close window and OpenGL context
    UnloadFont(mathFont);
    UnloadRenderTexture(screen);
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

void openBuildingFile(const char *path, const char *audioPath, const char *videoPath){
    if (!recording || !path) return;
    buildingFile = fopen(path, "w");
    if (!buildingFile){
        fprintf(stderr, "Could not open building file: %s\n", path);
        return;
    }
    fprintf(buildingFile, "# build trace for %s\n", version);
    if (audioPath) fprintf(buildingFile, "audio_path %s\n", audioPath);
    if (videoPath) fprintf(buildingFile, "video_path %s\n", videoPath);
    fprintf(buildingFile, "# frame_index_zero_based time_seconds\n");
    fprintf(buildingFile, "fps %d\n", FPS);
    fflush(buildingFile);
}

void closeBuildingFile(void){
    if (!buildingFile) return;
    fclose(buildingFile);
    buildingFile = NULL;
}

void logBuildEvent(const char *eventName, int frame, double timeSeconds, double durationSeconds){
    if (!buildingFile || !eventName) return;
    if (durationSeconds >= 0.0){
        fprintf(buildingFile, "%s frame=%d time=%.6f duration=%.6f\n",
            eventName, frame, timeSeconds, durationSeconds);
    }
    else{
        fprintf(buildingFile, "%s frame=%d time=%.6f\n",
            eventName, frame, timeSeconds);
    }
    fflush(buildingFile);
}

void toggleMicrophoneTrace(void){
    double now = GetTime() - sessionStartTime;
    if (rec.isRecording){
        StopAudioRecorder(&rec);
        if (buildingFile){
            fprintf(buildingFile,
                "mic_stop frame=%d time=%.6f duration=%.6f start_frame=%d start_time=%.6f\n",
                currentFrame, now, now - micStartTime, micStartFrame, micStartTime);
            fflush(buildingFile);
        }
        micStartFrame = -1;
        micStartTime = -1.0;
    }
    else{
        StartAudioRecorder(&rec);
        micStartFrame = currentFrame;
        micStartTime = now;
        logBuildEvent("mic_start", micStartFrame, micStartTime, -1.0);
    }
}

void drawText(Vector2 pos, State *state){
    static bool initialized = false;
    static Token tokens[256];
    static RenderBox boxes[256];
    static rmviFloatList listWidth;
    static float heighTotal = 0.0f;
    static AnimText *animText = NULL;
    static int boxCount = 0;
    if (!initialized) {
        animText = initAnimText();
        initialized = true;
    }
    if(readScenario(&lecture)){
        animText->animTime = 0;
        int tokenCount = rmviTokenizeLatex(lecture.content, tokens, 256);
        boxCount = rmviBuildRenderBoxes(tokens, tokenCount, boxes, state);
    }
    if(ecrit){
        rmviCalcWidthLine(boxes, boxCount, &listWidth);
        heighTotal = rmviCalcHeightTotal(boxes, boxCount);
        //rmviDrawRenderBoxes(boxes, boxCount, pos);
        rmviDrawRenderBoxesCentered(&listWidth, heighTotal, boxes, boxCount, pos);
        //rmviDrawRenderBoxesCenteredAnimed(boxes, boxCount, pos, animText);
    }
}

int main(int argc, char *argv[]){
    /*
    On utilise un format de fonction qui prend 0 1 pour l'enregistrement, si 1 alors ensuite on donne le noms
    */
    recording = (argc >= 2) ? (strcmp(argv[1], "1") == 0) : false;
    version = (argc >= 3) ? argv[2] : "output"; 
    bm_visual_initialisation();
    printf("Audio ready: %d\n", IsAudioDeviceReady());
    bm_visual_main();
    bm_visual_uninitialisation();
    if (recording && buildingDocumentPath[0] != '\0') {
        videoAssembler(buildingDocumentPath);
    }
    return 0;
}

void processInput(){
    if(IsKeyPressed(KEY_RIGHT)) lecture.currentParagraph++;
    if(IsKeyPressed(KEY_LEFT)) lecture.currentParagraph--;
    if(IsKeyPressed(KEY_E)) ecrit = !ecrit;
    if(IsKeyPressed(KEY_M) && recording) toggleMicrophoneTrace();
    if(IsKeyPressed(KEY_SPACE)) spaceCount++;
}

