#include <QApplication>
#include <QMainWindow>
#include <QWebEngineView>
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
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
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

static const char* HISTORY_HTML_HEADER = R"HTML_HEADER(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: system-ui;
            padding: 40px;
            background: #f5f5f5;
        }
        h1 {
            color: #333;
            display: inline-block;
        }
        #clearBtn {
            margin-left: 20px;
            padding: 8px 16px;
            background: #e74c3c;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
        }
        #clearBtn:hover {
            background: #c0392b;
        }
        .entry {
            background: white;
            padding: 15px;
            margin: 10px 0;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .entry a {
            color: #0066cc;
            text-decoration: none;
            font-weight: 500;
        }
        .entry a:hover {
            text-decoration: underline;
        }
        .time {
            color: #666;
            font-size: 0.9em;
            margin-top: 5px;
        }
    </style>
    <script>
        function clearHistory() {
            if (confirm('Delete all browsing history?')) {
                fetch('onu://clear-history')
                    .then(() => location.reload())
                    .catch(err => alert('Failed'));
            }
        }
    </script>
</head>
<body>
    <h1>History</h1>
    <button id="clearBtn" onclick="clearHistory()">Clear All History</button>
)HTML_HEADER";

static const char* HISTORY_HTML_FOOTER = R"HTML_FOOTER(
</body>
</html>
)HTML_FOOTER";

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
                QByteArray data = file.readAll();
                QBuffer *buffer = new QBuffer();
                buffer->setData(data);
                buffer->open(QIODevice::ReadOnly);
                request->reply("text/html", buffer);
                file.close();
            }
        } else if (host == "game") {
            QFile file(":/game.html");
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                QBuffer *buffer = new QBuffer();
                buffer->setData(data);
                buffer->open(QIODevice::ReadOnly);
                request->reply("text/html", buffer);
                file.close();
            }
        } else if (host == "history") {
            QString html = generateHistoryHtml();
            QBuffer *buffer = new QBuffer();
            buffer->setData(html.toUtf8());
            buffer->open(QIODevice::ReadOnly);
            request->reply("text/html", buffer);
        } else if (host == "clear-history") {
            QSettings settings;
            settings.beginGroup("history");
            settings.remove("");
            settings.endGroup();
            QBuffer *buffer = new QBuffer();
            buffer->setData("OK");
            buffer->open(QIODevice::ReadOnly);
            request->reply("text/plain", buffer);
        } else {
            QString safeHost = host.toHtmlEscaped();
            QString body = "<html><body style='font-family:Monospace;padding:50px;'><h1>onu://" + safeHost + "</h1><p>Page not found</p></body></html>";
            QBuffer *buffer = new QBuffer();
            buffer->setData(body.toUtf8());
            buffer->open(QIODevice::ReadOnly);
            request->reply("text/html", buffer);
        }
    }
private:
    QString generateHistoryHtml() {
        QSettings settings;
        settings.beginGroup("history");
        QStringList keys = settings.childKeys();
        QString html = QString::fromUtf8(HISTORY_HTML_HEADER);
        QList<QPair<qint64, QVariantMap>> entries;
        for (const QString &key : keys) {
            QVariantMap entry = settings.value(key).toMap();
            entries.append({key.toLongLong(), entry});
        }
        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            return a.first > b.first;
        });
        for (const auto &[timestamp, entry] : entries) {
            QString url = entry["url"].toString().toHtmlEscaped();
            QString title = (entry["title"].toString().isEmpty() ? entry["url"].toString() : entry["title"].toString()).toHtmlEscaped();
            QString time = entry["time"].toString().toHtmlEscaped();
            html += QString("<div class='entry'><a href='%1'>%2</a><div class='time'>%3</div></div>")
                        .arg(url, title, time);
        }
        html += QString::fromUtf8(HISTORY_HTML_FOOTER);
        settings.endGroup();
        return html;
    }
};

class SettingsManager {
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

class onuWebPage : public QWebEnginePage {
    Q_OBJECT
public:
    explicit onuWebPage(QWebEngineProfile *profile, QObject *parent = nullptr)
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
        if (!SettingsManager::value("adblock/enabled", true).toBool()) return;
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
private:
    bool isAdBlocked(const QString &urlStr) const {
        if (!SettingsManager::value("adblock/enabled", true).toBool()) return false;
        QUrl url(urlStr);
        QString host = url.host().toLower();
        return blockedHosts.contains(host);
    }
    QSet<QString> blockedHosts;
};

class QtDownloader : public QObject {
    Q_OBJECT
public:
    explicit QtDownloader(QObject *parent = nullptr) : QObject(parent) {}
    void startDownload(const QUrl &url, const QString &filePath) {
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setHeader(QNetworkRequest::UserAgentHeader, "onuBrowser/0.3");
        QNetworkReply *reply = m_manager.get(request);
        m_activeDownloads[reply] = filePath;
        QFile *file = new QFile(filePath);
        if (!file->open(QIODevice::WriteOnly)) {
            emit downloadError(filePath, "Cannot open file for writing");
            file->deleteLater();
            reply->abort();
            m_activeDownloads.remove(reply);
            return;
        }
        m_files[reply] = file;
        m_timer.start();
        connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 bytesReceived, qint64 bytesTotal) {
            if (bytesTotal > 0) {
                qint64 elapsed = m_timer.elapsed();
                double speed = elapsed > 0 ? (bytesReceived / 1024.0) / (elapsed / 1000.0) : 0.0;
                emit progress(reply, bytesReceived, bytesTotal, speed);
            }
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            QFile *file = m_files.take(reply);
            QString path = m_activeDownloads.take(reply);
            if (reply->error() == QNetworkReply::NoError) {
                file->write(reply->readAll());
                emit downloadFinished(path, true, QString());
            } else {
                QFile::remove(path);
                emit downloadFinished(path, false, reply->errorString());
            }
            file->close();
            file->deleteLater();
            reply->deleteLater();
        });
    }
signals:
    void progress(QNetworkReply* reply, qint64 bytesReceived, qint64 bytesTotal, double speedKBps);
    void downloadFinished(const QString &filePath, bool success, const QString &error);
    void downloadError(const QString &filePath, const QString &message);
private:
    QNetworkAccessManager m_manager;
    QHash<QNetworkReply*, QString> m_activeDownloads;
    QHash<QNetworkReply*, QFile*> m_files;
    QElapsedTimer m_timer;
};

class DownloadManagerWidget : public QDialog {
    Q_OBJECT
public:
    explicit DownloadManagerWidget(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Downloads");
        setWindowIcon(QIcon::fromTheme("folder-download"));
        resize(500, 300);
        listWidget = new QListWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(listWidget);
    }
    void addDownload(QtDownloader *downloader, const QString &fileName) {
        show();
        raise();
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("emblem-downloads"), fileName + " - 0% (0 KB/s)");
        listWidget->addItem(item);
        m_itemMap[downloader] = item;
        m_fileNameMap[downloader] = fileName;
        connect(downloader, &QtDownloader::progress, this, [this, downloader, item](QNetworkReply*, qint64 received, qint64 total, double speed) {
            if (total > 0) {
                int percent = static_cast<int>((received * 100) / total);
                QString speedStr = QString::number(speed, 'f', 1);
                item->setText(m_fileNameMap[downloader] + QString(" - %1% (%2 KB/s)").arg(percent).arg(speedStr));
            }
        });
        connect(downloader, &QtDownloader::downloadFinished, this, [this, downloader, item](const QString &, bool success, const QString &error) {
            QString name = m_fileNameMap[downloader];
            if (success) {
                item->setIcon(QIcon::fromTheme("emblem-default"));
                item->setText(name + " - Completed");
                QPushButton *btn = m_controlButtons.value(downloader);
                if (btn) btn->setEnabled(false);
            } else {
                item->setIcon(QIcon::fromTheme("dialog-error"));
                item->setText(name + " - Failed: " + error);
                QPushButton *btn = m_controlButtons.value(downloader);
                if (btn) btn->setEnabled(false);
            }
            m_itemMap.remove(downloader);
            m_fileNameMap.remove(downloader);
        });
        QWidget *controlWidget = new QWidget();
        QHBoxLayout *hbox = new QHBoxLayout(controlWidget);
        QPushButton *pauseBtn = new QPushButton("Pause");
        QPushButton *resumeBtn = new QPushButton("Resume");
        QPushButton *cancelBtn = new QPushButton("Cancel");
        pauseBtn->setEnabled(false);
        resumeBtn->setEnabled(false);
        cancelBtn->setIcon(QIcon::fromTheme("process-stop"));
        connect(cancelBtn, &QPushButton::clicked, downloader, [this, downloader]() {
            emit downloadCancelled(downloader);
        });
        hbox->addWidget(pauseBtn);
        hbox->addWidget(resumeBtn);
        hbox->addWidget(cancelBtn);
        QListWidgetItem *controlItem = new QListWidgetItem();
        controlItem->setSizeHint(controlWidget->sizeHint());
        listWidget->addItem(controlItem);
        listWidget->setItemWidget(controlItem, controlWidget);
        m_controlButtons[downloader] = cancelBtn;
    }
signals:
    void downloadCancelled(QtDownloader *downloader);
private:
    QListWidget *listWidget;
    QHash<QtDownloader*, QListWidgetItem*> m_itemMap;
    QHash<QtDownloader*, QString> m_fileNameMap;
    QHash<QtDownloader*, QPushButton*> m_controlButtons;
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Onu Settings");
        setWindowIcon(QIcon::fromTheme("preferences-system"));
        resize(700, 500);
        QTabWidget *tabs = new QTabWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(tabs);
        QWidget *generalTab = new QWidget;
        QFormLayout *generalLayout = new QFormLayout(generalTab);
        homepageEdit = new QLineEdit(SettingsManager::value("homepage", "onu://home").toString());
        generalLayout->addRow("Homepage:", homepageEdit);
        QWidget *appearanceTab = new QWidget;
        QFormLayout *appearanceLayout = new QFormLayout(appearanceTab);
        themeCombo = new QComboBox;
        themeCombo->addItems({"System", "Light", "Dark", "Custom"});
        int themeIndex = SettingsManager::value("theme", 0).toInt();
        themeCombo->setCurrentIndex(themeIndex);
        appearanceLayout->addRow("Theme:", themeCombo);
        stylesheetEdit = new QTextEdit(SettingsManager::value("stylesheet", "").toString());
        stylesheetEdit->setMaximumHeight(100);
        stylesheetEdit->setEnabled(themeIndex == 3);
        connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            stylesheetEdit->setEnabled(index == 3);
        });
        appearanceLayout->addRow("Custom Stylesheet:", stylesheetEdit);
        fontCombo = new QFontComboBox;
        fontCombo->setCurrentFont(QFont(SettingsManager::value("ui/fontFamily", QApplication::font().family()).toString()));
        appearanceLayout->addRow("Font:", fontCombo);
        fontSizeSpin = new QSpinBox;
        fontSizeSpin->setRange(8, 32);
        fontSizeSpin->setValue(SettingsManager::value("ui/fontSize", QApplication::font().pointSize()).toInt());
        appearanceLayout->addRow("Font size:", fontSizeSpin);
        QWidget *privacyTab = new QWidget;
        QVBoxLayout *privacyLayout = new QVBoxLayout(privacyTab);
        adblockCheck = new QCheckBox("Enable AdBlock (blocks known ad hosts)");
        adblockCheck->setChecked(SettingsManager::value("adblock/enabled", true).toBool());
        privacyLayout->addWidget(adblockCheck);
        jsCheck = new QCheckBox("JavaScript");
        jsCheck->setChecked(SettingsManager::value("privacy/javascript", true).toBool());
        privacyLayout->addWidget(jsCheck);
        imagesCheck = new QCheckBox("Load Images");
        imagesCheck->setChecked(SettingsManager::value("privacy/images", true).toBool());
        privacyLayout->addWidget(imagesCheck);
        flagsGroup = new QGroupBox("Engine Flags", privacyTab);
        QGridLayout *flagsLayout = new QGridLayout(flagsGroup);
        flagGpuCheck = new QCheckBox("Disable GPU acceleration", flagsGroup);
        flagWebRtcCheck = new QCheckBox("Disable WebRTC", flagsGroup);
        flagJsConsoleCheck = new QCheckBox("Verbose JS console logs", flagsGroup);
        flagDoNotTrackCheck = new QCheckBox("Send Do Not Track header", flagsGroup);
        flagForceDarkModeCheck = new QCheckBox("Force Dark Mode (Chromium)", flagsGroup);
        flagGpuCheck->setChecked(SettingsManager::value("flags/disable_gpu", false).toBool());
        flagWebRtcCheck->setChecked(SettingsManager::value("flags/disable_webrtc", false).toBool());
        flagJsConsoleCheck->setChecked(SettingsManager::value("flags/verbose_js_console", false).toBool());
        flagDoNotTrackCheck->setChecked(SettingsManager::value("flags/do_not_track", false).toBool());
        flagForceDarkModeCheck->setChecked(SettingsManager::value("flags/force_dark_mode", false).toBool());
        flagsLayout->addWidget(flagGpuCheck, 0, 0);
        flagsLayout->addWidget(flagWebRtcCheck, 0, 1);
        flagsLayout->addWidget(flagJsConsoleCheck, 1, 0);
        flagsLayout->addWidget(flagDoNotTrackCheck, 1, 1);
        flagsLayout->addWidget(flagForceDarkModeCheck, 2, 0, 1, 2);
        flagsGroup->setLayout(flagsLayout);
        privacyLayout->addWidget(flagsGroup);
        privacyLayout->addStretch();
        QWidget *downloadsTab = new QWidget;
        QFormLayout *downloadsLayout = new QFormLayout(downloadsTab);
        QString defaultDownload = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        downloadPathEdit = new QLineEdit(SettingsManager::value("download/path", defaultDownload).toString());
        QPushButton *browseBtn = new QPushButton(QIcon::fromTheme("folder-open"), "Browse");
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Download Folder", downloadPathEdit->text());
            if (!dir.isEmpty()) downloadPathEdit->setText(dir);
        });
        QHBoxLayout *downloadLayout = new QHBoxLayout;
        downloadLayout->addWidget(downloadPathEdit);
        downloadLayout->addWidget(browseBtn);
        downloadsLayout->addRow("Download Folder:", downloadLayout);
        duplicateDownloadCheck = new QCheckBox("Prevent duplicate downloads");
        duplicateDownloadCheck->setChecked(SettingsManager::value("download/prevent_duplicates", true).toBool());
        downloadsLayout->addRow("", duplicateDownloadCheck);
        QWidget *developerTab = new QWidget;
        QVBoxLayout *devLayout = new QVBoxLayout(developerTab);
        developerModeCheck = new QCheckBox("Enable Developer Mode");
        developerModeCheck->setChecked(SettingsManager::value("developer/enabled", false).toBool());
        devLayout->addWidget(developerModeCheck);
        QLabel *devLabel = new QLabel(
            "Developer mode enables:\n"
            "• Inspect HTML Element in context menu\n"
            "• F4: Console output\n"
            "• F5: Reload\n"
            "• Ctrl+F5: Hard reload\n"
            "• Ctrl+Esc: Clear cache\n"
            "• Ctrl+K: View keybindings"
            );
        devLabel->setWordWrap(true);
        devLayout->addWidget(devLabel);
        devLayout->addStretch();
        tabs->addTab(generalTab, QIcon::fromTheme("preferences-other"), "General");
        tabs->addTab(appearanceTab, QIcon::fromTheme("preferences-desktop-theme"), "Appearance");
        tabs->addTab(privacyTab, QIcon::fromTheme("preferences-system-privacy"), "Privacy");
        tabs->addTab(downloadsTab, QIcon::fromTheme("folder-download"), "Downloads");
        tabs->addTab(developerTab, QIcon::fromTheme("applications-development"), "Developer");
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
        mainLayout->addWidget(buttonBox);
    }
    void accept() override {
        SettingsManager::setValue("homepage", homepageEdit->text());
        SettingsManager::setValue("theme", themeCombo->currentIndex());
        SettingsManager::setValue("stylesheet", stylesheetEdit->toPlainText());
        SettingsManager::setValue("adblock/enabled", adblockCheck->isChecked());
        SettingsManager::setValue("privacy/javascript", jsCheck->isChecked());
        SettingsManager::setValue("privacy/images", imagesCheck->isChecked());
        SettingsManager::setValue("ui/fontFamily", fontCombo->currentFont().family());
        SettingsManager::setValue("ui/fontSize", fontSizeSpin->value());
        SettingsManager::setValue("download/path", downloadPathEdit->text());
        SettingsManager::setValue("download/prevent_duplicates", duplicateDownloadCheck->isChecked());
        SettingsManager::setValue("developer/enabled", developerModeCheck->isChecked());
        SettingsManager::setValue("flags/disable_gpu", flagGpuCheck->isChecked());
        SettingsManager::setValue("flags/disable_webrtc", flagWebRtcCheck->isChecked());
        SettingsManager::setValue("flags/verbose_js_console", flagJsConsoleCheck->isChecked());
        SettingsManager::setValue("flags/do_not_track", flagDoNotTrackCheck->isChecked());
        SettingsManager::setValue("flags/force_dark_mode", flagForceDarkModeCheck->isChecked());
        QDialog::accept();
        emit settingsChanged();
    }
signals:
    void settingsChanged();
private:
    QLineEdit *homepageEdit;
    QComboBox *themeCombo;
    QTextEdit *stylesheetEdit;
    QCheckBox *adblockCheck;
    QLineEdit *downloadPathEdit;
    QCheckBox *jsCheck;
    QCheckBox *imagesCheck;
    QFontComboBox *fontCombo;
    QSpinBox *fontSizeSpin;
    QCheckBox *duplicateDownloadCheck;
    QCheckBox *developerModeCheck;
    QGroupBox *flagsGroup;
    QCheckBox *flagGpuCheck;
    QCheckBox *flagWebRtcCheck;
    QCheckBox *flagJsConsoleCheck;
    QCheckBox *flagDoNotTrackCheck;
    QCheckBox *flagForceDarkModeCheck;
};

class BrowserWindow : public QMainWindow {
    Q_OBJECT
public:
    BrowserWindow() {
        setWindowTitle("Onu Browser");
        resize(1300, 768);
        setWindowIcon(QIcon::fromTheme("onu"));
        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        layout = new QVBoxLayout(centralWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        tabWidget = new QTabWidget(this);
        tabWidget->setTabsClosable(true);
        tabWidget->setMovable(true);
        tabWidget->setDocumentMode(true);
        QTabBar *bar = tabWidget->tabBar();
        bar->setExpanding(false);
        bar->setUsesScrollButtons(true);
        bar->setElideMode(Qt::ElideRight);
        layout->addWidget(tabWidget);
        createToolBars();
        restoreToolbars();
        createMenuBar();
        statusBar = new QStatusBar(this);
        setStatusBar(statusBar);
        downloadManager = new DownloadManagerWidget(this);
        connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested,
                this, &BrowserWindow::handleDownload);
        connect(downloadManager, &DownloadManagerWidget::downloadCancelled, this, [](QtDownloader *d) {
            d->deleteLater();
        });
        tabWidget->tabBar()->setAcceptDrops(true);
        tabWidget->tabBar()->setMouseTracking(true);
        tabWidget->tabBar()->installEventFilter(this);
        restoreSession();
        loadHistoryIntoCompleter();
        connect(tabWidget, &QTabWidget::tabCloseRequested, this, &BrowserWindow::closeTab);
        connect(tabWidget, &QTabWidget::currentChanged, this, &BrowserWindow::updateToolbar);
        setupShortcuts();
        applyTheme(SettingsManager::value("theme", 0).toInt());
    }
    ~BrowserWindow() {
        saveSession();
        saveToolbars();
    }
    int addNewTab(const QUrl &url = QUrl("onu://home")) {
        QWidget *tab = new QWidget();
        QVBoxLayout *tabLayout = new QVBoxLayout(tab);
        tabLayout->setContentsMargins(0, 0, 0, 0);
        QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();
        onuWebPage *page = new onuWebPage(profile);
        QWebEngineView *webView = new QWebEngineView(tab);
        webView->setPage(page);
        QWebEngineSettings *settings = page->settings();
        settings->setAttribute(QWebEngineSettings::JavascriptEnabled,
                               SettingsManager::value("privacy/javascript", true).toBool());
        settings->setAttribute(QWebEngineSettings::AutoLoadImages,
                               SettingsManager::value("privacy/images", true).toBool());
        webView->setUrl(url);
        webView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(webView, &QWidget::customContextMenuRequested, this, [this, webView](const QPoint &pos) {
            QMenu menu(this);
            if (SettingsManager::value("developer/enabled", false).toBool()) {
                QAction *inspectAction = menu.addAction(QIcon::fromTheme("tools-report-bug"), "Inspect Element");
                connect(inspectAction, &QAction::triggered, this, [this, webView]() {
                    QWebEngineView *devTools = new QWebEngineView();
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
            menu.addAction(QIcon::fromTheme("text-html"), "View Page Source", this, &BrowserWindow::viewPageSource);
            menu.exec(webView->mapToGlobal(pos));
        });
        connect(page, &onuWebPage::newTabRequested, this, [this](const QUrl &url) {
            addNewTab(url);
        });
        tabLayout->addWidget(webView);
        int index = tabWidget->addTab(tab, "New Tab");
        tabWidget->setCurrentIndex(index);
        tab->setProperty("url", url);
        connect(webView, &QWebEngineView::loadStarted, this, &BrowserWindow::loadStarted);
        connect(webView, &QWebEngineView::loadFinished, this, &BrowserWindow::loadFinished);
        connect(webView, &QWebEngineView::urlChanged, this, &BrowserWindow::urlChanged);
        connect(webView, &QWebEngineView::titleChanged, this, [this, index](const QString &title) {
            tabWidget->setTabText(index, title.isEmpty() ? "New Tab" : title);
            tabWidget->setTabToolTip(index, title);
        });
        connect(webView, &QWebEngineView::iconChanged, this, [this, index, webView](const QIcon &icon) {
            tabWidget->setTabIcon(index, icon.isNull() ? getFallbackIcon(webView->url()) : icon);
        });
        connect(webView->page(), &QWebEnginePage::linkHovered, this, &BrowserWindow::linkHovered);
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
                    if (SettingsManager::value(key).toBool()) {
                        bool allowed = SettingsManager::value(key).toBool();
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
                    SettingsManager::setValue(key, allow);
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
        QTabBar *tb = tabWidget->tabBar();
        if (obj == tb) {
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
                int index = tb->tabAt(tb->mapFromGlobal(QCursor::pos()));
                if (index >= 0) {
                    showTabTooltip(index);
                }
            } else if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::RightButton) {
                    int index = tb->tabAt(me->pos());
                    if (index >= 0) {
                        showTabContextMenu(index, me->globalPos());
                        return true;
                    }
                }
            }
        }
        return QMainWindow::eventFilter(obj, event);
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
    void homePage() {
        QString home = SettingsManager::value("homepage", "onu://home").toString();
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
        SettingsDialog dialog(this);
        connect(&dialog, &SettingsDialog::settingsChanged, this, [this]() {
            applyTheme(SettingsManager::value("theme", 0).toInt());
            QStringList flags;
            if (SettingsManager::value("flags/disable_gpu", false).toBool())
                flags << "--disable-gpu" << "--disable-software-rasterizer" << "--disable-features=VizDisplayCompositor";
            if (SettingsManager::value("flags/disable_webrtc", false).toBool())
                flags << "--disable-webrtc";
            if (SettingsManager::value("flags/force_dark_mode", false).toBool())
                flags << "--force-dark-mode";
            if (!flags.isEmpty()) {
                qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.join(' ').toUtf8());
            }
            for (int i = 0; i < tabWidget->count(); ++i) {
                QWidget *tab = tabWidget->widget(i);
                QWebEngineView *view = tab->findChild<QWebEngineView*>();
                if (view) {
                    onuWebPage *page = qobject_cast<onuWebPage*>(view->page());
                    if (page) page->loadHostsFilter();
                    QWebEngineSettings *s = view->page()->settings();
                    s->setAttribute(QWebEngineSettings::JavascriptEnabled,
                                    SettingsManager::value("privacy/javascript", true).toBool());
                    s->setAttribute(QWebEngineSettings::AutoLoadImages,
                                    SettingsManager::value("privacy/images", true).toBool());
                }
            }
        });
        dialog.exec();
    }
    void showDownloads() {
        downloadManager->show();
        downloadManager->raise();
    }
    void handleDownload(QWebEngineDownloadRequest *download) {
        download->cancel();
        QString fileName = download->downloadFileName();
        if (fileName.isEmpty()) {
            QUrl url = download->url();
            fileName = url.fileName();
            if (fileName.isEmpty()) fileName = "download";
        }
        QString downloadDir = SettingsManager::value("download/path",
                                                     QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
        QString fullPath = QDir(downloadDir).filePath(fileName);
        if (SettingsManager::value("download/prevent_duplicates", true).toBool()) {
            if (QFile::exists(fullPath)) {
                QMessageBox::warning(this, "Duplicate Download", "File already exists: " + fileName);
                return;
            }
        }
        QtDownloader *downloader = new QtDownloader(this);
        downloadManager->addDownload(downloader, fileName);
        downloader->startDownload(download->url(), fullPath);
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
            }
        }
    }
    void linkHovered(const QString &url) {
        statusBar->showMessage(url, 3000);
    }
    void updateToolbar(int index) {
        if (index < 0) return;
        QWebEngineView *view = currentWebView();
        if (!view) return;
        backAction->setEnabled(view->history()->canGoBack());
        forwardAction->setEnabled(view->history()->canGoForward());
        updateUrl(view->url());
    }
private:
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
        QString title = tabWidget->tabText(tabIndex);
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
        QString title = tabWidget->tabText(tabIndex);
        QIcon icon = tabWidget->tabIcon(tabIndex);
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
        connect(reloadShortcut, &QShortcut::activated, this, &BrowserWindow::reloadPage);
        QShortcut *hardReloadShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5), this);
        connect(hardReloadShortcut, &QShortcut::activated, this, &BrowserWindow::hardReload);
        if (SettingsManager::value("developer/enabled", false).toBool()) {
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
                                         "F5 - Reload\n"
                                         "Ctrl+F5 - Hard Reload\n"
                                         "F4 - Console (Dev Mode)\n"
                                         "Ctrl+Esc - Clear Cache (Dev Mode)\n"
                                         "Ctrl+K - This help\n"
                                         "Ctrl+T - New Tab\n"
                                         "Ctrl+W - Close Tab\n"
                                         "Ctrl+Q - Quit");
            });
        }
    }
    void createToolBars() {
        navToolBar = addToolBar("Navigation");
        navToolBar->setMovable(true);
        navToolBar->setFloatable(true);
        backAction = navToolBar->addAction(QIcon::fromTheme("go-previous"), "Back", this, &BrowserWindow::goBack);
        forwardAction = navToolBar->addAction(QIcon::fromTheme("go-next"), "Forward", this, &BrowserWindow::goForward);
        reloadAction = navToolBar->addAction(QIcon::fromTheme("view-refresh"), "Reload", this, &BrowserWindow::reloadPage);
        stopAction = navToolBar->addAction(QIcon::fromTheme("process-stop"), "Stop", this, &BrowserWindow::stopPage);
        stopAction->setVisible(false);
        navToolBar->addAction(QIcon::fromTheme("go-home"), "Home", this, &BrowserWindow::homePage);
        searchToolBar = addToolBar("Search");
        searchToolBar->setMovable(true);
        searchToolBar->setFloatable(true);
        urlBar = new QLineEdit();
        urlBar->setPlaceholderText("Search or enter URL");
        urlBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(urlBar, &QLineEdit::returnPressed, this, &BrowserWindow::navigateToUrl);
        historyModel = new QStringListModel(this);
        urlCompleter = new QCompleter(historyModel, this);
        urlCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        urlCompleter->setCompletionMode(QCompleter::PopupCompletion);
        urlCompleter->setFilterMode(Qt::MatchContains);
        urlBar->setCompleter(urlCompleter);
        searchToolBar->addWidget(urlBar);
        newTabButton = new QToolButton(this);
        newTabButton->setIcon(QIcon::fromTheme("tab-new"));
        newTabButton->setToolTip("New Tab");
        newTabButton->setFixedSize(28, 28);
        connect(newTabButton, &QToolButton::clicked, this, &BrowserWindow::newTab);
        navToolBar->addWidget(newTabButton);
    }
    void createMenuBar() {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);
        menuBar->installEventFilter(this);
        menuBar->setMouseTracking(true);
        QMenu *onuMenu = menuBar->addMenu(QIcon::fromTheme("onu"), "Onu");
        QAction *newTabAct = onuMenu->addAction(QIcon::fromTheme("tab-new"), "New Tab");
        newTabAct->setShortcut(QKeySequence::AddTab);
        connect(newTabAct, &QAction::triggered, this, &BrowserWindow::newTab);
        onuMenu->addSeparator();
        QAction *backAct = onuMenu->addAction(QIcon::fromTheme("go-previous"), "Back");
        backAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
        connect(backAct, &QAction::triggered, this, &BrowserWindow::goBack);
        QAction *forwardAct = onuMenu->addAction(QIcon::fromTheme("go-next"), "Forward");
        forwardAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
        connect(forwardAct, &QAction::triggered, this, &BrowserWindow::goForward);
        QAction *reloadAct = onuMenu->addAction(QIcon::fromTheme("view-refresh"), "Reload");
        reloadAct->setShortcut(QKeySequence::Refresh);
        connect(reloadAct, &QAction::triggered, this, &BrowserWindow::reloadPage);
        QAction *homeAct = onuMenu->addAction(QIcon::fromTheme("go-home"), "Home");
        homeAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Home));
        connect(homeAct, &QAction::triggered, this, &BrowserWindow::homePage);
        onuMenu->addSeparator();
        QAction *gameAct = onuMenu->addAction(QIcon::fromTheme("games-config-tiles"), "Play Game");
        gameAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
        connect(gameAct, &QAction::triggered, this, &BrowserWindow::openGame);
        QAction *historyAct = onuMenu->addAction(QIcon::fromTheme("document-open-recent"), "History");
        historyAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
        connect(historyAct, &QAction::triggered, this, [this]() { addNewTab(QUrl("onu://history")); });
        QAction *downloadsAct = onuMenu->addAction(QIcon::fromTheme("folder-download"), "Downloads");
        downloadsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
        connect(downloadsAct, &QAction::triggered, this, &BrowserWindow::showDownloads);
        onuMenu->addSeparator();
        QAction *settingsAct = onuMenu->addAction(QIcon::fromTheme("preferences-system"), "Settings");
        settingsAct->setShortcut(QKeySequence::Preferences);
        connect(settingsAct, &QAction::triggered, this, &BrowserWindow::openSettings);
        QAction *aboutAct = onuMenu->addAction(QIcon::fromTheme("help-about"), "About Onu");
        connect(aboutAct, &QAction::triggered, this, [this]() {
            QMessageBox::about(this, "About Onu Browser",
                               "<p><b>Version:</b> 0.3</p>"
                               "<p>Built with Qt 6.8 WebEngine</p>");
        });
        onuMenu->addSeparator();
        QAction *quitAct = onuMenu->addAction(QIcon::fromTheme("application-exit"), "Quit");
        quitAct->setShortcut(QKeySequence::Quit);
        connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    }
    QWebEngineView* currentWebView() {
        int index = tabWidget->currentIndex();
        if (index < 0) return nullptr;
        QWidget *tab = tabWidget->widget(index);
        return tab ? tab->findChild<QWebEngineView*>() : nullptr;
    }
    void loadHistoryIntoCompleter() {
        QSettings settings;
        settings.beginGroup("history");
        QStringList completions;
        const QStringList keys = settings.childKeys();
        for (const QString &key : keys) {
            QVariantMap entry = settings.value(key).toMap();
            QString url = entry["url"].toString();
            QString title = entry["title"].toString();
            if (!url.isEmpty()) {
                QString display = title.isEmpty() ? url : (title + " — " + url);
                completions.append(display);
            }
        }
        settings.endGroup();
        historyModel->setStringList(completions);
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
            appliedCSS = BuiltInDarkCSS;
        } else if (themeIndex == 1) {
            palette = QApplication::style()->standardPalette();
            appliedCSS = BuiltInLightCSS;
        } else if (themeIndex == 3) {
            palette = QApplication::style()->standardPalette();
            appliedCSS = SettingsManager::value("stylesheet", "").toString();
        } else {
            palette = QApplication::style()->standardPalette();
            appliedCSS.clear();
        }
        QApplication::setPalette(palette);
        QFont appFont = QApplication::font();
        appFont.setFamily(SettingsManager::value("ui/fontFamily", appFont.family()).toString());
        appFont.setPointSize(SettingsManager::value("ui/fontSize", appFont.pointSize()).toInt());
        QApplication::setFont(appFont);
        qApp->setStyleSheet(appliedCSS);
        if (themeIndex == 2) {
            tabWidget->tabBar()->setStyleSheet(R"(
                QTabBar::tab { background: #252525; color: #a0d6a0; padding: 8px 12px; min-width: 120px; max-width: 180px; }
                QTabBar::tab:selected { background: #1a1a1a; border-bottom: 2px solid #66ff66; }
                QTabBar::tab:hover { background: #333; }
            )");
        } else if (themeIndex == 1) {
            tabWidget->tabBar()->setStyleSheet(R"(
                QTabBar::tab { background: #e9ecef; color: #495057; padding: 8px 12px; min-width: 120px; max-width: 180px; }
                QTabBar::tab:selected { background: #ffffff; border-bottom: 2px solid #3498db; }
                QTabBar::tab:hover { background: #d0d0d0; }
            )");
        } else {
            tabWidget->tabBar()->setStyleSheet("QTabBar::tab { padding: 8px 12px; min-width: 120px; max-width: 180px; }");
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
        settings.setValue("toolbars/searchArea", toolBarArea(searchToolBar));
    }
    void restoreToolbars() {
        QSettings settings;
        addToolBar(Qt::ToolBarArea(settings.value("toolbars/navArea", Qt::TopToolBarArea).toInt()), navToolBar);
        addToolBar(Qt::ToolBarArea(settings.value("toolbars/searchArea", Qt::TopToolBarArea).toInt()), searchToolBar);
    }
    static const QString BuiltInLightCSS;
    static const QString BuiltInDarkCSS;
    QWidget *centralWidget{};
    QVBoxLayout *layout{};
    QTabWidget *tabWidget{};
    QToolBar *navToolBar{};
    QToolBar *searchToolBar{};
    QLineEdit *urlBar{};
    QCompleter *urlCompleter = nullptr;
    QStringListModel *historyModel = nullptr;
    QStatusBar *statusBar{};
    DownloadManagerWidget *downloadManager{};
    QToolButton *newTabButton{};
    QAction *backAction{};
    QAction *forwardAction{};
    QAction *reloadAction{};
    QAction *stopAction{};
    bool menuBarDragging = false;
    QPoint menuBarDragStartPos;
};

const QString BrowserWindow::BuiltInLightCSS = R"(
    QMainWindow, QWidget { background: #ffffff; color: #2c3e50; }
    QLineEdit, QTextEdit { background: #f8f9fa; border: 1px solid #ced4da; border-radius: 4px; padding: 4px; }
    QTabBar::tab { background: #e9ecef; color: #495057; padding: 8px 12px; min-width: 120px; max-width: 180px; }
    QTabBar::tab:selected { background: #ffffff; border-bottom: 2px solid #3498db; }
    QToolBar { background: #f1f3f5; border-bottom: 1px solid #dee2e6; }
    QStatusBar { background: #f8f9fa; color: #6c757d; }
)";

const QString BrowserWindow::BuiltInDarkCSS = R"(
    QMainWindow, QWidget { background: #1e1e1e; color: #e0e0e0; }
    QLineEdit, QTextEdit { background: #2d2d2d; border: 1px solid #444; border-radius: 4px; padding: 4px; color: #00ff00; }
    QTabBar::tab { background: #252525; color: #a0d6a0; padding: 8px 12px; min-width: 120px; max-width: 180px; }
    QTabBar::tab:selected { background: #1a1a1a; border-bottom: 2px solid #66ff66; }
    QToolBar { background: #252525; border-bottom: 1px solid #333; }
    QStatusBar { background: #2d2d2d; color: #88cc88; }
)";

#include "main.moc"

int main(int argc, char *argv[]) {
    QStringList flags;
    if (QFile::exists("/dev/dri/renderD128")) {
        if (SettingsManager::value("flags/disable_gpu", false).toBool()) {
            flags << "--disable-gpu" << "--disable-software-rasterizer" << "--disable-features=VizDisplayCompositor";
        }
    } else {
        flags << "--disable-gpu" << "--disable-software-rasterizer" << "--disable-features=VizDisplayCompositor";
    }
    if (SettingsManager::value("flags/disable_webrtc", false).toBool())
        flags << "--disable-webrtc";
    if (SettingsManager::value("flags/force_dark_mode", false).toBool())
        flags << "--force-dark-mode";
    if (!flags.isEmpty())
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.join(' ').toUtf8());
    if (!qEnvironmentVariableIsSet("ONU_DEBUG"))
        qputenv("QT_LOGGING_RULES", "qt.webengine.*=false");

    QApplication::setApplicationName("onu");
    QApplication::setOrganizationName("Onu.");
    QApplication::setApplicationVersion("0.3");
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Onu Browser — a tiny Qt-WebEngine shell");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"nogpu", "Disable GPU acceleration completely."});
    parser.addOption({"force-dark", "Enable Chromium-level force dark mode."});
    parser.process(app);
    if (parser.isSet("nogpu")) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
                "--disable-gpu --disable-software-rasterizer --disable-features=VizDisplayCompositor");
    }
    if (parser.isSet("force-dark")) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
                (QString(getenv("QTWEBENGINE_CHROMIUM_FLAGS")) + " --force-dark-mode").trimmed().toUtf8());
    }

    QWebEngineUrlScheme onuScheme("onu");
    onuScheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::LocalScheme);
    QWebEngineUrlScheme::registerScheme(onuScheme);
    auto *onuHandler = new OnuSchemeHandler;
    QWebEngineProfile::defaultProfile()->installUrlSchemeHandler("onu", onuHandler);
    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();
    profile->setPersistentStoragePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/webengine");
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    profile->setNotificationPresenter([](std::unique_ptr<QWebEngineNotification> n) {
        QSystemTrayIcon tray;
        tray.show();
        tray.showMessage(n->title(), n->message(), QIcon(QPixmap::fromImage(n->icon())), 5000);
        n->show();
        QTimer::singleShot(5000, Qt::VeryCoarseTimer, qApp, [ptr = std::move(n)]{ ptr->close(); });
    });

    BrowserWindow w;
    w.show();
    return app.exec();
}
