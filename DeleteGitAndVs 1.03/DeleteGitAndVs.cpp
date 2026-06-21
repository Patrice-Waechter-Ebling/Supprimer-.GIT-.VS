#include <windows.h>
#include <shlobj.h>
#include <stdio.h>

#define INI_FILE "DeleteGitAndVs.ini"

char g_LogFile[MAX_PATH] = "DeleteGitAndVs.log";
bool g_DryRun = false;
const int MAX_ITEMS = 128;
char* g_Extensions[MAX_ITEMS];
int   g_ExtCount = 0;
char* g_Folders[MAX_ITEMS];
int   g_FolderCount = 0;
static int g_TotalFoldersToDelete = 0;
static int g_DeletedFolders = 0;
HANDLE g_hLogMutex = NULL;
FILE*  g_Log = NULL;

void Log(const char* fmt, ...)
{
    if (!g_Log) return;
    WaitForSingleObject(g_hLogMutex, INFINITE);
    va_list args;
    va_start(args, fmt);
    vfprintf(g_Log, fmt, args);
    fprintf(g_Log, "\n");
    fflush(g_Log);
    va_end(args);
    ReleaseMutex(g_hLogMutex);
}
void ShowProgress()
{
    float ratio = 0.0f;
    if (g_TotalFoldersToDelete > 0)ratio = (float)g_DeletedFolders / (float)g_TotalFoldersToDelete;
	char buf[80];
	sprintf(buf,"Progression %d/%d %d%%", g_DeletedFolders, g_TotalFoldersToDelete,ratio);
	SetConsoleTitle(buf);
}
void TrierListe(const char* src, char** destArray, int* count)
{
    *count = 0;
    if (!src || !src[0]) return;
    char buffer[1024];
    lstrcpynA(buffer, src, sizeof(buffer));
    char* p = buffer;
    while (*p && *count < MAX_ITEMS)
    {
        while (*p == ';' || *p == ' ') p++;
        if (!*p) break;
        char* start = p;
        while (*p && *p != ';') p++;
        if (*p) *p = 0;
        int len = lstrlenA(start);
        if (len > 0)
        {
            destArray[*count] = (char*)malloc(len + 1);
            lstrcpyA(destArray[*count], start);
            (*count)++;
        }
        if (!*p) break;
        p++;
    }
}

void ChargerParametres()
{
    char buf[1024];
    GetPrivateProfileStringA("General", "DryRun", "0", buf, sizeof(buf), INI_FILE);
    g_DryRun = (buf[0] == '1');
    GetPrivateProfileStringA("General", "LogFile", "DeleteGitAndVs.log", g_LogFile, sizeof(g_LogFile), INI_FILE);
    GetPrivateProfileStringA("Extensions", "List", "", buf, sizeof(buf), INI_FILE);
    TrierListe(buf, g_Extensions, &g_ExtCount);
    GetPrivateProfileStringA("Folders", "List", "", buf, sizeof(buf), INI_FILE);
    TrierListe(buf, g_Folders, &g_FolderCount);
}
bool ExtensionCiblee(const char* name)
{
    const char* ext = strrchr(name, '.');
    if (!ext) return false;
    ext++;

    for (int i = 0; i < g_ExtCount; i++)
    {
        if (!lstrcmpiA(ext, g_Extensions[i])) return true;
    }
    return false;
}

bool DossierCiblee(const char* name)
{
    for (int i = 0; i < g_FolderCount; i++)
    {
        if (!lstrcmpiA(name, g_Folders[i]))return true;
    }
    return false;
}
bool SuppressionRecursiveDossier(const char* path)
{
    char searchPath[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    wsprintfA(searchPath, "%s\\*.*", path);
    h = FindFirstFileA(searchPath, &fd);
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, ".."))continue;
        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", path, fd.cFileName);
        SetFileAttributesA(fullPath, FILE_ATTRIBUTE_NORMAL);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            SuppressionRecursiveDossier(fullPath);
            if (!g_DryRun) RemoveDirectoryA(fullPath);
            Log("Suppression: %s", fullPath);
        }
        else
        {
            if (!g_DryRun)DeleteFileA(fullPath);
            Log("Supression: %s", fullPath);
        }

    } while (FindNextFileA(h, &fd));
    FindClose(h);
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    if (!g_DryRun)return RemoveDirectoryA(path);
    Log("DIR DEL ROOT: %s", path);
    return TRUE;
}
void CalculerCible(const char* root)
{
    char searchPath[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    wsprintfA(searchPath, "%s\\*.*", root);
    h = FindFirstFileA(searchPath, &fd);
    if (h == INVALID_HANDLE_VALUE)return;
    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, "..")) continue;
        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", root, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (DossierCiblee(fd.cFileName)){g_TotalFoldersToDelete++;}
            else{CalculerCible(fullPath);}
        }

    } while (FindNextFileA(h, &fd));

    FindClose(h);
}
void AnalyseSuppression(const char* root)
{
    char searchPath[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    wsprintfA(searchPath, "%s\\*.*", root);
    h = FindFirstFileA(searchPath, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, "..")) continue;
        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", root, fd.cFileName);
        SetFileAttributesA(fullPath, FILE_ATTRIBUTE_NORMAL);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (DossierCiblee(fd.cFileName))
            {
                printf("\nSuppression dossier : %s\n", fullPath);
                Log("TARGET DIR: %s", fullPath);
                SuppressionRecursiveDossier(fullPath);
                g_DeletedFolders++;
                ShowProgress();
            }
            else{AnalyseSuppression(fullPath);}
        }
        else
        {
            if (ExtensionCiblee(fd.cFileName) ||(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                printf("Fichier supprimé : %s\n", fullPath);
                Log("TARGET FILE: %s", fullPath);
                if (!g_DryRun)DeleteFileA(fullPath);
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
bool ParcourirArborescence(char* outPath)
{
    BROWSEINFOA bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.lpszTitle = "Sélectionnez le dossier à nettoyer";
    bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_DONTGOBELOWDOMAIN|BIF_STATUSTEXT|BIF_RETURNFSANCESTORS|BIF_EDITBOX |BIF_VALIDATE;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl) return false;
    BOOL ok = SHGetPathFromIDListA(pidl, outPath);
    CoTaskMemFree(pidl);
    return ok ? true : false;
}
int main()
{
    ChargerParametres();
    g_hLogMutex = CreateMutexA(NULL, FALSE, NULL);
    g_Log = fopen(g_LogFile, "a");
    if (g_Log) Log("=== START CLEANER (DryRun=%d) ===", g_DryRun ? 1 : 0);
    char path[MAX_PATH];
    if (!ParcourirArborescence(path))
    {
        printf("Aucun dossier selectionne.\n");
        Log("Aucun dossier selectionne.");
        if (g_Log) fclose(g_Log);
        return 0;
    }
    printf("\nDossier sélectionné : %s\n", path);
    Log("Traitement: %s", path);
    printf("\nAnalyse des dossiers...\n");
    CalculerCible(path);
    printf("Dossiers à supprimer : %d\n", g_TotalFoldersToDelete);
    Log("Dossier: %d", g_TotalFoldersToDelete);
    printf("Nettoyage en cours... (DryRun=%d)\n", g_DryRun ? 1 : 0);
    ShowProgress();
    AnalyseSuppression(path);
    printf("\n\nTerminé.\n");
    Log("=== Fin ===");
    if (g_Log) fclose(g_Log);
    if (g_hLogMutex) CloseHandle(g_hLogMutex);
    return 0;
}
