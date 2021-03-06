#ifndef UNIXFILEINFORETRIEVER_H
#define UNIXFILEINFORETRIEVER_H

#include <experimental/filesystem>

#include "Shell/FileInfoRetriever.h"

namespace fs = std::experimental::filesystem;

class UnixFileInfoRetriever : public FileInfoRetriever
{
public:
    UnixFileInfoRetriever(QObject *parent = nullptr);
    QIcon getIcon(FileSystemItem *item) const override;

protected:
    void getChildrenBackground(FileSystemItem *parent) override;
    bool getParentBackground(FileSystemItem *parent) override;
    void getExtendedInfo(FileSystemItem *parent) override;

private:
    bool hasSubFolders(fs::path path);
};

#endif // UNIXFILEINFORETRIEVER_H
