// DeleteGitAndVs.cpp 
#include <windows.h>
#include <stdio.h>
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

static int g_TotalFoldersToDelete = 0;
static int g_DeletedFolders = 0;
void ShowProgress()
{
    const int barWidth = 50;
    float ratio = 0.0f;

    if (g_TotalFoldersToDelete > 0)
        ratio = (float)g_DeletedFolders / (float)g_TotalFoldersToDelete;

    int filled = (int)(ratio * barWidth);

    printf("\r[");
    for (int i = 0; i < filled; i++) printf("#");
    for (int j = filled; j < barWidth; j++) printf(" ");
    printf("] %d / %d", g_DeletedFolders, g_TotalFoldersToDelete);
    fflush(stdout);
}
void CountTargets(const char* root)
{
    char searchPath[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    wsprintfA(searchPath, "%s\\*.*", root);
    h = FindFirstFileA(searchPath, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, ".."))
            continue;

        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", root, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!lstrcmpiA(fd.cFileName, ".git") ||
                !lstrcmpiA(fd.cFileName, ".vs"))
            {
                g_TotalFoldersToDelete++;
				printf("%.5d\t%s\n",g_TotalFoldersToDelete,fullPath);
            }
            else
            {
                CountTargets(fullPath);
            }
        }

    } while (FindNextFileA(h, &fd));

    FindClose(h);
}
bool DeleteDirectoryRecursive(const char* path)
{
    char searchPath[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;

    wsprintfA(searchPath, "%s\\*.*", path);
    h = FindFirstFileA(searchPath, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, ".."))
            continue;

        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", path, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            DeleteDirectoryRecursive(fullPath);
            RemoveDirectoryA(fullPath);
        }
        else
        {
            SetFileAttributesA(fullPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(fullPath);
        }

    } while (FindNextFileA(h, &fd));

    FindClose(h);

    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    return RemoveDirectoryA(path);
}
void ScanAndDelete(const char* root)
{
    char searchPath[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;

    wsprintfA(searchPath, "%s\\*.*", root);
    h = FindFirstFileA(searchPath, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, ".."))
            continue;

        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", root, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!lstrcmpiA(fd.cFileName, ".git") ||
                !lstrcmpiA(fd.cFileName, ".vs"))
            {
                printf("\nSuppression : %s\n", fullPath);

                DeleteDirectoryRecursive(fullPath);

                g_DeletedFolders++;
                ShowProgress();
            }
            else
            {
                ScanAndDelete(fullPath);
            }
        }

    } while (FindNextFileA(h, &fd));

    FindClose(h);
}
int main()
{
    char path[MAX_PATH];

    printf("Chemin racine a nettoyer : ");
    gets(path);

    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        printf("Chemin invalide.\n");
        return 1;
    }

    printf("\nAnalyse des dossiers...\n");
    CountTargets(path);

    printf("Dossiers à supprimer : %d\n", g_TotalFoldersToDelete);
    printf("Nettoyage en cours...\n");

    ShowProgress();
    ScanAndDelete(path);

    printf("\n\nTerminé.\n");
    return 0;
}
