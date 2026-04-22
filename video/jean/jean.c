#include "config.h"
#include <stdio.h>

Image image;    // pour ffmpeg
bool recording;
const char *version;
Lecture lecture;
bool ecrit = false;
void processInput();
void updateMouse();
void updateZoom(Vector2 wheel);
void rmviDraw();
void addImageToffmpeg(void *ffmpeg);
void drawText(Vector2 pos, State *state);


int bm_visual_main(void){   
    float startTime = GetTime();
    lecture = rmviGetLecture(HERE_PATH "/presentation.txt");
    Vector2 paragraphPos = CENTER;
    rmviCartesian cartesian = rmviGetCartesian((Vector2) {CENTER.x, CENTER.y}, (Vector2){CENTER.x/2, CENTER.y/2}, (Vector2){5, 5});
    rmviSetCartesianGridStepPx(&cartesian, (Vector2){50.0f, 50.0f});
    rmviSetCartesianAxesLabel(&cartesian, "x", "y");
    rmviSetCartesianTitle(&cartesian, "Repere");
    float xs[] = {-2, -1, 0, 1, 2};
    float ys[] = { 1,  0, 1, 4, 9};
    State state = rmviGetStateClassic();
    state.widthMax = CENTER.x*0.8;
    while (!WindowShouldClose()){
        UpdateCursorToggle();
        processInput();
        BeginTextureMode(screen);
            ClearBackground(BG);
            //rmviDrawList2(&cartesian, xs, ys, 5, true, RED);
            rmviPolar polar = rmviGetPolar(CENTER, 360.0f, 1.0f);
            rmviSetPolarGridStepPx(&polar, 80.0f);
            rmviSetPolarAxesLabel(&polar, "r", "/theta");
            rmviSetPolarTitle(&polar, "Polar Plot");
            rmviDrawPolarFull(&polar, 12.0f, 4.0f, WHITE, true, true);
            //rmviDrawListPolar(&polar, listR, listTheta, count, true, RED);
            drawText((Vector2){3.0f/2.0f*CENTER.x, CENTER.y}, &state);
        EndTextureMode();
        BeginTextureMode(screen);
            DrawFPS(10, 10);
        EndTextureMode();
        rmviDraw();
        if (recording) addImageToffmpeg(ffmpeg);
    }
    return 0;
}

int bm_visual_initialisation(void) {
    // Initialize raylib and audio
    char audioPath[256];
    char videoPath[256];
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int screenWidth = WIDTH;
    int screenHeight = HEIGHT;
    InitWindow(screenWidth, screenHeight, "test example");
    SetTargetFPS(FPS); 
    rlEnableDepthTest();
    screen = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    rmviGetCustomFont(FONT_PATH, 80);
    if (recording) {
        snprintf(audioPath, sizeof(audioPath), "%s/%s.wav", OUTPUT_DIR, version);
        snprintf(videoPath, sizeof(videoPath), "%s/%s.mp4", OUTPUT_DIR, version);
        ffmpeg = ffmpeg_start_rendering(GetScreenWidth(), GetScreenHeight(), FPS, videoPath);
        InitAudioDevice();
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
    return 0;
}

void processInput(){
    if(IsKeyPressed(KEY_RIGHT)) lecture.currentParagraph++;
    if(IsKeyPressed(KEY_LEFT)) lecture.currentParagraph--;
    if(IsKeyPressed(KEY_E)) ecrit = !ecrit;
}
