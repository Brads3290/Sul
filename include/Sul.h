#ifndef SULLY_MAIN_H
#define SULLY_MAIN_H

#include <vector>
#include <string>
#include <windows.h>
#include <stdexcept>

using std::string;

namespace Sul {
    class DynamicLibrary {
        string DLLPath;
        HINSTANCE hDLL = nullptr;

        //static std::vector<DynamicLibrary*> _List;

    public:
        DynamicLibrary() {
            //_List.push_back(this);
        }
        DynamicLibrary(string path): DLLPath(path) {
            //_List.push_back(this);
        }
        ~DynamicLibrary() {
            if (isDLLLoaded()) {
                freeDLL();
            }
        }

        void loadDLL() {
            if (isDLLLoaded()) {
                freeDLL();
            }

            hDLL = LoadLibrary(DLLPath.c_str());

            if (!hDLL) {
                auto err = GetLastError();

                switch (err) {
                    case 87: throw std::runtime_error("Invalid argument passed to LoadLibrary: \"" + DLLPath + "\"");
                    case 126: throw std::runtime_error("Library not found at path: \"" + DLLPath + "\"");
                    default: throw std::runtime_error("LoadLibrary failed with error code " + std::to_string(err) + " when trying to load DLL from path: " + DLLPath);
                }
            }
        }
        void freeDLL() {
            requireDLL("Cannot free DLL \"" + DLLPath + "\" as it is not currently loaded.");

            auto success = FreeLibrary(hDLL);

            if (!success) {
                throw std::runtime_error("FreeLibrary failed with error code " + std::to_string(success) + " when trying to free DLL at path: " + DLLPath);
            }
        }

        FARPROC getProcAddress(string proc_name) {
            requireDLL("getProcAddress cannot run as the DLL \"" + DLLPath + "\" isn't loaded.");

            auto proc = GetProcAddress(hDLL, proc_name.c_str());
            if (!proc) {
                auto err = GetLastError();
                throw std::runtime_error("GetProcAddress failed with error code " + std::to_string(err) + " when trying to load procedure \"" + proc_name +"\" from path: " + DLLPath);
            }

            return proc;
        }

        const string &getDLLPath() const {
            return DLLPath;
        }
        void setDLLPath(const string &DLLPath) {
            if (isDLLLoaded()) {
                throw std::runtime_error("Cannot set a new DLL path because the DLL from \"" + DLLPath + "\" is currently loaded in memory");
            }
            DynamicLibrary::DLLPath = DLLPath;
        }

        bool isDLLLoaded() {
            return hDLL != nullptr;
        }
        HINSTANCE getDLL() {
            return hDLL;
        }
    protected:
        void requireDLL(string err) {
            if (!isDLLLoaded()) {
                throw std::runtime_error(err);
            }
        }
    };
}

#endif //SULLY_MAIN_H
