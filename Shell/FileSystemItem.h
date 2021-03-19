#ifndef FILESYSTEMITEM_H
#define FILESYSTEMITEM_H

#include <QHash>
#include <QIcon>
#include <QString>
#include <QVariant>
#include <QDateTime>

#include <limits>

// Capabilities
#define FSI_CAN_COPY    0x0001
#define FSI_CAN_MOVE    0x0002
#define FSI_CAN_LINK    0x0004
#define FSI_CAN_RENAME  0x0008
#define FSI_CAN_DELETE  0x0010
#define FSI_DROP_TARGET 0x0020

class FileSystemItem
{
public:

    enum DataInfo {
        Name,
        Extension,
        Size,
        Type,
        LastChangeTime,
        MaxColumns
    };

    enum MediaType {
        Unknown,
        Removable,
        Fixed,
        Remote,
        CDROM,
        RamDisk
    };

    FileSystemItem(QString path);
    ~FileSystemItem();

    QString getDisplayName() const;
    void setDisplayName(const QString &value);

    void addChild(FileSystemItem *child);
    FileSystemItem *getChildAt(int n);
    FileSystemItem *getChild(QString path);
    void removeChild(QString path);
    QList<FileSystemItem *> getChildren();
    void removeChildren();
    void updateChildPath(FileSystemItem *child, QString path);

    int childrenCount();
    int childRow(FileSystemItem *child);

    void clear();

    FileSystemItem *getParent() const;
    void setParent(FileSystemItem *value);

    bool getHasSubFolders() const;
    void setHasSubFolders(bool value);

    bool areAllChildrenFetched() const;
    void setAllChildrenFetched(bool value);

    bool isDrive() const;
    bool isInADrive() const;

    QVariant getData(int column);

    QString getPath() const;
    void setPath(const QString &value);

    QIcon getIcon() const;
    void setIcon(const QIcon &value);

    bool isFolder() const;
    void setFolder(bool value);

    bool isHidden() const;
    void setHidden(bool value);

    bool hasFakeIcon() const;
    void setFakeIcon(bool value);

    QString getType() const;
    void setType(const QString &value);

    quint64 getSize() const;
    void setSize(const quint64 &value);

    QString getExtension() const;

    QDateTime getCreationTime() const;
    void setCreationTime(const QDateTime &value);

    QDateTime getLastAccessTime() const;
    void setLastAccessTime(const QDateTime &value);

    QDateTime getLastChangeTime() const;
    void setLastChangeTime(const QDateTime &value);

    FileSystemItem *clone();

    quint16 getCapabilities() const;
    void setCapabilities(const quint16 &value);

    MediaType getMediaType() const;
    void setMediaType(const MediaType &value);

    qint32 getErrorCode() const;
    void setErrorCode(const qint32 &value);

    QString getErrorMessage() const;
    void setErrorMessage(const QString &value);

    bool getLock() const;
    void setLock(bool value);

    inline void incRefCounter()     { refCounter++; }
    inline void decRefCounter()     { refCounter--; }
    inline int  getRefCounter()     { return refCounter; }

private:

    QString     path                {};
    QString     displayName         {};
    QString     extension           {};
    QIcon       icon                {};
    quint64     size                { std::numeric_limits<quint64>::max() };
    QString     type                {};
    QDateTime   creationTime        {};
    QDateTime   lastAccessTime      {};
    QDateTime   lastChangeTime      {};
    quint16     capabilities        {};
    MediaType   mediaType           {};
    qint32      errorCode           {};
    QString     errorMessage        {};
    quint32     refCounter          {};

    bool folder                     {};
    bool hidden                     {};
    bool hasSubFolders              {};
    bool allChildrenFetched         {};
    bool fakeIcon                   {};
    bool lock                       {};

    FileSystemItem *parent          {};

    // Children
    QHash<QString, FileSystemItem *> children;
    QList<FileSystemItem *> indexedChildren;
};

#endif // FILESYSTEMITEM_H
