/*
    Initial author: Convery (tcn@ayria.se)
    Started: 29-07-2017
    License: MIT
    Notes:
        Module entrypoint.
*/

#include "Stdinclude.h"

// Delete the last sessions log on startup for windows.
#if defined (_WIN32)
    namespace { struct Deletelog { Deletelog() { Clearlog(); } }; static Deletelog Deleted{}; }
#endif

// The callback system for Ayria plugins.
extern "C"
{
    EXPORT_ATTR void onInitializationStart(bool Reserved)
    {
        // Perform STMSIG processing in a separate thread.
        auto Lambda = []()
        {
            SteamIPC IPC{};
            SteamDRM DRM{};
            SteamStart Start{};

            // Initialize the IPC and wait for data.
            InitializeIPC(IPC);
            WaitForSingleObject(IPC.Consumesemaphore, 1500);

            // Initialze the DRM struct from the data.
            InitializeDRM(DRM, (char *)IPC.Sharedfilemapping);

            // Read the startup file, even though steam just copies this to a "./STF" temp-file.
            InitializeSteamstart(Start, DRM.Startupmodule.c_str());

            // Remove the startupfile.
            std::remove(DRM.Startupmodule.c_str());

            // Acknowledge that the game has started.
            auto Event = OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, DRM.Startevent.c_str());
            SetEvent(Event);
            CloseHandle(Event);

            // Notify the game that we are done.
            ReleaseSemaphore(IPC.Producesemaphore, 1, NULL);

            // Clean up the IPC.
            UnmapViewOfFile(IPC.Sharedfilemapping);
            CloseHandle(IPC.Sharedfilehandle);
            CloseHandle(IPC.Consumesemaphore);
            CloseHandle(IPC.Producesemaphore);

            // Hook the modulehandle to return the wrong modulename.
            HookModulehandle();
        };

        // Start the thread.
        std::thread(Lambda).detach();
    }
    EXPORT_ATTR void onInitializationDone(bool Reserved)
    {
        /*
            ----------------------------------------------------------------------
            This callback is called when the platform notifies the bootstrapper,
            or at most 3 seconds after startup. This is the perfect time to start
            communicating with other plugins and modify the games .data segment.
            ----------------------------------------------------------------------
        */
    }
    EXPORT_ATTR void onMessage(uint32_t MessageID, uint32_t Messagesize, const void *Messagedata)
    {
        /*
            ----------------------------------------------------------------------
            This callback is called when another plugin broadcasts a message. They
            can safely be ignored, but if you want to make use of the system you
            should create a unique name for your messages. We recommend that you
            base it on your pluginname as shown below, we also recommend that you
            use the bytebuffer format for data.
            ----------------------------------------------------------------------
        */

        // MessageID is a FNV1a_32 hash of a string.
        switch (MessageID)
        {
            case Hash::FNV1a_32(MODULENAME "_Default"):
            default: break;
        }
    }
}

// Default entrypoint for windows.
#if defined (_WIN32)
BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    switch (nReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            // Rather not handle all thread updates.
            DisableThreadLibraryCalls(hDllHandle);
            break;
        }
    }

    return TRUE;
}
#endif
