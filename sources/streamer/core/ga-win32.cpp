// Copyright (C) 2022 Intel Corporation

/*
 * Copyright (c) 2013 Chun-Ying Huang
 *
 * This file is part of GamingAnywhere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * GamingAnywhere's common functions for Windows
 *
 * This includes Windows specific functions and
 * common UNIX function implementations for Windows.
 */

#include "ga-common.h"
#include "ga-conf.h"
#include <sstream>
#include <map>
#include <dxgi.h>
#include <atlbase.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS) || defined(__WATCOMC__)
#define DELTA_EPOCH_IN_USEC    11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_USEC    11644473600000000ULL
#endif

typedef unsigned __int64 u_int64_t;

/**
 * Convert Windows FILETIME to UNIX timestamp. This is an internal function.
 *
 * @param ft [in] Pointer to a FILETIME.
 * @return UNIX timestamp time in microsecond unit.
 */
static u_int64_t
filetime_to_unix_epoch(const FILETIME *ft) {
    u_int64_t res = (u_int64_t) ft->dwHighDateTime << 32;
    res |= ft->dwLowDateTime;
    res /= 10;                   /* from 100 nano-sec periods to usec */
    res -= DELTA_EPOCH_IN_USEC;  /* from Win epoch to Unix epoch */
    return (res);
}

/**
 * gettimeofday() implementation
 *
 * @param tv [in] \a timeval to store the timestamp.
 * @param tz [in] timezone: unused.
 */
int
gettimeofday(struct timeval *tv, void *tz) {
    FILETIME  ft;
    u_int64_t tim;
    if (!tv) {
        //errno = EINVAL;
        return (-1);
    }
    GetSystemTimeAsFileTime(&ft);
    tim = filetime_to_unix_epoch (&ft);
    tv->tv_sec  = (long) (tim / 1000000L);
    tv->tv_usec = (long) (tim % 1000000L);
    return (0);
}

long long tvdiff_us(struct timeval *tv1, struct timeval *tv2);

/**
 * usleep() function: sleep in microsecond scale.
 *
 * @param waitTime [in] time to sleep (in microseconds).
 * @return Always return 0.
 */
int
usleep(long long waitTime) {
#if 0
    LARGE_INTEGER t1, t2, freq;
#else
    struct timeval t1, t2;
#endif
    long long ms, elapsed;
    if(waitTime <= 0)
        return 0;
#if 0
    QueryPerformanceCounter(&t1);
    QueryPerformanceFrequency(&freq);
    if(freq.QuadPart == 0) {
        // not supported
        Sleep(waitTime/1000);
        return 0;
    }
#else
    gettimeofday(&t1, NULL);
#endif
    // Sleep() may be fine
    ms = waitTime / 1000;
    waitTime %= 1000;
    if(ms > 0) {
        Sleep((DWORD)ms);
    }
    // Sleep for the rest
    if(waitTime > 0) do {
#if 0
        QueryPerformanceCounter(&t2);

        elapsed = 1000000.0 * (t2.QuadPart - t1.QuadPart) / freq.QuadPart;
#else
        gettimeofday(&t2, NULL);
        elapsed = tvdiff_us(&t2, &t1);
#endif
    } while(elapsed < waitTime);
    //
    return 0;
}

/**
 * read() function to read from a socket.
 *
 * @param fd [in] The SOCKET identifier.
 * @param buf [in] Buffer to receive data.
 * @param count [in] Size limit of the \a buf.
 * @return Number of bytes received, see MSDN recv() function.
 */
int
read(SOCKET fd, void *buf, int count) {
    return recv(fd, (char *) buf, count, 0);
}

/**
 * write() function to write to a socket.
 *
 * @param fd [in] The SOCKET identifier.
 * @param buf [in] Buffer to be sent.
 * @param count [in] Number of bytes in the \a buf.
 * @return Number of bytes sent, see MSDN send() function.
 */
int
write(SOCKET fd, const void *buf, int count) {
    return send(fd, (const char*) buf, count, 0);
}

/**
 * close() function to close a socket.
 *
 * @param fd [in] The SOCKET identifier.
 * @return Zero on success, otherwise see MSDN closesocket() function.
 */
int
close(SOCKET fd) {
    return closesocket(fd);
}

/**
 * dlerror() to report error message of dl* functions
 *
 * Not supported on Windows.
 */
char *
dlerror() {
    static char notsupported[] = "dlerror() on Windows is not supported.";
    return notsupported;
}

/**
 * Fill BITMAPINFO data structure
 *
 * @param pinfo [in,out] The BITMAPINFO structure to be filled.
 * @param w [in] The width of the bitmap image.
 * @param h [in] The height of the bitmap image.
 * @param bitsPerPixel [in] The bits-per-pixel of the bitmap image.
 */
void
ga_win32_fill_bitmap_info(BITMAPINFO *pinfo, int w, int h, int bitsPerPixel) {
    ZeroMemory(pinfo, sizeof(BITMAPINFO));
    pinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pinfo->bmiHeader.biBitCount = bitsPerPixel;
    pinfo->bmiHeader.biCompression = BI_RGB;
    pinfo->bmiHeader.biWidth = w;
    pinfo->bmiHeader.biHeight = h;
    pinfo->bmiHeader.biPlanes = 1; // must be 1
    pinfo->bmiHeader.biSizeImage = pinfo->bmiHeader.biHeight
                    * pinfo->bmiHeader.biWidth
                    * pinfo->bmiHeader.biBitCount/8;
    return;
}

/**
 * Compute time difference based on Windows performance counter.
 *
 * @param t1 [in] The first counter.
 * @param t2 [in] The second counter.
 * @param freq [in] The performance frequency obtained by \em QueryPerformanceFrequency().
 * @return Time difference in microsecond unit, e.g., \a t1 - \a t2.
 */
long long
pcdiff_us(LARGE_INTEGER t1, LARGE_INTEGER t2, LARGE_INTEGER freq) {
    return 1000000LL * (t1.QuadPart - t2.QuadPart) / freq.QuadPart;
}

typedef enum GA_PROCESS_DPI_AWARENESS {
    GA_PROCESS_DPI_UNAWARE = 0,
    GA_PROCESS_SYSTEM_DPI_AWARE = 1,
    GA_PROCESS_PER_MONITOR_DPI_AWARE = 2
} GA_PROCESS_DPI_AWARENESS;
typedef BOOL (WINAPI * setProcessDpiAware_t)(void);
typedef HRESULT (WINAPI * setProcessDpiAwareness_t)(GA_PROCESS_DPI_AWARENESS);

/**
 * Platform dependent call to SetProcessDpiAware(PROCESS_PER_MONITOR_DPI_AWARE)
 */
EXPORT
int
ga_set_process_dpi_aware() {
    HMODULE shcore, user32;
    setProcessDpiAware_t     aw = NULL;
    setProcessDpiAwareness_t awness = NULL;
    int ret = 0;
    if((shcore = LoadLibraryA("shcore.dll")))
        awness = (setProcessDpiAwareness_t) GetProcAddress(shcore, "SetProcessDpiAwareness");
    if((user32 = LoadLibraryA("user32.dll")))
        aw = (setProcessDpiAware_t) GetProcAddress(user32, "SetProcessDPIAware");
    if(awness) {
        ret = (int) (awness(GA_PROCESS_PER_MONITOR_DPI_AWARE) == S_OK);
    } else if(aw) {
        ret = (int) (aw() != 0);
    }
    if(user32) FreeLibrary(user32);
    if(shcore) FreeLibrary(shcore);
    return ret;
}

std::string
HRESULT_to_string(HRESULT hr) {
  TCHAR szMsgBuf[1024] = {};
  DWORD dwSize = sizeof(szMsgBuf), dwLen = 0;
  std::stringstream tmp;

  dwLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
    NULL,
    hr,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) /* Default language */,
    szMsgBuf,
    dwSize,
    NULL);

  if (dwLen > 0) {
      tmp << "HRESULT(0x" << std::hex << hr << ") : " << std::string(szMsgBuf);
  }
  else {
      tmp << "HRESULT(0x" << std::hex << hr << ") : Unknown error code ..";
  }

  return tmp.str();
}

std::string
LastError_to_string(DWORD dwErr) {
    TCHAR szMsgBuf[1024] = {};
    DWORD dwSize = sizeof(szMsgBuf), dwLen = 0;
    std::stringstream tmp;

    dwLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        dwErr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        szMsgBuf,
        dwSize,
        NULL);

    if (dwLen > 0) {
        tmp << "Error (0x" << std::hex << dwErr << ") " << std::string(szMsgBuf);
    }
    else {
        tmp << "Error (0x" << std::hex << dwErr << ") Unknown error code.";
    }

    return tmp.str();
}

std::wstring
IID_to_wstring(const IID& riid) {
    LPOLESTR s_iid;
    if (FAILED(StringFromIID(riid, &s_iid))) {
        return L"Unknown";
    }

    std::wstring w_iid(s_iid, s_iid + wcslen(s_iid));
    CoTaskMemFree(s_iid);

    return w_iid;
}

//adopted from Titan SDK
struct HandleInfo
{
    unsigned long   processId{ 0 };
    HWND            windowHandle{ 0 };
};

BOOL CALLBACK EnumWindowsCallback(HWND pCurHandle, LPARAM pLParam)
{
    HandleInfo& targetWindowInfo = *(HandleInfo*)pLParam;
    unsigned long curProcessId = 0;
    GetWindowThreadProcessId(pCurHandle, &curProcessId);
    if (targetWindowInfo.processId == curProcessId)
    {
        targetWindowInfo.windowHandle = pCurHandle;
        //stop enumeration if we found window created by current process
        return FALSE;
    }
    return TRUE;
}

HWND GetWindowHandleOfCurrentProcess() {
    constexpr size_t timeToWaitInMillisecond = 30 * 1000;
    constexpr size_t waitStepInMillisecond = 50;

    HandleInfo renderedWindowInfo;
    renderedWindowInfo.processId = static_cast<unsigned long>(GetCurrentProcessId());
    renderedWindowInfo.windowHandle = NULL;

    for (size_t i = 0; i < timeToWaitInMillisecond; i += waitStepInMillisecond) {
        EnumWindows(EnumWindowsCallback, (LPARAM)&renderedWindowInfo);

        if (renderedWindowInfo.windowHandle != NULL)
            break;

        Sleep(waitStepInMillisecond);
    }
    return renderedWindowInfo.windowHandle;
}

/**
 * Create child process executes system command and return result in string
 *
 * @param cmd [in] The command to execute
 * @param result [out] The return result in string
 * 
 * @return Returns true if success, false otherwise
 */
bool RunSystemCommand(const std::string& cmdline, std::string& result) {
    result.clear();

    HANDLE h_child_stdin_read = nullptr, h_child_stdin_write = nullptr;
    HANDLE h_child_stdout_read = nullptr, h_child_stdout_write = nullptr;
    SECURITY_ATTRIBUTES sa = {};

    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle       = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    /*
     * +---------------------+                +---------------------+
     * |   Parent Process    |                |   Child Process     |
     * +---------------------+                +---------------------+
     * |                     |                |                     |
     * | h_child_stdin_write | -------------> | h_child_stdin_Read  |
     * |                     |                |                     |
     * | h_child_stdout_read | <------------- | h_child_stdout_write|
     * |                     |                |                     |
     * +---------------------+                +---------------------+
     *
     */

    // Create child stdin read/write pipe
    if (CreatePipe(&h_child_stdin_read, &h_child_stdin_write, &sa, 0) == FALSE)
        return false;

    // Create child stdout read/write pipe
    if (CreatePipe(&h_child_stdout_read, &h_child_stdout_write, &sa, 0) == FALSE) {
        CloseHandle(h_child_stdin_read);
        CloseHandle(h_child_stdin_write);
        return false;
    }

    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};

    // Set STRTUPINFO for the spawned process
    // The dwFlags member tells CreateProcess how to make the process
    //   STARTF_USESTDHANDLES: validates the hStd* members.
    //   STARTF_USESHOWWINDOW: validates the wShowWindow member
    GetStartupInfo(&si);

    si.dwFlags      = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow  = SW_HIDE;  // Silent command window
    si.hStdOutput   = h_child_stdout_write;
    si.hStdError    = h_child_stdout_write;
    si.hStdInput    = h_child_stdin_read;

    // spawn the child process to executes the command
    if (CreateProcess(nullptr, (LPSTR)cmdline.c_str(), nullptr, nullptr, TRUE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
        unsigned long bytes_read = 0;
        unsigned long bytes_avail = 0;
        char buf[1024] = {};

        // Command executes normally quick respond.
        // Wait until child process exits just in-case.
        WaitForSingleObject(pi.hProcess, INFINITE);

        for (;;) {
            PeekNamedPipe(h_child_stdout_read, buf, sizeof(buf) - 1, &bytes_read, &bytes_avail, nullptr);
            // Check to see if there is any data ready to read from stdout
            if (bytes_read != 0) {
                if (ReadFile(h_child_stdout_read, buf, sizeof(buf) - 1, &bytes_read, nullptr)) {
                    result += std::string(buf);
                    break;
                }
            }
        }
        // Cloase thread and process handles
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    // Close all pipe handles
    CloseHandle(h_child_stdin_read);
    CloseHandle(h_child_stdin_write);
    CloseHandle(h_child_stdout_read);
    CloseHandle(h_child_stdout_write);

    // Remove last newline if appeared
    if (!result.empty() && result[result.size() - 1] == '\n') {
        result.pop_back();
    }

    return true;
}

std::map<unsigned int, LUID> luid_map; //<adapterIndex, LUID>

// Function to get adapter index from provided adapter-luid
unsigned int
get_adapter_index() {
    int uid[2]                   = {};
    LUID luid                    = {};
    unsigned int default_adapter = 0;
    bool find_adapter            = false;

    if (ga_conf_readints("adapter-luid", uid, 2) == 2) {
        luid.HighPart = uid[0];
        luid.LowPart  = uid[1];
        for (auto& kv : luid_map) {
            if (luid.HighPart == kv.second.HighPart && luid.LowPart == kv.second.LowPart) {
                default_adapter = kv.first;
                find_adapter    = true;
                break;
            }
        }
    }
    if (find_adapter != true) {
        ga_logger(Severity::INFO, "*** Cannot Find Target LUID = %x.%x ==> Use default adapter = %d\n", luid.HighPart, luid.LowPart, default_adapter);
    }

    return default_adapter;
}

// Function to initialize adapter luid map
bool
init_adapters_luid_map() {
    CComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(__uuidof(factory), (void**)&factory))) {
        ga_logger(Severity::ERR, "Failed to create factory object\n");
        return false;
    }

    luid_map.clear();

    HRESULT hr      = S_OK;
    unsigned int it = 0;
    ga_logger(Severity::INFO, "All EnumAdapters:\n");
    while (true) {
        CComPtr<IDXGIAdapter1> adapter;

        ga_logger(Severity::INFO, "Adapter index: %d\n", it);
        hr = factory->EnumAdapters1(it, &adapter);
        if (FAILED(hr)) break;

        DXGI_ADAPTER_DESC1 desc{};
        (void)adapter->GetDesc1(&desc);

        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            char path[256]{};

            ga_logger(Severity::INFO, "\t* AdapterOrder:%u\n", it);
            ga_logger(Severity::INFO, "\t* LuidHigh.Low:%x.%x (%d,%d)\n", desc.AdapterLuid.HighPart, desc.AdapterLuid.LowPart, desc.AdapterLuid.HighPart, desc.AdapterLuid.LowPart);
            ga_logger(Severity::INFO, "\t* VendorID:%u\n", desc.VendorId);
            ga_logger(Severity::INFO, "\t* DeviceID:%u\n", desc.DeviceId);
            WideCharToMultiByte(CP_ACP, 0, desc.Description, static_cast<int>(wcslen(desc.Description) + 1), path, 256, NULL, NULL);
            ga_logger(Severity::INFO, "\t* Description:%s\n", path);

            if (luid_map.find(it) == luid_map.end()) {
                luid_map[it] = desc.AdapterLuid;
            }
        }

        it++;
    }
    return true;
}
