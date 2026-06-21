#include <windows.h>
#include <shlobj.h>
#include <stdio.h>

const char* Extensions[] =
{
    "tmp",
    "suo",
    "old",
    "log",
    "bak"
};
const int nbrExtension = sizeof(Extensions) / sizeof(Extensions[0]);
const char* Dossiers[] ={".git",".vs","node_modules","Package","bin","obj","backup",".github",".vscode",".history",".idea",".builds"};
const int nbDossier = sizeof(Dossiers) / sizeof(Dossiers[0]);
static int nbTotalDossier = 0;
static int nbDossiersSupprimes = 0;
void ShowProgress()
{
    const int lngBarre = 50;
    float ratio = 0.0f;
    if (nbTotalDossier > 0)
        ratio = (float)nbDossiersSupprimes / (float)nbTotalDossier;
    int filled = (int)(ratio * lngBarre);
    printf("\r[");
    for (int i = 0; i < filled; i++) printf("#");
    for (int j = filled; j < lngBarre; j++) printf(" ");
    printf("] %d / %d", nbDossiersSupprimes, nbTotalDossier);
    fflush(stdout);
}
bool IsTargetExtension(const char* name)
{
    const char* ext = strrchr(name, '.');
    if (!ext) return false;
    ext++;
    for (int i = 0; i < nbrExtension; i++)
    {
        if (!lstrcmpiA(ext, Extensions[i])) return true;
    }
    return false;
}
bool DossierCibleValide(const char* name)
{
    for (int i = 0; i < nbDossier; i++)
    {
        if (!lstrcmpiA(name, Dossiers[i]))return true;
    }
    return false;
}
bool SuppressionDossierRecursive(const char* path)
{
    char chemin[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    wsprintfA(chemin, "%s\\*.*", path);
    h = FindFirstFileA(chemin, &fd);
    if (h == INVALID_HANDLE_VALUE)return FALSE;
    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, ".."))continue;
        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", path, fd.cFileName);
        SetFileAttributesA(fullPath, FILE_ATTRIBUTE_NORMAL);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            SuppressionDossierRecursive(fullPath);
            RemoveDirectoryA(fullPath);
        }
        else{DeleteFileA(fullPath);}
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    return RemoveDirectoryA(path);
}
void CountTargets(const char* root)
{
    char chemin[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    wsprintfA(chemin, "%s\\*.*", root);
    h = FindFirstFileA(chemin, &fd);
    if (h == INVALID_HANDLE_VALUE)return;
    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, ".."))continue;
        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", root, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (DossierCibleValide(fd.cFileName)){nbTotalDossier++;}
            else{CountTargets(fullPath);}
        }

    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
void ScanAndDelete(const char* root)
{
    char chemin[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    wsprintfA(chemin, "%s\\*.*", root);
    h = FindFirstFileA(chemin, &fd);
    if (h == INVALID_HANDLE_VALUE)return;
    do
    {
        if (!lstrcmpA(fd.cFileName, ".") || !lstrcmpA(fd.cFileName, ".."))continue;
        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", root, fd.cFileName);
        SetFileAttributesA(fullPath, FILE_ATTRIBUTE_NORMAL);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (DossierCibleValide(fd.cFileName))
            {
                printf("\nSuppression dossier : %s\n", fullPath);
                SuppressionDossierRecursive(fullPath);
                nbDossiersSupprimes++;
                ShowProgress();
            }
            else{ScanAndDelete(fullPath);}
        }
        else
        {
            if (IsTargetExtension(fd.cFileName) ||(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                printf("Fichier supprime : %s\n", fullPath);
                DeleteFileA(fullPath);
            }
        }

    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
bool BrowseForFolder(char* outPath)
{
    BROWSEINFOA bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.lpszTitle = "Selectionnez le dossier à nettoyer";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN|BIF_STATUSTEXT|BIF_EDITBOX|BIF_VALIDATE|BIF_BROWSEFORCOMPUTER;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (!pidl)return false;
    BOOL ok = SHGetPathFromIDListA(pidl, outPath);
    CoTaskMemFree(pidl);
    return ok ? true : false;
}
int main()
{
    char path[MAX_PATH];
    if (!BrowseForFolder(path)){printf("Aucun dossier selectionne.\n");return 0;}
    printf("\nDossier selectionne : %s\n", path);
    printf("\nAnalyse des dossiers...\n");
    CountTargets(path);
    printf("Dossiers à supprimer : %d\n", nbTotalDossier);
    printf("Nettoyage en cours...\n");
    ShowProgress();
    ScanAndDelete(path);
    printf("\n\nTermine.\n");
    return 0;
}
