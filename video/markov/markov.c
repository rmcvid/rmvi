#include "config.h"
#include <stdio.h>

int spaceCount;
bool recording;
const char *version;
void processInput();
int bm_visual_initialisation(void) {
    // Initialize raylib
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int screenWidth = WIDTH;
    int screenHeight = HEIGHT;

    InitWindow(screenWidth, screenHeight, "Markov Video");
    SetTargetFPS(FPS); 

    return 0;  // Success

}
int bm_visual_uninitialisation(void) {
    // De-Initialization
    CloseWindow();  // Close window and OpenGL context
    return 0;  // Success
}

int bm_visual_main(void)
{   
    rmviGetCustomFont(FONT_PATH, 80);
    char videoPath[256];
    void *ffmpeg = NULL;
    if (recording) {
        snprintf(videoPath, sizeof(videoPath), "%s/%s.mp4", OUTPUT_DIR, version);
        ffmpeg = ffmpeg_start_rendering(GetScreenWidth(), GetScreenHeight(), FPS, videoPath);
    }
    int countFrame = 0;
    int frameInit = -1;
    Vector2 origin = {0.0f, 0.0f};
    Vector2 center = { (float)GetScreenWidth()/2.0f, (float)GetScreenHeight()/2.0f };
    Vector2 speed = {0, 0};
    //--------------------------------------------------------------------------------------
    RenderTexture2D screen = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    //rmviFrame frame = rmviGetFrameBGCentered(center.x*2/3, center.y, 12, 6, 10, "Square", mathFont);
    bool atom_defined = false;
    rmviAtom atom;
    State state = rmviGetStateClassic();
    state.color = (Color) {255,0,0,255};
    State state2 = rmviGetStateClassic();
    rmviFrame electron = rmviGetRoundFrameBGCentered(center.x*2/3, center.y, 12, 10, "e^-", &state);
    float ratioX = 15.0f;
    float ratioY = 7.5f;
    float lineThick = 5.0f;
    rmviGraph *graph = rmviGetGraph();

    rmviFrame root0 = rmviGetFrameFromText(center.x/8, center.y, "Interface", &state);
    rmviIntList intList0 = {0};
    rmviAddFrame2Graph(graph,&root0,&intList0);
    rmviFrame distri = rmviGetFrameFromText(0,0,"Distributeur", &state2);
    rmviIntList intListdistri = rmviGetIntList(root0.num);
    rmviAddFrame2Graph(graph,&distri,&intListdistri);
    rmviFrame root2 = rmviGetFrameFromText(0, 0,"Unité Centrale", &state2);
    rmviIntList intList2 = rmviGetIntList(distri.num);
    rmviAddFrame2Graph(graph,&root2,&intList2);
    rmviFrame GCP2 = root2;
    rmviAddFrame2Graph(graph,&GCP2,&intList2);
    rmviFrame GCP3 = root2;
    rmviAddFrame2Graph(graph,&GCP3,&intList2);
    rmviFrame GCP4 = root2;
    rmviAddFrame2Graph(graph,&GCP4,&intList2);
    rmviFrame cachel2 = rmviGetFrameFromText(0, 0,"Cache L2", &state);
    rmviIntList intListCache = rmviGetIntList(root2.num);
    rmviFrame vram = rmviGetFrameFromText(0, 0,"VRAM", &state);
    rmviIntList intListVram = rmviGetIntList(cachel2.num);
    rmviAddFrame2Graph(graph,&vram,&intListVram);
    rmviIntListAdd(&intListCache,GCP2.num);
    rmviIntListAdd(&intListCache,GCP3.num);
    rmviIntListAdd(&intListCache,GCP4.num);
    rmviAddFrame2Graph(graph,&cachel2,&intListCache);

    rmviFrame parametre, bleu, vert, rouge;
    State paraState = rmviGetStateClassic();
    State rougeState = rmviGetStateClassic();
    State bleuState = rmviGetStateClassic();
    State vertState = rmviGetStateClassic();
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Généralement // r = %d, g = %d, b = %d", paraState.color.r, paraState.color.g, paraState.color.b);
    parametre = rmviGetFrameFromText(center.x, center.y/2, buffer, &paraState);
    snprintf(buffer, sizeof(buffer), "rouge  = %d", rougeState.color.r);
    rouge = rmviGetFrameFromText(center.x/2, 3*center.y/2, buffer, &rougeState);
    snprintf(buffer, sizeof(buffer), "bleu  = %d", bleuState.color.b);
    bleu = rmviGetFrameFromText(center.x, 3*center.y/2, buffer, &bleuState);
    snprintf(buffer, sizeof(buffer), "vert  = %d", vertState.color.g);
    vert = rmviGetFrameFromText(3*center.x/2, 3*center.y/2, buffer, &vertState);

    spaceCount = 0;
    InitAudioDevice();              // Initialise audio
    RecordDevice rec = InitAudioRecorder("output.wav");
    while (!WindowShouldClose())
    {
        BeginTextureMode(screen);
            ClearBackground(BG);
            processInput();
            if(spaceCount == 0){
                rmviPositioningGraph(graph,50, SPACETREERATIOY);
                rmviDrawGraph(graph,ARROWDIRECT);
                //rmviDrawFrame(root0);
                /*rmviPositioningTree(tree,SPACETREERATIOX, SPACETREERATIOY);
                rmviDrawTree(tree, ARROWSQUARE);*/
            }
            if(spaceCount == 1){
                rougeState.color.r = (rougeState.color.r > 0) ?  rougeState.color.r -1 :  255;
                bleuState.color.b =  (bleuState.color.b > 0) ?  bleuState.color.b -1 :  255;
                vertState.color.g = (vertState.color.g > 0) ?  vertState.color.g -1 :  255;
                snprintf(buffer, sizeof(buffer), "Généralement // r = %d, g = %d, b = %d", paraState.color.r, paraState.color.g, paraState.color.b);
                rmviRewriteFrame(&parametre,buffer);
                rmviDrawFrame(&parametre);
                snprintf(buffer, sizeof(buffer), "rouge  = %d", rougeState.color.r);
                rmviRewriteFrame(&rouge,buffer);
                rmviDrawFrame(&rouge);
                snprintf(buffer, sizeof(buffer), "bleu  = %d", bleuState.color.b);
                rmviRewriteFrame(&bleu,buffer);
                rmviDrawFrame(&bleu);
                snprintf(buffer, sizeof(buffer), "vert  = %d", vertState.color.g);
                rmviRewriteFrame(&vert,buffer);
                rmviDrawFrame(&vert);
            }
            if(spaceCount == 2){
                if(!atom_defined){
                    rmviAtom he_3 = rmviGetAtom(NULL, "He_3", -1.00f, NULL);
                    rmviFrame h_3_frame = rmviGetRoundFrameBGCentered(center.x*2/3, center.y, 12, 10, "H_3", &state);
                    atom = rmviGetAtom(&h_3_frame, "H_3", 12.32f, &he_3);
                    atom_defined = true;
                }
                if(rmviDesintegration(&atom, 1.00f/FPS)) {
                    rmviAtomDecay(&atom);
                }
                rmviDrawAtom(atom);
            }
            if(spaceCount == 3){
                rmviDrawFrame(&electron);
            }
            DrawFPS(10, 10);
        EndTextureMode();
        BeginDrawing();
            ClearBackground(BG);
            DrawTexturePro(screen.texture, (Rectangle){ 0, 0, (float)screen.texture.width, -(float)screen.texture.height }, (Rectangle){ 0, 0, (float)screen.texture.width, (float)screen.texture.height}, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
        //----------------------------------------------------------------------------------
        
        if (recording) {
            Image image = LoadImageFromTexture(screen.texture);
            ffmpeg_send_frame_flipped(ffmpeg, image.data, GetScreenWidth(), GetScreenHeight());
            UnloadImage(image);
        }
        countFrame++;
    }
    UnloadFont(mathFont);
    UnloadRenderTexture(screen);
    CloseAudioDevice();
    if (recording) ffmpeg_end_rendering(ffmpeg);
    return 0;
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
    if(IsKeyPressed(KEY_SPACE)){
        spaceCount ++;
        if(spaceCount > 4) spaceCount = 0;
    }
}
