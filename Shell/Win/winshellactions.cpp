#include <qt_windows.h>
#include <initguid.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <QDebug>

#include "winshellactions.h"

WinShellActions::WinShellActions(QObject *parent) : ShellActions(parent)
{
}

void WinShellActions::renameItemBackground(QUrl srcPath, QString newName)
{
    qDebug() << "WinShellActions::renameItem";
    // Initialize COM as STA.
    if (SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE))) {

        IFileOperation *pfo {nullptr};

        // ToDo: Request elevation through pfo->SetOperationFlags

        // Create the IFileOperation interface
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_IFileOperation, reinterpret_cast<void**>(&pfo)))) {

            // Create an IShellItem from the supplied source path.
            IShellItem *psiFrom {nullptr};

            QString source = srcPath.toLocalFile().replace('/','\\');
            if (SUCCEEDED(SHCreateItemFromParsingName(source.toStdWString().c_str(), nullptr, IID_PPV_ARGS(&psiFrom)))) {

                pfo->RenameItem(psiFrom, newName.toStdWString().c_str(), nullptr);
                psiFrom->Release();
            }

            // Perform operations
            pfo->PerformOperations();

            // Release the IFileOperation interface.
            pfo->Release();
        }
        CoUninitialize();
    }
    running.store(false);
}

void WinShellActions::copyItemsBackground(QList<QUrl> srcUrls, QString dstPath)
{
    performFileOperations(srcUrls, dstPath, true);
}

void WinShellActions::moveItemsBackground(QList<QUrl> srcUrls, QString dstPath)
{
    performFileOperations(srcUrls, dstPath, false);
}

void WinShellActions::performFileOperations(QList<QUrl> srcUrls, QString dstPath, bool copy)
{
    qDebug() << "WinShellActions::copyItemsBackground";
    // Initialize COM as STA.
    if (SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE))) {

        IFileOperation *pfo {nullptr};

        // ToDo: Request elevation through pfo->SetOperationFlags

        // Create the IFileOperation interface
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_IFileOperation, reinterpret_cast<void**>(&pfo)))) {

            for (QUrl srcUrl : srcUrls) {
                // Create an IShellItem from the supplied source path.
                IShellItem *psiFrom {nullptr};

                QString source = srcUrl.toLocalFile().replace('/','\\');
                if (SUCCEEDED(SHCreateItemFromParsingName(source.toStdWString().c_str(), nullptr, IID_PPV_ARGS(&psiFrom)))) {
                    IShellItem *psiTo {nullptr};

                    // Create an IShellItem from the supplied destination path.
                    if (SUCCEEDED(SHCreateItemFromParsingName(dstPath.toStdWString().c_str(), nullptr, IID_PPV_ARGS(&psiTo)))) {

                        // Add the operation
                        if (copy)
                            pfo->CopyItem(psiFrom, psiTo, nullptr, nullptr);
                        else
                            pfo->MoveItem(psiFrom, psiTo, nullptr, nullptr);

                        if (psiTo != nullptr)
                            psiTo->Release();
                    }
                    psiFrom->Release();
                }
            }

            // Perform operations
            pfo->PerformOperations();

            // Release the IFileOperation interface.
            pfo->Release();
        }

        CoUninitialize();
    }
    running.store(false);
}

void WinShellActions::linkItemsBackground(QList<QUrl> srcUrls, QString dstPath)
{
    qDebug() << "WinShellActions::linkItemsBackground";

    // Initialize COM as STA.
    if (SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE))) {

        IShellLinkW *sl {nullptr};

        // Create the IShellLinkW interface
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_ALL, IID_IShellLinkW, reinterpret_cast<void**>(&sl)))) {

            IPersistFile *pf {nullptr};
            if (SUCCEEDED(sl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pf)))) {
                for (QUrl srcUrl : srcUrls) {
                    QString source = srcUrl.toLocalFile().replace('/','\\');
                    QString sourceDir = source.left(source.lastIndexOf('\\'));

                    QString linkName = getUniqueLinkName(source, dstPath);

                    sl->SetWorkingDirectory(sourceDir.toStdWString().c_str());
                    sl->SetPath(source.toStdWString().c_str());
                    pf->Save(linkName.toStdWString().c_str(), FALSE);
                }
                pf->Release();
            }
            sl->Release();
        }
        CoUninitialize();
    }
    running.store(false);
}

QString WinShellActions::getUniqueLinkName(QString linkTo, QString destDir)
{
    wchar_t pszName[MAX_PATH];
    BOOL pfMustCopy;

    std::wstring wstrPszLinkTo = linkTo.toStdWString();
    std::wstring wstrPszDir = destDir.toStdWString();

    LPCWSTR pszLinkTo = wstrPszLinkTo.c_str();
    LPCWSTR pszDir = wstrPszDir.c_str();
    if (::SHGetNewLinkInfoW(pszLinkTo, pszDir, pszName, &pfMustCopy, SHGNLI_PREFIXNAME | SHGNLI_NOUNIQUE)) {

        // The SHGetNewLinkInfoW with the SHGNLI_NOUNIQUE flag will return something like "File.txt - Shortcut ().lnk"
        // We want to remove the extension if any and the empty round brackets ()

        QString linkName = destDir + (destDir.right(1) == '\\' ? "" : "\\") + QString::fromWCharArray(pszName).remove(QRegExp(" \\(\\).lnk$")) + ".lnk";
        qDebug() << "Original name" << linkName;

        // Check if the original file has an extension
        int i;
        if ((i = linkTo.lastIndexOf('.')) >= 0) {

            // Remove the extension from the shortcut name
            QString fileNameWithExt = linkTo.right(linkTo.size() - (linkTo.lastIndexOf('\\') + 1));
            QString fileName = fileNameWithExt.left(fileNameWithExt.lastIndexOf('.'));
            linkName = linkName.replace(fileNameWithExt, fileName);
        }

        // Loop until we find a unique name
        int attempt = 2;
        bool found = false;
        do {
            DWORD dwAttrib = GetFileAttributes(linkName.toStdWString().c_str());
            if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
                int pos = linkName.indexOf(QRegExp(" \\([0-9]+\\).lnk$"));
                if (pos > 0) {
                    QString number = linkName.right(linkName.length() - pos).remove(QRegExp(".lnk$")).remove(QRegExp("[ ()]"));
                    linkName = linkName.left(pos);
                    attempt = number.toInt() + 1;
                } else {
                    linkName = linkName.remove(QRegExp(".lnk$"));
                }
                linkName += " (" + QString::number(attempt) + ").lnk";
            } else
                found = true;

        } while (!found);

        return linkName;
    }

    return QString();
}
