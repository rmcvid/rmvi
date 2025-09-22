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

FFMPEG *ffmpeg_start_rendering(size_t width, size_t height, size_t fps)
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
        "-pix_fmt yuv420p output.mp4",
        (int)width, (int)height, (int)fps);

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