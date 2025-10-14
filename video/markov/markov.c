#include "config.h"


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
    void *ffmpeg = NULL; 
    if (RECORDING) {
        ffmpeg = ffmpeg_start_rendering(GetScreenWidth(), GetScreenHeight(), FPS);
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
    rmviFrame frame = rmviGetFrameBGCentered(center.x*2/3, center.y, 12, 6, 10, "square", mathFont);
    rmviFrame electron = rmviGetRoundFrameBGCentered(center.x*2/3, center.y, 12, 10, "e^-", mathFont);
    float ratioX = 15.0f;
    float ratioY = 7.5f;
    float lineThick = 5.0f;
    Vector4 count = {0, 0, 0, 0};
    float sum = 0.0f;
    rmviCarthesian carthesian = rmviGetCarthesian(center, (Vector2){SIZEXPLOT, SIZEXPLOT/RATIOYPLOT}, (Vector2){SIZEXPLOT/UNITRATIO, SIZEXPLOT/(UNITRATIO)});
    rmviTree *tree = rmviCreateTree();
    rmviFrame root0 = rmviGetFrameBGCentered(center.x*6.0f/5.0f, center.y, ratioX, ratioY, lineThick, "Initial", mathFont);
    rmviFrame root1 = rmviGetFrameBGCentered(0,0, ratioX, ratioY, lineThick, "Haut//", mathFont);
    rmviFrame root2 = rmviGetFrameBGCentered(0, 0, ratioX, ratioY, lineThick, "Droite//", mathFont);
    rmviFrame root3 = rmviGetFrameBGCentered(0, 0, ratioX, ratioY, lineThick, "Bas//", mathFont);
    rmviFrame root4 = rmviGetFrameBGCentered(0, 0, ratioX, ratioY, lineThick, "Gauche//", mathFont);
    rmviAddFrame2Tree(tree, &root0, -1);
    rmviAddFrame2Tree(tree, &root1, 0);
    rmviAddFrame2Tree(tree, &root2, 0);
    rmviAddFrame2Tree(tree, &root3, 0);
    rmviAddFrame2Tree(tree, &root4, 0);
    const char *textArrowsTree[] = {"P(e_i -> e_h) = 0.5// test // décale trop là", "Droite", "Bas", "Gauche","test",NULL};
    rmviPositioningTree(tree, SPACETREERATIOX, SPACETREERATIOY);
    int space_count = 0;
    InitAudioDevice();              // Initialise audio
    RecordDevice rec = InitAudioRecorder("output.wav");


    while (!WindowShouldClose())
    {
        BeginTextureMode(screen);
            ClearBackground(BG);
            if( IsKeyPressed(KEY_R)) {
                speed = rmviRandomSpeed(&count);
            }
            if(IsKeyPressed(KEY_M)){
                rec.isRecording ? StopAudioRecorder(&rec) : StartAudioRecorder(&rec);
            }
            if (IsKeyPressed(KEY_A)) {
                frameInit = countFrame;
            }
            if(IsKeyPressed(KEY_E)) {
               frameInit = -1; 
            }
            if (IsKeyPressed(KEY_B)) {
                frame = rmviGetFrameBGCentered(center.x*2/3, center.y, 12, 6, 10, "square", mathFont);
                speed = (Vector2){0, 0};
            }
            if (IsKeyPressed(KEY_Z)) {
                count = (Vector4){0, 0, 0, 0};
            }
            if(IsKeyPressed(KEY_SPACE)){
                space_count ++;
                if(space_count > 3) space_count = 0;
            }
            if(space_count == 0){
                sum = (float)(count.x + count.y + count.z + count.w);
                rmviUpdateFrame(&frame, frame.outerRect.x + speed.x, frame.outerRect.y + speed.y, frame.outerRect.width, frame.outerRect.height, frame.lineThick, frame.innerColor, frame.outerColor, frame.text, mathFont);
                rmviUpdateFrame(&root1, root1.outerRect.x , root1.outerRect.y, root1.outerRect.width, root1.outerRect.height, root1.lineThick, root1.innerColor, root1.outerColor, TextFormat( "Haut // %.3f", SAFE_RATIO(count.x, sum)) , mathFont);
                rmviUpdateFrame(&root2, root2.outerRect.x , root2.outerRect.y, root2.outerRect.width, root2.outerRect.height, root2.lineThick, root2.innerColor, root2.outerColor, TextFormat(" Droite // %.3f", SAFE_RATIO(count.y, sum)) , mathFont);
                rmviUpdateFrame(&root3, root3.outerRect.x , root3.outerRect.y, root3.outerRect.width, root3.outerRect.height, root3.lineThick, root3.innerColor, root3.outerColor, TextFormat(" Bas // %.3f", SAFE_RATIO(count.z, sum)) , mathFont);
                rmviUpdateFrame(&root4, root4.outerRect.x , root4.outerRect.y, root4.outerRect.width, root4.outerRect.height, root4.lineThick, root4.innerColor, root4.outerColor, TextFormat(" Gauche // %.3f", SAFE_RATIO(count.w, sum)) , mathFont);
                //rmviDrawArrowRect2((Vector2){root0.outerRect.x + root0.innerRect.width, root0.outerRect.y + root0.innerRect.height/2}, (Vector2){root1.outerRect.x, root1.outerRect.y + root1.outerRect.height/2}, 10, 1.0f, 0.5f, WHITE);
                rmviPositioningTree(tree,SPACETREERATIOX, SPACETREERATIOY);
                rmviDrawFrame(frame, RATIODEFAULT);
                //rmviDrawTree(tree, 2);
                rmviDrawTreeSquareWrite(tree, SPLIT_LEFT_ARROWS, textArrowsTree , DRAWCOLOR, RATIOWRITEARROWS, root0.innerRect.height/RATIODEFAULT);
                rmviMyDrawText(TextFormat("N =  %d", (int)sum), (Vector2) {root0.outerRect.x + root0.innerRect.width*(2+SPACETREERATIOX), root0.outerRect.y + root0.outerRect.height/2-SIZETEXT/2}, SIZETEXT, WHITE);
                if(frameInit != -1) {
                    rmviWriteAnimText(TEST_DOUBLE_BEGIN, (Vector2) {center.x/2, center.y/2}, SIZETEXT, WHITE, frameInit, countFrame);
                }
            }
            if(space_count == 1){
                if(!atom_defined){
                    rmviAtom he_3 = rmviGetAtom(NULL, "He_3", -1.00f, NULL);
                    rmviFrame h_3_frame = rmviGetRoundFrameBGCentered(center.x*2/3, center.y, 12, 10, "H_3", mathFont);
                    atom = rmviGetAtom(&h_3_frame, "H_3", 12.32f, &he_3);
                    atom_defined = true;
                }
                if(rmviDesintegration(&atom, 1.00f/FPS)) {
                    rmviAtomDecay(&atom);
                }
                rmviDrawAtom(atom);
            }
            if(space_count == 2){
                rmviDrawFrame(electron,RATIODEFAULT/2);
            }
            DrawFPS(10, 10);
        EndTextureMode();
        BeginDrawing();
            ClearBackground(BG);
            DrawTexturePro(screen.texture, (Rectangle){ 0, 0, (float)screen.texture.width, -(float)screen.texture.height }, (Rectangle){ 0, 0, (float)screen.texture.width, (float)screen.texture.height}, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
        //----------------------------------------------------------------------------------
        
        if (RECORDING) {
            Image image = LoadImageFromTexture(screen.texture);
            ffmpeg_send_frame_flipped(ffmpeg, image.data, GetScreenWidth(), GetScreenHeight());
            UnloadImage(image);
        }
        countFrame++;
    }
    UnloadFont(mathFont);
    UnloadRenderTexture(screen);
    CloseAudioDevice();
    if (RECORDING) ffmpeg_end_rendering(ffmpeg);
    return 0;
}

int main(void){
    bm_visual_initialisation();
    bm_visual_main();
    bm_visual_uninitialisation();
    return 0;
}