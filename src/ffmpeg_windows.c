#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

typedef struct {
    HANDLE hProcess;
    // HANDLE hPipeRead;
    HANDLE hPipeWrite;
} FFMPEG;

typedef struct {
    int startFrame;
    int stopFrame;
    double startTime;
    double stopTime;
    double duration;
} VideoTraceSegment;

static LPSTR GetLastErrorAsString(void)
{
    // https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror
    DWORD errorMessageId = GetLastError();
    assert(errorMessageId != 0);
    LPSTR messageBuffer = NULL;
    DWORD size =
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // DWORD   dwFlags,
            NULL, // LPCVOID lpSource,
            errorMessageId, // DWORD   dwMessageId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // DWORD   dwLanguageId,
            (LPSTR) &messageBuffer, // LPTSTR  lpBuffer,
            0, // DWORD   nSize,
            NULL // va_list *Arguments
        );

    return messageBuffer;
}

static void TrimLine(char *line)
{
    size_t len = 0;
    if (!line) return;
    len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }
}

static void TrimLeading(char **text)
{
    if (!text || !*text) return;
    while (**text == ' ' || **text == '\t') {
        (*text)++;
    }
}

static void UnquoteInPlace(char *text)
{
    size_t len = 0;
    if (!text) return;
    len = strlen(text);
    if (len >= 2 && text[0] == '"' && text[len - 1] == '"') {
        memmove(text, text + 1, len - 2);
        text[len - 2] = '\0';
    }
}

static void CopyValueAfterKey(const char *line, const char *key, char *out, size_t outCap)
{
    const char *value = NULL;
    size_t keyLen = 0;
    if (!line || !key || !out || outCap == 0) return;
    keyLen = strlen(key);
    if (strncmp(line, key, keyLen) != 0) return;
    value = line + keyLen;
    while (*value == ' ' || *value == '\t') value++;
    snprintf(out, outCap, "%s", value);
    TrimLine(out);
    UnquoteInPlace(out);
}

static BOOL RunCommandLine(char *commandLine)
{
    STARTUPINFO siStartInfo;
    PROCESS_INFORMATION piProcInfo;
    DWORD result = 0;
    DWORD exitStatus = 0;

    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(siStartInfo);
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&piProcInfo, sizeof(piProcInfo));

    if (!CreateProcess(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
        fprintf(stderr, "ERROR: Could not create assembler process: %s\n", GetLastErrorAsString());
        return FALSE;
    }

    CloseHandle(piProcInfo.hThread);
    result = WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    if (result == WAIT_FAILED) {
        fprintf(stderr, "ERROR: Could not wait on assembler process: %s\n", GetLastErrorAsString());
        CloseHandle(piProcInfo.hProcess);
        return FALSE;
    }

    if (GetExitCodeProcess(piProcInfo.hProcess, &exitStatus) == 0) {
        fprintf(stderr, "ERROR: Could not get assembler exit code: %s\n", GetLastErrorAsString());
        CloseHandle(piProcInfo.hProcess);
        return FALSE;
    }

    CloseHandle(piProcInfo.hProcess);
    if (exitStatus != 0) {
        fprintf(stderr, "ERROR: assembler command exited with code %lu\n", exitStatus);
        return FALSE;
    }

    return TRUE;
}

static BOOL PushVideoTraceSegment(VideoTraceSegment **segments, int *count, int *capacity, VideoTraceSegment segment)
{
    VideoTraceSegment *newSegments = NULL;
    int newCapacity = 0;
    if (!segments || !count || !capacity) return FALSE;
    if (*count >= *capacity) {
        newCapacity = (*capacity == 0) ? 8 : (*capacity * 2);
        newSegments = (VideoTraceSegment *)realloc(*segments, (size_t)newCapacity * sizeof(VideoTraceSegment));
        if (!newSegments) return FALSE;
        *segments = newSegments;
        *capacity = newCapacity;
    }
    (*segments)[(*count)++] = segment;
    return TRUE;
}

static void BuildAssembledOutputPath(const char *videoPath, char *outputPath, size_t outputCap)
{
    const char *dot = NULL;
    size_t prefixLen = 0;
    if (!videoPath || !outputPath || outputCap == 0) return;
    dot = strrchr(videoPath, '.');
    if (dot && strcmp(dot, ".mp4") == 0) {
        prefixLen = (size_t)(dot - videoPath);
        snprintf(outputPath, outputCap, "%.*s_with_audio.mp4", (int)prefixLen, videoPath);
    }
    else {
        snprintf(outputPath, outputCap, "%s_with_audio.mp4", videoPath);
    }
}


// NEW: robust write helper (handles short writes on pipes)
static BOOL WriteAll(HANDLE h, const void* buf, DWORD bytesTotal) {
    const uint8_t* p = (const uint8_t*)buf;
    while (bytesTotal > 0) {
        DWORD wrote = 0;
        if (!WriteFile(h, p, bytesTotal, &wrote, NULL)) return FALSE;
        if (wrote == 0) return FALSE; // pipe closed
        p += wrote;
        bytesTotal -= wrote;
    }
    return TRUE;
}

FFMPEG *ffmpeg_start_rendering(size_t width, size_t height, size_t fps, const char *version)
{
    printf("Starting ffmpeg rendering with width: %zu, height: %zu, fps: %zu\n", width, height, fps);
    HANDLE pipe_read;
    HANDLE pipe_write;

    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    // Larger pipe buffer to reduce stalls.
    DWORD pipeSize = 4 * 1024 * 1024; // 4 MB
    if (!CreatePipe(&pipe_read, &pipe_write, &saAttr, pipeSize)) {
        fprintf(stderr, "ERROR: Could not create pipe: %s\n", GetLastErrorAsString());
        return NULL;
    }

    if (!SetHandleInformation(pipe_write, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "ERROR: Could not SetHandleInformation: %s\n", GetLastErrorAsString());
        return NULL;
    }

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput  = pipe_read;
    siStartInfo.dwFlags   |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    char cmd_buffer[2048];
    // NOTE: -vf vflip flips the frame; we now always write in top->bottom order.
    // If your data is BGRA, change rgba->bgra here and in ffmpeg_send_frame().
    snprintf(cmd_buffer, sizeof(cmd_buffer),
        "ffmpeg.exe -loglevel verbose -y "
        "-f rawvideo -pix_fmt rgba -s %dx%d -r %d -i - "
        "-vf vflip "
        "-c:v hevc_qsv -preset veryfast -global_quality 28 "
        "-pix_fmt yuv420p %s",
        (int)width, (int)height, (int)fps, version);

    BOOL bSuccess = CreateProcess(
        NULL, cmd_buffer, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);

    if (!bSuccess) {
        fprintf(stderr, "ERROR: Could not create child process: %s\n", GetLastErrorAsString());
        return NULL;
    }

    CloseHandle(pipe_read);
    CloseHandle(piProcInfo.hThread);

    FFMPEG* ffmpeg = (FFMPEG*)malloc(sizeof(FFMPEG));
    assert(ffmpeg != NULL && "Buy MORE RAM lol!!");
    ffmpeg->hProcess   = piProcInfo.hProcess;
    ffmpeg->hPipeWrite = pipe_write;
    return ffmpeg;
}

void ffmpeg_send_frame(FFMPEG *ffmpeg, void *data, size_t width, size_t height)
{
    const DWORD bytes = (DWORD)(sizeof(uint32_t) * width * height); // RGBA32
    if (!WriteAll(ffmpeg->hPipeWrite, data, bytes)) {
        fprintf(stderr, "ERROR: WriteAll failed: %s\n", GetLastErrorAsString());
    }
}

// Kept for API compatibility: now just forwards to the single-write path.
// (ffmpeg handles the vertical flip.)
void ffmpeg_send_frame_flipped(FFMPEG *ffmpeg, void *data, size_t width, size_t height)
{
    ffmpeg_send_frame(ffmpeg, data, width, height);
}

void ffmpeg_end_rendering(FFMPEG *ffmpeg)
{
    FlushFileBuffers(ffmpeg->hPipeWrite);
    CloseHandle(ffmpeg->hPipeWrite);

    DWORD result = WaitForSingleObject(ffmpeg->hProcess, INFINITE);
    if (result == WAIT_FAILED) {
        fprintf(stderr, "ERROR: could not wait on child process: %s\n", GetLastErrorAsString());
        return;
    }

    DWORD exit_status;
    if (GetExitCodeProcess(ffmpeg->hProcess, &exit_status) == 0) {
        fprintf(stderr, "ERROR: could not get process exit code: %lu\n", GetLastError());
        return;
    }

    if (exit_status != 0) {
        fprintf(stderr, "ERROR: command exited with exit code %lu\n", exit_status);
        return;
    }

    CloseHandle(ffmpeg->hProcess);
}

void videoAssembler(const char *documentPath){
    FILE *document = NULL;
    char line[2048];
    char audioPath[1024] = "";
    char videoPath[1024] = "";
    char outputPath[1024] = "";
    char *filter = NULL;
    char *commandLine = NULL;
    VideoTraceSegment *segments = NULL;
    int segmentCount = 0;
    int segmentCapacity = 0;
    int fps = 0;
    size_t filterCap = 0;
    size_t commandCap = 0;
    size_t filterLen = 0;
    int i = 0;
    double audioCursor = 0.0;

    if (!documentPath) {
        fprintf(stderr, "ERROR: videoAssembler needs a document path.\n");
        return;
    }

    document = fopen(documentPath, "r");
    if (!document) {
        fprintf(stderr, "ERROR: Could not open build document: %s\n", documentPath);
        return;
    }

    while (fgets(line, sizeof(line), document)) {
        VideoTraceSegment segment = {0};
        TrimLine(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        if (strncmp(line, "audio_path", 10) == 0) {
            CopyValueAfterKey(line, "audio_path", audioPath, sizeof(audioPath));
            continue;
        }
        if (strncmp(line, "video_path", 10) == 0) {
            CopyValueAfterKey(line, "video_path", videoPath, sizeof(videoPath));
            continue;
        }
        if (sscanf(line, "fps %d", &fps) == 1) {
            continue;
        }
        if (sscanf(line, "mic_stop frame=%d time=%lf duration=%lf start_frame=%d start_time=%lf",
                &segment.stopFrame, &segment.stopTime, &segment.duration, &segment.startFrame, &segment.startTime) == 5) {
            if (!PushVideoTraceSegment(&segments, &segmentCount, &segmentCapacity, segment)) {
                fprintf(stderr, "ERROR: Could not store trace segments.\n");
                fclose(document);
                free(segments);
                return;
            }
        }
    }
    fclose(document);

    if (audioPath[0] == '\0' || videoPath[0] == '\0') {
        fprintf(stderr, "ERROR: Build document must contain audio_path and video_path.\n");
        free(segments);
        return;
    }
    if (segmentCount <= 0) {
        fprintf(stderr, "ERROR: No microphone segments found in build document.\n");
        free(segments);
        return;
    }

    BuildAssembledOutputPath(videoPath, outputPath, sizeof(outputPath));

    filterCap = 256 + (size_t)segmentCount * 192;
    filter = (char *)malloc(filterCap);
    if (!filter) {
        fprintf(stderr, "ERROR: Could not allocate assembler filter.\n");
        free(segments);
        return;
    }
    filter[0] = '\0';

    for (i = 0; i < segmentCount; i++) {
        long long delayMs = (long long)(segments[i].startTime * 1000.0 + 0.5);
        double audioEnd = audioCursor + segments[i].duration;
        filterLen += (size_t)snprintf(
            filter + filterLen,
            filterCap - filterLen,
            "[1:a]atrim=start=%.6f:end=%.6f,asetpts=PTS-STARTPTS,adelay=%lld|%lld[a%d];",
            audioCursor, audioEnd, delayMs, delayMs, i);
        audioCursor = audioEnd;
    }
    if (segmentCount == 1) {
        snprintf(filter + filterLen, filterCap - filterLen, "[a0]anull[aout]");
    }
    else {
        for (i = 0; i < segmentCount; i++) {
            filterLen += (size_t)snprintf(filter + filterLen, filterCap - filterLen, "[a%d]", i);
        }
        snprintf(filter + filterLen, filterCap - filterLen, "amix=inputs=%d:normalize=0[aout]", segmentCount);
    }

    commandCap = strlen(videoPath) + strlen(audioPath) + strlen(outputPath) + strlen(filter) + 512;
    commandLine = (char *)malloc(commandCap);
    if (!commandLine) {
        fprintf(stderr, "ERROR: Could not allocate assembler command line.\n");
        free(filter);
        free(segments);
        return;
    }

    snprintf(commandLine, commandCap,
        "ffmpeg.exe -loglevel verbose -y -i \"%s\" -i \"%s\" "
        "-filter_complex \"%s\" -map 0:v -map \"[aout]\" "
        "-c:v copy -c:a aac \"%s\"",
        videoPath, audioPath, filter, outputPath);

    printf("Assembling video from: %s\n", documentPath);
    printf("Output video will be: %s\n", outputPath);
    RunCommandLine(commandLine);

    free(commandLine);
    free(filter);
    free(segments);
}
