#ifndef UNIXFILEINFORETRIEVER_H
#define UNIXFILEINFORETRIEVER_H

#include <experimental/filesystem>

#include "fileinforetriever.h"

namespace fs = std::experimental::filesystem;

class UnixFileInfoRetriever : public FileInfoRetriever
{
public:
    UnixFileInfoRetriever(QObject *parent = nullptr);
    QIcon getIcon(FileSystemItem *item) const override;

protected:
    void getChildrenBackground(FileSystemItem *parent) override;
    void getParentInfo(FileSystemItem *parent) override;

private:
    bool hasSubFolders(fs::path path);
};

#endif // UNIXFILEINFORETRIEVER_H
