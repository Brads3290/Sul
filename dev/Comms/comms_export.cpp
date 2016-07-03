#include <windows.h>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "MailSlots.h"

using std::stringstream;
using std::string;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            srand((unsigned int) time(0));
            return TRUE;
        }
        default: {
            return TRUE;
        }
    }
}

string specialChars = "&=|";

//NodeBase
extern "C" HANDLE __declspec(dllexport) SUL_createNode(const char* slotName) {
    return CreateSlot(slotName);
}

extern "C" bool __declspec(dllexport) SUL_send(const char* strMsg, const char* strDest) {
    return static_cast<bool>(Write(const_cast<LPTSTR>(strDest), const_cast<LPTSTR>(strMsg)));
}

extern "C" bool __declspec(dllexport) SUL_mailslotExists(const char* path) {
    auto slot = CreateMailslot(path, 0, MAILSLOT_WAIT_FOREVER, NULL);

    if (!slot) {
        int err = (int) GetLastError();
        return true; //Exists
    }

    CloseHandle(slot);
    return false; //Doesn't exist
}

extern "C" unsigned int __declspec(dllexport) SUL_countNewMessages(HANDLE hSlot) {
    DWORD cbMessage;
    BOOL res = GetMailslotInfo(hSlot, NULL, NULL, &cbMessage, NULL);

    if (!res) {
        throw (unsigned int) GetLastError();
    }

    return cbMessage;
}

//LocalNode
extern "C" const char* __declspec(dllexport) SUL_getNextMessage(HANDLE hSlot) {
    auto str = Read(hSlot);
    auto cstr = new char[str.length() + 1];

    for (int j = 0; j < str.length(); ++j) {
        cstr[j] = str[j];
    }
    cstr[str.length()] = 0;

    return cstr;
}

//MessageBase
extern "C" const char* __declspec(dllexport) SUL_generateUID(unsigned int segs) {
    stringstream seg;
    for (int i = 0; i < segs; ++i) {
        seg << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << rand() * rand();

        if (i + 1 < segs) {
            seg << "-";
        }
    }

    auto str = seg.str();
    auto cstr = new char[str.length() + 1];

    for (int j = 0; j < str.length(); ++j) {
        cstr[j] = str[j];
    }
    cstr[str.length()] = 0;

    return cstr;
}

extern "C" const char* __declspec(dllexport) SUL_messageEncode(const char* cmsg) {
    string ret;
    string msg(cmsg);

    for (int i = 0; i < msg.length(); ++i) {
        if (specialChars.find(msg[i]) == -1) { //str[i] is not a special character
            ret += msg[i];
        } else { //str[i] is a special character - add an escape sequence
            ret += "|";
            ret += msg[i];
        }
    }

    char* cret = new char[ret.length() + 1];
    for (int j = 0; j < ret.length(); ++j) {
        cret[j] = ret[j];
    }
    cret[ret.length()] = 0;

    return cret;
}

extern "C" const char* __declspec(dllexport) SUL_messageDecode(const char* cmsg) {
    string ret;
    string msg(cmsg);

    for (int i = 0; i < msg.length(); ++i) {
        if (msg[i] == '|') { //Escape sequence character is reached
            ret += msg[++i]; //Ignore the escape sequence character
        } else {
            ret += msg[i];
        }
    }

    char* cret = new char[ret.length() + 1];
    for (int j = 0; j < ret.length(); ++j) {
        cret[j] = ret[j];
    }
    cret[ret.length()] = 0;

    return cret;
}