
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif //_WIN32

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct SInterface{
#ifdef _WIN32
    HMODULE dll;
#else
    void* dll;
#endif
    int (*init)(void);
    void (*ende)(void);
    void (*setParameter)(const char*,int);
    int (*getParameter)(const char*);
    int (*start)(void);
    int (*stop)(void);
	
} SInterface;

//! \return 0 alles okay, -1 bei fehler
SInterface* interface_loadDll(const char* dllname);
void interface_close(SInterface* interface);

    

#ifdef __cplusplus
}
#endif
