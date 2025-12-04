#include <QApplication>
#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineDownloadRequest>
#include <QWebEngineHistory>
#include <QToolBar>
#include <QLineEdit>
#include <QAction>
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QUrl>
#include <QIcon>
#include <QVBoxLayout>
#include <QWidget>
#include <QCompleter>
#include <QStringListModel>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QListWidget>
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
#include <QPointer>
#include <QRegularExpression>
#include <QClipboard>
#include <QToolButton>
#include <QDropEvent>
#include <QMimeData>

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
    explicit onuWebPage(QObject *parent = nullptr) : QWebEnginePage(parent) {
        loadUserscripts();
        loadHostsFilter();
    }

    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override {
        if (isAdBlocked(url.toString())) {
            if (isMainFrame) {
                setHtml("<h2>Blocked by AdBlock</h2><p>" + url.toString() + "</p>");
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
        if (type == QWebEnginePage::WebBrowserTab) {
            emit newTabRequested(QUrl());
        }
        return new QWebEnginePage(this);
    }

private:
    void loadUserscripts() {
        QString scriptDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/userscripts";
        QDir().mkpath(scriptDir);
        QStringList scriptFiles = QDir(scriptDir).entryList(QStringList{"*.js"}, QDir::Files);
        this->scripts().clear();
        for (const QString &file : scriptFiles) {
            QFile f(scriptDir + "/" + file);
            if (f.open(QIODevice::ReadOnly)) {
                QWebEngineScript script;
                script.setName(file);
                script.setSourceCode(QString::fromUtf8(f.readAll()));
                script.setInjectionPoint(QWebEngineScript::DocumentReady);
                script.setRunsOnSubFrames(true);
                script.setWorldId(QWebEngineScript::MainWorld);
                this->scripts().insert(script);
                f.close();
            }
        }
    }

    bool isAdBlocked(const QString &urlStr) const {
        if (!SettingsManager::value("adblock/enabled", true).toBool()) return false;
        QUrl url(urlStr);
        QString host = url.host().toLower();
        return blockedHosts.contains(host);
    }

signals:
    void newTabRequested(const QUrl &url);

private:
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
        request.setHeader(QNetworkRequest::UserAgentHeader, "onuBrowser/0.2");
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
        connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 bytesReceived, qint64 bytesTotal) {
            if (bytesTotal > 0) {
                emit progress(reply, bytesReceived, bytesTotal);
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
    void progress(QNetworkReply* reply, qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &filePath, bool success, const QString &error);
    void downloadError(const QString &filePath, const QString &message);
private:
    QNetworkAccessManager m_manager;
    QHash<QNetworkReply*, QString> m_activeDownloads;
    QHash<QNetworkReply*, QFile*> m_files;
};

class DownloadManagerWidget : public QDialog {
    Q_OBJECT
public:
    explicit DownloadManagerWidget(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Downloads");
        resize(500, 300);
        listWidget = new QListWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(listWidget);
    }
    void addDownload(QtDownloader *downloader, const QString &fileName) {
        QString itemText = "📥 " + fileName + " — 0%";
        QListWidgetItem *item = new QListWidgetItem(itemText);
        listWidget->addItem(item);
        m_itemMap[downloader] = item;
        m_fileNameMap[downloader] = fileName;
        connect(downloader, &QtDownloader::progress, this, [this, downloader, item](QNetworkReply*, qint64 received, qint64 total) {
            int percent = static_cast<int>((received * 100) / total);
            item->setText("📥 " + m_fileNameMap[downloader] + " — " + QString::number(percent) + "%");
        });
        connect(downloader, &QtDownloader::downloadFinished, this, [this, downloader, item](const QString &filePath, bool success, const QString &error) {
            QString name = m_fileNameMap[downloader];
            if (success) {
                item->setText("✅ " + name + " — Completed");
            } else {
                item->setText("❌ " + name + " — Failed: " + error);
            }
            m_itemMap.remove(downloader);
            m_fileNameMap.remove(downloader);
        });
    }
private:
    QListWidget *listWidget;
    QHash<QtDownloader*, QListWidgetItem*> m_itemMap;
    QHash<QtDownloader*, QString> m_fileNameMap;
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("onu Settings");
        resize(600, 400);
        QTabWidget *tabs = new QTabWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(tabs);
        QWidget *generalTab = new QWidget;
        QFormLayout *generalLayout = new QFormLayout(generalTab);
        homepageEdit = new QLineEdit(SettingsManager::value("homepage", "qrc:/home.html").toString());
        generalLayout->addRow("Homepage:", homepageEdit);
        QWidget *appearanceTab = new QWidget;
        QFormLayout *appearanceLayout = new QFormLayout(appearanceTab);
        themeCombo = new QComboBox;
        themeCombo->addItems({"System", "Light", "Dark"});
        int themeIndex = SettingsManager::value("theme", 0).toInt();
        themeCombo->setCurrentIndex(themeIndex);
        appearanceLayout->addRow("Theme:", themeCombo);
        stylesheetEdit = new QTextEdit(SettingsManager::value("stylesheet", "").toString());
        appearanceLayout->addRow("Custom Stylesheet:", stylesheetEdit);
        QWidget *privacyTab = new QWidget;
        QVBoxLayout *privacyLayout = new QVBoxLayout(privacyTab);
        adblockCheck = new QCheckBox("Enable AdBlock (blocks known ad hosts)");
        adblockCheck->setChecked(SettingsManager::value("adblock/enabled", true).toBool());
        privacyLayout->addWidget(adblockCheck);
        privacyLayout->addStretch();
        QWidget *downloadsTab = new QWidget;
        QFormLayout *downloadsLayout = new QFormLayout(downloadsTab);
        QString defaultDownload = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        downloadPathEdit = new QLineEdit(SettingsManager::value("download/path", defaultDownload).toString());
        QPushButton *browseBtn = new QPushButton("Browse");
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Download Folder", downloadPathEdit->text());
            if (!dir.isEmpty()) downloadPathEdit->setText(dir);
        });
        QHBoxLayout *downloadLayout = new QHBoxLayout;
        downloadLayout->addWidget(downloadPathEdit);
        downloadLayout->addWidget(browseBtn);
        downloadsLayout->addRow("Download Folder:", downloadLayout);
        QWidget *userscriptsTab = new QWidget;
        QVBoxLayout *userscriptsLayout = new QVBoxLayout(userscriptsTab);
        QString userscriptPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/userscripts";
        userscriptsLayout->addWidget(new QLabel("Place .js files in:"));
        QTextEdit *pathEdit = new QTextEdit(userscriptPath);
        pathEdit->setReadOnly(true);
        pathEdit->setMaximumHeight(60);
        userscriptsLayout->addWidget(pathEdit);
        userscriptsLayout->addWidget(new QLabel("Scripts run on page load (like Tampermonkey)."));
        tabs->addTab(generalTab, "General");
        tabs->addTab(appearanceTab, "Appearance");
        tabs->addTab(privacyTab, "Privacy");
        tabs->addTab(downloadsTab, "Downloads");
        tabs->addTab(userscriptsTab, "Userscripts");
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
        SettingsManager::setValue("download/path", downloadPathEdit->text());
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
};

class BrowserWindow : public QMainWindow {
    Q_OBJECT
public:
    BrowserWindow() {
        setWindowTitle("onu Browser");
        resize(1300, 768);
        setWindowIcon(QIcon::fromTheme("onu"));
        applyTheme(SettingsManager::value("theme", 0).toInt());
        applyStylesheet(SettingsManager::value("stylesheet", "").toString());
        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        layout = new QVBoxLayout(centralWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        tabWidget = new QTabWidget(this);
        tabWidget->setTabsClosable(true);
        tabWidget->setMovable(true);
        tabWidget->setDocumentMode(true);
        layout->addWidget(tabWidget);

        newTabButton = new QToolButton(this);
        newTabButton->setText("+");
        newTabButton->setToolTip("New Tab");
        newTabButton->setAutoRaise(true);
        newTabButton->setFixedSize(20, 20);
        connect(newTabButton, &QToolButton::clicked, this, &BrowserWindow::newTab);
        tabWidget->tabBar()->setTabButton(0, QTabBar::LeftSide, newTabButton);

        createToolBars();
        createMenuBar();
        statusBar = new QStatusBar(this);
        setStatusBar(statusBar);
        downloadManager = new DownloadManagerWidget(this);

        connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested,
                this, &BrowserWindow::handleDownload);

        tabWidget->tabBar()->setAcceptDrops(true);
        tabWidget->tabBar()->setElideMode(Qt::ElideRight);
        tabWidget->tabBar()->installEventFilter(this);

        restoreSession();
        connect(tabWidget, &QTabWidget::tabCloseRequested, this, &BrowserWindow::closeTab);
        connect(tabWidget, &QTabWidget::currentChanged, this, &BrowserWindow::updateToolbar);
    }

    ~BrowserWindow() { saveSession(); }

    int addNewTab(const QUrl &url = QUrl("qrc:/home.html")) {
        QWidget *tab = new QWidget();
        QVBoxLayout *tabLayout = new QVBoxLayout(tab);
        tabLayout->setContentsMargins(0, 0, 0, 0);
        QWebEngineView *webView = new QWebEngineView(tab);
        onuWebPage *page = new onuWebPage(webView);
        webView->setPage(page);
        webView->setUrl(url);

        webView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(webView, &QWidget::customContextMenuRequested, this, [this, webView](const QPoint &pos) {
            QMenu menu(this);
            QAction *inspectAction = menu.addAction("Inspect Element");
            connect(inspectAction, &QAction::triggered, this, [this, webView]() {
                QWebEngineView *devTools = new QWebEngineView();
                webView->page()->setDevToolsPage(devTools->page());
                devTools->resize(800, 600);
                devTools->show();
            });
            menu.exec(webView->mapToGlobal(pos));
        });

        connect(page, &onuWebPage::newTabRequested, this, [this](const QUrl &) {
            addNewTab();
        });

        tabLayout->addWidget(webView);
        int index = tabWidget->addTab(tab, "New Tab");
        tabWidget->setCurrentIndex(index);
        tab->setProperty("url", url);

        connect(webView, &QWebEngineView::loadStarted, this, &BrowserWindow::loadStarted);
        connect(webView, &QWebEngineView::loadFinished, this, &BrowserWindow::loadFinished);
        connect(webView, &QWebEngineView::urlChanged, this, &BrowserWindow::urlChanged);
        connect(webView, &QWebEngineView::titleChanged, this, [=, this](const QString &title) {
            tabWidget->setTabText(index, title.isEmpty() ? "New Tab" : title);
        });
        connect(webView, &QWebEngineView::iconChanged, this, [=, this](const QIcon &icon) {
            tabWidget->setTabIcon(index, icon);
        });
        connect(webView->page(), &QWebEnginePage::linkHovered, this, &BrowserWindow::linkHovered);

        if (url != QUrl("qrc:/home.html") && url != QUrl("qrc:/game.html")) {
            addToHistory(url);
        }
        return index;
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == tabWidget->tabBar() && event->type() == QEvent::Drop) {
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
        }
        return QMainWindow::eventFilter(obj, event);
    }

private slots:
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
    }
    void goBack() { if (currentWebView()) currentWebView()->back(); }
    void goForward() { if (currentWebView()) currentWebView()->forward(); }
    void reloadPage() { if (currentWebView()) currentWebView()->reload(); }
    void stopPage() { if (currentWebView()) currentWebView()->stop(); }
    void homePage() {
        QString home = SettingsManager::value("homepage", "qrc:/home.html").toString();
        currentWebView()->setUrl(QUrl(home));
    }
    void newTab() { addNewTab(); }
    void closeTab(int index) {
        if (tabWidget->count() > 1) {
            delete tabWidget->widget(index);
        } else {
            saveSession();
            QApplication::quit();
        }
    }
    void openGame() {
        addNewTab(QUrl("qrc:/game.html"));
    }
    void openSettings() {
        SettingsDialog dialog(this);
        connect(&dialog, &SettingsDialog::settingsChanged, this, [this]() {
            applyTheme(SettingsManager::value("theme", 0).toInt());
            applyStylesheet(SettingsManager::value("stylesheet", "").toString());
            for (int i = 0; i < tabWidget->count(); ++i) {
                QWidget *tab = tabWidget->widget(i);
                QWebEngineView *view = tab->findChild<QWebEngineView*>();
                if (view) {
                    onuWebPage *page = qobject_cast<onuWebPage*>(view->page());
                    if (page) page->loadHostsFilter();
                }
            }
        });
        dialog.exec();
    }
    void manageBookmarks() {
        QMenu menu(this);
        QSettings settings;
        settings.beginGroup("bookmarks");
        QStringList keys = settings.childKeys();
        if (keys.isEmpty()) {
            menu.addAction("No bookmarks")->setEnabled(false);
        } else {
            for (const QString &key : keys) {
                QString title = settings.value(key).toString();
                QAction *act = menu.addAction(title);
                act->setData(key);
                connect(act, &QAction::triggered, this, [this, key]() {
                    currentWebView()->setUrl(QUrl(key));
                });
            }
            menu.addSeparator();
            QAction *clearAction = menu.addAction("Clear All");
            connect(clearAction, &QAction::triggered, this, [&]() {
                settings.clear();
                QMessageBox::information(this, "Cleared", "All bookmarks deleted.");
            });
        }
        menu.exec(QCursor::pos());
    }
    void addBookmark() {
        QWebEngineView *view = currentWebView();
        if (!view) return;
        QUrl url = view->url();
        if (url.scheme() == "qrc") return;
        QString title = view->title().isEmpty() ? url.toString() : view->title();
        QSettings settings;
        settings.beginGroup("bookmarks");
        settings.setValue(url.toString(), title);
        QMessageBox::information(this, "Bookmark Added", "Saved: " + title);
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
            if (url.scheme() != "qrc") {
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
    void createToolBars() {
        tabToolBar = addToolBar("Tabs");
        tabToolBar->setMovable(true);
        tabToolBar->setFloatable(true);
        tabToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        tabToolBar->addAction(QIcon::fromTheme("games-puzzle"), "Game", this, &BrowserWindow::openGame);
        tabToolBar->addSeparator();
        tabToolBar->addAction(QIcon::fromTheme("bookmarks"), "Bookmarks", this, &BrowserWindow::manageBookmarks);
        tabToolBar->addAction(QIcon::fromTheme("document-properties"), "Settings", this, &BrowserWindow::openSettings);
        connect(tabToolBar, &QToolBar::orientationChanged, this, [this](Qt::Orientation orientation) {
            tabToolBar->setToolButtonStyle(
                orientation == Qt::Vertical ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
        });

        navToolBar = addToolBar("Navigation");
        navToolBar->setMovable(true);
        navToolBar->setFloatable(true);
        navToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        backAction = navToolBar->addAction(QIcon::fromTheme("go-previous"), "Back", this, &BrowserWindow::goBack);
        forwardAction = navToolBar->addAction(QIcon::fromTheme("go-next"), "Forward", this, &BrowserWindow::goForward);
        reloadAction = navToolBar->addAction(QIcon::fromTheme("view-refresh"), "Reload", this, &BrowserWindow::reloadPage);
        stopAction = navToolBar->addAction(QIcon::fromTheme("process-stop"), "Stop", this, &BrowserWindow::stopPage);
        stopAction->setVisible(false);
        navToolBar->addAction(QIcon::fromTheme("go-home"), "Home", this, &BrowserWindow::homePage);
        connect(navToolBar, &QToolBar::orientationChanged, this, [this](Qt::Orientation orientation) {
            navToolBar->setToolButtonStyle(
                orientation == Qt::Vertical ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
        });

        searchToolBar = addToolBar("Search");
        searchToolBar->setMovable(true);
        searchToolBar->setFloatable(true);
        urlBar = new QLineEdit();
        urlBar->setPlaceholderText("Search or enter URL");
        urlBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(urlBar, &QLineEdit::returnPressed, this, &BrowserWindow::navigateToUrl);
        searchToolBar->addWidget(urlBar);
        connect(searchToolBar, &QToolBar::orientationChanged, this, [this](Qt::Orientation orientation) {
            if (orientation == Qt::Vertical) {
                urlBar->setPlaceholderText("");
            } else {
                urlBar->setPlaceholderText("Search or enter URL");
            }
        });
    }

    void createMenuBar() {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);
        QMenu *fileMenu = menuBar->addMenu("&File");
        {
            auto act = fileMenu->addAction("New Tab");
            act->setShortcut(QKeySequence::AddTab);
            connect(act, &QAction::triggered, this, &BrowserWindow::newTab);
            act = fileMenu->addAction("i do not have internet");
            act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
            connect(act, &QAction::triggered, this, &BrowserWindow::openGame);
            fileMenu->addSeparator();
            act = fileMenu->addAction("Downloads");
            act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
            connect(act, &QAction::triggered, this, &BrowserWindow::showDownloads);
            fileMenu->addSeparator();
            act = fileMenu->addAction("Quit");
            act->setShortcut(QKeySequence::Quit);
            connect(act, &QAction::triggered, qApp, &QApplication::quit);
        }
        QMenu *navMenu = menuBar->addMenu("&Navigation");
        navMenu->addAction(backAction);
        navMenu->addAction(forwardAction);
        navMenu->addAction(reloadAction);
        auto homeAct = navMenu->addAction("Home");
        connect(homeAct, &QAction::triggered, this, &BrowserWindow::homePage);
        QMenu *bookmarksMenu = menuBar->addMenu("&Bookmarks");
        {
            auto act = bookmarksMenu->addAction("Add Bookmark");
            act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
            connect(act, &QAction::triggered, this, &BrowserWindow::addBookmark);
            bookmarksMenu->addAction("Manage Bookmarks", this, &BrowserWindow::manageBookmarks);
        }
        QMenu *toolsMenu = menuBar->addMenu("&Tools");
        {
            auto act = toolsMenu->addAction("Settings");
            act->setShortcut(QKeySequence::Preferences);
            connect(act, &QAction::triggered, this, &BrowserWindow::openSettings);
        }
    }

    QWebEngineView* currentWebView() {
        int index = tabWidget->currentIndex();
        if (index < 0) return nullptr;
        QWidget *tab = tabWidget->widget(index);
        return tab ? tab->findChild<QWebEngineView*>() : nullptr;
    }

    void addToHistory(const QUrl &url) {
        QSettings settings;
        settings.beginGroup("history");
        QDateTime now = QDateTime::currentDateTime();
        settings.setValue(url.toString(), now.toString(Qt::ISODate));
        settings.endGroup();
    }

    void applyTheme(int themeIndex) {
        QPalette palette;
        if (themeIndex == 2) {
            palette.setColor(QPalette::Window, QColor(53, 53, 53));
            palette.setColor(QPalette::WindowText, Qt::white);
            palette.setColor(QPalette::Base, QColor(25, 25, 25));
            palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
            palette.setColor(QPalette::ToolTipBase, Qt::white);
            palette.setColor(QPalette::ToolTipText, Qt::white);
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor(53, 53, 53));
            palette.setColor(QPalette::ButtonText, Qt::white);
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Link, QColor(42, 130, 218));
            palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
            palette.setColor(QPalette::HighlightedText, Qt::black);
        } else if (themeIndex == 1) {
            palette = QApplication::style()->standardPalette();
        }
        QApplication::setPalette(themeIndex == 0 ? QApplication::style()->standardPalette() : palette);
    }

    void applyStylesheet(const QString &stylesheet) {
        qApp->setStyleSheet(stylesheet);
    }

    void saveSession() {
        QSettings settings;
        settings.beginWriteArray("tabs");
        for (int i = 0; i < tabWidget->count(); ++i) {
            QWidget *tab = tabWidget->widget(i);
            QUrl url = tab->property("url").toUrl();
            if (url.isEmpty()) {
                QWebEngineView *view = tab->findChild<QWebEngineView*>();
                if (view) url = view->url();
            }
            settings.setArrayIndex(i);
            settings.setValue("url", url.toString());
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

    QWidget *centralWidget{};
    QVBoxLayout *layout{};
    QTabWidget *tabWidget{};
    QToolBar *tabToolBar{};
    QToolBar *navToolBar{};
    QToolBar *searchToolBar{};
    QLineEdit *urlBar{};
    QStatusBar *statusBar{};
    DownloadManagerWidget *downloadManager{};
    QToolButton *newTabButton{};
    QAction *backAction{};
    QAction *forwardAction{};
    QAction *reloadAction{};
    QAction *stopAction{};
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication::setApplicationName("onu");
    QApplication::setOrganizationName("Onu.");
    QApplication::setApplicationVersion("0.2");
    QApplication app(argc, argv);

    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();
    profile->setPersistentStoragePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/webengine");
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    BrowserWindow window;
    window.show();
    return app.exec();
}
