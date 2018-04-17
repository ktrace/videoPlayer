#ifndef FMHELPER_HPP
#define FMHELPER_HPP

#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>
#include <QStandardPaths>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

class FM : public QObject
{   Q_OBJECT

    Q_PROPERTY (QString sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY (bool moveMode READ isMoveMode WRITE setMoveMode)
    Q_PROPERTY (bool cpResult READ cpResult NOTIFY cpResultChanged)

    signals:
        void sourceUrlChanged();
        void cpResultChanged();
    private:
        QString m_sourceUrl;
        bool m_moveMode;
        QString m_dataDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        QFutureWatcher<bool> watcher;
        bool m_cpResult;
    private slots:
        void setSourceUrl(const QString &url) { m_sourceUrl = url; emit sourceUrlChanged();}
        void setMoveMode(bool &mode) { m_moveMode = mode;}
    public:
        QString sourceUrl() {return m_sourceUrl;}
        bool isMoveMode() {return m_moveMode;}
        bool cpResult() {return m_cpResult;}
    public slots:
        void remove(const QString &url)
        {    //qDebug() << "Called the C++ slot and request removal of:" << url;
             QFile(url).remove();
        }
        void removeDir(const QString &url)
        {
            QDir(url).removeRecursively();
        }
        QString getHome()
        {    //qDebug() << "Called the C++ slot and request removal of:" << url;
             return QDir::homePath();
        }
        QString getRoot()
        {    //qDebug() << "Called the C++ slot and request removal of:" << url;
             return QDir::rootPath();
        }
        QString data_dir()
        {
            return m_dataDir;
        }
        bool existsPath(const QString &url)
        {
            return QDir(url).exists();
        }
        bool isFile(const QString &url)
        {
            return QFileInfo(url).isFile();
        }
        bool cpFile(const QString &source, const QString &target)
        {
            QFileInfo srcFileInfo(source);
            if (srcFileInfo.isDir()) {
                QDir targetDir(target);
                if (!targetDir.isRoot()) targetDir.cdUp();
                if (!targetDir.mkdir(QFileInfo(target).fileName()))
                    return false;
                QDir sourceDir(source);
                QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
                foreach (const QString &fileName, fileNames) {
                    const QString newSrcFilePath
                            = source + QLatin1Char('/') + fileName;
                    const QString newTgtFilePath
                            = target + QLatin1Char('/') + fileName;
                    if (!copyFile(newSrcFilePath, newTgtFilePath))
                        return false;
                }
            }
            else return QFile(source).copy(target);
        }
        bool copyFile(const QString &source, const QString &target) {
            connect(&watcher, SIGNAL(finished()), this, SLOT(cpFinished()));
            QFuture<bool> future = QtConcurrent::run(this, &FM::cpFile, source, target);
            watcher.setFuture(future);
        }
        bool moveFile(const QString &source, const QString &target)
        {
            if (copyFile(source,target))
            {
                QFileInfo srcFileInfo(source);
                if (srcFileInfo.isDir()) { removeDir(source); }
                else remove(source);
                return true;
            }
            else return false;
        }
        int getSize(const QString &url)
        {
            return QFileInfo(url).size();
        }
        QString getMime(const QString &url)
        {
            QMimeDatabase db;
            QUrl path(url);
            QMimeType mime;

            QRegExp regex(QRegExp("[_\\d\\w\\-\\. ]+\\.[_\\d\\w\\-\\. ]+"));
            QString filename = url.split('/').last();
            int idx = filename.indexOf(regex);

            if(filename.isEmpty() || (idx == -1))
                mime = db.mimeTypeForUrl(path);
            else
                mime = db.mimeTypeForFile(filename.mid(idx, regex.matchedLength()));
            return mime.name();
        }
        void cpFinished()
        {
           m_cpResult = watcher.future().result();
           qDebug() << "m_cpResult = " << m_cpResult;
           emit cpResultChanged();
        }
};


#endif // FMHELPER_HPP
