#include "i.H"
class Settings_Backend;

enum class DownloadCondition {
    Queued,
    Downloading,  
    Paused,
    Finished,
    Cancelled_Failed
};

inline QString conditionToString(DownloadCondition condition) {
    switch (condition) {
    case DownloadCondition::Queued: return "queued";
    case DownloadCondition::Downloading: return "downloading";
    case DownloadCondition::Paused: return "paused";
    case DownloadCondition::Finished: return "finished";
    case DownloadCondition::Cancelled_Failed: return "cancelled_failed";
    default: return "cancelled_failed";
    }
}

inline DownloadCondition stringToCondition(const QString &str) {
    if (str == "queued") return DownloadCondition::Queued;
    if (str == "paused") return DownloadCondition::Paused;
    if (str == "finished") return DownloadCondition::Finished;
    if (str == "cancelled_failed") return DownloadCondition::Cancelled_Failed;
    return DownloadCondition::Cancelled_Failed;  
}


class Qtdl : public QObject {
    Q_OBJECT

private:
    
    struct SegmentInfo {
        int id;
        QUrl url;
        QString cachePath;  
        qint64 startByte;
        qint64 endByte;
        qint64 currentByte;
        qint64 bytesReceived;
        QNetworkAccessManager* manager;
        QNetworkReply* reply;
        std::unique_ptr<QFile> file;
        bool isActive;
        mutable QMutex mutex;

        SegmentInfo(int segId, const QUrl& u, const QString& cache, qint64 start, qint64 end)
            : id(segId), url(u), cachePath(cache), startByte(start), endByte(end)
            , currentByte(start), bytesReceived(0), manager(nullptr)
            , reply(nullptr), isActive(false) {}

        ~SegmentInfo() {
            cleanup();
        }

        void cleanup() {
            if (reply) {
                reply->disconnect();
                reply->abort();
                reply->deleteLater();
                reply = nullptr;
            }
            QMutexLocker locker(&mutex);
            file.reset();
            if (manager) {
                delete manager;
                manager = nullptr;
            }
        }

        bool isComplete() const {
            QMutexLocker locker(&mutex);
            return endByte > 0 && currentByte >= endByte;
        }

        qint64 getBytesDownloaded() const {
            QMutexLocker locker(&mutex);
            return bytesReceived;
        }
    };

public:
    explicit Qtdl(int numSegments = 8) : QObject(),
        m_manager(new QNetworkAccessManager()),
        m_activeReply(nullptr),
        m_bytesReceived(0),
        m_bytesTotal(-1),
        m_isDownloading(false),
        m_cancelled(false),
        m_paused(false),
        m_numSegments(qBound(1, numSegments, 32)),
        m_supportsRangeRequests(false),
        m_isUnknownLength(false),
        m_useSegmented(false) {
        m_manager->moveToThread(&m_thread);
        moveToThread(&m_thread);
        m_thread.start();
    }

    ~Qtdl() override {
        if (QThread::currentThread() == thread()) {
            forceCleanup();
        } else {
            QMetaObject::invokeMethod(this, &Qtdl::forceCleanup, Qt::QueuedConnection);
            m_thread.quit();
            m_thread.wait(3000);
        }
        if (m_manager) {
            m_manager->deleteLater();
            m_manager = nullptr;
        }
    }

    void startDownload(const QUrl &url, const QString &finalPath, const QString &cacheDir) {
        QMetaObject::invokeMethod(this, [this, url, finalPath, cacheDir]() {
            doDownload(url, finalPath, cacheDir);
        }, Qt::QueuedConnection);
    }

    void pauseDownload() {
        if (QThread::currentThread() == thread()) {
            doPause();
        } else {
            QMetaObject::invokeMethod(this, &Qtdl::doPause, Qt::QueuedConnection);
        }
    }

    void resumeDownload() {
        if (QThread::currentThread() == thread()) {
            doResume();
        } else {
            QMetaObject::invokeMethod(this, &Qtdl::doResume, Qt::QueuedConnection);
        }
    }

    void cancelDownload() {
        if (QThread::currentThread() == thread()) {
            doCancel();
        } else {
            QMetaObject::invokeMethod(this, &Qtdl::doCancel, Qt::QueuedConnection);
        }
    }

    QStringList getSegmentPaths() const {
        QMutexLocker locker(&m_mutex);
        QStringList paths;
        for (const auto& seg : m_segments) {
            if (seg) {
                paths.append(seg->cachePath);
            }
        }
        return paths;
    }

    bool isDownloading() const { return m_isDownloading.load(); }
    bool isCancelled() const { return m_cancelled.load(); }
    bool isPaused() const { return m_paused.load(); }
    bool isUnknownLength() const { return m_isUnknownLength.load(); }

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal, double speedKBps);
    void downloadFinished(const QString &finalPath, bool success, const QString &error);
    void downloadPaused(const QString &finalPath);
    void downloadResumed(const QString &finalPath);

private slots:
    void doDownload(const QUrl &url, const QString &finalPath, const QString &cacheDir) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_isDownloading) cleanupInternal();

            m_isDownloading = true;
            m_cancelled = false;
            m_paused = false;
            m_currentFinalPath = finalPath;
            m_currentCacheDir = cacheDir;
            m_currentUrl = url;
            m_bytesReceived = 0;
            m_bytesTotal = -1;
            m_supportsRangeRequests = false;
            m_isUnknownLength = false;
            m_useSegmented = false;
        }

        
        QDir().mkpath(cacheDir);
        QDir().mkpath(QFileInfo(finalPath).absolutePath());

        checkServerCapabilities(url, finalPath, cacheDir);
    }

    void checkServerCapabilities(const QUrl &url, const QString &finalPath, const QString &cacheDir) {
        QNetworkRequest request(url);
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 onuBrowser/1.1");

        QNetworkReply* reply = m_manager->head(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, url, finalPath, cacheDir]() {
            QMutexLocker locker(&m_mutex);

            if (m_cancelled || m_paused) {
                reply->deleteLater();
                return;
            }

            if (reply->error() != QNetworkReply::NoError) {
                locker.unlock();
                reply->deleteLater();
                startSingleDownload(url, finalPath, cacheDir);
                return;
            }

            
            if (reply->hasRawHeader("Content-Length")) {
                m_bytesTotal = reply->rawHeader("Content-Length").toLongLong();
            } else {
                m_bytesTotal = -1;
                m_isUnknownLength = true;
            }

            
            if (reply->hasRawHeader("Accept-Ranges")) {
                QString acceptRanges = reply->rawHeader("Accept-Ranges").toLower();
                m_supportsRangeRequests = (acceptRanges == "bytes");
            }

            
            if (reply->hasRawHeader("Transfer-Encoding")) {
                QString transferEncoding = reply->rawHeader("Transfer-Encoding").toLower();
                if (transferEncoding.contains("chunked")) {
                    m_supportsRangeRequests = false;
                    m_bytesTotal = -1;
                    m_isUnknownLength = true;
                }
            }

            bool useSegmented = m_supportsRangeRequests && m_bytesTotal > 0 && m_numSegments > 1;

            locker.unlock();
            reply->deleteLater();

            if (useSegmented) {
                startSegmentedDownload(url, finalPath, cacheDir);
            } else {
                startSingleDownload(url, finalPath, cacheDir);
            }
        });
    }

    void startSegmentedDownload(const QUrl &url, const QString &finalPath, const QString &cacheDir) {
        QMutexLocker locker(&m_mutex);

        if (m_cancelled || m_paused) return;

        qint64 totalBytes = m_bytesTotal;
        if (totalBytes <= 0) {
            locker.unlock();
            startSingleDownload(url, finalPath, cacheDir);
            return;
        }

        qint64 segmentSize = totalBytes / m_numSegments;

        m_segments.clear();

        QString baseFileName = QFileInfo(finalPath).fileName();

        for (int i = 0; i < m_numSegments; ++i) {
            qint64 startByte = i * segmentSize;
            qint64 endByte = (i == m_numSegments - 1) ? totalBytes - 1 : (startByte + segmentSize - 1);
            QString cachePath = cacheDir + "/" + baseFileName + QString(".part%1").arg(i);

            m_segments.push_back(std::make_unique<SegmentInfo>(i, url, cachePath, startByte, endByte));
        }

        m_useSegmented = true;
        locker.unlock();

        for (auto& seg : m_segments) {
            startSegment(seg.get());
        }

        m_timer.start();
    }

    void startSegment(SegmentInfo* seg) {
        if (!seg) return;

        QMutexLocker locker(&seg->mutex);

        if (seg->isActive) return;
        seg->isActive = true;

        if (seg->currentByte >= seg->endByte && seg->endByte > 0) {
            return;
        }

        seg->file = std::make_unique<QFile>(seg->cachePath);

        QIODevice::OpenMode mode = (seg->file->exists() && seg->currentByte > seg->startByte)
                                       ? QIODevice::Append : QIODevice::WriteOnly;

        if (seg->file->isOpen()) seg->file->close();

        if (!seg->file->open(mode)) {
            return;
        }

        if (!seg->manager) {
            seg->manager = new QNetworkAccessManager();
            seg->manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
        }

        QNetworkRequest request(seg->url);
        QString rangeHeader = QString("bytes=%1-%2").arg(seg->currentByte).arg(seg->endByte);
        request.setRawHeader("Range", rangeHeader.toLatin1());
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 onuBrowser/1.1");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setTransferTimeout(60000);

        seg->reply = seg->manager->get(request);
        if (!seg->reply) return;

        connect(seg->reply, &QNetworkReply::readyRead, this, [this, seg]() {
            onSegmentReadyRead(seg);
        });
        connect(seg->reply, &QNetworkReply::finished, this, [this, seg]() {
            onSegmentFinished(seg);
        });
        connect(seg->reply, &QNetworkReply::downloadProgress, this, [this, seg](qint64 r, qint64 t) {
            Q_UNUSED(r); Q_UNUSED(t);
            onSegmentProgress();
        });
    }

    void onSegmentReadyRead(SegmentInfo* seg) {
        if (!seg || !seg->file || !seg->reply) return;

        QMutexLocker locker(&seg->mutex);
        if (!seg->isActive) return;

        QByteArray data = seg->reply->readAll();
        if (!data.isEmpty()) {
            qint64 written = seg->file->write(data);
            if (written == data.size()) {
                seg->currentByte += written;
                seg->bytesReceived += written;
                seg->file->flush();
            }
        }
    }

    void onSegmentProgress() {
        QMutexLocker locker(&m_mutex);
        if (m_paused || m_cancelled) return;

        qint64 total = 0;
        for (const auto& seg : m_segments) {
            if (seg) total += seg->getBytesDownloaded();
        }

        qint64 elapsed = m_timer.elapsed();
        double speedKBps = (elapsed > 100) ? (total / 1024.0) / (elapsed / 1000.0) : 0.0;

        locker.unlock();
        emit progress(total, m_bytesTotal, speedKBps);
    }

    void onSegmentFinished(SegmentInfo* seg) {
        if (!seg || !seg->reply) return;

        QMutexLocker segLocker(&seg->mutex);

        QNetworkReply::NetworkError error = seg->reply->error();

        if (error == QNetworkReply::OperationCanceledError) {
            return;
        }

        if (error != QNetworkReply::NoError) {
            segLocker.unlock();
            QMutexLocker locker(&m_mutex);
            m_isDownloading = false;
            QString finalPath = m_currentFinalPath;
            QString errorMsg = seg->reply->errorString();
            locker.unlock();

            emit downloadFinished(finalPath, false, QString("Segment %1: %2").arg(seg->id).arg(errorMsg));
            return;
        }

        if (seg->file) {
            seg->file->flush();
            seg->file->close();
        }

        segLocker.unlock();

        bool allComplete = true;
        for (const auto& s : m_segments) {
            if (s && !s->isComplete()) {
                allComplete = false;
                break;
            }
        }

        if (allComplete) {
            mergeSegments();
        }
    }

    void mergeSegments() {
        QMutexLocker locker(&m_mutex);

        QString finalPath = m_currentFinalPath;
        bool cancelled = m_cancelled;
        bool paused = m_paused;

        locker.unlock();

        if (cancelled || paused) return;

        QFile finalFile(finalPath);
        if (!finalFile.open(QIODevice::WriteOnly)) {
            emit downloadFinished(finalPath, false, "Cannot open final file: " + finalFile.errorString());
            return;
        }

        bool success = true;
        for (const auto& seg : m_segments) {
            if (!seg) continue;

            QFile tempFile(seg->cachePath);
            if (!tempFile.open(QIODevice::ReadOnly)) {
                success = false;
                break;
            }

            char buffer[8192];
            qint64 bytesRead;
            while ((bytesRead = tempFile.read(buffer, sizeof(buffer))) > 0) {
                if (finalFile.write(buffer, bytesRead) != bytesRead) {
                    success = false;
                    break;
                }
            }

            tempFile.close();
            if (success) tempFile.remove();  

            if (!success) break;
        }

        finalFile.flush();
        finalFile.close();

        QMutexLocker locker2(&m_mutex);
        m_isDownloading = false;
        locker2.unlock();

        if (success) {
            emit downloadFinished(finalPath, true, QString());
        } else {
            emit downloadFinished(finalPath, false, "Failed to merge segments");
        }
    }

    void startSingleDownload(const QUrl &url, const QString &finalPath, const QString &cacheDir) {
        Q_UNUSED(cacheDir);

        QMutexLocker locker(&m_mutex);

        if (m_cancelled || m_paused) return;

        m_file = std::make_unique<QFile>(finalPath);
        bool resuming = m_file->exists() && m_file->size() > 0;
        qint64 existingBytes = resuming ? m_file->size() : 0;

        if (!m_file->open(resuming ? QIODevice::Append : QIODevice::WriteOnly)) {
            QString errorMsg = "Cannot open file: " + m_file->errorString();
            m_file.reset();
            m_isDownloading = false;
            locker.unlock();
            emit downloadFinished(finalPath, false, errorMsg);
            return;
        }

        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 onuBrowser/1.1");
        request.setTransferTimeout(60000);

        if (resuming && m_supportsRangeRequests) {
            QString rangeHeader = QString("bytes=%1-").arg(existingBytes);
            request.setRawHeader("Range", rangeHeader.toLatin1());
            m_bytesReceived = existingBytes;
        }

        m_activeReply = m_manager->get(request);

        if (!m_activeReply) {
            m_file.reset();
            m_isDownloading = false;
            locker.unlock();
            emit downloadFinished(finalPath, false, "Failed to create network request");
            return;
        }

        connect(m_activeReply, &QNetworkReply::readyRead, this, &Qtdl::onReadyRead);
        connect(m_activeReply, &QNetworkReply::downloadProgress, this, &Qtdl::onDownloadProgress);
        connect(m_activeReply, &QNetworkReply::finished, this, &Qtdl::onFinished);
        m_timer.start();
    }

    void onReadyRead() {
        if (!m_activeReply || !m_file) return;

        QMutexLocker locker(&m_mutex);
        if (m_paused || m_cancelled) return;
        locker.unlock();

        QByteArray data = m_activeReply->readAll();
        if (data.isEmpty()) return;
        m_file->write(data);
        m_file->flush();
    }

    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
        QMutexLocker locker(&m_mutex);
        if (m_paused || m_cancelled) return;

        qint64 totalReceived = m_bytesReceived + bytesReceived;
        m_bytesTotal = (bytesTotal > 0) ? bytesTotal : -1;

        qint64 elapsed = m_timer.elapsed();
        double speedKBps = (elapsed > 100) ? (totalReceived / 1024.0) / (elapsed / 1000.0) : 0.0;
        locker.unlock();

        emit progress(totalReceived, m_bytesTotal, speedKBps);
    }

    void onFinished() {
        QNetworkReply *reply = nullptr;
        QString finalPath;
        bool cancelled = false;
        bool paused = false;
        bool success = false;
        QString errorString;

        {
            QMutexLocker locker(&m_mutex);
            if (!m_activeReply) return;

            reply = m_activeReply;
            QNetworkReply::NetworkError netError = reply->error();

            cancelled = m_cancelled.load();
            paused = m_paused.load();
            finalPath = m_currentFinalPath;

            if (paused) {
                m_isDownloading = false;
                m_activeReply = nullptr;
                locker.unlock();

                if (m_file) {
                    m_file->flush();
                    m_file->close();
                }
                if (reply) {
                    reply->disconnect();
                    reply->deleteLater();
                }

                emit downloadPaused(finalPath);
                return;
            }

            if (netError == QNetworkReply::OperationCanceledError || cancelled) {
                success = false;
                errorString = "Cancelled by user";
            } else {
                success = (netError == QNetworkReply::NoError);
                errorString = success ? QString() : reply->errorString();
            }

            m_isDownloading = false;
            m_activeReply = nullptr;
        }

        if (m_file) {
            m_file->flush();
            m_file->close();
        }

        if (cancelled && !paused) {
            QFile::remove(finalPath);
        }

        if (reply) {
            reply->disconnect();
            reply->deleteLater();
        }

        emit downloadFinished(finalPath, success, errorString);
    }

    void doPause() {
        QNetworkReply *reply = nullptr;

        {
            QMutexLocker locker(&m_mutex);
            if (!m_isDownloading || m_paused) return;

            m_paused = true;
            reply = m_activeReply;
        }

        for (auto& seg : m_segments) {
            if (seg) {
                QMutexLocker segLock(&seg->mutex);
                seg->isActive = false;
                if (seg->file) {
                    seg->file->flush();
                    seg->file->close();
                }
                segLock.unlock();

                if (seg->reply) {
                    seg->reply->disconnect();
                    seg->reply->abort();
                    seg->reply->deleteLater();
                    seg->reply = nullptr;
                }
            }
        }

        if (reply) {
            reply->abort();
        }
    }

    void doResume() {
        QMutexLocker locker(&m_mutex);
        if (!m_paused) return;

        QUrl url = m_currentUrl;
        QString finalPath = m_currentFinalPath;
        QString cacheDir = m_currentCacheDir;
        bool isUnknown = m_isUnknownLength;
        bool supportsRange = m_supportsRangeRequests;

        m_paused = false;

        if (isUnknown || !supportsRange) {
            cleanupTempFiles();
            m_bytesReceived = 0;
            m_segments.clear();
        }

        locker.unlock();

        doDownload(url, finalPath, cacheDir);
        emit downloadResumed(finalPath);
    }

    void doCancel() {
        QNetworkReply *reply = nullptr;

        {
            QMutexLocker locker(&m_mutex);
            if (!m_isDownloading) return;

            m_cancelled = true;
            m_paused = false;
            reply = m_activeReply;
        }

        for (auto& seg : m_segments) {
            if (seg) {
                QMutexLocker segLock(&seg->mutex);
                seg->isActive = false;
                segLock.unlock();

                if (seg->reply) {
                    seg->reply->disconnect();
                    seg->reply->abort();
                    seg->reply->deleteLater();
                    seg->reply = nullptr;
                }
            }
        }

        if (reply) {
            reply->abort();
        }

        cleanupTempFiles();
    }

    void forceCleanup() {
        QNetworkReply *reply = nullptr;
        std::unique_ptr<QFile> file;

        {
            QMutexLocker locker(&m_mutex);
            m_isDownloading = false;
            m_cancelled = true;
            m_paused = false;

            reply = m_activeReply;
            m_activeReply = nullptr;
            file = std::move(m_file);
        }

        for (auto& seg : m_segments) {
            if (seg) seg->cleanup();
        }
        m_segments.clear();

        if (reply) {
            reply->disconnect();
            reply->abort();
            reply->deleteLater();
        }
        if (file && file->isOpen()) {
            file->flush();
            file->close();
        }

        cleanupTempFiles();
    }

private:
    void cleanupInternal() {
        if (m_activeReply) {
            m_activeReply->disconnect();
            m_activeReply->deleteLater();
            m_activeReply = nullptr;
        }
        if (m_file && m_file->isOpen()) {
            m_file->flush();
            m_file->close();
        }

        for (auto& seg : m_segments) {
            if (seg) seg->cleanup();
        }
        m_segments.clear();
    }

    void cleanupTempFiles() {
        for (const auto& seg : m_segments) {
            if (seg) {
                QFile::remove(seg->cachePath);
            }
        }
    }

    QNetworkAccessManager *m_manager;
    QNetworkReply *m_activeReply;
    std::unique_ptr<QFile> m_file;
    QString m_currentFinalPath;
    QString m_currentCacheDir;
    QUrl m_currentUrl;
    QElapsedTimer m_timer;
    QThread m_thread;
    mutable QMutex m_mutex;
    std::atomic<qint64> m_bytesReceived;
    std::atomic<qint64> m_bytesTotal;
    std::atomic<bool> m_isDownloading;
    std::atomic<bool> m_cancelled;
    std::atomic<bool> m_paused;

    int m_numSegments;
    bool m_supportsRangeRequests;
    std::atomic<bool> m_isUnknownLength;
    bool m_useSegmented;
    std::vector<std::unique_ptr<SegmentInfo>> m_segments;
};


class DL_Man : public QDialog {
    Q_OBJECT

public:
    struct DownloadItem {
        std::unique_ptr<Qtdl> downloader;
        QListWidgetItem *listItem;
        QWidget *widget;
        QLabel *iconLabel;
        QLabel *fileLabel;
        QLabel *statusLabel;
        QLabel *ageLabel;
        QProgressBar *progressBar;
        QPushButton *actionBtn;
        QPushButton *openBtn;
        QString fileName;
        QString finalPath;  
        QUrl downloadUrl;
        DownloadCondition condition;
        std::atomic<bool> fileExists{true};
        QDateTime timestamp;

        DownloadItem() : listItem(nullptr), widget(nullptr), iconLabel(nullptr),
            fileLabel(nullptr), statusLabel(nullptr), ageLabel(nullptr),
            progressBar(nullptr), actionBtn(nullptr), openBtn(nullptr),
            condition(DownloadCondition::Queued) {}
    };

    explicit DL_Man(QWidget *parent = nullptr) : QDialog(parent), m_defaultSegments(8), m_initialized(false) {
        setWindowTitle("Downloads");
        setWindowIcon(QIcon::fromTheme(QStringLiteral("folder-download")));
        resize(750, 500);

        m_fileWatcher = new QFileSystemWatcher(this);
        connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
                this, &DL_Man::onFileChanged);

        setupUI();

        
        checkAndLoadIndex();

        
        updateEmptyState();
    }

    ~DL_Man() override {
        QMutexLocker locker(&m_mutex);
        for (auto &item : m_downloads) {
            if (item && item->downloader) {
                if (item->condition == DownloadCondition::Downloading) {
                    item->downloader->cancelDownload();
                    item->condition = DownloadCondition::Cancelled_Failed;
                }
            }
        }
        saveIndex();
        m_downloads.clear();
        m_itemMap.clear();
        if (m_listWidget) m_listWidget->clear();
    }

    void addDownload(const QUrl &url, const QString &fileName) {
        if (url.isEmpty() || fileName.isEmpty()) return;

        QMetaObject::invokeMethod(this, [this, url, fileName]() {
            show();
            raise();
            activateWindow();

            
            if (!m_initialized) {
                initializeEnvironment();
            }

            auto &settings = Settings_Backend::instance();
            QString downloadRoot = settings.downloadPath();
            if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
            if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();

            QString relativePath;
            QString absolutePath;

            if (settings.smartPath()) {
                
                relativePath = applySmartPath(fileName);
                absolutePath = downloadRoot + "/" + relativePath;
            } else {
                
                QString uniqueName = generateUniqueFileName(downloadRoot, fileName);
                absolutePath = uniqueName;
                
                relativePath = QFileInfo(absolutePath).fileName();
            }

            QString cacheDir = downloadRoot + "/.dlcache";

            Qtdl *downloaderPtr = nullptr;
            DownloadItem *rawPtr = nullptr;
            {
                QMutexLocker locker(&m_mutex);
                if (!m_listWidget) return;

                auto item = std::make_unique<DownloadItem>();
                item->downloader = std::make_unique<Qtdl>(m_defaultSegments);
                downloaderPtr = item->downloader.get();
                rawPtr = item.get();
                item->fileName = fileName;
                item->finalPath = relativePath;  
                item->downloadUrl = url;
                item->condition = DownloadCondition::Downloading;
                item->timestamp = QDateTime::currentDateTime();

                QWidget *itemWidget = createDownloadWidget(item.get(), false);
                if (!itemWidget) return;

                QListWidgetItem *listItem = new QListWidgetItem();
                listItem->setSizeHint(itemWidget->sizeHint());
                insertItemInOrder(item.get(), listItem, itemWidget);
                item->listItem = listItem;
                item->widget = itemWidget;

                connect(downloaderPtr, &Qtdl::progress, this,
                        [this, downloaderPtr](qint64 r, qint64 t, double s) {
                            updateDownloadProgress(downloaderPtr, r, t, s);
                        }, Qt::QueuedConnection);
                connect(downloaderPtr, &Qtdl::downloadFinished, this,
                        [this, downloaderPtr](const QString &p, bool suc, const QString &e) {
                            handleDownloadFinished(downloaderPtr, p, suc, e);
                        }, Qt::QueuedConnection);
                connect(downloaderPtr, &Qtdl::downloadPaused, this,
                        [this, downloaderPtr](const QString &p) {
                            handleDownloadPaused(downloaderPtr, p);
                        }, Qt::QueuedConnection);
                connect(downloaderPtr, &Qtdl::downloadResumed, this,
                        [this, downloaderPtr](const QString &p) {
                            handleDownloadResumed(downloaderPtr, p);
                        }, Qt::QueuedConnection);

                connect(item->actionBtn, &QPushButton::clicked, this,
                        [this, rawPtr]() { if (rawPtr) handleActionButton(rawPtr); });
                connect(item->openBtn, &QPushButton::clicked, this,
                        [this, rawPtr]() { if (rawPtr) openDownloadFile(rawPtr->downloader.get()); });

                m_fileWatcher->addPath(absolutePath);

                m_itemMap[listItem] = rawPtr;
                m_downloads.push_back(std::move(item));
            }

            if (downloaderPtr) downloaderPtr->startDownload(url, absolutePath, cacheDir);
            updateEmptyState();  
        }, Qt::QueuedConnection);
    }

    void saveAllStatesOnExit() {
        QMutexLocker locker(&m_mutex);
        for (auto &item : m_downloads) {
            if (item && item->downloader) {
                if (item->condition == DownloadCondition::Downloading) {
                    item->downloader->cancelDownload();
                    item->condition = DownloadCondition::Cancelled_Failed;
                }
            }
        }
        saveIndex();
    }

signals:
    void downloadAdded();

protected:
    void closeEvent(QCloseEvent *event) override {
        bool hasActiveDownloads = false;
        for (const auto &item : m_downloads) {
            if (item && item->condition == DownloadCondition::Downloading) {
                hasActiveDownloads = true;
                break;
            }
        }

        if (hasActiveDownloads) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Active Downloads",
                "There are active downloads. Closing will cancel them. Continue?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
                );

            if (reply == QMessageBox::No) {
                event->ignore();
                return;
            }

            QMutexLocker locker(&m_mutex);
            for (auto &item : m_downloads) {
                if (item && item->condition == DownloadCondition::Downloading) {
                    if (item->downloader) {
                        item->downloader->cancelDownload();
                    }
                    item->condition = DownloadCondition::Cancelled_Failed;
                }
            }
            saveIndex();
        }

        QDialog::closeEvent(event);
    }

private slots:
    void onFileChanged(const QString &path) {
        QMutexLocker locker(&m_mutex);

        for (auto &item : m_downloads) {
            if (item && getAbsolutePath(item->finalPath) == path) {
                bool exists = QFileInfo(path).exists();
                if (item->fileExists != exists) {
                    item->fileExists = exists;
                    locker.unlock();
                    updateItemVisuals(item.get());
                    return;
                }
                break;
            }
        }
    }

    void showContextMenu(const QPoint &pos) {
        if (!m_listWidget) return;
        QListWidgetItem *item = m_listWidget->itemAt(pos);
        if (!item || item->flags() == Qt::NoItemFlags) return;
        DownloadItem *data = m_itemMap.value(item, nullptr);
        if (!data) return;

        QMenu menu(this);

        bool canOpen = (data->condition == DownloadCondition::Finished && data->fileExists);
        bool canPause = (data->condition == DownloadCondition::Downloading);
        bool canResume = (data->condition == DownloadCondition::Paused);
        bool canRetry = (data->condition == DownloadCondition::Cancelled_Failed);
        bool canCancel = (data->condition == DownloadCondition::Downloading || data->condition == DownloadCondition::Paused);

        menu.addAction("Open")->setEnabled(canOpen);
        menu.addAction("Show in Folder")->setEnabled(canOpen);
        menu.addSeparator();
        menu.addAction("Pause")->setEnabled(canPause);
        menu.addAction("Resume")->setEnabled(canResume);
        menu.addAction("Retry")->setEnabled(canRetry);
        menu.addSeparator();
        menu.addAction("Cancel")->setEnabled(canCancel);
        menu.addAction("Remove");

        QAction *selected = menu.exec(m_listWidget->mapToGlobal(pos));
        if (!selected) return;

        QString action = selected->text();
        if (action == "Remove") removeDownloadItem(data);
        else if (action == "Open" && canOpen)
            QDesktopServices::openUrl(QUrl::fromLocalFile(getAbsolutePath(data->finalPath)));
        else if (action == "Show in Folder" && canOpen)
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(getAbsolutePath(data->finalPath)).absolutePath()));
        else if (action == "Pause" && canPause)
            pauseDownload(data);
        else if (action == "Resume" && canResume)
            resumeDownload(data);
        else if (action == "Retry" && canRetry)
            retryDownload(data);
        else if (action == "Cancel" && canCancel)
            cancelDownload(data);
    }

    void clearCompleted() {
        if (!m_listWidget) return;
        QMutexLocker locker(&m_mutex);

        for (auto it = m_downloads.begin(); it != m_downloads.end(); ) {
            if ((*it) && (*it)->condition == DownloadCondition::Finished) {
                QString absPath = getAbsolutePath((*it)->finalPath);
                m_fileWatcher->removePath(absPath);

                
                if (QFile::exists(absPath)) {
                    if (!QFile::remove(absPath)) {
                        locker.unlock();
                        QMessageBox::warning(this, "Error", "Something went wrong while deleting the file.");
                        return;
                    }
                }

                
                if ((*it)->downloader) {
                    disconnect((*it)->downloader.get(), nullptr, this, nullptr);
                }

                delete (*it)->listItem;
                m_itemMap.remove((*it)->listItem);
                it = m_downloads.erase(it);
            } else {
                ++it;
            }
        }
        rebuildSeparators();
        saveIndex();
        updateEmptyState();  
    }

private:
    enum class AgeGroup { Today, Yesterday, ThisWeek, Older };

    void initializeEnvironment() {
        auto &settings = Settings_Backend::instance();
        QString downloadRoot = settings.downloadPath();
        if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();

        QString cacheDir = downloadRoot + "/.dlcache";

        
        QDir().mkpath(cacheDir);

        
        QString indexPath = cacheDir + "/.onudl";
        if (!QFile::exists(indexPath)) {
            QFile file(indexPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "[global]\n";
                out << "segments=" << m_defaultSegments << "\n\n";
                out << "[finished]\n\n";
                out << "[cancelled_failed]\n\n";
                out << "[queued]\n\n";
                out << "[paused]\n\n";
                file.close();
            }
        }

        m_initialized = true;
    }

    void checkAndLoadIndex() {
        auto &settings = Settings_Backend::instance();
        QString downloadRoot = settings.downloadPath();
        if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();

        QString indexPath = downloadRoot + "/.dlcache/.onudl";

        if (QFile::exists(indexPath)) {
            m_initialized = true;
            loadIndex();
            updateEmptyState();  
        }
        
    }

    void loadIndex() {
        auto &settings = Settings_Backend::instance();
        QString downloadRoot = settings.downloadPath();
        if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();

        QString indexPath = downloadRoot + "/.dlcache/.onudl";

        QFile file(indexPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream in(&file);
        QString currentSection;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.isEmpty() || line.startsWith(";") || line.startsWith("#")) {
                continue;
            }

            if (line.startsWith("[") && line.endsWith("]")) {
                currentSection = line.mid(1, line.length() - 2);
                continue;
            }

            if (currentSection == "global") {
                if (line.startsWith("segments=")) {
                    m_defaultSegments = line.mid(9).toInt();
                    if (m_segmentsSpinBox) {
                        m_segmentsSpinBox->setValue(m_defaultSegments);
                    }
                }
            }
            else if (currentSection == "finished") {
                QStringList parts = line.split("=");
                if (parts.size() == 2) {
                    QStringList data = parts[1].split("|");
                    if (data.size() == 2) {
                        QString url = data[0];
                        QString relativePath = data[1];

                        
                        QString absPath = downloadRoot + "/" + relativePath;
                        if (QFile::exists(absPath)) {
                            addHistoricalDownload(url, relativePath, DownloadCondition::Finished);
                        } else {
                            
                            addHistoricalDownload(url, "", DownloadCondition::Cancelled_Failed);
                        }
                    }
                }
            }
            else if (currentSection == "paused") {
                QStringList parts = line.split("=");
                if (parts.size() == 2) {
                    QStringList data = parts[1].split("|");
                    if (data.size() >= 1) {
                        QString url = data[0];

                        
                        bool allPartsExist = false;
                        if (data.size() == 2) {
                            QStringList partPaths = data[1].split(",");
                            allPartsExist = true;
                            for (const QString& partPath : partPaths) {
                                QString absPath = downloadRoot + "/" + partPath;
                                if (!QFile::exists(absPath)) {
                                    allPartsExist = false;
                                    break;
                                }
                            }
                        }

                        if (allPartsExist && data.size() == 2) {
                            addHistoricalDownload(url, data[1], DownloadCondition::Paused);
                        } else {
                            
                            addHistoricalDownload(url, "", DownloadCondition::Cancelled_Failed);
                        }
                    }
                }
            }
            else if (currentSection == "cancelled_failed") {
                QStringList parts = line.split("=");
                if (parts.size() == 2) {
                    QString url = parts[1];
                    addHistoricalDownload(url, "", DownloadCondition::Cancelled_Failed);
                }
            }
        }

        file.close();
    }

    void saveIndex() {
        if (!m_initialized) return;

        auto &settings = Settings_Backend::instance();
        QString downloadRoot = settings.downloadPath();
        if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();

        QString indexPath = downloadRoot + "/.dlcache/.onudl";

        QFile file(indexPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Something went wrong while saving download index.");
            return;
        }

        QTextStream out(&file);

        
        out << "[global]\n";
        out << "segments=" << m_defaultSegments << "\n\n";

        
        out << "[finished]\n";
        int finishedCount = 1;
        for (const auto &item : m_downloads) {
            if (item && item->condition == DownloadCondition::Finished) {
                out << finishedCount++ << "=" << item->downloadUrl.toString() << "|" << item->finalPath << "\n";
            }
        }
        out << "\n";

        
        out << "[cancelled_failed]\n";
        int cancelledCount = 1;
        for (const auto &item : m_downloads) {
            if (item && item->condition == DownloadCondition::Cancelled_Failed) {
                out << cancelledCount++ << "=" << item->downloadUrl.toString() << "\n";
            }
        }
        out << "\n";

        
        out << "[queued]\n";
        out << "\n";

        
        out << "[paused]\n";
        int pausedCount = 1;
        for (const auto &item : m_downloads) {
            if (item && item->condition == DownloadCondition::Paused && item->downloader) {
                QStringList segmentPaths = item->downloader->getSegmentPaths();
                QStringList relativePaths;
                for (const QString& absPath : segmentPaths) {
                    QString relativePath = absPath;
                    relativePath.replace(downloadRoot + "/", "");
                    relativePaths.append(relativePath);
                }
                out << pausedCount++ << "=" << item->downloadUrl.toString() << "|" << relativePaths.join(",") << "\n";
            }
        }

        file.close();
    }

    QString getAbsolutePath(const QString& relativePath) const {
        auto &settings = Settings_Backend::instance();
        QString downloadRoot = settings.downloadPath();
        if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();

        return downloadRoot + "/" + relativePath;
    }

    QString applySmartPath(const QString &fileName) {
        QString ext = QFileInfo(fileName).suffix().toLower();

        static const QMap<QString, QString> folders = {
            {"pdf", "Documents"}, {"doc", "Documents"}, {"docx", "Documents"},
            {"txt", "Documents"}, {"odt", "Documents"},
            {"jpg", "Images"}, {"jpeg", "Images"}, {"png", "Images"},
            {"gif", "Images"}, {"bmp", "Images"}, {"svg", "Images"},
            {"mp4", "Videos"}, {"avi", "Videos"}, {"mkv", "Videos"},
            {"mov", "Videos"}, {"wmv", "Videos"}, {"flv", "Videos"},
            {"mp3", "Music"}, {"wav", "Music"}, {"flac", "Music"},
            {"ogg", "Music"}, {"m4a", "Music"},
            {"zip", "Archives"}, {"rar", "Archives"}, {"7z", "Archives"},
            {"tar", "Archives"}, {"gz", "Archives"}
        };

        QString folder = folders.value(ext, "Others");

        auto &settings = Settings_Backend::instance();
        QString downloadRoot = settings.downloadPath();
        if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();

        QString targetDir = downloadRoot + "/" + folder;
        QDir().mkpath(targetDir);  

        QString uniqueName = generateUniqueFileName(targetDir, fileName);
        return folder + "/" + QFileInfo(uniqueName).fileName();
    }

    QString generateUniqueFileName(const QString &dir, const QString &fileName) {
        QDir directory(dir);
        if (!directory.exists()) directory.mkpath(dir);

        QString baseName = QFileInfo(fileName).completeBaseName();
        QString suffix = QFileInfo(fileName).suffix();
        QString filePath = directory.filePath(fileName);

        if (!QFile::exists(filePath))
            return filePath;

        int counter = 1;
        while (QFile::exists(filePath) && counter < 1000) {
            QString newName = QString("%1 (%2)").arg(baseName).arg(counter);
            if (!suffix.isEmpty()) newName += "." + suffix;
            filePath = directory.filePath(newName);
            counter++;
        }
        return filePath;
    }

    

    AgeGroup getAgeGroup(const QDateTime &dt) const {
        if (!dt.isValid()) return AgeGroup::Older;
        QDate today = QDateTime::currentDateTime().date();
        QDate fileDate = dt.date();
        if (fileDate == today) return AgeGroup::Today;
        if (fileDate == today.addDays(-1)) return AgeGroup::Yesterday;
        if (fileDate > today.addDays(-7)) return AgeGroup::ThisWeek;
        return AgeGroup::Older;
    }

    QString getGroupLabel(AgeGroup group) const {
        switch (group) {
        case AgeGroup::Today: return "Today";
        case AgeGroup::Yesterday: return "Yesterday";
        case AgeGroup::ThisWeek: return "This Week";
        default: return "Older";
        }
    }

    QWidget* createSeparator(const QString &text) {
        QWidget *container = new QWidget();
        container->setFixedHeight(28);
        QHBoxLayout *layout = new QHBoxLayout(container);
        layout->setContentsMargins(10, 5, 10, 5);

        QFrame *line1 = new QFrame();
        line1->setFrameShape(QFrame::HLine);
        line1->setFrameShadow(QFrame::Sunken);
        line1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QLabel *label = new QLabel(text);
        QFont font = label->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        label->setFont(font);
        label->setStyleSheet("color: #555; padding: 0 5px;");

        QFrame *line2 = new QFrame();
        line2->setFrameShape(QFrame::HLine);
        line2->setFrameShadow(QFrame::Sunken);
        line2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(line1);
        layout->addWidget(label);
        layout->addWidget(line2);
        return container;
    }

    QDateTime getFileDate(const QString &relativePath) const {
        if (relativePath.isEmpty()) return QDateTime();
        QString absPath = getAbsolutePath(relativePath);
        QFileInfo fileInfo(absPath);
        if (!fileInfo.exists()) return QDateTime();
        return fileInfo.lastModified();
    }

    QString getDownloadAge(const QString &relativePath) const {
        if (relativePath.isEmpty()) return "Unknown";
        QDateTime dt = getFileDate(relativePath);
        if (!dt.isValid()) return "Unknown";
        qint64 seconds = dt.secsTo(QDateTime::currentDateTime());
        if (seconds < 0) seconds = 0;
        if (seconds < 60) return "Just now";
        if (seconds < 3600) return QString("%1 minutes ago").arg(seconds / 60);
        if (seconds < 86400) return QString("%1 hours ago").arg(seconds / 3600);
        return QString("%1 days ago").arg(seconds / 86400);
    }

    void setupUI() {
        QVBoxLayout *layout = new QVBoxLayout(this);

        
        m_stackedWidget = new QStackedWidget();

        
        m_listWidget = new QListWidget();
        m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        m_listWidget->setSpacing(2);
        connect(m_listWidget, &QListWidget::customContextMenuRequested, this, &DL_Man::showContextMenu);
        m_stackedWidget->addWidget(m_listWidget);

        
        m_emptyStateWidget = createEmptyStateWidget();
        m_stackedWidget->addWidget(m_emptyStateWidget);

        layout->addWidget(m_stackedWidget);

        
        QHBoxLayout *segmentLayout = new QHBoxLayout();

        QLabel *segmentLabel = new QLabel("Segments:");
        segmentLabel->setToolTip("Number of segments for new downloads (parallel download chunks)");

        m_segmentsSpinBox = new QSpinBox();
        m_segmentsSpinBox->setRange(1, 32);
        m_segmentsSpinBox->setValue(m_defaultSegments);
        m_segmentsSpinBox->setFixedWidth(70);
        m_segmentsSpinBox->setToolTip("1-32 segments (applies to new downloads)");

        QLabel *infoLabel = new QLabel("(applies to new downloads)");
        infoLabel->setStyleSheet("color: gray; font-size: 10px;");

        segmentLayout->addWidget(segmentLabel);
        segmentLayout->addWidget(m_segmentsSpinBox);
        segmentLayout->addWidget(infoLabel);
        segmentLayout->addStretch();

        layout->addLayout(segmentLayout);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        m_clearBtn = new QPushButton("Clear Completed");
        m_clearBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
        m_openFolderBtn = new QPushButton("Open Download Folder");
        m_openFolderBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FolderOpen));
        QPushButton *closeBtn = new QPushButton("Close");
        closeBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::WindowClose));

        btnLayout->addWidget(m_clearBtn);
        btnLayout->addWidget(m_openFolderBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(closeBtn);
        layout->addLayout(btnLayout);

        connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
        connect(m_clearBtn, &QPushButton::clicked, this, &DL_Man::clearCompleted);
        connect(m_openFolderBtn, &QPushButton::clicked, this, [this]() {
            QString path = Settings_Backend::instance().downloadPath();
            if (path.isEmpty()) path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
            if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });

        connect(m_segmentsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
            m_defaultSegments = value;
            if (m_initialized) {
                saveIndex();  
            }
        });
    }

    QWidget* createEmptyStateWidget() {
        QWidget *emptyWidget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(emptyWidget);
        layout->setAlignment(Qt::AlignCenter);

        
        QLabel *iconLabel = new QLabel();
        QIcon folderIcon = QIcon::fromTheme("folder-download");
        QPixmap pixmap = folderIcon.pixmap(256, 256, QIcon::Disabled);
        iconLabel->setPixmap(pixmap);
        iconLabel->setAlignment(Qt::AlignCenter);

        
        QLabel *textLabel = new QLabel("No downloads yet");
        QFont font = textLabel->font();
        font.setPointSize(18);
        font.setBold(true);
        textLabel->setFont(font);
        textLabel->setStyleSheet("color: gray;");
        textLabel->setAlignment(Qt::AlignCenter);

        layout->addStretch();
        layout->addWidget(iconLabel);
        layout->addSpacing(20);
        layout->addWidget(textLabel);
        layout->addStretch();

        return emptyWidget;
    }

    void updateEmptyState() {
        if (!m_stackedWidget) return;

        if (m_downloads.empty()) {
            m_stackedWidget->setCurrentIndex(1);  
        } else {
            m_stackedWidget->setCurrentIndex(0);  
        }
    }

    void insertItemInOrder(DownloadItem *item, QListWidgetItem *listItem, QWidget *widget) {
        if (!m_listWidget || !item || !listItem || !widget) return;

        AgeGroup itemGroup = getAgeGroup(item->timestamp);
        int insertRow = m_listWidget->count();
        bool foundGroup = false;

        for (int i = 0; i < m_listWidget->count(); ++i) {
            QListWidgetItem *current = m_listWidget->item(i);
            if (!current) continue;
            if (current->flags() == Qt::NoItemFlags) {
                QWidget *currentWidget = m_listWidget->itemWidget(current);
                if (currentWidget) {
                    QLabel *label = currentWidget->findChild<QLabel*>();
                    if (label) {
                        QString sepText = label->text();
                        AgeGroup sepGroup = AgeGroup::Older;
                        if (sepText == "Today") sepGroup = AgeGroup::Today;
                        else if (sepText == "Yesterday") sepGroup = AgeGroup::Yesterday;
                        else if (sepText == "This Week") sepGroup = AgeGroup::ThisWeek;

                        if (sepGroup == itemGroup) foundGroup = true;
                        else if (foundGroup) { insertRow = i; break; }
                    }
                }
            } else if (foundGroup) {
                insertRow = i;
                break;
            }
        }

        if (!foundGroup) {
            QString sepLabel = getGroupLabel(itemGroup);
            QListWidgetItem *sepItem = new QListWidgetItem();
            sepItem->setFlags(Qt::NoItemFlags);
            QWidget *sepWidget = createSeparator(sepLabel);
            sepItem->setSizeHint(sepWidget->sizeHint());
            m_listWidget->insertItem(insertRow++, sepItem);
            m_listWidget->setItemWidget(sepItem, sepWidget);
        }

        m_listWidget->insertItem(insertRow, listItem);
        m_listWidget->setItemWidget(listItem, widget);
    }

    void removeDownloadItem(DownloadItem *data) {
        if (!data || !data->listItem || !m_listWidget) return;
        QMutexLocker locker(&m_mutex);

        QString absPath = getAbsolutePath(data->finalPath);
        DownloadCondition condition = data->condition;

        
        if (data->downloader &&
            (condition == DownloadCondition::Downloading || condition == DownloadCondition::Paused)) {
            data->downloader->cancelDownload();
        }

        
        if (condition == DownloadCondition::Finished && QFile::exists(absPath)) {
            if (!QFile::remove(absPath)) {
                locker.unlock();
                QMessageBox::warning(this, "Error", "Something went wrong while deleting the file.");
                return;
            }
        } else if (condition == DownloadCondition::Paused && data->downloader) {
            
            QStringList segPaths = data->downloader->getSegmentPaths();
            for (const QString& partPath : segPaths) {
                if (QFile::exists(partPath)) {
                    QFile::remove(partPath);
                }
            }
        }

        m_fileWatcher->removePath(absPath);

        
        if (data->downloader) {
            disconnect(data->downloader.get(), nullptr, this, nullptr);
        }

        delete data->listItem;
        m_itemMap.remove(data->listItem);
        for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it) {
            if (it->get() == data) {
                m_downloads.erase(it);
                break;
            }
        }
        rebuildSeparators();
        saveIndex();
        updateEmptyState();  
    }

    void addHistoricalDownload(const QString &url, const QString &relativePath, DownloadCondition condition) {
        if (!m_listWidget || url.isEmpty()) return;
        QMutexLocker locker(&m_mutex);

        auto item = std::make_unique<DownloadItem>();
        item->fileName = QFileInfo(relativePath).fileName();
        if (item->fileName.isEmpty()) item->fileName = QUrl(url).fileName();
        if (item->fileName.isEmpty()) item->fileName = "Unknown";

        item->finalPath = relativePath;
        item->downloadUrl = QUrl(url);
        item->condition = condition;
        item->timestamp = getFileDate(relativePath);
        if (!item->timestamp.isValid()) {
            item->timestamp = QDateTime::currentDateTime();
        }
        item->fileExists = condition == DownloadCondition::Finished && QFile::exists(getAbsolutePath(relativePath));

        QWidget *itemWidget = createDownloadWidget(item.get(), true);
        if (!itemWidget) return;

        QListWidgetItem *listItem = new QListWidgetItem();
        listItem->setSizeHint(itemWidget->sizeHint());
        m_listWidget->addItem(listItem);
        m_listWidget->setItemWidget(listItem, itemWidget);
        item->listItem = listItem;
        item->widget = itemWidget;
        m_itemMap[listItem] = item.get();
        updateItemVisuals(item.get());

        DownloadItem *rawPtr = item.get();
        connect(item->openBtn, &QPushButton::clicked, this, [this, rawPtr]() {
            if (rawPtr && rawPtr->fileExists && QFile::exists(getAbsolutePath(rawPtr->finalPath)))
                QDesktopServices::openUrl(QUrl::fromLocalFile(getAbsolutePath(rawPtr->finalPath)));
        });
        connect(item->actionBtn, &QPushButton::clicked, this, [this, rawPtr]() {
            if (rawPtr) handleActionButton(rawPtr);
        });

        if (item->fileExists) {
            m_fileWatcher->addPath(getAbsolutePath(relativePath));
        }

        m_downloads.push_back(std::move(item));
    }

    void rebuildSeparators() {
        if (!m_listWidget) return;
        QList<QPair<QListWidgetItem*, DownloadItem*>> items;

        for (int i = 0; i < m_listWidget->count(); ++i) {
            QListWidgetItem *item = m_listWidget->item(i);
            if (item && item->flags() != Qt::NoItemFlags) {
                DownloadItem *data = m_itemMap.value(item, nullptr);
                if (data) items.append(qMakePair(item, data));
            }
        }

        m_listWidget->clear();
        m_itemMap.clear();

        AgeGroup lastGroup = AgeGroup::Older;
        for (const auto &pair : items) {
            AgeGroup currentGroup = getAgeGroup(pair.second->timestamp);
            if (currentGroup != lastGroup) {
                QListWidgetItem *sepItem = new QListWidgetItem();
                sepItem->setFlags(Qt::NoItemFlags);
                QWidget *sepWidget = createSeparator(getGroupLabel(currentGroup));
                sepItem->setSizeHint(sepWidget->sizeHint());
                m_listWidget->addItem(sepItem);
                m_listWidget->setItemWidget(sepItem, sepWidget);
                lastGroup = currentGroup;
            }
            m_listWidget->addItem(pair.first);
            m_listWidget->setItemWidget(pair.first, pair.second->widget);
            m_itemMap[pair.first] = pair.second;
        }
    }

    void updateDownloadProgress(Qtdl *downloader, qint64 received, qint64 total, double speed) {
        if (!downloader || !m_listWidget) return;
        QMutexLocker locker(&m_mutex);

        for (auto &item : m_downloads) {
            if (item && item->downloader && item->downloader.get() == downloader) {
                if (total > 0) {
                    item->progressBar->setRange(0, 100);
                    item->progressBar->setValue(static_cast<int>((received * 100) / total));
                    item->statusLabel->setText(QString("%1% - %2 KB/s - %3 / %4")
                                                   .arg(static_cast<int>((received * 100) / total))
                                                   .arg(speed, 0, 'f', 1)
                                                   .arg(formatBytes(received))
                                                   .arg(formatBytes(total)));
                } else {
                    item->progressBar->setRange(0, 0);
                    item->statusLabel->setText(QString("Unknown Size - %1 KB/s - %2")
                                                   .arg(speed, 0, 'f', 1)
                                                   .arg(formatBytes(received)));
                }
                break;
            }
        }
    }

    void handleDownloadFinished(Qtdl *downloader, const QString &absPath, bool success, const QString &error) {
        if (!downloader || !m_listWidget) return;
        QMutexLocker locker(&m_mutex);

        for (auto &item : m_downloads) {
            if (item && item->downloader && item->downloader.get() == downloader) {
                if (success) {
                    item->condition = DownloadCondition::Finished;
                } else if (downloader->isCancelled()) {
                    item->condition = DownloadCondition::Cancelled_Failed;
                } else {
                    item->condition = DownloadCondition::Cancelled_Failed;
                }

                item->timestamp = QDateTime::currentDateTime();
                item->progressBar->setRange(0, 100);

                if (success) {
                    QFileInfo fileInfo(absPath);
                    item->statusLabel->setText("Completed - " + formatBytes(fileInfo.size()));
                    item->progressBar->setValue(100);
                    item->actionBtn->setText("Remove");
                    item->actionBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditDelete));
                    item->openBtn->setVisible(true);
                    item->fileExists = true;
                    if (item->ageLabel) item->ageLabel->setText(getDownloadAge(item->finalPath));
                } else {
                    item->statusLabel->setText(downloader->isCancelled() ? "Cancelled" : ("Failed: " + error));
                    item->actionBtn->setText("Retry");
                    item->actionBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh));
                    item->openBtn->setVisible(false);
                }

                rebuildSeparators();
                saveIndex();
                break;
            }
        }
    }

    void handleDownloadPaused(Qtdl *downloader, const QString &absPath) {
        if (!downloader || !m_listWidget) return;
        QMutexLocker locker(&m_mutex);

        for (auto &item : m_downloads) {
            if (item && item->downloader && item->downloader.get() == downloader) {
                item->condition = DownloadCondition::Paused;
                item->statusLabel->setText("Paused - segments cached");
                item->actionBtn->setText("Resume");
                item->actionBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));

                saveIndex();
                break;
            }
        }
    }

    void handleDownloadResumed(Qtdl *downloader, const QString &absPath) {
        if (!downloader || !m_listWidget) return;
        QMutexLocker locker(&m_mutex);

        for (auto &item : m_downloads) {
            if (item && item->downloader && item->downloader.get() == downloader) {
                item->condition = DownloadCondition::Downloading;
                item->statusLabel->setText("Resuming...");
                item->actionBtn->setText("Pause");
                item->actionBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
                item->progressBar->setRange(0, 0);
                break;
            }
        }
    }

    void handleActionButton(DownloadItem *item) {
        if (!item) return;

        switch (item->condition) {
        case DownloadCondition::Downloading:
            pauseDownload(item);
            break;
        case DownloadCondition::Paused:
            resumeDownload(item);
            break;
        case DownloadCondition::Cancelled_Failed:
            retryDownload(item);
            break;
        case DownloadCondition::Finished:
            removeDownloadItem(item);
            break;
        default:
            break;
        }
    }

    void pauseDownload(DownloadItem *item) {
        if (!item || !item->downloader) return;
        item->downloader->pauseDownload();
    }

    void resumeDownload(DownloadItem *item) {
        if (!item || !item->downloader) return;
        item->downloader->resumeDownload();
    }

    void cancelDownload(DownloadItem *item) {
        if (!item || !item->downloader) return;
        item->downloader->cancelDownload();
        item->condition = DownloadCondition::Cancelled_Failed;
        saveIndex();
    }

    void openDownloadFile(Qtdl *downloader) {
        if (!downloader || !m_listWidget) return;
        QMutexLocker locker(&m_mutex);

        for (auto &item : m_downloads) {
            if (item && item->downloader && item->downloader.get() == downloader) {
                if (item->condition == DownloadCondition::Finished) {
                    QString absPath = getAbsolutePath(item->finalPath);
                    if (QFile::exists(absPath)) {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(absPath));
                    } else {
                        updateItemVisuals(item.get());
                    }
                }
                break;
            }
        }
    }

    void retryDownload(DownloadItem *item) {
        if (!item) return;

        QMutexLocker locker(&m_mutex);

        item->downloader = std::make_unique<Qtdl>(m_defaultSegments);
        Qtdl *newDownloader = item->downloader.get();

        connect(newDownloader, &Qtdl::progress, this,
                [this, newDownloader](qint64 r, qint64 t, double s) {
                    updateDownloadProgress(newDownloader, r, t, s);
                }, Qt::QueuedConnection);
        connect(newDownloader, &Qtdl::downloadFinished, this,
                [this, newDownloader](const QString &p, bool suc, const QString &e) {
                    handleDownloadFinished(newDownloader, p, suc, e);
                }, Qt::QueuedConnection);
        connect(newDownloader, &Qtdl::downloadPaused, this,
                [this, newDownloader](const QString &p) {
                    handleDownloadPaused(newDownloader, p);
                }, Qt::QueuedConnection);
        connect(newDownloader, &Qtdl::downloadResumed, this,
                [this, newDownloader](const QString &p) {
                    handleDownloadResumed(newDownloader, p);
                }, Qt::QueuedConnection);

        item->condition = DownloadCondition::Downloading;
        item->statusLabel->setText("Retrying...");
        item->progressBar->setRange(0, 0);
        item->progressBar->setValue(0);
        item->actionBtn->setText("Pause");
        item->actionBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));

        QString absPath = getAbsolutePath(item->finalPath);
        auto &settings = Settings_Backend::instance();
        QString downloadRoot = settings.downloadPath();
        if (downloadRoot.isEmpty()) downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadRoot.isEmpty()) downloadRoot = QDir::homePath();
        QString cacheDir = downloadRoot + "/.dlcache";

        locker.unlock();

        newDownloader->startDownload(item->downloadUrl, absPath, cacheDir);
    }

    QString formatBytes(qint64 bytes) {
        if (bytes < 0) return "0 B";
        if (bytes < 1024) return QString::number(bytes) + " B";
        if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
        if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
    }

    QWidget* createDownloadWidget(DownloadItem *item, bool isHistorical) {
        if (!item) return nullptr;

        QWidget *itemWidget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(itemWidget);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->setSpacing(3);

        QHBoxLayout *topLayout = new QHBoxLayout();

        QFileIconProvider iconProvider;
        item->iconLabel = new QLabel();
        item->iconLabel->setPixmap(iconProvider.icon(QFileInfo(item->fileName)).pixmap(32, 32));
        item->iconLabel->setFixedSize(32, 32);

        item->fileLabel = new QLabel(item->fileName);
        item->fileLabel->setStyleSheet("font-weight: bold;");
        item->fileLabel->setWordWrap(true);

        topLayout->addWidget(item->iconLabel);
        topLayout->addWidget(item->fileLabel, 1);

        item->ageLabel = new QLabel();
        item->ageLabel->setStyleSheet("color: gray; font-size: 11px;");
        item->ageLabel->setText(isHistorical && item->fileExists ? getDownloadAge(item->finalPath) : "Just now");
        topLayout->addWidget(item->ageLabel);

        if (isHistorical) {
            QString absPath = getAbsolutePath(item->finalPath);
            QFileInfo fileInfo(absPath);
            QString statusText;

            switch (item->condition) {
            case DownloadCondition::Finished:
                statusText = fileInfo.exists() ? "Completed - " + formatBytes(fileInfo.size()) : "File not found";
                break;
            case DownloadCondition::Paused:
                statusText = "Paused";
                break;
            case DownloadCondition::Cancelled_Failed:
                statusText = "Cancelled/Failed";
                break;
            default:
                statusText = "Unknown";
            }
            item->statusLabel = new QLabel(statusText);
        } else {
            item->statusLabel = new QLabel("Preparing...");
        }

        item->progressBar = new QProgressBar();
        item->progressBar->setRange(0, isHistorical ? 100 : 0);

        if (isHistorical) {
            if (item->condition == DownloadCondition::Finished) {
                item->progressBar->setValue(100);
            } else {
                item->progressBar->setValue(0);
            }
        } else {
            item->progressBar->setValue(0);
        }

        QHBoxLayout *btnLayout = new QHBoxLayout();

        QString btnText, btnIcon;
        switch (item->condition) {
        case DownloadCondition::Downloading:
            btnText = "Pause";
            btnIcon = "media-playback-pause";
            break;
        case DownloadCondition::Paused:
            btnText = "Resume";
            btnIcon = "media-playback-start";
            break;
        case DownloadCondition::Cancelled_Failed:
            btnText = "Retry";
            btnIcon = "view-refresh";
            break;
        case DownloadCondition::Finished:
            btnText = "Remove";
            btnIcon = "edit-delete";
            break;
        default:
            btnText = "Cancel";
            btnIcon = "process-stop";
        }

        item->actionBtn = new QPushButton(btnText);
        item->actionBtn->setIcon(QIcon::fromTheme(btnIcon));
        item->actionBtn->setFixedSize(80, 28);

        item->openBtn = new QPushButton("Open");
        item->openBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen));
        item->openBtn->setFixedSize(80, 28);
        item->openBtn->setVisible(item->condition == DownloadCondition::Finished && item->fileExists);

        btnLayout->addWidget(item->actionBtn);
        btnLayout->addWidget(item->openBtn);
        btnLayout->addStretch();

        layout->addLayout(topLayout);
        layout->addWidget(item->statusLabel);
        layout->addWidget(item->progressBar);
        layout->addLayout(btnLayout);

        return itemWidget;
    }

    void updateItemVisuals(DownloadItem *item) {
        if (!item) return;

        QString absPath = getAbsolutePath(item->finalPath);
        bool exists = QFile::exists(absPath);
        item->fileExists = exists;

        if (item->ageLabel) {
            item->ageLabel->setText(exists ? getDownloadAge(item->finalPath) : "File not found");
        }

        if (!exists && item->condition == DownloadCondition::Finished) {
            QFont font = item->fileLabel->font();
            font.setStrikeOut(true);
            item->fileLabel->setFont(font);
            item->iconLabel->setPixmap(QFileIconProvider().icon(QFileInfo(item->fileName)).pixmap(32, 32, QIcon::Disabled));
            item->statusLabel->setText("File not found");
            item->openBtn->setEnabled(false);
        } else {
            QFont font = item->fileLabel->font();
            font.setStrikeOut(false);
            item->fileLabel->setFont(font);
            item->iconLabel->setPixmap(QFileIconProvider().icon(QFileInfo(item->fileName)).pixmap(32, 32));

            if (item->condition == DownloadCondition::Finished) {
                item->statusLabel->setText("Completed - " + formatBytes(QFileInfo(absPath).size()));
                item->openBtn->setEnabled(true);
            }
        }
    }

    QListWidget *m_listWidget = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QWidget *m_emptyStateWidget = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_openFolderBtn = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QSpinBox *m_segmentsSpinBox = nullptr;
    std::vector<std::unique_ptr<DownloadItem>> m_downloads;
    QMap<QListWidgetItem*, DownloadItem*> m_itemMap;
    mutable QMutex m_mutex;
    int m_defaultSegments;
    bool m_initialized;
};

/*--------------------------

                 Zynomon aelius <zynomon@proton.me> ©️2026, Apache 2.0
                                                     project : Onu 0.5

                                                             -----------------------*/

