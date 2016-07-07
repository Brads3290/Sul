#ifndef PROJECT_THREADING_H
#define PROJECT_THREADING_H

#include <functional>
#include <windows.h>

namespace Sul {
    namespace Threading {
        template<class Type, class... Args>
        class Thread {
            std::function<Type(Args)> _thread_func;
            Args _thread_args;
            HANDLE hThread;

            DWORD WINAPI ThreadProc(LPVOID ref) {

            }

        public:
            Thread(std::function<Type(Args)> fn) {
                _thread_func = fn;
            }

            void start(Args) {
                if (_thread_func) {
                    hThread = CreateThread(NULL, 0, ThreadProc, this, NULL, NULL);
                }

            }
        };
    }
}

#endif //PROJECT_THREADING_H
