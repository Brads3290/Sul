#include <windows.h>
#include <string>
#include <stdexcept>

using std::string;
using std::runtime_error;

#ifndef PROJECT_MAIL_H
#define PROJECT_MAIL_H

string StrConvert(LPTSTR str) {
    return string(str);
}
LPTSTR StrConvert(string str) {
    auto ret = new TCHAR[str.length() + 1];

    for (int i = 0; i < str.length(); ++i) {
        ret[i] = str[i];
    }

    ret[str.length()] = '\0';

    return ret;
}
HANDLE WINAPI CreateSlot(string slotName) {
    HANDLE hSlot = CreateMailslot(const_cast<LPTSTR>(slotName.c_str()),
                                  0,                             // no maximum message size
                                  MAILSLOT_WAIT_FOREVER,         // no time-out for operations
                                  (LPSECURITY_ATTRIBUTES) NULL); // default security

    if (hSlot == INVALID_HANDLE_VALUE) {
        throw (unsigned int) GetLastError();
    }

    return hSlot;
}
HANDLE WINAPI GetFile(string slotName) {
    HANDLE hFile = CreateFile(const_cast<LPTSTR>(slotName.c_str()),
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              (LPSECURITY_ATTRIBUTES) NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              (HANDLE) NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("[" + std::to_string(GetLastError()) + "] Mail::GetFile - CreateFile returned INVALID_HANDLE_VALUE");
    }

    return hFile;
};
HANDLE WINAPI CreateSlotAndGetFile(LPTSTR slotName) {
    CreateSlot(slotName);
    return GetFile(slotName);
}
BOOL WINAPI Write(LPTSTR slotName, LPTSTR lpszMessage) {
    BOOL fResult;
    DWORD cbWritten;
    HANDLE mailSlot = GetFile(slotName);

    if (mailSlot) {
        fResult = WriteFile(mailSlot,
                            lpszMessage,
                            (DWORD) (lstrlen(lpszMessage) + 1) * sizeof(TCHAR),
                            &cbWritten,
                            (LPOVERLAPPED) NULL);

        if (!fResult) {
            throw std::runtime_error("[" + std::to_string(GetLastError()) + "] Mail::Write - WriteFile returned nullable value.");
        }

        return TRUE;
    }

    throw std::runtime_error("[" + std::to_string(GetLastError()) + "] Mail::Write - 'mailSlot' is not a valid handle.");
}
string WINAPI Read(HANDLE hSlot) {
    DWORD cbMessage, cMessage, cbRead;
    BOOL fResult;
    LPTSTR lpszBuffer;
    TCHAR achID[80];
    HANDLE hEvent;
    OVERLAPPED ov;

    cbMessage = cMessage = cbRead = 0;

    fResult = GetMailslotInfo(hSlot, // mailslot handle
                              (LPDWORD) NULL,               // no maximum message size
                              &cbMessage,                   // size of next message
                              &cMessage,                    // number of messages
                              (LPDWORD) NULL);              // no read time-out

    if (!fResult) {
        throw (unsigned int) GetLastError();
    }

    if (cMessage == 0 || cbMessage == MAILSLOT_NO_MESSAGE) {
        return FALSE; //No Message
    }

    hEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("MailSlot"));
    if (NULL == hEvent){
        throw (unsigned int) GetLastError();
    }
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    ov.hEvent = hEvent;

    // Allocate memory for the message.
    lpszBuffer = (LPTSTR) GlobalAlloc(GPTR,
                                      lstrlen((LPTSTR) achID) * sizeof(TCHAR) + cbMessage);

    if (NULL == lpszBuffer) {
        return FALSE;
    }

    lpszBuffer[0] = '\0';

    fResult = ReadFile(hSlot,
                       lpszBuffer,
                       cbMessage,
                       &cbRead,
                       &ov);

    if (!fResult) {
        GlobalFree((HGLOBAL) lpszBuffer);
        throw (unsigned int) GetLastError();
    }

    string ret(lpszBuffer);

    //Free the buffer memory
    GlobalFree((HGLOBAL) lpszBuffer);

    //Close the event handle
    CloseHandle(hEvent);

    return ret;
}

#endif //PROJECT_MAIL_H