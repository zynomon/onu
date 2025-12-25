/****************************************************************
 *               YO best up looking the code ?                  *
 *                                                              *
 *                         Apache 2.0                           *
 *                                                              *
 *     Copyright ©️  Zynomon aelius <zynomon@proton.me>  2025   *
 *                                                              *
 *            Project         :      Onu.                       *
 *            Version         :      0.4 (BETA)                 *
 *                                                              *
 ****************************************************************/

#define qt_disable_deprecated_before 0x060800
#include <QAudioOutput>                                       /* single include for audio support */
#include <QApplication>                                       /* includes for gui + web */
#include <QApplication>
#include <QMainWindow>
#include <QWebEngineView>
#include <QDockWidget>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineSettings>
#include <QBuffer>
#include <QWebEngineDownloadRequest>
#include <QWebEngineHistory>
#include <QToolBar>
#include <QThread>
#include <QProcess>
#include <QListWidget>
#include <QLineEdit>
#include <QAction>
#include <QTabWidget>
#include <QTabBar>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QUrl>
#include <QProgressBar>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>                    /* ohh hellish bloats */
#include <QWidget>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDialog>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QDateTime>
#include <QPalette>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFileInfo>
#include <QToolButton>
#include <QDropEvent>
#include <QMimeData>
#include <QFontComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QTimer>
#include <QShortcut>
#include <QClipboard>
#include <QCompleter>
#include <QStringListModel>
#include <QSystemTrayIcon>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QWebEngineNotification>
#include <QWebEnginePermission>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QToolTip>
#include <QCursor>
#include <QRadioButton>

class OnuSchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT
public:
    explicit OnuSchemeHandler(QObject *parent = nullptr) : QWebEngineUrlSchemeHandler(parent) {}

    void requestStarted(QWebEngineUrlRequestJob *request) override {
        QUrl url = request->requestUrl();
        QString host = url.host();

        if (host == "home") {
            QFile file(":/home.html");
            if (file.open(QIODevice::ReadOnly)) {
                QBuffer *buffer = new QBuffer();
                buffer->setData(file.readAll());
                buffer->open(QIODevice::ReadOnly);
                request->reply("text/html", buffer);
                return;
            }
        } else if (host == "game") {
            QFile file(":/game.html");
            if (file.open(QIODevice::ReadOnly)) {
                QBuffer *buffer = new QBuffer();
                buffer->setData(file.readAll());
                buffer->open(QIODevice::ReadOnly);
                request->reply("text/html", buffer);
                return;
            }
        } else if (host == "about") {
            QString flags = qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS");
            QString profilePath = QWebEngineProfile::defaultProfile()->persistentStoragePath();
            QString cachePath = QWebEngineProfile::defaultProfile()->cachePath();

            QString body =
                "<html><head><meta charset='utf-8'>"
                "<style>"
                "body { background-color:#0d1b2a; color:#00bfff; font-family:monospace; padding:20px; }"
                "h1 { color:#61dafb; }"
                "ul { list-style-type:none; padding:0; }"
                "li { margin:6px 0; }"
                ".icon { width:64px; height:64px; vertical-align:middle; margin-right:10px; }"
                "</style></head>"
                "<body>"
                "<h1><img src='file:///usr/share/icons/hicolor/scalable/apps/onu.svg' class='icon'/> Onu Browser – About</h1>"
                "<ul>"
                "<li><b>Application:</b> " + QApplication::applicationName() + "</li>"
                                                    "<li><b>Version:</b> " + QApplication::applicationVersion() + "</li>"
                                                       "<li><b>Qt Version:</b> " + QString::fromLatin1(QT_VERSION_STR) + "</li>"
                                                        "<li><b>Compiled using:</b> Qt 6.8.5</li>"
                                                        "<li><b>Blame for all codes:</b> Zynomon aelius</li>"
                                                        "<li><b>Chromium Flags:</b> " + flags.toHtmlEscaped() + "</li>"
                                          "<li><b>Profile Path:</b> " + profilePath.toHtmlEscaped() + "</li>"
                                                "<li><b>Cache Path:</b> " + cachePath.toHtmlEscaped() + "</li>"
                                              "</ul>"
                                              "<p style='color:#8899aa;'>Onu License: apache 2.0</p>"
                                              "<p style='color:#8899aa;'>Qt License: LGPL</p>"
                                              "</body></html>";

            QBuffer *buffer = new QBuffer();
            buffer->setData(body.toUtf8());
            buffer->open(QIODevice::ReadOnly);
            request->reply("text/html", buffer);
            return;
        }

        QString safeHost = host.toHtmlEscaped();
        QString body = "<html><body style='font-family:Monospace;padding:50px;'><h1>onu://" + safeHost + " not found</h1><p>Supported: onu://home, onu://game, onu://about</p></body></html>";
        QBuffer *buffer = new QBuffer();
        buffer->setData(body.toUtf8());
        buffer->open(QIODevice::ReadOnly);
        request->reply("text/html", buffer);
    }
};
class Settings_Man {  // Man = manager
public:
    static QSettings* instance() {
        static QSettings* settings = new QSettings(QApplication::organizationName(), QApplication::applicationName());
        return settings;
    }
    static void setValue(const QString &key, const QVariant &value) {
        instance()->setValue(key, value);
    }
    static QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) {
        return instance()->value(key, defaultValue);
    }
};
class Onu_Web : public QWebEnginePage {
    Q_OBJECT
public:
    void muteAudio(bool mute) {
        setAudioMuted(mute);
    }

    explicit Onu_Web(QWebEngineProfile *profile, QObject *parent = nullptr)
        : QWebEnginePage(profile, parent) {
        loadHostsFilter();
    }
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override {
        if (isAdBlocked(url.toString())) {
            if (isMainFrame) {
                QString safeUrl = url.toString().toHtmlEscaped();
                setHtml("<h2>Blocked by AdBlock</h2><p>" + safeUrl + "</p>");
            }
            return false;
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }
    void loadHostsFilter() {
        blockedHosts.clear();
        if (!Settings_Man::value("adblock/enabled", true).toBool()) return;
        QString hostsFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/blocked_hosts.txt";
        QFile file(hostsFile);
        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) {
                QByteArray line = file.readLine().trimmed();
                if (!line.isEmpty() && !line.startsWith('#')) {
                    QString host = QString::fromUtf8(line).toLower();
                    blockedHosts.insert(host);
                }
            }
            file.close();
        }
    }
    QWebEnginePage* createWindow(WebWindowType type) override {
        if (type != QWebEnginePage::WebBrowserTab)
            return QWebEnginePage::createWindow(type);
        auto *tempPage = new QWebEnginePage(this);
        connect(tempPage, &QWebEnginePage::urlChanged, this,
                [this, tempPage](const QUrl &url) {
                    emit newTabRequested(url.isEmpty() ? QUrl() : url);
                    tempPage->deleteLater();
                });
        return tempPage;
    }
signals:
    void newTabRequested(const QUrl &url);
    void gameModeRequested(const QUrl& url);
public slots:
    void onUrlChanged(const QUrl& url) {
        if (isGameUrl(url) && !m_gameModeActive) {
            emit gameModeRequested(url);
            m_gameModeActive = true;
        }
    }
private:
    bool isAdBlocked(const QString &urlStr) const {
        if (!Settings_Man::value("adblock/enabled", true).toBool()) return false;
        QUrl url(urlStr);
        QString host = url.host().toLower();
        return blockedHosts.contains(host);
    }
    bool isGameUrl(const QUrl& url) const {
        if (!Settings_Man::value("gaming/enabled", false).toBool()) return false;
        QString host = url.host().toLower();
        QStringList sites = Settings_Man::value("gaming/sites", QStringList{"itch.io"}).toStringList();
        for (const QString& site : sites) if (host.contains(site)) return true;
        return host.contains("itch.io") || host.contains("gamejolt.com") ||
               host.contains("kongregate") || host.contains("crazygames") ||
               url.toString().contains("webgl");
    }
    QSet<QString> blockedHosts;
    bool m_gameModeActive = false;
};
class Settings_window : public QDialog {
    Q_OBJECT
public:
    explicit Settings_window(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Onu Settings");
        setWindowIcon(QIcon::fromTheme("settings"));
        resize(700, 500);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        tabs = new QTabWidget(this);
        mainLayout->addWidget(tabs);

        // -------------------------------------------------------G
        QWidget *generalTab = new QWidget;
        QFormLayout *generalLayout = new QFormLayout(generalTab);

        homepageEdit = new QLineEdit(Settings_Man::value("homepage", "onu://home").toString());
        generalLayout->addRow("Homepage:", homepageEdit);

        QString defaultDownload = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        downloadPathEdit = new QLineEdit(Settings_Man::value("download/path", defaultDownload).toString());
        QPushButton *browseBtn = new QPushButton(QIcon::fromTheme("folder-open"), "Browse");
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Download Folder", downloadPathEdit->text());
            if (!dir.isEmpty()) downloadPathEdit->setText(dir);
        });
        QHBoxLayout *downloadLayout = new QHBoxLayout;
        downloadLayout->addWidget(downloadPathEdit);
        downloadLayout->addWidget(browseBtn);
        generalLayout->addRow("Download Folder:", downloadLayout);

        duplicateDownloadCheck = new QCheckBox("Prevent duplicate downloads");
        duplicateDownloadCheck->setChecked(Settings_Man::value("download/prevent_duplicates", true).toBool());
        generalLayout->addRow("", duplicateDownloadCheck);


        developerModeCheck = new QCheckBox("Enable Developer Mode");
        developerModeCheck->setChecked(Settings_Man::value("developer/enabled", false).toBool());
        generalLayout->addRow(developerModeCheck);

        gameModeMaster = new QCheckBox("Enable Game Mode globally");
        gameModeMaster->setChecked(Settings_Man::value("gaming/enabled", false).toBool());
        generalLayout->addRow(gameModeMaster);

        QGroupBox *autoModeGroup = new QGroupBox("Game Mode Auto-enable:");
        QGridLayout *autoLayout = new QGridLayout(autoModeGroup);
        autoPrompt = new QRadioButton("Always ask");
        autoAlways = new QRadioButton("Always enable");
        autoNever = new QRadioButton("Never (blocked)");
        QString mode = Settings_Man::value("gaming/auto_mode", "prompt").toString();
        if (mode == "always") autoAlways->setChecked(true);
        else if (mode == "never") autoNever->setChecked(true);
        else autoPrompt->setChecked(true);
        autoLayout->addWidget(autoPrompt, 0, 0);
        autoLayout->addWidget(autoAlways, 0, 1);
        autoLayout->addWidget(autoNever, 0, 2);
        generalLayout->addRow(autoModeGroup);

        QPushButton *resetBtn = new QPushButton(QIcon::fromTheme("edit-clear"), "Reset Session");
        connect(resetBtn, &QPushButton::clicked, this, [this]() {
            Settings_Man::instance()->clear();
            QMessageBox::information(this, "Session Reset",
                                     "All saved settings and session data have been cleared.\n"
                                     "Restart Onu Browser to regenerate fresh defaults.");
        });
        generalLayout->addRow(resetBtn);

        favRestore = new QCheckBox("Show Favorites toolbar on next startup");
        favRestore->setChecked(Settings_Man::value("favorites/enabled", true).toBool());
        generalLayout->addRow(favRestore);

        recentRestore = new QCheckBox("Show Recent toolbar on next startup");
        recentRestore->setChecked(Settings_Man::value("recent/enabled", true).toBool());
        generalLayout->addRow(recentRestore);

        tabs->addTab(generalTab, QIcon::fromTheme("preferences-other"), "General");


        // ---------------------------------------------P
        QWidget *privacyTab = new QWidget;
        QVBoxLayout *privacyLayout = new QVBoxLayout(privacyTab);

        adblockCheck = new QCheckBox("Enable AdBlock (blocks known ad hosts)");
        adblockCheck->setChecked(Settings_Man::value("adblock/enabled", true).toBool());
        privacyLayout->addWidget(adblockCheck);

        jsCheck = new QCheckBox("Enable JavaScript");
        jsCheck->setChecked(Settings_Man::value("privacy/javascript", true).toBool());
        privacyLayout->addWidget(jsCheck);

        imagesCheck = new QCheckBox("Load Images");
        imagesCheck->setChecked(Settings_Man::value("privacy/images", true).toBool());
        privacyLayout->addWidget(imagesCheck);

        chromiumFlagsEdit = new QLineEdit(Settings_Man::value("flags/chromium_flags",
                                                              "--disable-gpu --disable-webrtc --max_renderer_process_memory=512").toString());
        chromiumFlagsEdit->setToolTip("Space-separated flags. Restart required.");
        privacyLayout->addWidget(new QLabel("Chromium Flags:"));
        privacyLayout->addWidget(chromiumFlagsEdit);

        privacyLayout->addStretch();
        tabs->addTab(privacyTab, QIcon::fromTheme("preferences-system-privacy"), "Privacy");

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &Settings_window::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &Settings_window::reject);
        mainLayout->addWidget(buttonBox);

        // -------------------------------------------A
        QWidget *appearanceTab = new QWidget;
        QFormLayout *appearanceLayout = new QFormLayout(appearanceTab);

        themeCombo = new QComboBox;
        themeCombo->addItems({"System", "Light", "Dark", "Custom"});
        int themeIndex = Settings_Man::value("theme", 0).toInt();
        themeCombo->setCurrentIndex(themeIndex);
        appearanceLayout->addRow("Theme:", themeCombo);

        stylesheetEdit = new QTextEdit(Settings_Man::value("stylesheet", "").toString());
        stylesheetEdit->setMaximumHeight(100);
        stylesheetEdit->setEnabled(themeIndex == 3);
        connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            stylesheetEdit->setEnabled(index == 3);
        });
        appearanceLayout->addRow("Custom Stylesheet:", stylesheetEdit);

        fontCombo = new QFontComboBox;
        fontCombo->setCurrentFont(QFont(Settings_Man::value("ui/fontFamily", QApplication::font().family()).toString()));
        appearanceLayout->addRow("Font:", fontCombo);

        fontSizeSpin = new QSpinBox;
        fontSizeSpin->setRange(8, 32);
        fontSizeSpin->setValue(Settings_Man::value("ui/fontSize", QApplication::font().pointSize()).toInt());
        appearanceLayout->addRow("Font size:", fontSizeSpin);

        tabs->addTab(appearanceTab, QIcon::fromTheme("preferences-desktop-theme"), "Appearance");
    } // yeah GPA 6.7

    void accept() override {
        Settings_Man::setValue("homepage", homepageEdit->text());
        Settings_Man::setValue("download/path", downloadPathEdit->text());
        Settings_Man::setValue("download/prevent_duplicates", duplicateDownloadCheck->isChecked());
        Settings_Man::setValue("favorites/enabled", favRestore->isChecked());
        Settings_Man::setValue("recent/enabled", recentRestore->isChecked());
        Settings_Man::setValue("developer/enabled", developerModeCheck->isChecked());
        Settings_Man::setValue("gaming/enabled", gameModeMaster->isChecked());
        QString autoMode = "prompt";
        if (autoAlways->isChecked()) autoMode = "always";
        else if (autoNever->isChecked()) autoMode = "never";
        Settings_Man::setValue("gaming/auto_mode", autoMode);

        Settings_Man::setValue("theme", themeCombo->currentIndex());
        Settings_Man::setValue("stylesheet", stylesheetEdit->toPlainText());
        Settings_Man::setValue("ui/fontFamily", fontCombo->currentFont().family());
        Settings_Man::setValue("ui/fontSize", fontSizeSpin->value());

        Settings_Man::setValue("adblock/enabled", adblockCheck->isChecked());
        Settings_Man::setValue("privacy/javascript", jsCheck->isChecked());
        Settings_Man::setValue("privacy/images", imagesCheck->isChecked());
        Settings_Man::setValue("flags/chromium_flags", chromiumFlagsEdit->text());


        emit settingsChanged();
        QDialog::accept();
    }

signals:
    void settingsChanged();

private:
    QTabWidget *tabs;

    // G
    QLineEdit *homepageEdit;
    QLineEdit *downloadPathEdit;
    QCheckBox *duplicateDownloadCheck;
    QCheckBox *favRestore;
    QCheckBox *recentRestore;
    QCheckBox *developerModeCheck;
    QCheckBox *gameModeMaster;
    QRadioButton *autoPrompt;
    QRadioButton *autoAlways;
    QRadioButton *autoNever;

    // P
    QCheckBox *adblockCheck;
    QCheckBox *jsCheck;
    QCheckBox *imagesCheck;
    QLineEdit *chromiumFlagsEdit;

    // A
    QComboBox *themeCombo;
    QTextEdit *stylesheetEdit;
    QFontComboBox *fontCombo;
    QSpinBox *fontSizeSpin;
};
class Qtdl : public QObject {
    Q_OBJECT
public:
    explicit Qtdl(QObject *parent = nullptr) : QObject(parent) {

        m_manager.moveToThread(&m_thread);

        connect(&m_thread, &QThread::finished, this, &Qtdl::doCancel);

        m_thread.start();
    }

    ~Qtdl() {
         m_thread.requestInterruption();
        cancelDownload();
        m_thread.quit();
        if (!m_thread.wait(3000)) {
            m_thread.terminate();
            m_thread.wait();
        }
    }

    void startDownload(const QUrl &url, const QString &filePath) {
        QMetaObject::invokeMethod(this, [this, url, filePath]() {
            doDownload(url, filePath);
        }, Qt::QueuedConnection);
    }

    void cancelDownload() {
        QMetaObject::invokeMethod(this, &Qtdl::doCancel, Qt::QueuedConnection);
    }

    bool isDownloading() const {
        return m_activeReply != nullptr;
    }

    QString getCurrentFilePath() const {
        return m_currentFilePath;
    }

private slots:
    void doDownload(const QUrl &url, const QString &filePath) {

        if (m_activeReply) {
            m_activeReply->abort();
            m_activeReply->deleteLater();
            m_activeReply = nullptr;
        }

        QDir().mkpath(QFileInfo(filePath).absolutePath());

        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setHeader(QNetworkRequest::UserAgentHeader, "onuBrowser/0.4");

        request.setTransferTimeout(30000);

        QNetworkReply *reply = m_manager.get(request);
        m_activeReply = reply;
        m_currentFilePath = filePath;
        m_bytesReceived = 0;
        m_bytesTotal = 0;

        auto *file = new QFile(filePath);
        if (!file->open(QIODevice::WriteOnly)) {
            emit downloadError(filePath, "Cannot open file for writing: " + file->errorString());
            file->deleteLater();
            reply->abort();
            reply->deleteLater();
            m_activeReply = nullptr;
            return;
        }
        m_file = file;

        m_timer.start();


        m_currentReply = reply;

        connect(reply, &QNetworkReply::readyRead, this, [this]() {
            if (!m_activeReply || !m_file || !m_activeReply->isOpen()) return;

            QByteArray data = m_activeReply->readAll();
            m_file->write(data);
            m_bytesReceived += data.size();
        });

        connect(reply, &QNetworkReply::downloadProgress,
                this, [this](qint64 bytesReceived, qint64 bytesTotal) {
                    if (!m_activeReply) return;

                    m_bytesReceived = bytesReceived;
                    m_bytesTotal = bytesTotal;

                    if (bytesTotal > 0) {
                        qint64 elapsed = m_timer.elapsed();
                        double speedKBps = elapsed > 0
                                               ? (bytesReceived / 1024.0) / (elapsed / 1000.0)
                                               : 0.0;
                        emit progress(bytesReceived, bytesTotal, speedKBps);
                    }
                });

        connect(reply, &QNetworkReply::errorOccurred,
                this, [this](QNetworkReply::NetworkError error) {
                    if (!m_activeReply) return;

                    QString errorMsg;
                    switch (error) {
                    case QNetworkReply::TimeoutError:
                        errorMsg = "Timeout";
                        break;
                    case QNetworkReply::ConnectionRefusedError:
                        errorMsg = "Connection refused";
                        break;
                    case QNetworkReply::RemoteHostClosedError:
                        errorMsg = "Remote host closed connection";
                        break;
                    case QNetworkReply::HostNotFoundError:
                        errorMsg = "Host not found";
                        break;
                    default:
                        errorMsg = QString("Network error: %1").arg(error);
                    }
                    emit downloadError(m_currentFilePath, errorMsg);
                });

        connect(reply, &QNetworkReply::finished, this, [this]() {
            if (!m_activeReply) return;

            bool success = m_activeReply->error() == QNetworkReply::NoError;
            QString errorString = success ? QString() : m_activeReply->errorString();

            if (success && m_file) {
                m_file->flush();
                m_file->close();
                emit downloadFinished(m_currentFilePath, true, QString());
            } else {
                if (m_file && m_file->exists() && m_bytesReceived == 0) {
                    m_file->remove();
                }
                emit downloadFinished(m_currentFilePath, false, errorString);
            }


            if (m_file) {
                m_file->deleteLater();
                m_file = nullptr;
            }

            m_activeReply->deleteLater();
            m_activeReply = nullptr;


            if (!parent()) {
                deleteLater();
            }
        });
    }

    void doCancel() {
        if (m_activeReply) {
            m_activeReply->abort();
            m_activeReply->deleteLater();
            m_activeReply = nullptr;
        }
        if (m_file) {
            m_file->close();
            if (m_file->exists() && m_bytesReceived == 0) {
                m_file->remove();
            }
            m_file->deleteLater();
            m_file = nullptr;
        }
        emit downloadFinished(m_currentFilePath, false, "Cancelled by user");
    }

private:
    QNetworkAccessManager m_manager;
    QNetworkReply *m_activeReply = nullptr;
    QFile *m_file = nullptr;
    QString m_currentFilePath;
    QElapsedTimer m_timer;
    QThread m_thread;
    qint64 m_bytesReceived = 0;
    qint64 m_bytesTotal = 0;
    QNetworkReply *m_currentReply = nullptr;

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal, double speedKBps);
    void downloadFinished(const QString &filePath, bool success, const QString &error);
    void downloadError(const QString &filePath, const QString &message);
};
class DL_Man : public QDialog {
    Q_OBJECT
public:
    explicit DL_Man(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Downloads");
        setWindowIcon(QIcon::fromTheme("folder-download"));
        resize(600, 400);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        listWidget = new QListWidget(this);
        listWidget->setAlternatingRowColors(true);
        listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        mainLayout->addWidget(listWidget);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &DL_Man::close);
        mainLayout->addWidget(buttonBox);

        setAttribute(Qt::WA_DeleteOnClose);
    }

    ~DL_Man() {
        for (auto &item : m_downloadItems) {
            if (item.downloader && item.downloader->isDownloading()) {
                item.downloader->cancelDownload();
            }
        }
        m_downloadItems.clear();
    }

    void addDownload(const QUrl &url, const QString &fileName) {
        show();
        raise();
        activateWindow();

        QString downloadDir = Settings_Man::value("download/path",
                                                  QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
        QString fullPath = generateUniqueFilePath(downloadDir, fileName);

        Qtdl *downloader = new Qtdl(this);

        QWidget *downloadWidget = new QWidget();
        QVBoxLayout *vbox = new QVBoxLayout(downloadWidget);
        vbox->setContentsMargins(8, 8, 8, 8);

        QLabel *fileLabel = new QLabel(QFileInfo(fileName).fileName());
        fileLabel->setStyleSheet("font-weight: bold;");

        QLabel *statusLabel = new QLabel("Preparing... - 0% (0 KB/s)");

        QProgressBar *progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);

        QHBoxLayout *hbox = new QHBoxLayout();
        QPushButton *actionBtn = new QPushButton("Cancel");
        actionBtn->setIcon(QIcon::fromTheme("process-stop"));

        hbox->addStretch();
        hbox->addWidget(actionBtn);

        vbox->addWidget(fileLabel);
        vbox->addWidget(statusLabel);
        vbox->addWidget(progressBar);
        vbox->addLayout(hbox);

        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(downloadWidget->sizeHint());
        listWidget->addItem(item);
        listWidget->setItemWidget(item, downloadWidget);

        DownloadItem di;
        di.downloader = downloader;
        di.item = item;
        di.widget = downloadWidget;
        di.fileLabel = fileLabel;
        di.statusLabel = statusLabel;
        di.progressBar = progressBar;
        di.actionBtn = actionBtn;
        di.fileName = fileName;
        di.filePath = fullPath;
        di.url = url;
        m_downloadItems.append(di);

        connect(downloader, &Qtdl::progress,
                this, [this, downloader](qint64 received, qint64 total, double speed) {
                    updateDownloadProgress(downloader, received, total, speed);
                });

        connect(downloader, &Qtdl::downloadFinished,
                this, [this, downloader](const QString &filePath, bool success, const QString &error) {
                    handleDownloadFinished(downloader, filePath, success, error);
                });

        connect(downloader, &Qtdl::downloadError,
                this, [this, downloader](const QString &filePath, const QString &message) {
                    handleDownloadError(downloader, filePath, message);
                });

        connect(actionBtn, &QPushButton::clicked,
                this, [this, downloader]() {
                    cancelOrRemoveDownload(downloader);
                });

        downloader->startDownload(url, fullPath);
    }

signals:
    void downloadCancelled(Qtdl *downloader);

private:
    struct DownloadItem {
        Qtdl *downloader = nullptr;
        QListWidgetItem *item = nullptr;
        QWidget *widget = nullptr;
        QLabel *fileLabel = nullptr;
        QLabel *statusLabel = nullptr;
        QProgressBar *progressBar = nullptr;
        QPushButton *actionBtn = nullptr;
        QString fileName;
        QString filePath;
        QUrl url;
        bool completed = false;
        bool failed = false;
    };

    QString generateUniqueFilePath(const QString &directory, const QString &fileName) {
        QDir dir(directory);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString baseName = QFileInfo(fileName).completeBaseName();
        QString suffix = QFileInfo(fileName).suffix();
        QString filePath = dir.filePath(fileName);

        if (!QFile::exists(filePath)) {
            return filePath;
        }

        int counter = 1;
        while (QFile::exists(filePath)) {
            QString newFileName = QString("%1 (%2)").arg(baseName).arg(counter);
            if (!suffix.isEmpty()) {
                newFileName += "." + suffix;
            }
            filePath = dir.filePath(newFileName);
            counter++;
        }

        return filePath;
    }

    void updateDownloadProgress(Qtdl *downloader, qint64 received, qint64 total, double speed) {
        for (auto &di : m_downloadItems) {
            if (di.downloader == downloader) {
                if (total > 0) {
                    int percent = static_cast<int>((received * 100) / total);
                    QString speedStr = QString::number(speed, 'f', 1);
                    QString sizeStr = formatFileSize(received);
                    if (total > 0) {
                        sizeStr += " / " + formatFileSize(total);
                    }

                    di.progressBar->setValue(percent);
                    di.statusLabel->setText(
                        QString("%1 - %2% (%3 KB/s)").arg(sizeStr).arg(percent).arg(speedStr));
                }
                break;
            }
        }
    }

    void handleDownloadFinished(Qtdl *downloader, const QString &filePath, bool success,
                                const QString &error) {
         Q_UNUSED(filePath);
        for (auto &di : m_downloadItems) {
            if (di.downloader == downloader) {
                di.completed = true;
                di.failed = !success;

                if (success) {
                    di.statusLabel->setText("Completed");
                    di.progressBar->setValue(100);
                    di.actionBtn->setText("Remove");
                    di.actionBtn->setIcon(QIcon::fromTheme("edit-delete"));

                    QTimer::singleShot(10000, this, [this, downloader]() {
                        removeDownloadItem(downloader);
                    });
                } else {
                    di.statusLabel->setText("Failed: " + error);
                    di.progressBar->setValue(0);
                    di.actionBtn->setText("Retry");
                    di.actionBtn->setIcon(QIcon::fromTheme("view-refresh"));
                }
                break;
            }
        }
    }

    void handleDownloadError(Qtdl *downloader, const QString &filePath, const QString &message) {
         Q_UNUSED(filePath);
        for (auto &di : m_downloadItems) {
            if (di.downloader == downloader) {
                di.statusLabel->setText("Error: " + message);
                di.progressBar->setValue(0);
                di.actionBtn->setText("Retry");
                di.actionBtn->setIcon(QIcon::fromTheme("view-refresh"));
                di.failed = true;
                break;
            }
        }
    }

    void cancelOrRemoveDownload(Qtdl *downloader) {
        for (int i = 0; i < m_downloadItems.size(); ++i) {
            if (m_downloadItems[i].downloader == downloader) {
                auto &di = m_downloadItems[i];

                if (di.completed) {
                    removeDownloadItem(downloader);
                } else if (di.failed) {
                    di.failed = false;
                    di.statusLabel->setText("Retrying...");
                    di.actionBtn->setText("Cancel");
                    di.actionBtn->setIcon(QIcon::fromTheme("process-stop"));
                    di.downloader->startDownload(di.url, di.filePath);
                } else {
                    di.downloader->cancelDownload();
                    di.statusLabel->setText("Cancelled");
                    di.actionBtn->setText("Remove");
                    di.actionBtn->setIcon(QIcon::fromTheme("edit-delete"));

                    QTimer::singleShot(5000, this, [this, downloader]() {
                        removeDownloadItem(downloader);
                    });
                }
                break;
            }
        }
    }

    void removeDownloadItem(Qtdl *downloader) {
        for (int i = 0; i < m_downloadItems.size(); ++i) {
            if (m_downloadItems[i].downloader == downloader) {
                QListWidgetItem *item = m_downloadItems[i].item;
                int row = listWidget->row(item);
                QWidget *widget = listWidget->itemWidget(item);
                listWidget->takeItem(row);
                delete widget;
                delete item;

                downloader->deleteLater();

                m_downloadItems.removeAt(i);

                if (m_downloadItems.isEmpty()) {
                    close();
                }
                break;
            }
        }
    }

    QString formatFileSize(qint64 bytes) {
        const qint64 KB = 1024;
        const qint64 MB = KB * 1024;
        const qint64 GB = MB * 1024;

        if (bytes >= GB) {
            return QString("%1 GB").arg(bytes / (double)GB, 0, 'f', 1);
        } else if (bytes >= MB) {
            return QString("%1 MB").arg(bytes / (double)MB, 0, 'f', 1);
        } else if (bytes >= KB) {
            return QString("%1 KB").arg(bytes / (double)KB, 0, 'f', 1);
        } else {
            return QString("%1 bytes").arg(bytes);
        }
    }

    QListWidget *listWidget;
    QList<DownloadItem> m_downloadItems;
};
class Fav_Window : public QDialog
{
    Q_OBJECT
public:
    Fav_Window(QVariantList list, QWidget *parent=nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Favorites");
        setWindowIcon(QIcon::fromTheme("star"));
        auto *lay = new QVBoxLayout(this);

        listW = new QListWidget;
        listW->setSelectionMode(QAbstractItemView::SingleSelection);
        listW->setEditTriggers(QAbstractItemView::NoEditTriggers);
        lay->addWidget(listW);

        net = new QNetworkAccessManager(this);

        for (const auto &v : list) {
            auto m = v.toMap();
            auto *it = new QListWidgetItem(m["title"].toString() + " – " + m["url"].toString());
            it->setData(Qt::UserRole, m);
            listW->addItem(it);
            fetchFavicon(it, m["url"].toString());
        }

        auto *bbox = new QDialogButtonBox(QDialogButtonBox::Close);
        lay->addWidget(bbox);
        connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        connect(listW, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it) {
            it->setFlags(it->flags() | Qt::ItemIsEditable);
            listW->editItem(it);
        });

        auto *shortcut = new QShortcut(QKeySequence(Qt::Key_Return), listW);
        connect(shortcut, &QShortcut::activated, this, [this]() {
            QVariantMap m; m["title"] = "New Favorite"; m["url"] = "http://";
            auto *it = new QListWidgetItem(m["title"].toString() + " – " + m["url"].toString());
            it->setData(Qt::UserRole, m);
            it->setFlags(it->flags() | Qt::ItemIsEditable);
            listW->addItem(it);
            listW->setCurrentItem(it);
            listW->editItem(it);
            fetchFavicon(it, m["url"].toString());
        });

        connect(listW, &QListWidget::itemChanged, this, [](QListWidgetItem *it) {
            QStringList parts = it->text().split("–");
            QVariantMap m;
            if (parts.size() >= 2) {
                m["title"] = parts[0].trimmed();
                m["url"]   = parts[1].trimmed();
            } else {
                m["title"] = it->text();
                m["url"]   = "";
            }
            it->setData(Qt::UserRole, m);
        });

        resize(500,400);
    }

    QVariantList result() const
    {
        QVariantList out;
        for (int i=0; i<listW->count(); ++i) {
            auto *it = listW->item(i);
            out.append(it->data(Qt::UserRole));
        }
        return out;
    }

private:
    QListWidget *listW;
    QNetworkAccessManager *net;

    void fetchFavicon(QListWidgetItem *it, const QString &urlStr) {
        QUrl url(urlStr);
        if (!url.isValid()) {
            it->setIcon(QIcon::fromTheme("emblem-favorite"));
            return;
        }
        QUrl iconUrl = url.resolved(QUrl("/favicon.ico"));
        QNetworkReply *reply = net->get(QNetworkRequest(iconUrl));
        connect(reply, &QNetworkReply::finished, this, [reply, it]() {
            QByteArray data = reply->readAll();
            QPixmap pix;
            if (pix.loadFromData(data)) {
                it->setIcon(QIcon(pix));
            } else {
                it->setIcon(QIcon::fromTheme("emblem-favorite"));
            }
            reply->deleteLater();
        });
    }
};
class Browser_WManager : public QMainWindow { // W = window
    Q_OBJECT
public:
    void openUrls(const QList<QUrl> &urls);
    Browser_WManager() {
        setWindowTitle("Onu Browser");
        resize(1300, 768);
        setWindowIcon(QIcon::fromTheme("onu"));
        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        tabWidget = new QTabWidget(this);
        tabWidget->setTabsClosable(true);
        tabWidget->setMovable(true);
        tabWidget->setDocumentMode(true);
        tabWidget->setTabPosition(QTabWidget::North);
        tabWidget->tabBar()->setShape(QTabBar::RoundedNorth);
        tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        layout = new QVBoxLayout(centralWidget);
        layout->setContentsMargins(0,0,0,0);
        layout->addWidget(tabWidget);


        createToolBars();
        createMenuBar();
        connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested,
                this, &Browser_WManager::handleDownload);
        tabBar->setAcceptDrops(true);
        tabBar->setMouseTracking(true);
        tabBar->installEventFilter(this);
        setupShortcuts();
        applyTheme(Settings_Man::value("theme", 0).toInt());
        QTimer::singleShot(0, this, [this]() {
            restoreSession();
            restoreToolbars();
            loadHistoryIntoCompleter();
        });
    }
    ~Browser_WManager() {
        saveSession();
        saveToolbars();
    }

    int addNewTab(const QUrl &url = QUrl("onu://home")) {
        QWidget *tab = new QWidget();
        QVBoxLayout *tabLayout = new QVBoxLayout(tab);
        tabLayout->setContentsMargins(0, 0, 0, 0);
        QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();
        Onu_Web *page = new Onu_Web(profile);
        QWebEngineView *webView = new QWebEngineView(tab);
        webView->setPage(page);
        QWebEngineSettings *settings = page->settings();
        settings->setAttribute(QWebEngineSettings::JavascriptEnabled,
                               Settings_Man::value("privacy/javascript", true).toBool());
        settings->setAttribute(QWebEngineSettings::AutoLoadImages,
                               Settings_Man::value("privacy/images", true).toBool());
        webView->setUrl(url);
        webView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(webView, &QWidget::customContextMenuRequested, this, [this, webView](const QPoint &pos) {
            QMenu menu(this);
            if (Settings_Man::value("developer/enabled", false).toBool()) {
                QAction *inspectAction = menu.addAction(QIcon::fromTheme("tools-report-bug"), "Inspect Element");
                connect(inspectAction, &QAction::triggered, this, [this, webView]() {
                    QWebEngineView *devTools = new QWebEngineView(this);
                    webView->page()->setDevToolsPage(devTools->page());
                    devTools->setWindowTitle("Developer Tools");
                    devTools->setWindowIcon(QIcon::fromTheme("applications-development"));
                    devTools->resize(1000, 600);
                    devTools->show();
                });
                menu.addSeparator();
            }
            QAction *backAct = menu.addAction(QIcon::fromTheme("go-previous"), "Back");
            backAct->setEnabled(webView->history()->canGoBack());
            connect(backAct, &QAction::triggered, webView, &QWebEngineView::back);
            QAction *forwardAct = menu.addAction(QIcon::fromTheme("go-next"), "Forward");
            forwardAct->setEnabled(webView->history()->canGoForward());
            connect(forwardAct, &QAction::triggered, webView, &QWebEngineView::forward);
            menu.addAction(QIcon::fromTheme("view-refresh"), "Reload", webView, &QWebEngineView::reload);
            menu.addSeparator();
            menu.addAction(QIcon::fromTheme("edit-copy"), "Copy URL", this, [webView]() {
                QApplication::clipboard()->setText(webView->url().toString());
            });
            menu.addAction(QIcon::fromTheme("text-html"), "View Page Source", this, &Browser_WManager::viewPageSource);
            menu.exec(webView->mapToGlobal(pos));
        });
        connect(page, &Onu_Web::newTabRequested, this, [this](const QUrl &url) {
            addNewTab(url);
        });
        connect(webView, &QWebEngineView::urlChanged, page, &Onu_Web::onUrlChanged);
        connect(page, &Onu_Web::gameModeRequested, this, &Browser_WManager::handleGameModeRequest);
        tabLayout->addWidget(webView);
        int index = tabWidget->addTab(tab, "New Tab");
        tabWidget->setCurrentIndex(index);
        tab->setProperty("url", url);
        connect(webView, &QWebEngineView::loadStarted, this, &Browser_WManager::loadStarted);
        connect(webView, &QWebEngineView::loadFinished, this, &Browser_WManager::loadFinished);
        connect(webView, &QWebEngineView::urlChanged, this, &Browser_WManager::urlChanged);
        connect(webView, &QWebEngineView::titleChanged, this, [this, index](const QString &title) {
            tabWidget->setTabText(index, title.isEmpty() ? "New Tab" : title);
            tabBar->setTabToolTip(index, title);
        });
        connect(webView, &QWebEngineView::iconChanged, this, [this, index, webView](const QIcon &icon) {
            tabBar->setTabIcon(index, icon.isNull() ? getFallbackIcon(webView->url()) : icon);
        });
        connect(webView->page(), &QWebEnginePage::linkHovered, this, &Browser_WManager::linkHovered);
        connect(webView->page(), &QWebEnginePage::permissionRequested, this,
                [](const QWebEnginePermission &permission) {
                    QString origin = permission.origin().host();
                    QString type;
                    switch (permission.permissionType()) {
                    case QWebEnginePermission::PermissionType::Notifications:
                        type = "Notifications";
                        break;
                    case QWebEnginePermission::PermissionType::Geolocation:
                        type = "Geolocation";
                        break;
                    case QWebEnginePermission::PermissionType::MediaAudioCapture:
                    case QWebEnginePermission::PermissionType::MediaVideoCapture:
                    case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
                        type = "Camera/Mic";
                        break;
                    case QWebEnginePermission::PermissionType::MouseLock:
                        type = "MouseLock";
                        break;
                    case QWebEnginePermission::PermissionType::DesktopVideoCapture:
                        type = "ScreenCapture";
                        break;
                    default:
                        type = "Other";
                    }
                    QString key = QString("permissions/%1/%2").arg(origin).arg(type);
                    if (Settings_Man::value(key).toBool()) {
                        bool allowed = Settings_Man::value(key).toBool();
                        if (allowed) {
                            permission.grant();
                        } else {
                            permission.deny();
                        }
                        return;
                    }
                    QMessageBox::StandardButton btn = QMessageBox::question(
                        nullptr, "Permission Request",
                        QString("%1 wants to use %2").arg(origin, type)
                        );
                    bool allow = (btn == QMessageBox::Yes);
                    Settings_Man::setValue(key, allow);
                    if (allow) {
                        permission.grant();
                    } else {
                        permission.deny();
                    }
                });
        if (url != QUrl("onu://home") && url != QUrl("onu://game")) {
            addToHistory(url);
        }
        return index;
    }
    void viewPageSource() {
        QWebEngineView *view = currentWebView();
        if (!view) return;
        view->page()->toHtml([this](const QString &html) {
            QDialog *dlg = new QDialog(this);
            dlg->setWindowTitle("Page Source");
            dlg->resize(800, 600);
            QVBoxLayout *lay = new QVBoxLayout(dlg);
            QTextEdit *te = new QTextEdit(dlg);
            te->setPlainText(html);
            te->setReadOnly(true);
            lay->addWidget(te);
            dlg->resize(800, 600);
            dlg->show();
        });
    }
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == menuBar()) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton && !menuBar()->actionAt(me->pos())) {
                    menuBarDragging = true;
                    menuBarDragStartPos = me->globalPosition().toPoint();
                    return true;
                }
            } else if (event->type() == QEvent::MouseMove && menuBarDragging) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                if ((me->globalPosition().toPoint() - menuBarDragStartPos).manhattanLength() > QApplication::startDragDistance()) {
                    QPoint delta = me->globalPosition().toPoint() - menuBarDragStartPos;
                    move(pos() + delta);
                    menuBarDragStartPos = me->globalPosition().toPoint();
                }
                return true;
            } else if (event->type() == QEvent::MouseButtonRelease) {
                if (menuBarDragging && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    menuBarDragging = false;
                    return true;
                }
            }
        }
        if (obj == tabBar) {
            if (event->type() == QEvent::Drop) {
                QDropEvent *drop = static_cast<QDropEvent*>(event);
                QUrl url;
                if (drop->mimeData()->hasUrls()) {
                    url = drop->mimeData()->urls().first();
                } else if (drop->mimeData()->hasText()) {
                    QString text = drop->mimeData()->text();
                    if (QUrl(text).isValid()) {
                        url = QUrl(text);
                    }
                }
                if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https")) {
                    addNewTab(url);
                    drop->acceptProposedAction();
                    return true;
                }
            } else if (event->type() == QEvent::HoverMove) {
                int index = tabBar->tabAt(tabBar->mapFromGlobal(QCursor::pos()));
                if (index >= 0) {
                    showTabTooltip(index);
                }
            } else if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::RightButton) {
                    int index = tabBar->tabAt(me->pos());
                    if (index >= 0) {
                        showTabContextMenu(index, me->globalPosition().toPoint());
                        return true;
                    }
                }
            }
        }
        return QMainWindow::eventFilter(obj, event);
    }
    void dragEnterEvent(QDragEnterEvent *e) override
    {
        if (e->mimeData()->hasUrls() || e->mimeData()->hasText()) e->acceptProposedAction();
    }

    void dropEvent(QDropEvent *e) override
    {
        QUrl url;
        if (e->mimeData()->hasUrls()) url = e->mimeData()->urls().first();
        else if (e->mimeData()->hasText()) url = QUrl(e->mimeData()->text());
        if (!url.isValid() || (url.scheme()!="http"&&url.scheme()!="https")) return;
        addToFavorites(url, QIcon(), url.toString());
        e->acceptProposedAction();
    }
private slots:
    void addToHistory(const QUrl &url) {
        QWebEngineView *view = currentWebView();
        if (!view || url.scheme() == "onu") return;
        QSettings settings;
        settings.beginGroup("history");
        QDateTime now = QDateTime::currentDateTime();
        QVariantMap entry{
            {"url", url.toString()},
            {"title", view->title().isEmpty() ? url.host() : view->title()},
            {"time", now.toString(Qt::ISODate)}
        };
        settings.setValue(QString::number(now.toMSecsSinceEpoch()), entry);
        settings.endGroup();
        loadHistoryIntoCompleter();
    }
    void navigateToUrl() {
        QString text = urlBar->text();
        if (text.isEmpty()) return;
        QUrl url(text);
        if (url.scheme().isEmpty()) {
            if (text.contains('.') && !text.contains(' ')) {
                url.setScheme("https");
            } else {
                url = QUrl("https://duckduckgo.com/?q=" + text);
            }
        }
        currentWebView()->setUrl(url);
    }
    void updateUrl(const QUrl &url) {
        urlBar->setText(url.toString());
        urlBar->setCursorPosition(0);
        urlBar->setPlaceholderText("Search or enter URL");
    }
    void goBack() { if (currentWebView()) currentWebView()->back(); }
    void goForward() { if (currentWebView()) currentWebView()->forward(); }
    void reloadPage() { if (currentWebView()) currentWebView()->reload(); }
    void hardReload() {
        if (currentWebView()) {
            QWebEnginePage *page = currentWebView()->page();
            page->triggerAction(QWebEnginePage::ReloadAndBypassCache);
        }
    }
    void stopPage() { if (currentWebView()) currentWebView()->stop(); }
    void newHomeTab() {
        QString home = Settings_Man::value("homepage", "onu://home").toString();
        addNewTab(QUrl(home));
    }

    void homePage() {
        QString home = Settings_Man::value("homepage", "onu://home").toString();
        currentWebView()->setUrl(QUrl(home));
    }


    void newTab() { addNewTab(); }
    void closeTab(int index) {
        if (tabWidget->count() > 1) {
            QWidget *w = tabWidget->widget(index);
            tabWidget->removeTab(index);
            delete w;
        } else {
            saveSession();
            QApplication::quit();
        }
    }
    void openGame() {
        addNewTab(QUrl("onu://game"));
    }
    void openSettings() {
        Settings_window dialog(this);
        connect(&dialog, &Settings_window::settingsChanged, this, [this]() {
            applyTheme(Settings_Man::value("theme", 0).toInt());
            for (int i = 0; i < tabWidget->count(); ++i) {
                QWidget *tab = tabWidget->widget(i);
                QWebEngineView *view = tab->findChild<QWebEngineView*>();
                if (view) {
                    Onu_Web *page = qobject_cast<Onu_Web*>(view->page());
                    if (page) page->loadHostsFilter();
                    QWebEngineSettings *s = view->page()->settings();
                    s->setAttribute(QWebEngineSettings::JavascriptEnabled,
                                    Settings_Man::value("privacy/javascript", true).toBool());
                    s->setAttribute(QWebEngineSettings::AutoLoadImages,
                                    Settings_Man::value("privacy/images", true).toBool());
                }
            }
            recentToolBar->setVisible(Settings_Man::value("recent/enabled",true).toBool());
            favToolBar->setVisible( Settings_Man::value("favorites/enabled",true).toBool());
        });
        dialog.exec();
    }
    void showDownloads() {
        if (!downloadManager) {
            downloadManager = new DL_Man(this);
            connect(downloadManager, &DL_Man::downloadCancelled,
                    [](Qtdl* d){ d->deleteLater(); });
        }
        downloadManager->show();
    }
    void handleDownload(QWebEngineDownloadRequest *download) {
        if (!download) return;

        QString fileName = download->downloadFileName();
        QUrl url = download->url();

        if (fileName.isEmpty()) {
            QString path = url.path();
            if (!path.isEmpty() && path != "/") {
                fileName = QFileInfo(path).fileName();
            }

            if (fileName.isEmpty()) {
                QString host = url.host();
                if (!host.isEmpty()) {
                    fileName = host + ".html";
                } else {
                    fileName = "download";
                }
            }


            if (!fileName.contains('.')) {

                QString mimeType = download->mimeType();
                if (mimeType.contains("pdf")) {
                    fileName += ".pdf";
                } else if (mimeType.contains("image")) {
                    fileName += ".jpg";
                } else if (mimeType.contains("text/html")) {
                    fileName += ".html";
                } else {
                    fileName += ".bin";
                }
            }
        }

        bool askBeforeDownload = Settings_Man::value("download/ask_before", true).toBool();
        if (askBeforeDownload) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Download File");
            msgBox.setText(QString("Download %1 from %2?").arg(fileName).arg(url.host()));
            msgBox.setInformativeText("Size: " + (download->totalBytes() > 0
                                                      ? QString::number(download->totalBytes() / 1024) + " KB"
                                                      : "Unknown size"));
            msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Save);
            msgBox.setIcon(QMessageBox::Question);

            if (msgBox.exec() != QMessageBox::Save) {
                download->cancel();
                download->deleteLater();
                return;
            }
        }


        download->cancel();
        download->deleteLater();

        if (!downloadManager) {
            downloadManager = new DL_Man(this);
            connect(downloadManager, &DL_Man::finished, this, [this]() {
                downloadManager->deleteLater();
                downloadManager = nullptr;
            });
        }

        downloadManager->addDownload(url, fileName);


        statusBar->showMessage(QString("Downloading %1...").arg(fileName), 3000);
    }
    void loadStarted() {
        stopAction->setVisible(true);
        reloadAction->setVisible(false);
        statusBar->showMessage("Loading...");
    }
    void loadFinished(bool ok) {
        stopAction->setVisible(false);
        reloadAction->setVisible(true);
        statusBar->showMessage(ok ? "Done" : "Failed to load page", 2000);
    }
    void urlChanged(const QUrl &url) {
        if (currentWebView() && sender() == currentWebView()) {
            updateUrl(url);
            if (url.scheme() != "onu") {
                addToHistory(url);
                updateRecentTabs(url, currentWebView()->icon(), currentWebView()->title());
            }
        }
    }
    void linkHovered(const QString &url) {
        statusBar->showMessage(url, 3000);
    }
    void updateToolbar(int index)
    {
        if (index < 0) return;
        QWebEngineView *view = currentWebView();
        if (!view) return;
        if (!backAction) return;
        backAction->setEnabled(view->history()->canGoBack());
        forwardAction->setEnabled(view->history()->canGoForward());
        updateUrl(view->url());
    }
    void handleGameModeRequest(const QUrl& gameUrl) {
        QString mode = Settings_Man::value("gaming/auto_mode", "prompt").toString();
        if (mode == "never") { currentWebView()->setUrl(QUrl("onu://home")); return; }
        if (mode == "always") { applyGameMode(); return; }
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "🎮 Game Detected", QString("Enable Game Mode for %1?").arg(gameUrl.host()));
        if (reply == QMessageBox::Yes) applyGameMode();
    }
    void applyGameMode() {
        QString flags = Settings_Man::value("flags/chromium_flags", "").toString();
        flags.replace("--disable-gpu", "").replace("--disable-software-rasterizer", "");
        flags += " --enable-gpu-rasterization --enable-accelerated-2d-canvas --max_renderer_process_memory=1024";
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.toUtf8());
        if (QWebEngineView* view = currentWebView()) {
            view->page()->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
            view->page()->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
        }
        statusBar->showMessage("🎮 Game Mode ON", 3000);
    }
    void toggleGameMode(bool enable) {
        Settings_Man::setValue("gaming/active", enable);
        if (enable) applyGameMode();
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
                Settings_Man::value("flags/chromium_flags","").toByteArray());
    }
    void refreshRecentToolbar()
    {
        recentToolBar->clear();
        QSettings s;
        recentTabs = s.value("recentTabs").value<QVariantList>();

        qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
        QVariantList filtered;
        for (const auto &v : recentTabs) {
            auto m = v.toMap();
            if (m.value("pinned").toBool() ||
                now - m["time"].toLongLong() < 24*3600*1000)
                filtered.append(m);
        }
        recentTabs = filtered;

        for (int i = 0; i < std::min(3LL, (long long)recentTabs.size()); ++i) {
            auto m = recentTabs[i].toMap();
            QUrl url(m["url"].toString());
            QIcon icon;
            QByteArray ba = QByteArray::fromBase64(m["icon"].toByteArray());
            QPixmap px; px.loadFromData(ba);
            icon = px.isNull() ? QIcon::fromTheme("applications-webapps") : QIcon(px);

            auto *act = recentToolBar->addAction(icon, {});
            act->setToolTip(m["title"].toString() + "\n" + url.toString());
            connect(act, &QAction::triggered, this, [this, url]{
                currentWebView()->setUrl(url);
            });
        }
        auto *allAct = recentToolBar->addAction(QIcon::fromTheme("document-open-recent"), {});
        allAct->setToolTip("Manage recent history…");
        connect(allAct, &QAction::triggered, this, &Browser_WManager::showHistoryManager);
    }
    void refreshFavToolbar()
    {
        favToolBar->clear();
        QSettings s;
        favorites = s.value("favorites").value<QVariantList>();

        for (int i = 0; i < std::min(4LL, (long long)favorites.size()); ++i) {
            auto m = favorites[i].toMap();
            QUrl url(m["url"].toString());
            QIcon icon;
            QByteArray ba = QByteArray::fromBase64(m["icon"].toByteArray());
            QPixmap px; px.loadFromData(ba);
            icon = px.isNull() ? QIcon::fromTheme("applications-webapps") : QIcon(px);

            auto *act = favToolBar->addAction(icon, {});
            act->setToolTip(m["title"].toString() + "\n" + url.toString());
            connect(act, &QAction::triggered, this, [this, url]{
                currentWebView()->setUrl(url);
            });
        }
        auto *editAct = favToolBar->addAction(QIcon::fromTheme("document-edit"), {});
        editAct->setToolTip("Edit favorites…");
        connect(editAct, &QAction::triggered, this, &Browser_WManager::showFav_Window);
    }
    void updateRecentTabs(const QUrl &url, const QIcon &icon, const QString &title)
    {
        for (auto it = recentTabs.begin(); it != recentTabs.end(); ++it)
            if ((*it).toMap()["url"].toString() == url.toString()) {
                recentTabs.erase(it); break;
            }

        QPixmap pm = icon.pixmap(32,32);
        QImage img = pm.toImage();
        QByteArray ba; QBuffer buf(&ba); buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");

        QVariantMap entry{
            {"url", url.toString()},
            {"icon", ba.toBase64()},
            {"title", title},
            {"pinned", false},
            {"time", QDateTime::currentDateTime().toMSecsSinceEpoch()}
        };
        recentTabs.prepend(entry);
        if (recentTabs.size() > 50) recentTabs.removeLast();
        QSettings s; s.setValue("recentTabs", QVariant::fromValue(recentTabs));
        refreshRecentToolbar();
    }
    void addToFavorites(const QUrl &url, const QIcon &icon, const QString &title)
    {
        for (const auto &v : favorites)
            if (v.toMap()["url"].toString() == url.toString()) return;

        QPixmap pm = icon.pixmap(32,32);
        QImage img = pm.toImage();
        QByteArray ba; QBuffer buf(&ba); buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");

        QVariantMap entry{
            {"url", url.toString()},
            {"icon", ba.toBase64()},
            {"title", title.isEmpty() ? url.host() : title}
        };
        favorites.append(entry);
        QSettings s; s.setValue("favorites", QVariant::fromValue(favorites));
        refreshFavToolbar();
    }
    void showFav_Window()
    {
        Fav_Window dlg(favorites, this);
        if (dlg.exec() == QDialog::Accepted) {
            favorites = dlg.result();
            QSettings s; s.setValue("favorites", QVariant::fromValue(favorites));
            refreshFavToolbar();
        }
    }
    void showHistoryManager()
    {
        auto *dlg = new QDialog(this);
        dlg->setWindowTitle("History Manager");
        dlg->resize(600, 500);

        auto *lay = new QVBoxLayout(dlg);

        auto *list = new QListWidget;
        list->setSelectionMode(QAbstractItemView::SingleSelection);

        QSettings s; s.beginGroup("history");
        const QStringList keys = s.childKeys();
        for (const QString &k : keys) {
            auto m = s.value(k).toMap();
            QString title = m["title"].toString();
            QString url   = m["url"].toString();
            QString time  = m["time"].toString();

            auto *item = new QListWidgetItem(QIcon::fromTheme("emblem-web"),
                                             title + " — " + time);
            item->setToolTip(url);
            item->setData(Qt::UserRole, url);
            list->addItem(item);
        }
        s.endGroup();

        lay->addWidget(list);

        auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Close);
        QPushButton *delAllBtn = new QPushButton(QIcon::fromTheme("edit-delete"), "Delete All");
        btnBox->addButton(delAllBtn, QDialogButtonBox::ActionRole);
        lay->addWidget(btnBox);

        connect(btnBox, &QDialogButtonBox::rejected, dlg, &QDialog::accept);

        connect(delAllBtn, &QPushButton::clicked, this, [list]() {
            list->clear();
            QSettings s; s.beginGroup("history"); s.remove(""); s.endGroup();
        });

        connect(list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it) {
            QUrl url(it->data(Qt::UserRole).toString());
            addNewTab(url);
        });

        list->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(list, &QWidget::customContextMenuRequested, this, [list](const QPoint &pos) {
            QListWidgetItem *it = list->itemAt(pos);
            if (!it) return;
            QMenu menu;
            QAction *delAct = menu.addAction(QIcon::fromTheme("edit-delete"), "Delete This Entry");
            if (menu.exec(list->viewport()->mapToGlobal(pos)) == delAct) {
                QString urlStr = it->data(Qt::UserRole).toString();
                QSettings s; s.beginGroup("history");
                for (const QString &k : s.childKeys()) {
                    auto m = s.value(k).toMap();
                    if (m["url"].toString() == urlStr) { s.remove(k); break; }
                }
                s.endGroup();
                delete it;
            }
        });

        dlg->exec();
    }

private:

    QToolBar *navToolBar      = nullptr;
    QToolBar *recentToolBar   = nullptr;
    QToolBar *favToolBar      = nullptr;
    QToolBar *searchToolBar   = nullptr;
    QToolBar *tabToolBar      = nullptr;
    QTabBar   *tabBar         = nullptr;
    QToolButton *tabAddButton = nullptr;

    QVariantList recentTabs;
    QVariantList favorites;

    QAudioOutput *audioOutput = nullptr;

    QIcon getFallbackIcon(const QUrl &url) const {
        if (url.scheme() == "onu") {
            return QIcon::fromTheme("onu");
        }
        return QIcon::fromTheme("applications-internet");
    }
    bool isSecure(const QUrl &url) const {
        return (url.scheme() == "https" || url.host().isEmpty());
    }
    void showTabTooltip(int tabIndex) {
        QWidget *tab = tabWidget->widget(tabIndex);
        if (!tab) return;
        QWebEngineView *view = tab->findChild<QWebEngineView*>();
        if (!view) return;
        QUrl url = view->url();
        QString title = tabBar->tabText(tabIndex);
        QString security = isSecure(url) ? "Secure (HTTPS)" : "Not Secure (HTTP)";
        QString text = QString("%1\n%2\n%3").arg(title, url.toString(), security);
        QToolTip::showText(QCursor::pos(), text, tabWidget);
    }
    void showTabContextMenu(int tabIndex, const QPoint &globalPos) {
        QWidget *tab = tabWidget->widget(tabIndex);
        if (!tab) return;
        QWebEngineView *view = tab->findChild<QWebEngineView*>();
        if (!view) return;
        QUrl url = view->url();
        QString title = tabBar->tabText(tabIndex);
        QIcon icon = tabBar->tabIcon(tabIndex);
        QMenu menu(this);
        QString security = isSecure(url) ? "Secure" : "Not Secure";
        QAction *info = menu.addAction(icon, title + " - " + security);
        info->setEnabled(false);
        menu.addSeparator();
        QAction *closeAct = menu.addAction(QIcon::fromTheme("window-close"), "Close Tab");
        QAction *duplicateAct = menu.addAction(QIcon::fromTheme("edit-copy"), "Duplicate Tab");
        QAction *viewSourceAct = menu.addAction(QIcon::fromTheme("text-editor"), "View Page Source");
        QAction *selected = menu.exec(globalPos);
        if (!selected) return;
        if (selected == closeAct) {
            closeTab(tabIndex);
        } else if (selected == duplicateAct) {
            addNewTab(view->url());
        } else if (selected == viewSourceAct) {
            view->page()->toHtml([this](const QString &html) {
                QDialog *dlg = new QDialog(this);
                dlg->setWindowTitle("Page Source");
                dlg->resize(800, 600);
                QVBoxLayout *lay = new QVBoxLayout(dlg);
                QTextEdit *te = new QTextEdit(dlg);
                te->setPlainText(html);
                te->setReadOnly(true);
                lay->addWidget(te);
                dlg->show();
            });
        }
    }
    void setupShortcuts() {
        QShortcut *reloadShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
        connect(reloadShortcut, &QShortcut::activated, this, &Browser_WManager::reloadPage);
        QShortcut *hardReloadShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5), this);
        connect(hardReloadShortcut, &QShortcut::activated, this, &Browser_WManager::hardReload);
        if (Settings_Man::value("developer/enabled", false).toBool()) {
            QShortcut *consoleShortcut = new QShortcut(QKeySequence(Qt::Key_F4), this);
            connect(consoleShortcut, &QShortcut::activated, this, [this]() {
                QMessageBox::information(this, "Console", "Console output feature coming soon");
            });
            QShortcut *clearCacheShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Escape), this);
            connect(clearCacheShortcut, &QShortcut::activated, this, [this]() {
                QWebEngineProfile::defaultProfile()->clearHttpCache();
                QMessageBox::information(this, "Cache Cleared", "HTTP cache has been cleared");
            });
            QShortcut *keybindingsShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this);
            connect(keybindingsShortcut, &QShortcut::activated, this, [this]() {
                QMessageBox::information(this, "Keybindings",
                                         "F5 - Reload\nCtrl+F5 - Hard Reload\nCtrl+Esc - Clear Cache (Dev Mode)\nCtrl+K - This help\nCtrl+T - New Tab\nCtrl+W - Close Tab\nCtrl+Q - Quit");
            });
        }
    }
    void createToolBars()
    {
        // --- Navigation toolbar ---
        navToolBar = addToolBar("Navigation");
        navToolBar->setMovable(true);
        navToolBar->setFloatable(true);
        // NAVT
        auto *backAct    = navToolBar->addAction(QIcon::fromTheme("go-previous"), "Back");
        auto *forwardAct = navToolBar->addAction(QIcon::fromTheme("go-next"),    "Forward");
        reloadAction     = navToolBar->addAction(QIcon::fromTheme("view-refresh"), "Reload");
        stopAction       = navToolBar->addAction(QIcon::fromTheme("process-stop"), "Stop");
        navToolBar->addAction(QIcon::fromTheme("go-home"), "Home", this, &Browser_WManager::homePage);

        connect(backAct,    &QAction::triggered, this, &Browser_WManager::goBack);
        connect(forwardAct, &QAction::triggered, this, &Browser_WManager::goForward);
        connect(reloadAction, &QAction::triggered, this, &Browser_WManager::reloadPage);
        connect(stopAction,   &QAction::triggered, this, &Browser_WManager::stopPage);

        // --SVOLUME
        int savedVolume = Settings_Man::value("audio/volume", 50).toInt();

        QSlider *volumeSlider = new QSlider(Qt::Horizontal, navToolBar);
        volumeSlider->setRange(0, 100);
        volumeSlider->setValue(savedVolume);

        QLabel *volumeIcon = new QLabel(navToolBar);
        auto updateIcon = [volumeIcon](int value) {
            QString iconName = (value == 0)
            ? "notification-audio-volume-muted"
            : (value < 50)
                ? "player-volume"
                : "audio-volume-overamplified-symbolic";
            volumeIcon->setPixmap(QIcon::fromTheme(iconName).pixmap(16,16));
        };
        updateIcon(savedVolume);

        navToolBar->addWidget(volumeIcon);
        navToolBar->addWidget(volumeSlider);

        audioOutput = new QAudioOutput(this);
        audioOutput->setVolume(savedVolume / 100.0);

        connect(volumeSlider, &QSlider::valueChanged, this,
                [this, updateIcon](int value) {
                    if (audioOutput) {
                        audioOutput->setVolume(value / 100.0);
                        Settings_Man::setValue("audio/volume", value);
                    }
                    updateIcon(value);
                });


        // RecT
        recentToolBar = addToolBar("Recent");
        recentToolBar->setMovable(true);
        recentToolBar->setFloatable(true);

        int dpi = QApplication::primaryScreen()->logicalDotsPerInch();
        int baseSize = dpi > 120 ? 24 : 22;
        recentToolBar->setIconSize(QSize(baseSize, baseSize));
        recentToolBar->setMinimumWidth(baseSize + 8);

        connect(recentToolBar, &QToolBar::orientationChanged, this, [this](Qt::Orientation o){
            recentToolBar->setToolButtonStyle(
                o == Qt::Vertical ? Qt::ToolButtonIconOnly
                                  : Qt::ToolButtonTextBesideIcon);
            recentToolBar->setIconSize(o == Qt::Vertical ? QSize(22,22) : QSize(16,16));
        });
        recentToolBar->setStyleSheet("QToolButton { margin: 2px; }");
        refreshRecentToolbar();

        // FaV
        favToolBar = addToolBar("Favorites");
        favToolBar->setMovable(true);
        favToolBar->setFloatable(true);
        favToolBar->setIconSize(QSize(baseSize, baseSize));
        favToolBar->setMinimumWidth(baseSize + 8);

        connect(favToolBar, &QToolBar::orientationChanged, this, [this](Qt::Orientation o){
            favToolBar->setToolButtonStyle(
                o == Qt::Vertical ? Qt::ToolButtonIconOnly
                                  : Qt::ToolButtonTextBesideIcon);
            favToolBar->setIconSize(o == Qt::Vertical ? QSize(22,22) : QSize(16,16));
        });
        favToolBar->setStyleSheet("QToolButton { margin: 2px; }");
        refreshFavToolbar();

        // URLBAR ( BTW SOMETHING EXPERIMENTAL WAS REMOVED )
        searchToolBar = addToolBar("Search");
        searchToolBar->setMovable(true);
        searchToolBar->setFloatable(true);

        urlBar = new QLineEdit;
        urlBar->setClearButtonEnabled(true);
        urlBar->setPlaceholderText("Search or enter address");
        historyModel = new QStringListModel(this);
        urlCompleter = new QCompleter(historyModel, urlBar);
        urlCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        urlCompleter->setFilterMode(Qt::MatchContains);
        urlBar->setCompleter(urlCompleter);

        searchToolBar->addWidget(urlBar);
        connect(urlBar, &QLineEdit::returnPressed, this, &Browser_WManager::navigateToUrl);

        // TT
        tabToolBar = addToolBar("Tabs");
        tabToolBar->setMovable(true);
        tabToolBar->setFloatable(true);
        tabToolBar->setAllowedAreas(Qt::AllToolBarAreas);

        tabBar = tabWidget->tabBar();
        tabBar->setParent(nullptr);
        tabToolBar->addWidget(tabBar);

        tabAddButton = new QToolButton(this);
        tabAddButton->setText("+");
        tabAddButton->setFixedSize(26, 26);
        tabToolBar->addWidget(tabAddButton);
        connect(tabAddButton, &QToolButton::clicked, this, &Browser_WManager::newTab);

        connect(tabBar, &QTabBar::tabCloseRequested, this, &Browser_WManager::closeTab);
        connect(tabBar, &QTabBar::currentChanged, this, &Browser_WManager::updateToolbar);

        connect(tabToolBar, &QToolBar::orientationChanged, this, [this](Qt::Orientation o) {
            if (o == Qt::Vertical) {
                tabWidget->setTabPosition(QTabWidget::West);
                tabBar->setShape(QTabBar::RoundedWest);
            } else {
                tabWidget->setTabPosition(QTabWidget::North);
                tabBar->setShape(QTabBar::RoundedNorth);
            }

            bool vertical = (o == Qt::Vertical);
            for (int i = 0; i < tabWidget->count(); ++i) {
                QString fullTitle = tabWidget->tabText(i);
                int maxWords = vertical ? 3 : 8;
                QStringList words = fullTitle.split(' ', Qt::SkipEmptyParts);
                QString elided = (words.size() <= maxWords)
                                     ? fullTitle
                                     : words.mid(0, maxWords).join(' ') + " ..";
                tabWidget->setTabText(i, elided);
            }
        });

        recentToolBar->setVisible(Settings_Man::value("recent/enabled",true).toBool());
        favToolBar->setVisible(Settings_Man::value("favorites/enabled",true).toBool());


        statusBar = new QStatusBar(this);
        setStatusBar(statusBar);
    }
    void createMenuBar() {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);
        menuBar->installEventFilter(this);
        menuBar->setMouseTracking(true);
        QMenu *onuMenu = menuBar->addMenu(QIcon::fromTheme("onu"), "Onu");
        QAction *newTabAct = onuMenu->addAction(QIcon::fromTheme("tab-new"), "New Tab");
        newTabAct->setShortcut(QKeySequence::AddTab);
        connect(newTabAct, &QAction::triggered, this, &Browser_WManager::newTab);
        onuMenu->addSeparator();
        QAction *backAct = onuMenu->addAction(QIcon::fromTheme("go-previous"), "Back");
        backAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
        connect(backAct, &QAction::triggered, this, &Browser_WManager::goBack);
        QAction *forwardAct = onuMenu->addAction(QIcon::fromTheme("go-next"), "Forward");
        forwardAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
        connect(forwardAct, &QAction::triggered, this, &Browser_WManager::goForward);
        QAction *reloadAct = onuMenu->addAction(QIcon::fromTheme("view-refresh"), "Reload");
        reloadAct->setShortcut(QKeySequence::Refresh);
        connect(reloadAct, &QAction::triggered, this, &Browser_WManager::reloadPage);
        QAction *homeAct = onuMenu->addAction(QIcon::fromTheme("go-home"), "Home");
        homeAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Home));
        connect(homeAct, &QAction::triggered, this, &Browser_WManager::homePage);
        onuMenu->addSeparator();

        QAction *gameModeAct = onuMenu->addAction(QIcon::fromTheme("input-gamepad"), "&Game Mode",
                                                  this, &Browser_WManager::toggleGameMode);
        gameModeAct->setCheckable(true);
        gameModeAct->setChecked(Settings_Man::value("gaming/active", false).toBool());

        QAction *downloadsAct = onuMenu->addAction(QIcon::fromTheme("folder-download"), "Downloads");
        downloadsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
        connect(downloadsAct, &QAction::triggered, this, &Browser_WManager::showDownloads);

        onuMenu->addSeparator();

        QAction *settingsAct = onuMenu->addAction(QIcon::fromTheme("preferences-system"), "Settings");
        settingsAct->setShortcut(QKeySequence::Preferences);
        connect(settingsAct, &QAction::triggered, this, &Browser_WManager::openSettings);

        QAction *aboutAct = onuMenu->addAction(QIcon::fromTheme("help-about"), "About Onu");
        connect(aboutAct, &QAction::triggered, this, [this]() {
            QMessageBox::about(this, "About Onu Browser",
                               "<p><b>Version:</b> 0.4</p>"
                               "<p>Built with Qt 6.8 WebEngine</p>"
                               "<p>-------------------------------</p>"
                               "<p>Made in a weekend So for better</p>"
                               "<p>performance expect new releases</p>"
                               "<p>for new releases look at the</p>"
                               "<p>github.com/zynomon/onu</p>");
        });
        onuMenu->addSeparator();
        QAction *quitAct = onuMenu->addAction(QIcon::fromTheme("application-exit"), "Quit");
        quitAct->setShortcut(QKeySequence::Quit);
        connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    }
    QWebEngineView* currentWebView() {
        int index = tabBar->currentIndex();
        if (index < 0) return nullptr;
        QWidget *tab = tabWidget->widget(index);
        return tab ? tab->findChild<QWebEngineView*>() : nullptr;
    }
    void loadHistoryIntoCompleter() {
        QSettings settings;
        settings.beginGroup("history");

        QStringList completions;
        const QStringList keys = settings.childKeys();

        completions.reserve(keys.size());

        for (const QString &key : keys) {
            QVariantMap entry = settings.value(key).toMap();
            QString url = entry["url"].toString();

            if (!url.isEmpty()) {
                QUrl qurl(url);
                QString host = qurl.host();

                if (!host.isEmpty()) {

                    if (host.startsWith("www.")) {
                        host = host.mid(4);
                    }


                    QStringList parts = host.split('.');
                    if (parts.size() >= 2) {

                        QString domain = parts[parts.size()-2] + "." + parts[parts.size()-1];
                        completions << domain;
                    } else {
                        completions << host;
                    }
                }
            }
        }
        settings.endGroup();


        completions.sort(Qt::CaseInsensitive);
        completions.removeDuplicates();

        historyModel->setStringList(completions);
    }
    QString formatFileSize(qint64 bytes) {
        const qint64 KB = 1024;
        const qint64 MB = KB * 1024;
        const qint64 GB = MB * 1024;

        if (bytes >= GB) {
            return QString("%1 GB").arg(bytes / (double)GB, 0, 'f', 1);
        } else if (bytes >= MB) {
            return QString("%1 MB").arg(bytes / (double)MB, 0, 'f', 1);
        } else if (bytes >= KB) {
            return QString("%1 KB").arg(bytes / (double)KB, 0, 'f', 1);
        } else {
            return QString("%1 bytes").arg(bytes);
        }
    }
    void applyTheme(int themeIndex) {
        QPalette palette;
        QString appliedCSS;

        if (themeIndex == 2) {

            palette.setColor(QPalette::Window, QColor(30, 30, 30));
            palette.setColor(QPalette::WindowText, QColor(224, 224, 224));
            palette.setColor(QPalette::Base, QColor(45, 45, 45));
            palette.setColor(QPalette::AlternateBase, QColor(37, 37, 37));
            palette.setColor(QPalette::ToolTipBase, Qt::black);
            palette.setColor(QPalette::ToolTipText, Qt::white);
            palette.setColor(QPalette::Text, QColor(224, 224, 224));
            palette.setColor(QPalette::Button, QColor(37, 37, 37));
            palette.setColor(QPalette::ButtonText, QColor(160, 214, 160));
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Link, QColor(102, 255, 102));
            palette.setColor(QPalette::Highlight, QColor(102, 255, 102));
            palette.setColor(QPalette::HighlightedText, Qt::black);
            appliedCSS = BuiltInDarkSS;
        } else if (themeIndex == 1) {

            palette = QApplication::style()->standardPalette();
            appliedCSS = BuiltInLightSS;
        } else if (themeIndex == 3) {

            palette = QApplication::style()->standardPalette();
            appliedCSS = Settings_Man::value("stylesheet", "").toString();
        } else {

            palette = QApplication::style()->standardPalette();
            appliedCSS.clear();
        }


        QApplication::setPalette(palette);
        QFont appFont = QApplication::font();
        appFont.setFamily(Settings_Man::value("ui/fontFamily", appFont.family()).toString());
        appFont.setPointSize(Settings_Man::value("ui/fontSize", appFont.pointSize()).toInt());
        QApplication::setFont(appFont);


        qApp->setStyleSheet(appliedCSS);


        if (themeIndex == 3) {
            tabBar->setStyleSheet("");
            if (navToolBar) navToolBar->setStyleSheet("");
            if (tabToolBar) tabToolBar->setStyleSheet("");
            if (searchToolBar) searchToolBar->setStyleSheet("");
            if (recentToolBar) recentToolBar->setStyleSheet("");
            if (favToolBar) favToolBar->setStyleSheet("");
        } else if (themeIndex == 2) {

            tabBar->setStyleSheet(R"(
            QTabBar::tab { background: #252525; color: #a0d6a0; padding: 8px 12px; min-width: 120px; max-width: 180px; }
            QTabBar::tab:selected { background: #1a1a1a; border-bottom: 2px solid #66ff66; }
            QTabBar::tab:hover { background: #333; }
        )");
        } else if (themeIndex == 1) {

            tabBar->setStyleSheet(R"(
            QTabBar::tab { background: #e9ecef; color: #495057; padding: 8px 12px; min-width: 120px; max-width: 180px; }
            QTabBar::tab:selected { background: #ffffff; border-bottom: 2px solid #3498db; }
            QTabBar::tab:hover { background: #d0d0d0; }
        )");
        } else {

            tabBar->setStyleSheet("");
        }
    }
    void saveSession() {
        QSettings settings;
        settings.beginWriteArray("tabs");
        int savedIndex = 0;
        for (int i = 0; i < tabWidget->count(); ++i) {
            QUrl url = tabWidget->widget(i)->property("url").toUrl();
            if (url.isEmpty()) {
                QWebEngineView *view = tabWidget->widget(i)->findChild<QWebEngineView*>();
                if (view) url = view->url();
            }
            if (!url.isEmpty() && url.scheme() != "onu") {
                settings.setArrayIndex(savedIndex++);
                settings.setValue("url", url.toString());
            }
        }
        settings.endArray();
    }
    void restoreSession() {
        QSettings settings;
        int size = settings.beginReadArray("tabs");
        if (size > 0) {
            for (int i = 0; i < size; ++i) {
                settings.setArrayIndex(i);
                QString urlStr = settings.value("url").toString();
                if (!urlStr.isEmpty()) {
                    addNewTab(QUrl(urlStr));
                }
            }
        } else {
            addNewTab();
        }
        settings.endArray();
    }
    void saveToolbars() {
        QSettings settings;
        settings.setValue("toolbars/navArea", toolBarArea(navToolBar));
        settings.setValue("toolbars/tabArea", toolBarArea(tabToolBar));
        settings.setValue("toolbars/recentArea", toolBarArea(recentToolBar));
        settings.setValue("toolbars/favArea", toolBarArea(favToolBar));
        settings.setValue("toolbars/searchArea", toolBarArea(searchToolBar));
    }
    void restoreToolbars() {
        QSettings settings;
        addToolBar(Qt::ToolBarArea(settings.value("toolbars/navArea", Qt::TopToolBarArea).toInt()), navToolBar);
        addToolBar(Qt::ToolBarArea(settings.value("toolbars/tabArea", Qt::TopToolBarArea).toInt()), tabToolBar);
        addToolBar(Qt::ToolBarArea(settings.value("toolbars/recentArea", Qt::LeftToolBarArea).toInt()), recentToolBar);
        addToolBar(Qt::ToolBarArea(settings.value("toolbars/favArea", Qt::LeftToolBarArea).toInt()), favToolBar);
        addToolBar(Qt::ToolBarArea(settings.value("toolbars/searchArea", Qt::TopToolBarArea).toInt()), searchToolBar);
    }
    static const QString BuiltInLightSS;
    static const QString BuiltInDarkSS;
    QWidget *centralWidget{};
    QVBoxLayout *layout{};
    QTabWidget *tabWidget{};
    QLineEdit *urlBar{};
    QCompleter *urlCompleter = nullptr;
    QStringListModel *historyModel = nullptr;
    QStatusBar *statusBar{};
    DL_Man* downloadManager = nullptr;
    QAction *backAction{};
    QAction *forwardAction{};
    QAction *reloadAction{};
    QAction *stopAction{};
    bool menuBarDragging = false;
    QPoint menuBarDragStartPos;
};
const QString Browser_WManager::BuiltInLightSS = R"(
    QMainWindow, QWidget { background: #ffffff; color: #2c3e50; }
    QLineEdit, QTextEdit { background: #f8f9fa; border: 1px solid #ced4da; border-radius: 4px; padding: 4px; }
    QTabBar::tab { background: #e9ecef; color: #495057; padding: 8px 12px; min-width: 120px; max-width: 180px; }
    QTabBar::tab:selected { background: #ffffff; border-bottom: 2px solid #3498db; }
    QToolBar { background: #f1f3f5; border-bottom: 1px solid #dee2e6; }
    QStatusBar { background: #f8f9fa; color: #6c757d; }
)";
const QString Browser_WManager::BuiltInDarkSS = R"(
    QMainWindow, QWidget { background: #1e1e1e; color: #e0e0e0; }
    QLineEdit, QTextEdit { background: #2d2d2d; border: 1px solid #444; border-radius: 4px; padding: 4px; color: #00ff00; }
    QTabBar::tab { background: #252525; color: #a0d6a0; padding: 8px 12px; min-width: 120px; max-width: 180px; }
    QTabBar::tab:selected { background: #1a1a1a; border-bottom: 2px solid #66ff66; }
    QToolBar { background: #252525; border-bottom: 1px solid #333; }
    QStatusBar { background: #2d2d2d; color: #88cc88; }
)";

void Browser_WManager::openUrls(const QList<QUrl> &urls) {
    if (urls.isEmpty()) {
        addNewTab();
        return;
    }
    for (const QUrl &url : urls) {
        addNewTab(url);
    }
}
#include "onu.moc"

int main(int argc, char *argv[]) {
    QWebEngineUrlScheme onuScheme("onu");
    onuScheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    onuScheme.setDefaultPort(0);
    onuScheme.setFlags(QWebEngineUrlScheme::LocalScheme |
                       QWebEngineUrlScheme::SecureScheme |
                       QWebEngineUrlScheme::ContentSecurityPolicyIgnored);
    QWebEngineUrlScheme::registerScheme(onuScheme);

    QApplication app(argc, argv);
    app.setApplicationName("onu");
    app.setOrganizationName("Onu.");
    app.setApplicationVersion("0.4");
    QCommandLineParser parser;
    parser.setApplicationDescription("Onu Browser");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("urls", "URLs or local files to open.");

    parser.process(app);

    QList<QUrl> startupUrls;
    for (const QString &arg : parser.positionalArguments()) {
        QUrl url = QUrl::fromUserInput(arg);
        if (url.isLocalFile()) {
            QFileInfo fi(arg);
            if (fi.exists()) {
                url = QUrl::fromLocalFile(fi.absoluteFilePath());
            }
        }
        startupUrls << url;
    }

    QString flagsStr = Settings_Man::value("flags/chromium_flags",
                                           "--disable-gpu --disable-webrtc --max_renderer_process_memory=512").toString();
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flagsStr.toUtf8());

    OnuSchemeHandler *handler = new OnuSchemeHandler(&app);
    QWebEngineProfile::defaultProfile()->installUrlSchemeHandler("onu", handler);

    Browser_WManager window;
    window.show();


    if (!startupUrls.isEmpty()) {
        for (const QUrl &url : startupUrls) {
            window.addNewTab(url);
        }
    } else {
        window.addNewTab();
    }

    return app.exec();
}
