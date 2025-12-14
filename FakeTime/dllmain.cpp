#include <windows.h>
#include <stdint.h>
#include "MinHook.h"

#pragma pack(push, 1)
struct FakeTimeConfig
{
    int64_t BaseFileTime;
    int32_t Mode;
    int32_t MoveForward;
    int32_t ReturnAfterSeconds;
    int32_t ImmediateMode;
};
#pragma pack(pop)

extern "C" __declspec(dllexport) int g_TimeMode = 0;
extern "C" __declspec(dllexport) FILETIME g_FakeTime = { 0, 0 };
extern "C" __declspec(dllexport) FakeTimeConfig g_ConfigData = { 0 };

static int64_t g_realFileTimeAtStart = 0;
static LARGE_INTEGER g_perfStartReal = { 0 };

typedef VOID(WINAPI* GetSystemTime_t)(LPSYSTEMTIME);
typedef VOID(WINAPI* GetLocalTime_t)(LPSYSTEMTIME);
typedef VOID(WINAPI* GetSystemTimeAsFileTime_t)(LPFILETIME);

static GetSystemTime_t             fpGetSystemTime = nullptr;
static GetLocalTime_t              fpGetLocalTime = nullptr;
static GetSystemTimeAsFileTime_t   fpGetSystemTimeAsFileTime = nullptr;

static void GetCurrentRealFileTime(FILETIME* outFt)
{
    if (fpGetSystemTimeAsFileTime)
    {
        fpGetSystemTimeAsFileTime(outFt);
    }
    else
    {
        ::GetSystemTimeAsFileTime(outFt);
    }
}

static void GetFakeFileTimeFromConfig(FILETIME* outFt)
{
    if (g_ConfigData.BaseFileTime == 0 &&
        g_ConfigData.Mode == 0 &&
        g_ConfigData.MoveForward == 0 &&
        g_ConfigData.ReturnAfterSeconds == 0)
    {
        GetCurrentRealFileTime(outFt);
        return;
    }

    FILETIME ftNowReal;
    GetCurrentRealFileTime(&ftNowReal);
    int64_t now =
        (static_cast<int64_t>(ftNowReal.dwHighDateTime) << 32) |
        static_cast<int64_t>(ftNowReal.dwLowDateTime);

    int64_t fake = g_ConfigData.BaseFileTime;

    if (g_ConfigData.Mode == 1)
    {
        fake = g_realFileTimeAtStart + g_ConfigData.BaseFileTime;
    }

    if (g_ConfigData.MoveForward != 0)
    {
        LARGE_INTEGER perfNow, perfFreq;
        QueryPerformanceCounter(&perfNow);
        QueryPerformanceFrequency(&perfFreq);

        double secondsPassed =
            double(perfNow.QuadPart - g_perfStartReal.QuadPart) /
            double(perfFreq.QuadPart);

        int64_t deltaTicks = static_cast<int64_t>(secondsPassed * 10000000.0);
        fake += deltaTicks;
    }

    if (g_ConfigData.ReturnAfterSeconds > 0)
    {
        LARGE_INTEGER perfNow, perfFreq;
        QueryPerformanceCounter(&perfNow);
        QueryPerformanceFrequency(&perfFreq);

        double secondsPassed =
            double(perfNow.QuadPart - g_perfStartReal.QuadPart) /
            double(perfFreq.QuadPart);

        if (secondsPassed >= g_ConfigData.ReturnAfterSeconds)
        {
            fake = now;
        }
    }

    outFt->dwLowDateTime = static_cast<DWORD>(fake & 0xFFFFFFFF);
    outFt->dwHighDateTime = static_cast<DWORD>((fake >> 32) & 0xFFFFFFFF);
}

static void GetEffectiveFileTime(FILETIME* outFt)
{
    if (g_TimeMode == 0 || g_TimeMode == 2)
    {
        GetCurrentRealFileTime(outFt);
        return;
    }

    FILETIME ftFake = {};
    GetFakeFileTimeFromConfig(&ftFake);

    g_FakeTime = ftFake;

    *outFt = ftFake;
}

VOID WINAPI MyGetSystemTime(LPSYSTEMTIME lpSystemTime)
{
    if (g_TimeMode == 0 || g_TimeMode == 2)
    {
        // Real
        if (fpGetSystemTime)
            fpGetSystemTime(lpSystemTime);
        else
            ::GetSystemTime(lpSystemTime);
        return;
    }

    FILETIME ftFake;
    GetEffectiveFileTime(&ftFake);
    FileTimeToSystemTime(&ftFake, lpSystemTime);
}

VOID WINAPI MyGetLocalTime(LPSYSTEMTIME lpSystemTime)
{
    if (g_TimeMode == 0 || g_TimeMode == 2)
    {
        // Real
        if (fpGetLocalTime)
            fpGetLocalTime(lpSystemTime);
        else
            ::GetLocalTime(lpSystemTime);
        return;
    }

    FILETIME ftFake;
    GetEffectiveFileTime(&ftFake);
    FileTimeToSystemTime(&ftFake, lpSystemTime);
}

VOID WINAPI MyGetSystemTimeAsFileTime(LPFILETIME lpFileTime)
{
    if (g_TimeMode == 0 || g_TimeMode == 2)
    {
        // Real
        if (fpGetSystemTimeAsFileTime)
            fpGetSystemTimeAsFileTime(lpFileTime);
        else
            ::GetSystemTimeAsFileTime(lpFileTime);
        return;
    }

    FILETIME ftFake;
    GetEffectiveFileTime(&ftFake);
    *lpFileTime = ftFake;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);

        if (MH_Initialize() != MH_OK)
            return TRUE;

        MH_CreateHookApi(
            L"kernel32",
            "GetSystemTime",
            reinterpret_cast<LPVOID>(&MyGetSystemTime),
            reinterpret_cast<LPVOID*>(&fpGetSystemTime)
        );

        MH_CreateHookApi(
            L"kernel32",
            "GetLocalTime",
            reinterpret_cast<LPVOID>(&MyGetLocalTime),
            reinterpret_cast<LPVOID*>(&fpGetLocalTime)
        );

        MH_CreateHookApi(
            L"kernel32",
            "GetSystemTimeAsFileTime",
            reinterpret_cast<LPVOID>(&MyGetSystemTimeAsFileTime),
            reinterpret_cast<LPVOID*>(&fpGetSystemTimeAsFileTime)
        );

        MH_EnableHook(MH_ALL_HOOKS);
        break;
    }

    case DLL_PROCESS_DETACH:
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;

    default:
        break;
    }

    return TRUE;
}