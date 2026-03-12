#include "onu.H"
#include "extense.H"
#include "i.H"
class Settings_Backend;
class Icon_Man {
public:
    static Icon_Man& instance() {
        static Icon_Man inst;
        return inst;
    }
    QString saveIcon(const QIcon &icon) {
        if (icon.isNull()) return QString();
        QPixmap pixmap = icon.pixmap(32, 32);
        if (pixmap.isNull()) return QString();
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");
        QString contentHash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex();
        QString fileName = contentHash + ".png";
        QString fullPath = Settings_Backend::instance().othersIconCache() + "/" + fileName;
        if (!QFile::exists(fullPath)) {
            QDir().mkpath(Settings_Backend::instance().othersIconCache());
            QFile file(fullPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(bytes);
            }
        }
        return fileName;
    }
    QIcon loadIcon(const QString &fileName) {
        if (fileName.isEmpty()) return QIcon::fromTheme("applications-internet");
        QString fullPath = (fileName.contains('/') || fileName.contains('\\'))
                               ? fileName
                               : Settings_Backend::instance().othersIconCache() + "/" + fileName;
        return QFile::exists(fullPath) ? QIcon(fullPath) : QIcon::fromTheme("applications-internet");
    }
    QString themeIconUrl(const QString &iconName, int size = 32) {
        QString cacheDir = Settings_Backend::instance().themeIconCache();
        QString fileName = QString("%1_%2.png").arg(iconName).arg(size);
        QString fullPath = cacheDir + "/" + fileName;
        if (QFile::exists(fullPath)) {
            return QUrl::fromLocalFile(fullPath).toString();
        }
        QDir().mkpath(cacheDir);
        QIcon icon = QIcon::fromTheme(iconName, QIcon::fromTheme("applications-internet"));
        QPixmap pix = icon.pixmap(size, size);
        if (!pix.isNull() && pix.save(fullPath, "PNG")) {
            return QUrl::fromLocalFile(fullPath).toString();
        }
        return QString();
    }
private:
    Icon_Man() = default;
};
class Browser_Backend : public QObject {
    Q_OBJECT
public:
    static Browser_Backend& instance() {
        static Browser_Backend inst;
        return inst;
    }

    void openInNewTab(const QUrl &url) {
        if (url.isValid()) {
            emit newTabRequested(url);
        }
    }

    void setFavorites(const QVariantList &favs) {
        m_favorites = favs;
        Settings_Backend::instance().saveFavorites(m_favorites);
        emit favoritesChanged();
    }
    void addToHistory(const QUrl &url, const QString &title, const QIcon &icon) {
        if (url.scheme() == "onu") return;
        m_recentTabs.erase(std::remove_if(m_recentTabs.begin(), m_recentTabs.end(),
                                          [&url](const QVariant &v) { return v.toMap()["url"].toString() == url.toString(); }), m_recentTabs.end());
        QString iPath = Icon_Man::instance().saveIcon(icon);
        m_recentTabs.prepend(QVariantMap{
            {"url", url.toString()},
            {"iconPath", iPath},
            {"title", title},
            {"time", QDateTime::currentMSecsSinceEpoch()}
        });
        if (m_recentTabs.size() > 50) m_recentTabs.removeLast();
        Settings_Backend::instance().saveHistory(m_recentTabs);
        emit recentTabsChanged();
    }
    void addToFavorites(const QString &title, const QUrl &url, const QIcon &icon) {
        QString iPath = Icon_Man::instance().saveIcon(icon);
        m_favorites.prepend(QVariantMap{{"title", title}, {"url", url.toString()}, {"iconPath", iPath}});
        Settings_Backend::instance().saveFavorites(m_favorites);
        emit favoritesChanged();
    }
    void saveSession(const QList<QUrl> &tabs) {
        QVariantList list;
        for (const QUrl &u : tabs) if (!u.isEmpty()) list << u.toString();
        Settings_Backend::instance().saveSession(list);
    }
    QList<QUrl> restoreSession() {
        QList<QUrl> urls;
        QVariantList list = Settings_Backend::instance().loadSession();
        for (const QVariant &v : list) urls << QUrl(v.toString());
        return urls;
    }
    void clearHistory() {
        m_recentTabs.clear();
        Settings_Backend::instance().clearHistory();
        emit recentTabsChanged();
    }
    const QVariantList& favorites() const { return m_favorites; }
    const QVariantList& recentTabs() const { return m_recentTabs; }
    void  close(QWidget* parent, const QList<QUrl>& currentTabs) {
        if (Settings_Backend::instance().confirmClose()) {
            QMessageBox box(parent);
            box.setIcon(QMessageBox::Critical);
            box.setText("Confirm Close");
            box.setInformativeText("Are you sure you want to close the browser?");
            box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            box.setDefaultButton(QMessageBox::No);
            QCheckBox *dontShowAgain = new QCheckBox("Don't show this dialog next time", &box);
            box.setCheckBox(dontShowAgain);
            if (box.exec() == QMessageBox::No) {
                return;
            }
            if (dontShowAgain->isChecked()) {
                Settings_Backend::instance().setPublicValue("privacy/confirmClose", false);
            }
        }
        if (Settings_Backend::instance().restoreSessions()) {
            saveSession(currentTabs);
        }
        COS::Tri_term();
    }
signals:
    void recentTabsChanged();
    void favoritesChanged();
    void newTabRequested(const QUrl &url);
private:
    Browser_Backend() {
        m_recentTabs = Settings_Backend::instance().loadHistory();
        m_favorites = Settings_Backend::instance().loadFavorites();
    }

    Q_DISABLE_COPY(Browser_Backend)

    QVariantList m_recentTabs;
    QVariantList m_favorites;
};
class Url_Scheme : public QWebEngineUrlSchemeHandler {
    Q_OBJECT
public:
    explicit Url_Scheme(Browser_Backend *backend, QObject *parent = nullptr)
        : QWebEngineUrlSchemeHandler(parent), backend(backend) {
        errorIcon = Icon_Man::instance().themeIconUrl("dialog-error", 128);
        infoIcon = Icon_Man::instance().themeIconUrl("dialog-information", 128);
        searchIcon = Icon_Man::instance().themeIconUrl("edit-find", 24);
        githubIcon = Icon_Man::instance().themeIconUrl("github", 24);
        wikipediaIcon = Icon_Man::instance().themeIconUrl("wikipedia", 24);
    }
    void requestStarted(QWebEngineUrlRequestJob *request) override {
        QUrl url = request->requestUrl();
        QString host = url.host();
        auto buffer = std::make_unique<QBuffer>();
        if (host == "home")
            buffer->setData(generateHomePage().toUtf8());
        else if (host == "about")
            buffer->setData(generateAboutPage().toUtf8());
        else if (host == "game")
            buffer->setData(generateGamePage().toUtf8());
#ifdef CMAKE_DEBUG
        else if (host == "test") {
            QFile testFile(":/test.html");
            if (testFile.open(QIODevice::ReadOnly)) {
                QString html = QString::fromUtf8(testFile.readAll());
                buffer->setData(html.toUtf8());
                testFile.close();
            }
        }
#endif
        else
            buffer->setData(generate404Page(host).toUtf8());
        buffer->open(QIODevice::ReadOnly);
        request->reply("text/html", buffer.release());
    }
private:
    Browser_Backend *backend;
    QString errorIcon;
    QString infoIcon;
    QString searchIcon;
    QString githubIcon;
    QString wikipediaIcon;
    QString generate404Page(const QString &host) {
        return QString("<div align=center><img src='%1' height=256 width=256><h1>onu://%2 wasn't found</h1><hr><p>likely because it simply does not exist, visit github.com/zynomon/onu for more info or report</p></div>").arg(errorIcon).arg(host.toHtmlEscaped());
    }
    QString generateHomePage() {
        QFile homeFile(":/home.html");
        homeFile.open(QIODevice::ReadOnly);
        QString html = QString::fromUtf8(homeFile.readAll());
        const QVariantList &favs = backend->favorites();
        QString favsJson = "[";
        for (int i = 0; i < favs.size(); ++i) {
            if (i > 0) favsJson += ",";
            QVariantMap fav = favs[i].toMap();
            QString iconRef = fav["iconPath"].toString();
            QString fullPath = (iconRef.contains('/') || iconRef.contains('\\'))
                                   ? iconRef : Settings_Backend::instance().othersIconCache() + "/" + iconRef;
            QString iconUrl = QFile::exists(fullPath)
                                  ? QUrl::fromLocalFile(fullPath).toString()
                                  : searchIcon;
            favsJson += "{\"title\":\"" + fav["title"].toString().replace('"', "\\\"") + "\","
                                                                                         "\"url\":\"" + fav["url"].toString().replace('"', "\\\"") + "\","
                                                                       "\"icon\":\"" + iconUrl + "\"}";
        }
        favsJson += "]";
        QString engine = Settings_Backend::instance().currentSearchEngineUrl().replace("%1", "%s");
        QString script = "\n<script>\nwindow.CPP_DATA = {\n";
        script += "    defaultSearchEngine: '" + engine + "',\n";
        script += "    searchIcon: '" + searchIcon + "',\n";
        script += "    githubIcon: '" + githubIcon + "',\n";
        script += "    wikipediaIcon: '" + wikipediaIcon + "',\n";
        script += "    favorites: " + favsJson + "\n";
        script += "};\n</script>\n";
        return html.replace("<head>", "<head>" + script);
    }
    QString generateAboutPage() {
        auto& s = Settings_Backend::instance();

        QFile aboutFile(":/about.html");
        if (!aboutFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open about.html resource";
            return QString();
        }

        QString html = QString::fromUtf8(aboutFile.readAll());
        aboutFile.close();

        return html
            .arg(infoIcon)
            .arg(QApplication::applicationName())
            .arg(QApplication::applicationVersion())
            .arg(QLatin1String(qVersion()))
            .arg(s.chromiumFlags().isEmpty() ? "None" : s.chromiumFlags())
            .arg(s.confPath())
            .arg(s.themesPath());


    }
    QString generateGamePage() {
        QFile gameFile(":/game.html");
        gameFile.open(QIODevice::ReadOnly);

        QString html = QString::fromUtf8(gameFile.readAll());
        QString script = "\n<script>\n";
        script += "window.CPP_DATA = {\n";
        script += "    infoIcon: '" + infoIcon + "',\n";
        script += "    errorIcon: '" + errorIcon + "'\n";
        script += "};\n</script>\n";
        if (html.contains("</head>")) {
            return html.replace("</head>", script + "</head>");
        } else {
            return script + html;
        }
    }
};
class SettingsDialog : public QDialog {
Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr, std::function<void()> applyCallback = nullptr)
    : QDialog(parent), m_applyCallback(applyCallback) {
        setWindowTitle("Settings");
        setWindowIcon(QIcon::fromTheme("preferences-system"));
        resize(900, 700);
        setupUI();
        loadSettings();
    }

private slots:
    void onSaveClicked() {
        saveSettings();
        if (m_applyCallback) m_applyCallback();
        accept();
    }

    void onCancelClicked() { reject(); }

    void onOpenConfigFolder() {
        Settings_Backend::instance().openConfigFolder();
    }

    void onClearIconCache() {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Clear Icon Cache",
            "This will delete all cached icons and regenerate them. Continue?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            Settings_Backend::instance().clearIconCache();
            QMessageBox::information(this, "Success", "Icon cache cleared successfully.");
        }
    }

    void onResetData() {
        QMessageBox::StandardButton reply = QMessageBox::warning(this, "Reset All Data",
            "This will delete ALL browser data including history, favorites, settings, and cache. This action cannot be undone! Continue?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            Settings_Backend::instance().resetAllData();
            QMessageBox::information(this, "Success", "All data has been reset. Onu will restart shortly");
            COS::Tri_reset();
        }
    }

    void onAddSearchEngine() {
        QDialog dialog(this);
        dialog.setWindowTitle("Add Search Engine");
        QFormLayout *form = new QFormLayout(&dialog);
        QLineEdit *nameEdit = new QLineEdit();
        QLineEdit *urlEdit = new QLineEdit();
        urlEdit->setPlaceholderText("https://example.com/search?q=%1");
        QLineEdit *iconEdit = new QLineEdit();
        iconEdit->setPlaceholderText("Optional: icon-name");
        form->addRow("Name:", nameEdit);
        form->addRow("URL (use %1 for query):", urlEdit);
        form->addRow("Icon Theme Name:", iconEdit);
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            QString name = nameEdit->text().trimmed();
            QString url = urlEdit->text().trimmed();
            QString icon = iconEdit->text().trimmed();
            if (!name.isEmpty() && !url.isEmpty() && url.contains("%1")) {
                Settings_Backend::instance().addSearchEngine(name, url, icon);
                updateSearchEngineList();
            } else {
                QMessageBox::warning(this, "Invalid Input", "Please provide a valid name and URL with %1 placeholder.");
            }
        }
    }

    void onThemeChanged(int index) {
        QString selectedTheme = m_themeCombo->currentText();
        QString qss = Settings_Backend::instance().loadThemeQSS(selectedTheme);
        m_qssEditor->setPlainText(qss);
        qApp->setStyleSheet(qss);
        Settings_Backend::instance().setCurrentTheme(selectedTheme);
        if (m_themeStackedWidget) m_themeStackedWidget->setCurrentIndex(1);
    }

    void onCreateTheme() {
        QString themeName = QInputDialog::getText(this, "Create Theme", "Enter theme name:");
        if (themeName.isEmpty() || themeName == "System") {
            QMessageBox::warning(this, "Invalid Name", "Please choose a different theme name.");
            return;
        }
        Settings_Backend::instance().saveThemeQSS(themeName, "/* New theme, see github.com/Ktiseos-Nyx/qss_themes/ for reference */");
        refreshThemeLists(themeName);
    }

    void onDeleteTheme() {
        QString currentTheme = m_themeCombo->currentText();
        if (currentTheme == "System") {
            QMessageBox::warning(this, "Cannot Delete", "System theme cannot be deleted.");
            return;
        }
        QString message = (currentTheme == "Onu-Dark" || currentTheme == "Onu-Light")
            ? "Reset '" + currentTheme + "' to default?" : "Delete theme '" + currentTheme + "'?";
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm", message, QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            Settings_Backend::instance().deleteTheme(currentTheme);
            refreshThemeLists("System");
        }
    }

    void onSaveCurrentTheme() {
        QString currentTheme = m_themeCombo->currentText();
        if (currentTheme == "System") {
            QMessageBox::warning(this, "Cannot Save", "System theme cannot be modified.");
            return;
        }
        QString qss = m_qssEditor->toPlainText();
        Settings_Backend::instance().saveThemeQSS(currentTheme, qss);
        qApp->setStyleSheet(qss);
        QMessageBox::information(this, "Saved", "Theme saved successfully.");
    }

    void refreshThemeLists(const QString &selectTheme = QString()) {
        m_themeCombo->clear();
        m_themeCombo->addItems(Settings_Backend::instance().availableThemes());
        if (!selectTheme.isEmpty()) m_themeCombo->setCurrentText(selectTheme);
        m_themeListWidget->clear();
        for (const QString &theme : Settings_Backend::instance().availableThemes()) {
            m_themeListWidget->addItem(theme);
        }
    }

    void onEditSearchEngine() {
        int index = m_searchEngineList->currentRow();
        if (index < 0) return;
        auto &s = Settings_Backend::instance();
        QVariantList engines = s.searchEngines();
        if (index >= engines.size()) return;
        QVariantMap engine = engines[index].toMap();
        QDialog dialog(this);
        dialog.setWindowTitle("Edit Search Engine");
        QFormLayout *form = new QFormLayout(&dialog);
        QLineEdit *nameEdit = new QLineEdit(engine["name"].toString());
        QLineEdit *urlEdit = new QLineEdit(engine["url"].toString());
        QLineEdit *iconEdit = new QLineEdit(engine["icon"].toString());
        form->addRow("Name:", nameEdit);
        form->addRow("URL (use %1 for query):", urlEdit);
        form->addRow("Icon Theme Name:", iconEdit);
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            engine["name"] = nameEdit->text().trimmed();
            engine["url"] = urlEdit->text().trimmed();
            engine["icon"] = iconEdit->text().trimmed();
            engines[index] = engine;
            s.setSearchEngines(engines);
            updateSearchEngineList();
        }
    }

    void onRemoveSearchEngine() {
        int index = m_searchEngineList->currentRow();
        if (index < 0) return;
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Remove Search Engine",
            "Remove this search engine?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            Settings_Backend::instance().removeSearchEngine(index);
            updateSearchEngineList();
        }
    }

    void onSetDefaultSearchEngine() {
        int index = m_searchEngineList->currentRow();
        if (index >= 0) {
            Settings_Backend::instance().setCurrentSearchEngineIndex(index);
            updateSearchEngineList();
        }
    }

    void onOpenAdBlockList() {
        QString blockedHostsFile = Settings_Backend::instance().blockedHostsFile();
        QDesktopServices::openUrl(QUrl::fromLocalFile(blockedHostsFile));
    }

    void onBrowseDownloadPath() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Download Directory", m_downloadPathEdit->text());
        if (!dir.isEmpty()) m_downloadPathEdit->setText(dir);
    }

    void onSetEncryptionPassword() {
        QDialog dialog(this);
        dialog.setWindowTitle("Set Encryption Password");
        dialog.setModal(true);
        dialog.resize(450, 280);

        QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        QLabel *titleLabel = new QLabel("Set Custom Encryption Password");
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(12);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        mainLayout->addWidget(titleLabel);

        QLabel *infoLabel = new QLabel(
            "Enter a strong password to encrypt your browsing data.\n"
            "You will need to enter this password every time you start Onu."
        );
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet("color: gray;");
        mainLayout->addWidget(infoLabel);

        QLabel *pass1Label = new QLabel("Enter password:");
        QLineEdit *pass1Edit = new QLineEdit();
        pass1Edit->setEchoMode(QLineEdit::Password);
        pass1Edit->setPlaceholderText("Password...");
        pass1Edit->setMinimumHeight(32);
        mainLayout->addWidget(pass1Label);
        mainLayout->addWidget(pass1Edit);

        QLabel *pass2Label = new QLabel("Re-enter password:");
        QLineEdit *pass2Edit = new QLineEdit();
        pass2Edit->setEchoMode(QLineEdit::Password);
        pass2Edit->setPlaceholderText("Password...");
        pass2Edit->setMinimumHeight(32);
        mainLayout->addWidget(pass2Label);
        mainLayout->addWidget(pass2Edit);

        QLabel *warningLabel = new QLabel(
            "⚠ Warning: If you forget this password, you will lose access to all encrypted data!"
        );
        warningLabel->setWordWrap(true);
        warningLabel->setStyleSheet("color: #ff5555; font-weight: bold; padding: 5px;");
        mainLayout->addWidget(warningLabel);

        mainLayout->addStretch();

        QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel
        );
        buttonBox->button(QDialogButtonBox::Ok)->setText("Set Password");
        mainLayout->addWidget(buttonBox);

        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(pass2Edit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

        if (dialog.exec() == QDialog::Accepted) {
            QString pass1 = pass1Edit->text();
            QString pass2 = pass2Edit->text();

            if (pass1.isEmpty()) {
                QMessageBox::warning(this, "Empty Password",
                    "Password cannot be empty. Please try again.");
                return;
            }

            if (pass1 != pass2) {
                QMessageBox::warning(this, "Password Mismatch",
                    "Passwords do not match. Please try again.");
                return;
            }

            if (pass1.length() < 4) {
                QMessageBox::warning(this, "Weak Password",
                    "Password should be at least 4 characters long.");
                return;
            }

            QByteArray key = QCryptographicHash::hash(
                pass1.toUtf8(),
                QCryptographicHash::Sha256
            );

            Settings_Backend::instance().setSessionKey(key);
            Settings_Backend::instance().setEncryptionMethod("custom");

            QMessageBox::information(this, "Success",
                "Encryption password set successfully.\n\n"
                "You will need to enter this password every time you start Onu.");
        }
    }

private:
    std::function<void()> m_applyCallback;
    QTabWidget *m_tabWidget = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLineEdit *m_homepageEdit = nullptr;
    QCheckBox *m_confirmCloseCheck = nullptr;
    QCheckBox *m_restoreSessionsCheck = nullptr;
    QCheckBox *m_adblockCheck = nullptr;
    QCheckBox *m_devModeCheck = nullptr;
    QListWidget *m_searchEngineList = nullptr;
    QCheckBox *m_suggestHistoryCheck = nullptr;
    QCheckBox *m_suggestFavoritesCheck = nullptr;
    QCheckBox *m_suggestRemoteCheck = nullptr;
    QLineEdit *m_apiUrlEdit = nullptr;
    QPlainTextEdit *m_parserScriptEdit = nullptr;
    QCheckBox *m_showEngineNamesCheck = nullptr;
    QCheckBox *m_navToolbarVisible = nullptr;
    QCheckBox *m_navToolbarLockDrag = nullptr;
    QCheckBox *m_navToolbarLockFloat = nullptr;
    QCheckBox *m_urlToolbarVisible = nullptr;
    QCheckBox *m_urlToolbarLockDrag = nullptr;
    QCheckBox *m_urlToolbarLockFloat = nullptr;
    QCheckBox *m_recToolbarVisible = nullptr;
    QCheckBox *m_recToolbarLockDrag = nullptr;
    QCheckBox *m_recToolbarLockFloat = nullptr;
    QCheckBox *m_favToolbarVisible = nullptr;
    QCheckBox *m_favToolbarLockDrag = nullptr;
    QCheckBox *m_favToolbarLockFloat = nullptr;
    QCheckBox *m_tabToolbarVisible = nullptr;
    QCheckBox *m_tabToolbarLockDrag = nullptr;
    QCheckBox *m_tabToolbarLockFloat = nullptr;
    QCheckBox *m_showVolumeSlider = nullptr;
    QCheckBox *m_showReload = nullptr;
    QCheckBox *m_showForward = nullptr;
    QCheckBox *m_showMenu = nullptr;
    QSpinBox *m_navIconSizeSpinBox = nullptr;
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_fontSizeSpinBox = nullptr;
    QComboBox *m_iconThemeCombo = nullptr;
    QComboBox *m_qtStyleCombo = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QStackedWidget *m_themeStackedWidget = nullptr;
    QListWidget *m_themeListWidget = nullptr;
    QPlainTextEdit *m_qssEditor = nullptr;
    QCheckBox *m_javascriptCheck = nullptr;
    QCheckBox *m_imagesCheck = nullptr;
    QCheckBox *m_deleteCookiesCheck = nullptr;
    QCheckBox *m_doNotTrackCheck = nullptr;
    QLineEdit *m_chromiumFlagsEdit = nullptr;
    QLineEdit *m_downloadPathEdit = nullptr;
    QCheckBox *m_askDownloadCheck = nullptr;
    QCheckBox *m_allowDuplicatesCheck = nullptr;
    QCheckBox *m_smartPathCheck = nullptr;
    QCheckBox *m_openWhenCompleteCheck = nullptr;
    QPlainTextEdit *m_userScriptEditor = nullptr;
    QComboBox *m_encryptionMethodCombo = nullptr;
    QPushButton *m_setPasswordBtn = nullptr;

    void setupUI() {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QHBoxLayout *searchLayout = new QHBoxLayout();
        m_searchEdit = new QLineEdit();
        m_searchEdit->setPlaceholderText("Search settings...");
        m_searchEdit->setClearButtonEnabled(true);
        searchLayout->addWidget(new QLabel("Search:"));
        searchLayout->addWidget(m_searchEdit);
        mainLayout->addLayout(searchLayout);

        connect(m_searchEdit, &QLineEdit::textChanged, this, &SettingsDialog::filterSettings);

        m_tabWidget = new QTabWidget();
        m_tabWidget->setTabPosition(QTabWidget::West);
        m_tabWidget->setIconSize(QSize(32, 32));
        m_tabWidget->setDocumentMode(true);

        m_tabWidget->addTab(createGeneralTab(), QIcon::fromTheme("preferences-system"), QString("General"));
        m_tabWidget->setTabToolTip(0, "General Settings");

        m_tabWidget->addTab(createToolbarsTab(), QIcon::fromTheme("view-list-icons"), QString("Toolbars"));
        m_tabWidget->setTabToolTip(1, "Toolbars & Search");

        m_tabWidget->addTab(createAppearanceTab(), QIcon::fromTheme("preferences-desktop-theme"), QString("Appearance"));
        m_tabWidget->setTabToolTip(2, "Appearance");

        m_tabWidget->addTab(createPrivacyTab(), QIcon::fromTheme("security-high"), QString("Privacy"));
        m_tabWidget->setTabToolTip(3, "Privacy & Downloads");

        m_tabWidget->addTab(createExtensionsTab(), QIcon::fromTheme("application-x-addon"), QString("Scripts"));
        m_tabWidget->setTabToolTip(4, "Extensions");

        mainLayout->addWidget(m_tabWidget);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onSaveClicked);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onCancelClicked);
        mainLayout->addWidget(buttonBox);
    }

    QWidget* createScrollableTab(QWidget *content) {
        QScrollArea *scrollArea = new QScrollArea();
        scrollArea->setWidget(content);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        return scrollArea;
    }

    QWidget* createGeneralTab() {
        QWidget *widget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(15, 15, 15, 15);
        layout->setSpacing(10);

        QLabel *headerLabel = new QLabel("General Settings");
        QFont headerFont = headerLabel->font();
        headerFont.setPointSize(14);
        headerFont.setBold(true);
        headerLabel->setFont(headerFont);
        layout->addWidget(headerLabel);

        layout->addSpacing(10);

        QGroupBox *homeGroup = new QGroupBox("Startup");
        QVBoxLayout *homeLayout = new QVBoxLayout(homeGroup);

        QHBoxLayout *homeUrlLayout = new QHBoxLayout();
        homeUrlLayout->addWidget(new QLabel("Home URL:"));
        m_homepageEdit = new QLineEdit();
        m_homepageEdit->setPlaceholderText("onu://home");
        homeUrlLayout->addWidget(m_homepageEdit);
        homeLayout->addLayout(homeUrlLayout);

        m_restoreSessionsCheck = new QCheckBox("Restore webpage sessions on startup");
        homeLayout->addWidget(m_restoreSessionsCheck);

        layout->addWidget(homeGroup);

        QGroupBox *behaviorGroup = new QGroupBox("Browser Behavior");
        QVBoxLayout *behaviorLayout = new QVBoxLayout(behaviorGroup);

        m_confirmCloseCheck = new QCheckBox("Ask for confirmation when closing the browser");
        behaviorLayout->addWidget(m_confirmCloseCheck);

        m_devModeCheck = new QCheckBox("Enable Developer Mode (Inspect Element)");
        behaviorLayout->addWidget(m_devModeCheck);

        layout->addWidget(behaviorGroup);

        QGroupBox *blockingGroup = new QGroupBox("Content Blocking");
        QVBoxLayout *blockingLayout = new QVBoxLayout(blockingGroup);

        m_adblockCheck = new QCheckBox("Enable AdBlock");
        blockingLayout->addWidget(m_adblockCheck);

        QHBoxLayout *adblockBtnLayout = new QHBoxLayout();
        QPushButton *editAdBlockBtn = new QPushButton("Edit blocked_hosts.txt");
        editAdBlockBtn->setIcon(QIcon::fromTheme("document-edit"));
        editAdBlockBtn->setMaximumWidth(200);
        connect(editAdBlockBtn, &QPushButton::clicked, this, &SettingsDialog::onOpenAdBlockList);
        adblockBtnLayout->addWidget(editAdBlockBtn);
        adblockBtnLayout->addStretch();
        blockingLayout->addLayout(adblockBtnLayout);

        layout->addWidget(blockingGroup);

        layout->addSpacing(20);

        QLabel *actionsLabel = new QLabel("Actions");
        actionsLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
        layout->addWidget(actionsLabel);

        QPushButton *keybindsBtn = new QPushButton("Edit Keybinds");
        keybindsBtn->setIcon(QIcon::fromTheme("preferences-desktop-keyboard-shortcuts"));
        keybindsBtn->setObjectName("btnKeybinds");
        layout->addWidget(keybindsBtn);

        QPushButton *openConfigBtn = new QPushButton("Open Config Folder");
        openConfigBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen));
        connect(openConfigBtn, &QPushButton::clicked, this, &SettingsDialog::onOpenConfigFolder);
        layout->addWidget(openConfigBtn);

        QPushButton *clearCacheBtn = new QPushButton("Clear Icon Cache");
        clearCacheBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
        connect(clearCacheBtn, &QPushButton::clicked, this, &SettingsDialog::onClearIconCache);
        layout->addWidget(clearCacheBtn);

        QPushButton *resetBtn = new QPushButton("Remove All Data (Reset)");
        resetBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditDelete));
        resetBtn->setStyleSheet("QPushButton { color: #ff5555; }");
        connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetData);
        layout->addWidget(resetBtn);

        layout->addStretch();

        return createScrollableTab(widget);
    }

    QWidget* createToolbarsTab() {
        QWidget *widget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);

        QTabWidget *toolbarTabs = new QTabWidget();

        QWidget *generalTab = new QWidget();
        QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);
        generalLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *headerLabel = new QLabel("Toolbar Visibility & Locking");
        headerLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
        generalLayout->addWidget(headerLabel);
        generalLayout->addSpacing(10);

        QWidget *tableContainer = new QWidget();
        QGridLayout *overviewGrid = new QGridLayout(tableContainer);
        overviewGrid->setHorizontalSpacing(30);
        overviewGrid->setVerticalSpacing(10);

        QStringList headers = {"Toolbar", "Visible", "Lock Dragging", "Lock Floating"};
        for (int i = 0; i < headers.size(); ++i) {
            QLabel *header = new QLabel(headers[i]);
            header->setStyleSheet("font-weight: bold;");
            overviewGrid->addWidget(header, 0, i);
        }

        struct ToolbarItem {
            QString name;
            QString displayName;
            QString icon;
            QCheckBox** visible;
            QCheckBox** lockDrag;
            QCheckBox** lockFloat;
        };

        QList<ToolbarItem> toolbars = {
            {"Navigation", "Navigation", "go-previous", &m_navToolbarVisible, &m_navToolbarLockDrag, &m_navToolbarLockFloat},
            {"Search", "Search", "edit-find", &m_urlToolbarVisible, &m_urlToolbarLockDrag, &m_urlToolbarLockFloat},
            {"Favorites", "Favorites", "favorites", &m_favToolbarVisible, &m_favToolbarLockDrag, &m_favToolbarLockFloat},
            {"Recent", "Recent", "document-open-recent", &m_recToolbarVisible, &m_recToolbarLockDrag, &m_recToolbarLockFloat},
            {"Tab", "Tab", "tab-new", &m_tabToolbarVisible, &m_tabToolbarLockDrag, &m_tabToolbarLockFloat}
        };

        int row = 1;
        for (const auto &tb : toolbars) {
            QWidget *nameWidget = new QWidget();
            QHBoxLayout *nameLayout = new QHBoxLayout(nameWidget);
            nameLayout->setContentsMargins(5, 0, 0, 0);
            QLabel *iconLabel = new QLabel();
            iconLabel->setPixmap(QIcon::fromTheme(tb.icon).pixmap(16, 16));
            nameLayout->addWidget(iconLabel);
            QLabel *nameLabel = new QLabel(tb.displayName);
            nameLayout->addWidget(nameLabel);
            nameLayout->addStretch();
            overviewGrid->addWidget(nameWidget, row, 0);

            QCheckBox *visibleCheck = new QCheckBox();
            *tb.visible = visibleCheck;
            visibleCheck->setToolTip("Show/hide this toolbar");
            overviewGrid->addWidget(visibleCheck, row, 1, Qt::AlignCenter);

            QCheckBox *lockDragCheck = new QCheckBox();
            *tb.lockDrag = lockDragCheck;
            lockDragCheck->setToolTip("Prevent toolbar from being dragged");
            overviewGrid->addWidget(lockDragCheck, row, 2, Qt::AlignCenter);

            QCheckBox *lockFloatCheck = new QCheckBox();
            *tb.lockFloat = lockFloatCheck;
            lockFloatCheck->setToolTip("Prevent toolbar from floating");
            overviewGrid->addWidget(lockFloatCheck, row, 3, Qt::AlignCenter);

            row++;
        }

        generalLayout->addWidget(tableContainer);

        QPushButton *applyToAllBtn = new QPushButton("Apply Current Settings to All Toolbars");
        applyToAllBtn->setIcon(QIcon::fromTheme("dialog-ok-apply"));
        connect(applyToAllBtn, &QPushButton::clicked, [this]() {
            if (!m_navToolbarVisible || !m_navToolbarLockDrag || !m_navToolbarLockFloat) return;
            bool visible = m_navToolbarVisible->isChecked();
            bool lockDrag = m_navToolbarLockDrag->isChecked();
            bool lockFloat = m_navToolbarLockFloat->isChecked();
            m_urlToolbarVisible->setChecked(visible);
            m_favToolbarVisible->setChecked(visible);
            m_recToolbarVisible->setChecked(visible);
            m_tabToolbarVisible->setChecked(visible);
            m_urlToolbarLockDrag->setChecked(lockDrag);
            m_favToolbarLockDrag->setChecked(lockDrag);
            m_recToolbarLockDrag->setChecked(lockDrag);
            m_tabToolbarLockDrag->setChecked(lockDrag);
            m_urlToolbarLockFloat->setChecked(lockFloat);
            m_favToolbarLockFloat->setChecked(lockFloat);
            m_recToolbarLockFloat->setChecked(lockFloat);
            m_tabToolbarLockFloat->setChecked(lockFloat);
            QMessageBox::information(this, "Settings Applied", "Visibility and lock settings have been applied to all toolbars.");
        });
        generalLayout->addWidget(applyToAllBtn);
        generalLayout->addStretch();

        toolbarTabs->addTab(createScrollableTab(generalTab), "General");

        QWidget *navTab = new QWidget();
        QVBoxLayout *navLayout = new QVBoxLayout(navTab);
        navLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *navHeader = new QLabel("Navigation Toolbar");
        navHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
        navLayout->addWidget(navHeader);
        navLayout->addSpacing(10);

        QGroupBox *buttonGroup = new QGroupBox("Button Visibility");
        QVBoxLayout *buttonLayout = new QVBoxLayout(buttonGroup);

        m_showVolumeSlider = new QCheckBox("Show Volume Slider");
        buttonLayout->addWidget(m_showVolumeSlider);

        m_showReload = new QCheckBox("Show Reload Button");
        buttonLayout->addWidget(m_showReload);

        m_showForward = new QCheckBox("Show Forward Button");
        buttonLayout->addWidget(m_showForward);

        m_showMenu = new QCheckBox("Show Menu Button");
        buttonLayout->addWidget(m_showMenu);

        navLayout->addWidget(buttonGroup);

        QGroupBox *iconGroup = new QGroupBox("Icon Size");
        QHBoxLayout *iconLayout = new QHBoxLayout(iconGroup);
        iconLayout->addWidget(new QLabel("Navigation icons:"));
        m_navIconSizeSpinBox = new QSpinBox();
        m_navIconSizeSpinBox->setRange(16, 48);
        m_navIconSizeSpinBox->setSuffix(" px");
        iconLayout->addWidget(m_navIconSizeSpinBox);
        iconLayout->addStretch();
        navLayout->addWidget(iconGroup);

        navLayout->addStretch();

        toolbarTabs->addTab(createScrollableTab(navTab), "Navigation");

        QWidget *searchTab = new QWidget();
        QVBoxLayout *searchLayout = new QVBoxLayout(searchTab);
        searchLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *searchHeader = new QLabel("Search Toolbar");
        searchHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
        searchLayout->addWidget(searchHeader);
        searchLayout->addSpacing(10);

        QGroupBox *pickerGroup = new QGroupBox("Search Engine Picker");
        QVBoxLayout *pickerLayout = new QVBoxLayout(pickerGroup);
        m_showEngineNamesCheck = new QCheckBox("Show search engine picker");
        pickerLayout->addWidget(m_showEngineNamesCheck);
        searchLayout->addWidget(pickerGroup);

        QGroupBox *engineGroup = new QGroupBox("Search Engines");
        QVBoxLayout *engineLayout = new QVBoxLayout(engineGroup);
        m_searchEngineList = new QListWidget();
        m_searchEngineList->setMaximumHeight(150);
        engineLayout->addWidget(m_searchEngineList);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        QPushButton *addEngineBtn = new QPushButton("Add");
        addEngineBtn->setIcon(QIcon::fromTheme("list-add"));
        QPushButton *editEngineBtn = new QPushButton("Edit");
        editEngineBtn->setIcon(QIcon::fromTheme("edit"));
        QPushButton *removeEngineBtn = new QPushButton("Remove");
        removeEngineBtn->setIcon(QIcon::fromTheme("list-remove"));
        QPushButton *setDefaultBtn = new QPushButton("Set as Default");
        setDefaultBtn->setIcon(QIcon::fromTheme("emblem-default"));
        btnLayout->addWidget(addEngineBtn);
        btnLayout->addWidget(editEngineBtn);
        btnLayout->addWidget(removeEngineBtn);
        btnLayout->addWidget(setDefaultBtn);
        btnLayout->addStretch();
        engineLayout->addLayout(btnLayout);

        connect(addEngineBtn, &QPushButton::clicked, this, &SettingsDialog::onAddSearchEngine);
        connect(editEngineBtn, &QPushButton::clicked, this, &SettingsDialog::onEditSearchEngine);
        connect(removeEngineBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveSearchEngine);
        connect(setDefaultBtn, &QPushButton::clicked, this, &SettingsDialog::onSetDefaultSearchEngine);

        searchLayout->addWidget(engineGroup);

        QGroupBox *suggestGroup = new QGroupBox("Suggestion Sources");
        QVBoxLayout *suggestLayout = new QVBoxLayout(suggestGroup);

        m_suggestHistoryCheck = new QCheckBox("Suggest from History");
        suggestLayout->addWidget(m_suggestHistoryCheck);

        m_suggestFavoritesCheck = new QCheckBox("Suggest from Favorites");
        suggestLayout->addWidget(m_suggestFavoritesCheck);

        m_suggestRemoteCheck = new QCheckBox("Suggest from Remote API");
        suggestLayout->addWidget(m_suggestRemoteCheck);

        QWidget *apiWidget = new QWidget();
        QVBoxLayout *apiLayout = new QVBoxLayout(apiWidget);
        apiLayout->setContentsMargins(20, 5, 5, 5);

        QHBoxLayout *urlLayout = new QHBoxLayout();
        urlLayout->addWidget(new QLabel("API URL:"));
        m_apiUrlEdit = new QLineEdit();
        m_apiUrlEdit->setPlaceholderText("https://api.example.com/suggest?q=%1");
        urlLayout->addWidget(m_apiUrlEdit);
        apiLayout->addLayout(urlLayout);

        QLabel *parserLabel = new QLabel("Parser Script:");
        m_parserScriptEdit = new QPlainTextEdit();
        m_parserScriptEdit->setFont(QFont("Monospace", 9));
        m_parserScriptEdit->setMaximumHeight(120);
        apiLayout->addWidget(parserLabel);
        apiLayout->addWidget(m_parserScriptEdit);

        connect(m_suggestRemoteCheck, &QCheckBox::toggled, apiWidget, &QWidget::setEnabled);

        suggestLayout->addWidget(apiWidget);
        searchLayout->addWidget(suggestGroup);
        searchLayout->addStretch();

        toolbarTabs->addTab(createScrollableTab(searchTab), "Search");

        QWidget *favTab = new QWidget();
        QVBoxLayout *favLayout = new QVBoxLayout(favTab);
        favLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *favHeader = new QLabel("Favorites Toolbar");
        favHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
        favLayout->addWidget(favHeader);
        favLayout->addSpacing(10);

        QGroupBox *favLimitGroup = new QGroupBox("Items Limit");
        QVBoxLayout *favLimitLayout = new QVBoxLayout(favLimitGroup);
        QLabel *favLimitLabel = new QLabel("Number of favorites to show:");
        favLimitLayout->addWidget(favLimitLabel);
        QComboBox *favLimitCombo = new QComboBox();
        QList<int> favLimits = {3, 5, 10, 15, 20, 30};
        for (int l : favLimits) favLimitCombo->addItem(QString::number(l), l);
        favLimitCombo->setCurrentText(QString::number(Settings_Backend::instance().favSidebarLimit()));
        connect(favLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [favLimitCombo](int index) {
            Settings_Backend::instance().setFavSidebarLimit(favLimitCombo->itemData(index).toInt());
        });
        favLimitLayout->addWidget(favLimitCombo);
        favLayout->addWidget(favLimitGroup);

        favLayout->addSpacing(10);

        QPushButton *manageFavBtn = new QPushButton("Open Favorites Manager");
        manageFavBtn->setIcon(QIcon::fromTheme("bookmarks-organize"));
        manageFavBtn->setObjectName("btnFavorites");
        favLayout->addWidget(manageFavBtn);

        favLayout->addStretch();

        toolbarTabs->addTab(createScrollableTab(favTab), "Favorites");

        QWidget *recTab = new QWidget();
        QVBoxLayout *recLayout = new QVBoxLayout(recTab);
        recLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *recHeader = new QLabel("Recent Toolbar");
        recHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
        recLayout->addWidget(recHeader);
        recLayout->addSpacing(10);

        QGroupBox *recLimitGroup = new QGroupBox("Items Limit");
        QVBoxLayout *recLimitLayout = new QVBoxLayout(recLimitGroup);
        QLabel *recLimitLabel = new QLabel("Number of recent items to show:");
        recLimitLayout->addWidget(recLimitLabel);
        QComboBox *recLimitCombo = new QComboBox();
        QList<int> recLimits = {3, 5, 10, 15, 20, 30};
        for (int l : recLimits) recLimitCombo->addItem(QString::number(l), l);
        recLimitCombo->setCurrentText(QString::number(Settings_Backend::instance().historySidebarLimit()));
        connect(recLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [recLimitCombo](int index) {
            Settings_Backend::instance().setHistorySidebarLimit(recLimitCombo->itemData(index).toInt());
        });
        recLimitLayout->addWidget(recLimitCombo);
        recLayout->addWidget(recLimitGroup);

        recLayout->addSpacing(10);

        QPushButton *clearHistoryBtn = new QPushButton("Clear Recent Items");
        clearHistoryBtn->setIcon(QIcon::fromTheme("edit-clear"));
        connect(clearHistoryBtn, &QPushButton::clicked, [this]() {
            Settings_Backend::instance().clearHistory();
            QMessageBox::information(this, "History Cleared", "Recent history has been cleared.");
        });
        recLayout->addWidget(clearHistoryBtn);

        recLayout->addSpacing(10);

        QPushButton *openHistoryBtn = new QPushButton("Open History Manager");
        openHistoryBtn->setIcon(QIcon::fromTheme("view-history"));
        openHistoryBtn->setObjectName("btnHistory");
        recLayout->addWidget(openHistoryBtn);

        recLayout->addStretch();

        toolbarTabs->addTab(createScrollableTab(recTab), "Recent");

        QWidget *tabBarTab = new QWidget();
        QVBoxLayout *tabBarLayout = new QVBoxLayout(tabBarTab);
        tabBarLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *tabBarHeader = new QLabel("Tab Bar");
        tabBarHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
        tabBarLayout->addWidget(tabBarHeader);
        tabBarLayout->addSpacing(10);

        QLabel *infoLabel = new QLabel("Tab bar settings managed in General subtab above.");
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet("color: gray;");
        tabBarLayout->addWidget(infoLabel);

        tabBarLayout->addStretch();

        toolbarTabs->addTab(createScrollableTab(tabBarTab), "Tab Bar");

        layout->addWidget(toolbarTabs);

        return widget;
    }

    QWidget* createAppearanceTab() {
        QWidget *widget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(15, 15, 15, 15);
        layout->setSpacing(15);

        QLabel *headerLabel = new QLabel("Appearance");
        QFont headerFont = headerLabel->font();
        headerFont.setPointSize(14);
        headerFont.setBold(true);
        headerLabel->setFont(headerFont);
        layout->addWidget(headerLabel);

        QGroupBox *fontGroup = new QGroupBox("Fonts");
        QVBoxLayout *fontGroupLayout = new QVBoxLayout(fontGroup);

        QHBoxLayout *fontLayout = new QHBoxLayout();
        fontLayout->addWidget(new QLabel("Font Family:"));
        m_fontCombo = new QFontComboBox();
        fontLayout->addWidget(m_fontCombo);
        fontGroupLayout->addLayout(fontLayout);

        QHBoxLayout *fontSizeLayout = new QHBoxLayout();
        fontSizeLayout->addWidget(new QLabel("Font Size:"));
        m_fontSizeSpinBox = new QSpinBox();
        m_fontSizeSpinBox->setRange(8, 24);
        m_fontSizeSpinBox->setValue(10);
        m_fontSizeSpinBox->setSuffix(" pt");
        fontSizeLayout->addWidget(m_fontSizeSpinBox);
        fontSizeLayout->addStretch();
        fontGroupLayout->addLayout(fontSizeLayout);

        layout->addWidget(fontGroup);

        QGroupBox *styleGroup = new QGroupBox("Icons and Style");
        QVBoxLayout *styleGroupLayout = new QVBoxLayout(styleGroup);

        QHBoxLayout *iconThemeLayout = new QHBoxLayout();
        iconThemeLayout->addWidget(new QLabel("Icon Theme:"));
        m_iconThemeCombo = new QComboBox();
        m_iconThemeCombo->addItems(Settings_Backend::instance().availableIconThemes());
        iconThemeLayout->addWidget(m_iconThemeCombo);
        styleGroupLayout->addLayout(iconThemeLayout);

        QHBoxLayout *qtStyleLayout = new QHBoxLayout();
        qtStyleLayout->addWidget(new QLabel("Qt Style:"));
        m_qtStyleCombo = new QComboBox();
        m_qtStyleCombo->addItems(QStyleFactory::keys());
        qtStyleLayout->addWidget(m_qtStyleCombo);
        styleGroupLayout->addLayout(qtStyleLayout);

        layout->addWidget(styleGroup);

        layout->addSpacing(10);

        QLabel *themeLabel = new QLabel("Theme Editor");
        themeLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
        layout->addWidget(themeLabel);

        QHBoxLayout *themeLayout = new QHBoxLayout();
        themeLayout->addWidget(new QLabel("Current Theme:"));
        m_themeCombo = new QComboBox();
        m_themeCombo->addItems(Settings_Backend::instance().availableThemes());
        connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onThemeChanged);
        themeLayout->addWidget(m_themeCombo);
        layout->addLayout(themeLayout);

        m_themeStackedWidget = new QStackedWidget();

        QWidget *themeListPage = new QWidget();
        QHBoxLayout *themeListLayout = new QHBoxLayout(themeListPage);

        QVBoxLayout *themeListLeftLayout = new QVBoxLayout();
        QPushButton *deleteThemeBtn = new QPushButton();
        deleteThemeBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditDelete));
        deleteThemeBtn->setMaximumWidth(30);
        deleteThemeBtn->setToolTip("Delete Theme");
        connect(deleteThemeBtn, &QPushButton::clicked, this, &SettingsDialog::onDeleteTheme);
        themeListLeftLayout->addWidget(deleteThemeBtn);

        QPushButton *addThemeBtn = new QPushButton();
        addThemeBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        addThemeBtn->setMaximumWidth(30);
        addThemeBtn->setToolTip("Create New Theme");
        connect(addThemeBtn, &QPushButton::clicked, this, &SettingsDialog::onCreateTheme);
        themeListLeftLayout->addWidget(addThemeBtn);
        themeListLeftLayout->addStretch();

        m_themeListWidget = new QListWidget();
        for (const QString &theme : Settings_Backend::instance().availableThemes()) {
            m_themeListWidget->addItem(theme);
        }

        themeListLayout->addLayout(themeListLeftLayout);
        themeListLayout->addWidget(m_themeListWidget);
        m_themeStackedWidget->addWidget(themeListPage);

        QWidget *qssEditorPage = new QWidget();
        QVBoxLayout *qssEditorLayout = new QVBoxLayout(qssEditorPage);

        m_qssEditor = new QPlainTextEdit();
        m_qssEditor->setFont(QFont("Monospace", 10));
        m_qssEditor->setPlaceholderText("Enter your QSS stylesheet here...");
        qssEditorLayout->addWidget(m_qssEditor);

        QHBoxLayout *qssButtonLayout = new QHBoxLayout();
        QPushButton *backToListBtn = new QPushButton("Back to List");
        backToListBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoPrevious));
        connect(backToListBtn, &QPushButton::clicked, [this]() {
            m_themeStackedWidget->setCurrentIndex(0);
        });
        qssButtonLayout->addWidget(backToListBtn);
        qssButtonLayout->addStretch();

        QPushButton *saveThemeBtn = new QPushButton("Save Theme");
        saveThemeBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave));
        connect(saveThemeBtn, &QPushButton::clicked, this, &SettingsDialog::onSaveCurrentTheme);
        qssButtonLayout->addWidget(saveThemeBtn);
        qssEditorLayout->addLayout(qssButtonLayout);

        m_themeStackedWidget->addWidget(qssEditorPage);
        layout->addWidget(m_themeStackedWidget);

        return createScrollableTab(widget);
    }

    QWidget* createPrivacyTab() {
        QWidget *widget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(15, 15, 15, 15);
        layout->setSpacing(15);

        QLabel *headerLabel = new QLabel("Privacy & Downloads");
        QFont headerFont = headerLabel->font();
        headerFont.setPointSize(14);
        headerFont.setBold(true);
        headerLabel->setFont(headerFont);
        layout->addWidget(headerLabel);

        QGroupBox *webpageGroup = new QGroupBox("Webpage Settings");
        QVBoxLayout *webpageLayout = new QVBoxLayout(webpageGroup);

        m_javascriptCheck = new QCheckBox("Enable JavaScript Execution");
        webpageLayout->addWidget(m_javascriptCheck);

        m_imagesCheck = new QCheckBox("Load Images");
        webpageLayout->addWidget(m_imagesCheck);

        m_deleteCookiesCheck = new QCheckBox("Delete Cookies in Every Session");
        webpageLayout->addWidget(m_deleteCookiesCheck);

        m_doNotTrackCheck = new QCheckBox("Send Do Not Track Headers to All Websites");
        webpageLayout->addWidget(m_doNotTrackCheck);

        layout->addWidget(webpageGroup);

        QGroupBox *advancedGroup = new QGroupBox("Advanced");
        QVBoxLayout *advancedLayout = new QVBoxLayout(advancedGroup);

        QHBoxLayout *flagsLayout = new QHBoxLayout();
        flagsLayout->addWidget(new QLabel("Chromium Flags:"));
        m_chromiumFlagsEdit = new QLineEdit();
        m_chromiumFlagsEdit->setPlaceholderText("--flag1 --flag2");
        flagsLayout->addWidget(m_chromiumFlagsEdit);
        advancedLayout->addLayout(flagsLayout);

        layout->addWidget(advancedGroup);

        QGroupBox *encryptionGroup = new QGroupBox("Encryption");
        QVBoxLayout *encryptionLayout = new QVBoxLayout(encryptionGroup);

        QHBoxLayout *methodLayout = new QHBoxLayout();
        methodLayout->addWidget(new QLabel("Encryption Method:"));
        m_encryptionMethodCombo = new QComboBox();
        m_encryptionMethodCombo->addItem("Default (System-based)", "default");
        m_encryptionMethodCombo->addItem("Custom Password", "custom");
        methodLayout->addWidget(m_encryptionMethodCombo);
        encryptionLayout->addLayout(methodLayout);

        QLabel *encWarning = new QLabel(
            "⚠ Custom encryption requires entering your password every session.\n"
            "Forgetting your password after 10 failed attempts requires deleting encrypted.conf\n"
            "which will permanently erase all encrypted data (history, favorites, sessions)."
        );
        encWarning->setWordWrap(true);
        encWarning->setStyleSheet("color: #ff9800; padding: 8px; font-size: 11px;");
        encryptionLayout->addWidget(encWarning);

        m_setPasswordBtn = new QPushButton("Set/Change Encryption Password");
        m_setPasswordBtn->setIcon(QIcon::fromTheme("dialog-password"));
        m_setPasswordBtn->setEnabled(false);
        encryptionLayout->addWidget(m_setPasswordBtn);

        connect(m_encryptionMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SettingsDialog::handleEncryptionMethodChange);


        connect(m_setPasswordBtn, &QPushButton::clicked, this, &SettingsDialog::onSetEncryptionPassword);

        layout->addWidget(encryptionGroup);

        QGroupBox *downloadsGroup = new QGroupBox("Downloads");
        QVBoxLayout *downloadsLayout = new QVBoxLayout(downloadsGroup);

        QHBoxLayout *downloadPathLayout = new QHBoxLayout();
        downloadPathLayout->addWidget(new QLabel("Download Path:"));
        m_downloadPathEdit = new QLineEdit();
        downloadPathLayout->addWidget(m_downloadPathEdit);
        QPushButton *browseBtn = new QPushButton("Browse");
        connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseDownloadPath);
        downloadPathLayout->addWidget(browseBtn);
        downloadsLayout->addLayout(downloadPathLayout);

        m_askDownloadCheck = new QCheckBox("Ask for Approval When Download Requested");
        downloadsLayout->addWidget(m_askDownloadCheck);

        m_allowDuplicatesCheck = new QCheckBox("Allow Duplicate Downloads");
        downloadsLayout->addWidget(m_allowDuplicatesCheck);

        m_smartPathCheck = new QCheckBox("Smart Path (organize downloads by file type)");
        downloadsLayout->addWidget(m_smartPathCheck);

        m_openWhenCompleteCheck = new QCheckBox("Open Folder When Download Completes");
        downloadsLayout->addWidget(m_openWhenCompleteCheck);

        QPushButton *openDownloadsBtn = new QPushButton("Open Downloads Manager");
        openDownloadsBtn->setIcon(QIcon::fromTheme("folder-download"));
        openDownloadsBtn->setToolTip("Use Ctrl+J");
        downloadsLayout->addWidget(openDownloadsBtn);

        layout->addWidget(downloadsGroup);

        layout->addStretch();


        QGroupBox *permissionsGroup = new QGroupBox("Site Permissions");
        QVBoxLayout *permissionsLayout = new QVBoxLayout(permissionsGroup);

        QLabel *permDesc = new QLabel("Control which sites can access device features and show notifications.");
        permDesc->setWordWrap(true);
        permissionsLayout->addWidget(permDesc);
        permissionsLayout->addSpacing(5);

        QPushButton *manageSitesBtn = new QPushButton("Manage Site Permissions");
        manageSitesBtn->setIcon(QIcon::fromTheme("preferences-system"));
        manageSitesBtn->setObjectName("btnSitePermissions");
        manageSitesBtn->setMinimumHeight(32);
        manageSitesBtn->setToolTip("View and manage permissions for all sites");
        permissionsLayout->addWidget(manageSitesBtn);

        layout->addWidget(permissionsGroup);

        return createScrollableTab(widget);
    }

    QWidget* createExtensionsTab() {
        QWidget *widget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(15, 15, 15, 15);
        layout->setSpacing(15);

        QLabel *headerLabel = new QLabel("Extensions and User Scripts");
        QFont headerFont = headerLabel->font();
        headerFont.setPointSize(14);
        headerFont.setBold(true);
        headerLabel->setFont(headerFont);
        layout->addWidget(headerLabel);

        QGroupBox *extGroup = new QGroupBox("Browser Extensions");
        QVBoxLayout *extLayout = new QVBoxLayout(extGroup);

        QLabel *extInfoLabel = new QLabel("Extensions are .so or .dll files in: " + Settings_Backend::instance().extensionsPath());
        extInfoLabel->setWordWrap(true);
        extInfoLabel->setStyleSheet("color: gray;");
        extLayout->addWidget(extInfoLabel);

        QHBoxLayout *extBtnLayout = new QHBoxLayout();
        QPushButton *openExtManagerBtn = new QPushButton("Open Extension Manager");
        openExtManagerBtn->setIcon(QIcon::fromTheme("application-x-addon"));
        openExtManagerBtn->setObjectName("btnExtensions");
        extBtnLayout->addWidget(openExtManagerBtn);

        QPushButton *openExtFolderBtn = new QPushButton("Open Extensions Folder");
        openExtFolderBtn->setIcon(QIcon::fromTheme("folder-open"));
        connect(openExtFolderBtn, &QPushButton::clicked, [this]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(Settings_Backend::instance().extensionsPath()));
        });
        extBtnLayout->addWidget(openExtFolderBtn);
        extBtnLayout->addStretch();
        extLayout->addLayout(extBtnLayout);

        layout->addWidget(extGroup);

        QGroupBox *scriptGroup = new QGroupBox("User Script (JavaScript)");
        QVBoxLayout *scriptLayout = new QVBoxLayout(scriptGroup);

        QLabel *scriptInfoLabel = new QLabel("User scripts execute on every page. Use with caution.");
        scriptInfoLabel->setWordWrap(true);
        scriptInfoLabel->setStyleSheet("color: #ff9800;");
        scriptLayout->addWidget(scriptInfoLabel);

        QHBoxLayout *scriptButtonLayout = new QHBoxLayout();
        QPushButton *newScriptBtn = new QPushButton();
        newScriptBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew));
        newScriptBtn->setMaximumWidth(30);
        newScriptBtn->setToolTip("New Script");
        connect(newScriptBtn, &QPushButton::clicked, [this]() { m_userScriptEditor->clear(); });
        scriptButtonLayout->addWidget(newScriptBtn);

        QPushButton *clearScriptBtn = new QPushButton();
        clearScriptBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
        clearScriptBtn->setMaximumWidth(30);
        clearScriptBtn->setToolTip("Clear Script");
        connect(clearScriptBtn, &QPushButton::clicked, [this]() { m_userScriptEditor->clear(); });
        scriptButtonLayout->addWidget(clearScriptBtn);
        scriptButtonLayout->addStretch();
        scriptLayout->addLayout(scriptButtonLayout);

        m_userScriptEditor = new QPlainTextEdit();
        m_userScriptEditor->setFont(QFont("Monospace", 10));
        m_userScriptEditor->setPlaceholderText("// Your JavaScript code here...");
        m_userScriptEditor->setMinimumHeight(200);
        scriptLayout->addWidget(m_userScriptEditor);

        layout->addWidget(scriptGroup);

        layout->addStretch();

        return createScrollableTab(widget);
    }

    void updateSearchEngineList() {
        m_searchEngineList->clear();
        auto &s = Settings_Backend::instance();
        QVariantList engines = s.searchEngines();
        int currentIndex = s.currentSearchEngineIndex();
        for (int i = 0; i < engines.size(); ++i) {
            QVariantMap engine = engines[i].toMap();
            QListWidgetItem *item = new QListWidgetItem();
            QString text = engine["name"].toString();
            if (i == currentIndex) {
                text += "   ← Default";
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
            item->setText(text);
            item->setToolTip(engine["url"].toString());
            QString iconName = engine["icon"].toString();
            if (!iconName.isEmpty()) {
                item->setIcon(QIcon::fromTheme(iconName, QIcon::fromTheme("edit-find")));
            } else {
                item->setIcon(QIcon::fromTheme("edit-find"));
            }
            m_searchEngineList->addItem(item);
        }
    }

    void loadSettings() {
        auto &s = Settings_Backend::instance();
        m_homepageEdit->setText(s.homepage());
        m_confirmCloseCheck->setChecked(s.confirmClose());
        m_restoreSessionsCheck->setChecked(s.restoreSessions());
        m_adblockCheck->setChecked(s.adblockEnabled());
        m_devModeCheck->setChecked(s.devMode());
        updateSearchEngineList();
        m_suggestHistoryCheck->setChecked(s.suggestionsFromHistory());
        m_suggestFavoritesCheck->setChecked(s.suggestionsFromFavorites());
        m_suggestRemoteCheck->setChecked(s.suggestionsFromRemote());
        m_apiUrlEdit->setText(s.suggestionApiUrl());
        m_parserScriptEdit->setPlainText(s.suggestionParserScript());
        m_showEngineNamesCheck->setChecked(s.showSearchEnginePicker());
        m_navToolbarVisible->setChecked(s.toolbarVisible("Navigation"));
        m_navToolbarLockDrag->setChecked(s.toolbarLockDragging("Navigation"));
        m_navToolbarLockFloat->setChecked(s.toolbarLockFloating("Navigation"));
        m_urlToolbarVisible->setChecked(s.toolbarVisible("Search"));
        m_urlToolbarLockDrag->setChecked(s.toolbarLockDragging("Search"));
        m_urlToolbarLockFloat->setChecked(s.toolbarLockFloating("Search"));
        m_recToolbarVisible->setChecked(s.toolbarVisible("Recent"));
        m_recToolbarLockDrag->setChecked(s.toolbarLockDragging("Recent"));
        m_recToolbarLockFloat->setChecked(s.toolbarLockFloating("Recent"));
        m_favToolbarVisible->setChecked(s.toolbarVisible("Favorites"));
        m_favToolbarLockDrag->setChecked(s.toolbarLockDragging("Favorites"));
        m_favToolbarLockFloat->setChecked(s.toolbarLockFloating("Favorites"));
        m_tabToolbarVisible->setChecked(s.toolbarVisible("Tab"));
        m_tabToolbarLockDrag->setChecked(s.toolbarLockDragging("Tab"));
        m_tabToolbarLockFloat->setChecked(s.toolbarLockFloating("Tab"));
        m_showVolumeSlider->setChecked(s.showVolumeSlider());
        m_showReload->setChecked(s.showReload());
        m_showForward->setChecked(s.showForward());
        m_showMenu->setChecked(s.showMenu());
        m_navIconSizeSpinBox->setValue(s.navigationIconSize());
        if (!s.fontFamily().isEmpty()) m_fontCombo->setCurrentFont(QFont(s.fontFamily()));
        m_fontSizeSpinBox->setValue(s.fontSize());
        m_iconThemeCombo->setCurrentText(s.iconTheme());
        m_qtStyleCombo->setCurrentText(s.qtStyle());
        m_themeCombo->setCurrentText(s.currentTheme());
        m_javascriptCheck->setChecked(s.javascriptEnabled());
        m_imagesCheck->setChecked(s.imagesEnabled());
        m_deleteCookiesCheck->setChecked(s.deleteCookies());
        m_doNotTrackCheck->setChecked(s.doNotTrack());
        m_chromiumFlagsEdit->setText(s.chromiumFlags());
        m_downloadPathEdit->setText(s.downloadPath());
        m_askDownloadCheck->setChecked(s.askDownloadConfirm());
        m_allowDuplicatesCheck->setChecked(s.allowDuplicates());
        m_smartPathCheck->setChecked(s.smartPath());
        m_openWhenCompleteCheck->setChecked(s.openWhenComplete());
        m_userScriptEditor->setPlainText(s.userScript());

        QString encMethod = s.useCustomEncryption() ? "custom" : "default";
        m_encryptionMethodCombo->setCurrentIndex(encMethod == "custom" ? 1 : 0);
        m_setPasswordBtn->setEnabled(encMethod == "custom");
    }
    void handleEncryptionMethodChange(int index) {
        m_setPasswordBtn->setEnabled(index == 1);

        auto &backend = Settings_Backend::instance();

        if (index == 0 && backend.useCustomEncryption()) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Switch to Default Encryption",
                "You are currently using a custom encryption key.\n\n"
                "Switching to Default will require your current password to decrypt data "
                "and then re-encrypt it with the system key.\n\n"
                "Do you want to continue?",
                QMessageBox::Yes | QMessageBox::No
            );

            if (reply == QMessageBox::Yes) {
                bool verified = false;
                QString pass = QInputDialog::getText(
                    this,
                    "Verify Encryption Password",
                    "Enter your current encryption password:",
                    QLineEdit::Password
                );

                if (!pass.isEmpty()) {
                    QByteArray key = QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256);
                    backend.setSessionKey(key);
                    verified = backend.verifyEncryptionKey();
                }

                if (!verified) {
                    QMessageBox::warning(this, "Failed",
                        "Could not verify your current encryption key. Data remains encrypted.");
                    m_encryptionMethodCombo->setCurrentIndex(1);

                    return;
                }

                QVariantList history = backend.loadHistory();
                QVariantList favorites = backend.loadFavorites();
                QVariantList session = backend.loadSession();
                QList<Settings_Backend::ExtState> extensions = backend.loadExtensionStates();

                backend.setEncryptionMethod("default");

                struct utsname b;
                QByteArray sysKey;
                if (uname(&b) == 0) {
                    sysKey.append(b.sysname);
                    sysKey.append(b.machine);
                }
                QByteArray defaultKey = QCryptographicHash::hash(sysKey, QCryptographicHash::Sha256);
                backend.setSessionKey(defaultKey);

                backend.saveHistory(history);
                backend.saveFavorites(favorites);
                backend.saveSession(session);
                backend.saveExtensionStates(extensions);

                backend.setEncryptionVerificationValue();

                QMessageBox::information(this, "Encryption Reset",
                    "Data has been successfully re-encrypted using the default system key.");
            } else {

                m_encryptionMethodCombo->setCurrentIndex(1);
            }
        }

    }
    void saveSettings() {
        auto &s = Settings_Backend::instance();
        s.setPublicValue("homepage", m_homepageEdit->text());
        s.setPublicValue("privacy/confirmClose", m_confirmCloseCheck->isChecked());
        s.setPublicValue("privacy/restoreSessions", m_restoreSessionsCheck->isChecked());
        s.setPublicValue("adblock/enabled", m_adblockCheck->isChecked());
        s.setPublicValue("developer/enabled", m_devModeCheck->isChecked());
        s.setSuggestionsFromHistory(m_suggestHistoryCheck->isChecked());
        s.setSuggestionsFromFavorites(m_suggestFavoritesCheck->isChecked());
        s.setSuggestionsFromRemote(m_suggestRemoteCheck->isChecked());
        s.setSuggestionApiUrl(m_apiUrlEdit->text());
        s.setSuggestionParserScript(m_parserScriptEdit->toPlainText());
        s.setShowSearchEnginePicker(m_showEngineNamesCheck->isChecked());
        s.setToolbarVisible("Navigation", m_navToolbarVisible->isChecked());
        s.setToolbarLockDragging("Navigation", m_navToolbarLockDrag->isChecked());
        s.setToolbarLockFloating("Navigation", m_navToolbarLockFloat->isChecked());
        s.setToolbarVisible("Search", m_urlToolbarVisible->isChecked());
        s.setToolbarLockDragging("Search", m_urlToolbarLockDrag->isChecked());
        s.setToolbarLockFloating("Search", m_urlToolbarLockFloat->isChecked());
        s.setToolbarVisible("Recent", m_recToolbarVisible->isChecked());
        s.setToolbarLockDragging("Recent", m_recToolbarLockDrag->isChecked());
        s.setToolbarLockFloating("Recent", m_recToolbarLockFloat->isChecked());
        s.setToolbarVisible("Favorites", m_favToolbarVisible->isChecked());
        s.setToolbarLockDragging("Favorites", m_favToolbarLockDrag->isChecked());
        s.setToolbarLockFloating("Favorites", m_favToolbarLockFloat->isChecked());
        s.setToolbarVisible("Tab", m_tabToolbarVisible->isChecked());
        s.setToolbarLockDragging("Tab", m_tabToolbarLockDrag->isChecked());
        s.setToolbarLockFloating("Tab", m_tabToolbarLockFloat->isChecked());
        s.setPublicValue("navigation/showVolume", m_showVolumeSlider->isChecked());
        s.setPublicValue("navigation/showReload", m_showReload->isChecked());
        s.setPublicValue("navigation/showForward", m_showForward->isChecked());
        s.setPublicValue("navigation/showMenu", m_showMenu->isChecked());
        s.setPublicValue("navigation/iconSize", m_navIconSizeSpinBox->value());
        s.setPublicValue("appearance/font", m_fontCombo->currentFont().family());
        s.setPublicValue("appearance/fontSize", m_fontSizeSpinBox->value());
        s.setIconTheme(m_iconThemeCombo->currentText());
        s.setPublicValue("appearance/qtStyle", m_qtStyleCombo->currentText());
        s.setCurrentTheme(m_themeCombo->currentText());
        s.setPublicValue("privacy/javascript", m_javascriptCheck->isChecked());
        s.setPublicValue("privacy/images", m_imagesCheck->isChecked());
        s.setPublicValue("privacy/deleteCookies", m_deleteCookiesCheck->isChecked());
        s.setPublicValue("privacy/doNotTrack", m_doNotTrackCheck->isChecked());
        s.setPublicValue("flags/chromium", m_chromiumFlagsEdit->text());
        s.setDownloadPath(m_downloadPathEdit->text());
        s.setPublicValue("downloads/askConfirm", m_askDownloadCheck->isChecked());
        s.setPublicValue("downloads/allowDuplicates", m_allowDuplicatesCheck->isChecked());
        s.setPublicValue("downloads/smartPath", m_smartPathCheck->isChecked());
        s.setPublicValue("downloads/openComplete", m_openWhenCompleteCheck->isChecked());
        s.setUserScript(m_userScriptEditor->toPlainText());

        QString selectedMethod = m_encryptionMethodCombo->currentData().toString();
        s.setEncryptionMethod(selectedMethod);
    }

    void filterSettings(const QString &query) {
        if (query.isEmpty()) {
            for (int i = 0; i < m_tabWidget->count(); ++i) m_tabWidget->setTabVisible(i, true);
            return;
        }
        QString lowerQuery = query.toLower();
        QMap<int, QStringList> tabKeywords = {
            {0, {"general", "home", "close", "adblock", "developer", "config", "cache", "reset", "startup", "behavior"}},
            {1, {"toolbar", "search", "engine", "suggest", "navigation", "volume", "reload", "forward", "favorites", "recent", "history"}},
            {2, {"appearance", "theme", "font", "icon", "style", "qss", "color"}},
            {3, {"privacy", "download", "javascript", "cookie", "track", "flag", "chromium", "encryption", "password", "secure"}},
            {4, {"extension", "plugin", "script", "user", "addon"}}
        };
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            bool matches = false;
            for (const QString &keyword : tabKeywords[i]) {
                if (keyword.contains(lowerQuery)) { matches = true; break; }
            }
            m_tabWidget->setTabVisible(i, matches);
        }
    }
};
class SessionNotifications {
public:
    struct Notification {
        QString origin;
        QString title;
        QString body;
        QString iconPath;
        qint64 timestamp;
    };

    static void add(const QString &origin, const QString &title,
                    const QString &body, const QString &iconPath) {
        Notification n{origin, title, body, iconPath, QDateTime::currentMSecsSinceEpoch()};
        instance().notifications.prepend(n);
        if (instance().notifications.size() > 50) instance().notifications.removeLast();
    }

    static QList<Notification> getForOrigin(const QString &origin) {
        QList<Notification> filtered;
        for (const auto &n : std::as_const(instance().notifications)) {
            if (n.origin == origin) filtered.append(n);
        }
        return filtered;
    }

    static void clearForOrigin(const QString &origin) {
        auto &inst = instance();
        inst.notifications.erase(
            std::remove_if(inst.notifications.begin(), inst.notifications.end(),
                [&origin](const Notification &n) { return n.origin == origin; }),
            inst.notifications.end()
        );
    }

    static void clear() { instance().notifications.clear(); }

private:
    static SessionNotifications& instance() {
        static SessionNotifications inst;
        return inst;
    }
    QList<Notification> notifications;
};
class Dialogs : public QObject {
    Q_OBJECT
public:
    static void fav(QWidget *parent, QVariantList &favorites, std::function<void()> refreshCallback) {
        auto dialog = std::make_unique<QDialog>(parent);
        dialog->setWindowTitle("Favorites Manager");
        dialog->setWindowIcon(QIcon::fromTheme("bookmark-new"));
        dialog->resize(800, 600);
        dialog->setMinimumSize(600, 400);

        QVBoxLayout *layout = new QVBoxLayout(dialog.get());
        layout->setSpacing(10);
        layout->setContentsMargins(10, 10, 10, 10);

        QHBoxLayout *headerLayout = new QHBoxLayout();
        QLabel *titleLabel = new QLabel("Favorites");
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(14);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        headerLayout->addWidget(titleLabel);

        QLabel *statsLabel = new QLabel();
        statsLabel->setStyleSheet("color: gray; padding: 5px;");
        headerLayout->addWidget(statsLabel);
        headerLayout->addStretch();
        layout->addLayout(headerLayout);

        QHBoxLayout *searchLayout = new QHBoxLayout();
        QLineEdit *searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Search favorites by title or URL...");
        searchEdit->setClearButtonEnabled(true);
        searchEdit->setMinimumHeight(32);
        searchLayout->addWidget(new QLabel("Search:"));
        searchLayout->addWidget(searchEdit);
        layout->addLayout(searchLayout);

        QListWidget *list = new QListWidget();
        list->setIconSize(QSize(32, 32));
        list->setSelectionMode(QAbstractItemView::ExtendedSelection);
        list->setDragDropMode(QAbstractItemView::InternalMove);
        list->setContextMenuPolicy(Qt::CustomContextMenu);
        list->setAlternatingRowColors(true);
        list->setSpacing(2);

        QWidget *emptyStateWidget = new QWidget();
        QVBoxLayout *emptyLayout = new QVBoxLayout(emptyStateWidget);
        emptyLayout->setAlignment(Qt::AlignCenter);

        QLabel *emptyIcon = new QLabel();
        emptyIcon->setPixmap(QIcon::fromTheme("bookmark-new").pixmap(96, 96, QIcon::Disabled));
        emptyIcon->setAlignment(Qt::AlignCenter);

        QLabel *emptyText = new QLabel("No favorites yet");
        emptyText->setAlignment(Qt::AlignCenter);
        emptyText->setStyleSheet("font-size: 16px; font-weight: bold; color: gray;");

        QLabel *emptyHint = new QLabel("Click 'Add New' to create your first favorite\nor drag & drop from the toolbar");
        emptyHint->setAlignment(Qt::AlignCenter);
        emptyHint->setStyleSheet("color: gray;");
        emptyHint->setWordWrap(true);

        emptyLayout->addStretch();
        emptyLayout->addWidget(emptyIcon);
        emptyLayout->addSpacing(10);
        emptyLayout->addWidget(emptyText);
        emptyLayout->addSpacing(5);
        emptyLayout->addWidget(emptyHint);
        emptyLayout->addStretch();
        emptyStateWidget->hide();

        QStackedWidget *stackedWidget = new QStackedWidget();
        stackedWidget->addWidget(list);
        stackedWidget->addWidget(emptyStateWidget);
        layout->addWidget(stackedWidget);

        auto updateStats = [statsLabel](int total, int filtered) {
            if (filtered < total) {
                statsLabel->setText(QString("Showing %1 of %2 favorites").arg(filtered).arg(total));
            } else {
                statsLabel->setText(QString("%1 favorite%2").arg(total).arg(total == 1 ? "" : "s"));
            }
        };

        auto populateList = [list, &favorites, stackedWidget, emptyStateWidget, updateStats](const QString &filter = "") {
            list->clear();
            int totalCount = favorites.size();
            int visibleCount = 0;

            for (const QVariant &favVar : favorites) {
                QVariantMap fav = favVar.toMap();
                QString title = fav["title"].toString();
                QString url = fav["url"].toString();

                if (!filter.isEmpty() &&
                    !title.contains(filter, Qt::CaseInsensitive) &&
                    !url.contains(filter, Qt::CaseInsensitive)) {
                    continue;
                }

                visibleCount++;
                QListWidgetItem *item = new QListWidgetItem(list);
                item->setText(title);
                item->setToolTip(QString("%1\n%2").arg(title, url));
                item->setData(Qt::UserRole, url);
                item->setData(Qt::UserRole + 1, fav["iconPath"].toString());
                item->setIcon(Icon_Man::instance().loadIcon(fav["iconPath"].toString()));
            }

            if (totalCount == 0) {
                stackedWidget->setCurrentWidget(emptyStateWidget);
            } else {
                stackedWidget->setCurrentWidget(list);
            }

            updateStats(totalCount, visibleCount);
        };

        populateList();

        connect(searchEdit, &QLineEdit::textChanged, [populateList](const QString &text) {
            populateList(text);
        });

        connect(list, &QListWidget::customContextMenuRequested, [list, &favorites, populateList, parent](const QPoint &pos) {
            QListWidgetItem *item = list->itemAt(pos);
            if (!item) return;

            QMenu menu(parent);

            menu.addAction(QIcon::fromTheme("window-new"), "Open in New Tab", [item]() {

                QUrl url(item->data(Qt::UserRole).toString());
                if (url.isValid()) {
                     Browser_Backend::instance().openInNewTab(url);
                }
            });

            menu.addAction(QIcon::fromTheme("edit-copy"), "Copy URL", [item]() {
                QApplication::clipboard()->setText(item->data(Qt::UserRole).toString());
            });

            menu.addSeparator();

            menu.addAction(QIcon::fromTheme("document-edit"), "Edit", [list, &favorites, populateList, parent, item]() {
                QString currentTitle = item->text();
                QString currentUrl = item->data(Qt::UserRole).toString();
                QString currentIconPath = item->data(Qt::UserRole + 1).toString();

                showFavoriteDialog(parent, favorites, populateList, currentTitle, currentUrl, currentIconPath, true);
            });

            menu.addAction(QIcon::fromTheme("edit-delete"), "Delete", [list, &favorites, populateList, item]() {
                QString url = item->data(Qt::UserRole).toString();
                for (int i = favorites.size() - 1; i >= 0; --i) {
                    if (favorites[i].toMap()["url"].toString() == url) {
                        favorites.removeAt(i);
                        break;
                    }
                }
                delete item;
                populateList();
            });

            menu.exec(list->mapToGlobal(pos));
        });

        connect(list, &QListWidget::itemDoubleClicked, [&favorites, populateList, parent](QListWidgetItem *item) {
            QString currentTitle = item->text();
            QString currentUrl = item->data(Qt::UserRole).toString();
            QString currentIconPath = item->data(Qt::UserRole + 1).toString();

            showFavoriteDialog(parent, favorites, populateList, currentTitle, currentUrl, currentIconPath, true);
        });

        QHBoxLayout *btnLayout = new QHBoxLayout();

        QPushButton *addBtn = new QPushButton("Add New");
        addBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        addBtn->setMinimumHeight(32);

        QPushButton *editBtn = new QPushButton("Edit");
        editBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties));
        editBtn->setMinimumHeight(32);
        editBtn->setEnabled(false);

        QPushButton *removeBtn = new QPushButton("Remove");
        removeBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
        removeBtn->setMinimumHeight(32);
        removeBtn->setEnabled(false);

        QPushButton *clearAllBtn = new QPushButton("Clear All");
        clearAllBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
        clearAllBtn->setMinimumHeight(32);
        clearAllBtn->setEnabled(!favorites.isEmpty());

        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(editBtn);
        btnLayout->addWidget(removeBtn);
        btnLayout->addWidget(clearAllBtn);
        btnLayout->addStretch();

        QPushButton *saveBtn = new QPushButton("Save");
        saveBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave));
        saveBtn->setMinimumHeight(32);
        saveBtn->setDefault(true);

        QPushButton *cancelBtn = new QPushButton("Cancel");
        cancelBtn->setIcon(QIcon::fromTheme("dialog-cancel"));
        cancelBtn->setMinimumHeight(32);

        btnLayout->addWidget(saveBtn);
        btnLayout->addWidget(cancelBtn);
        layout->addLayout(btnLayout);

        connect(list, &QListWidget::itemSelectionChanged, [list, editBtn, removeBtn]() {
            bool hasSelection = !list->selectedItems().isEmpty();
            editBtn->setEnabled(hasSelection && list->selectedItems().count() == 1);
            removeBtn->setEnabled(hasSelection);
        });

        connect(addBtn, &QPushButton::clicked, [&favorites, populateList, parent]() {
            showFavoriteDialog(parent, favorites, populateList, "", "", "", false);
        });

        connect(editBtn, &QPushButton::clicked, [list, &favorites, populateList, parent]() {
            auto selected = list->selectedItems();
            if (selected.isEmpty()) return;

            QListWidgetItem *item = selected.first();
            QString currentTitle = item->text();
            QString currentUrl = item->data(Qt::UserRole).toString();
            QString currentIconPath = item->data(Qt::UserRole + 1).toString();

            showFavoriteDialog(parent, favorites, populateList, currentTitle, currentUrl, currentIconPath, true);
        });

        connect(removeBtn, &QPushButton::clicked, [list, &favorites, populateList, clearAllBtn]() {
            auto selected = list->selectedItems();
            if (selected.isEmpty()) return;

            QString message = selected.count() == 1
                ? QString("Delete '%1'?").arg(selected.first()->text())
                : QString("Delete %1 favorites?").arg(selected.count());

            if (QMessageBox::question(list, "Confirm Delete", message,
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                return;
            }

            QStringList urlsToRemove;
            for (auto *item : selected) {
                urlsToRemove.append(item->data(Qt::UserRole).toString());
            }

            for (const QString &url : urlsToRemove) {
                for (int i = favorites.size() - 1; i >= 0; --i) {
                    if (favorites[i].toMap()["url"].toString() == url) {
                        favorites.removeAt(i);
                    }
                }
            }
            populateList();
            clearAllBtn->setEnabled(!favorites.isEmpty());
        });

        connect(clearAllBtn, &QPushButton::clicked, [&favorites, populateList, clearAllBtn, parent]() {
            if (QMessageBox::warning(parent, "Clear All Favorites",
                                    QString("Delete all %1 favorites?\n\nThis action cannot be undone.").arg(favorites.size()),
                                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                favorites.clear();
                populateList();
                clearAllBtn->setEnabled(false);
            }
        });

        connect(saveBtn, &QPushButton::clicked, dialog.get(), &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, dialog.get(), &QDialog::reject);

        if (dialog->exec() == QDialog::Accepted) {

            QVariantList reordered;
            for (int i = 0; i < list->count(); ++i) {
                QListWidgetItem *item = list->item(i);
                QString url = item->data(Qt::UserRole).toString();
                QString iconPath = item->data(Qt::UserRole + 1).toString();
                reordered.append(QVariantMap{
                    {"title", item->text()},
                    {"url", url},
                    {"iconPath", iconPath}
                });
            }
            favorites = reordered;
            Settings_Backend::instance().saveFavorites(favorites);
            if (refreshCallback) refreshCallback();
        }
    }
    static void history(QWidget *parent, const QVariantList &recentTabs, std::function<void()> clearCallback) {
        auto dialog = std::make_unique<QDialog>(parent);
        dialog->setWindowTitle("Browsing History");
        dialog->setWindowIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpenRecent));
        dialog->resize(900, 700);
        dialog->setMinimumSize(700, 500);

        QVBoxLayout *layout = new QVBoxLayout(dialog.get());
        layout->setSpacing(10);
        layout->setContentsMargins(10, 10, 10, 10);

        QHBoxLayout *headerLayout = new QHBoxLayout();
        QLabel *titleLabel = new QLabel("Browsing History");
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(14);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        headerLayout->addWidget(titleLabel);

        QLabel *statsLabel = new QLabel();
        statsLabel->setStyleSheet("color: gray; padding: 5px;");
        headerLayout->addWidget(statsLabel);
        headerLayout->addStretch();
        layout->addLayout(headerLayout);

        QHBoxLayout *filterLayout = new QHBoxLayout();

        QLineEdit *searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Search history by title or URL...");
        searchEdit->setClearButtonEnabled(true);
        searchEdit->setMinimumHeight(32);

        QComboBox *sortCombo = new QComboBox();
        sortCombo->addItem(QIcon::fromTheme("view-sort-descending"), "Recent First");
        sortCombo->addItem(QIcon::fromTheme("view-sort-ascending"), "Oldest First");
        sortCombo->addItem(QIcon::fromTheme("view-sort-ascending"), "A-Z");
        sortCombo->addItem(QIcon::fromTheme("view-sort-descending"), "Z-A");
        sortCombo->setMinimumWidth(150);

        filterLayout->addWidget(new QLabel("Search:"));
        filterLayout->addWidget(searchEdit, 1);
        filterLayout->addWidget(new QLabel("Sort:"));
        filterLayout->addWidget(sortCombo);
        layout->addLayout(filterLayout);

        QTreeWidget *tree = new QTreeWidget();
        tree->setHeaderLabels({"Title", "URL", "Time"});
        tree->setColumnWidth(0, 350);
        tree->setColumnWidth(1, 400);
        tree->setIconSize(QSize(24, 24));
        tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tree->setContextMenuPolicy(Qt::CustomContextMenu);
        tree->setAlternatingRowColors(true);
        tree->setRootIsDecorated(true);
        tree->setUniformRowHeights(true);

        QWidget *emptyStateWidget = new QWidget();
        QVBoxLayout *emptyLayout = new QVBoxLayout(emptyStateWidget);
        emptyLayout->setAlignment(Qt::AlignCenter);

        QLabel *emptyIcon = new QLabel();
        emptyIcon->setPixmap(QIcon::fromTheme("document-open-recent").pixmap(96, 96, QIcon::Disabled));
        emptyIcon->setAlignment(Qt::AlignCenter);

        QLabel *emptyText = new QLabel("No browsing history");
        emptyText->setAlignment(Qt::AlignCenter);
        emptyText->setStyleSheet("font-size: 16px; font-weight: bold; color: gray;");

        QLabel *emptyHint = new QLabel("Your browsing history will appear here");
        emptyHint->setAlignment(Qt::AlignCenter);
        emptyHint->setStyleSheet("color: gray;");

        emptyLayout->addStretch();
        emptyLayout->addWidget(emptyIcon);
        emptyLayout->addSpacing(10);
        emptyLayout->addWidget(emptyText);
        emptyLayout->addSpacing(5);
        emptyLayout->addWidget(emptyHint);
        emptyLayout->addStretch();
        emptyStateWidget->hide();

        QStackedWidget *stackedWidget = new QStackedWidget();
        stackedWidget->addWidget(tree);
        stackedWidget->addWidget(emptyStateWidget);
        layout->addWidget(stackedWidget);


        QVariantList localHistory = recentTabs;

        auto populateTree = [tree, &localHistory, stackedWidget, emptyStateWidget, statsLabel]
                           (const QString &filter = "", int sortMode = 0) {
            tree->clear();

            if (localHistory.isEmpty()) {
                stackedWidget->setCurrentWidget(emptyStateWidget);
                statsLabel->setText("0 items");
                return;
            }

            stackedWidget->setCurrentWidget(tree);

            QVariantList filtered = localHistory;

            if (!filter.isEmpty()) {
                QVariantList temp;
                for (const QVariant &item : filtered) {
                    QVariantMap entry = item.toMap();
                    QString title = entry["title"].toString();
                    QString url = entry["url"].toString();
                    if (title.contains(filter, Qt::CaseInsensitive) ||
                        url.contains(filter, Qt::CaseInsensitive)) {
                        temp.append(item);
                    }
                }
                filtered = temp;
            }

            std::sort(filtered.begin(), filtered.end(), [sortMode](const QVariant &a, const QVariant &b) {
                QVariantMap mapA = a.toMap();
                QVariantMap mapB = b.toMap();
                switch (sortMode) {
                case 0: return mapA["time"].toLongLong() > mapB["time"].toLongLong();
                case 1: return mapA["time"].toLongLong() < mapB["time"].toLongLong();
                case 2: return mapA["title"].toString() < mapB["title"].toString();
                case 3: return mapA["title"].toString() > mapB["title"].toString();
                default: return false;
                }
            });

            QMap<QString, QTreeWidgetItem*> dateGroups;
            QDateTime now = QDateTime::currentDateTime();

            for (const QVariant &item : filtered) {
                QVariantMap entry = item.toMap();
                qint64 timestamp = entry["time"].toLongLong();
                QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(timestamp);

                QString dateGroup;
                int daysAgo = dateTime.daysTo(now);
                if (daysAgo == 0) dateGroup = "Today";
                else if (daysAgo == 1) dateGroup = "Yesterday";
                else if (daysAgo < 7) dateGroup = "Last 7 Days";
                else if (daysAgo < 30) dateGroup = "Last Month";
                else dateGroup = "Older";

                if (!dateGroups.contains(dateGroup)) {
                    QTreeWidgetItem *groupItem = new QTreeWidgetItem(tree);
                    groupItem->setText(0, dateGroup);
                    groupItem->setIcon(0, QIcon::fromTheme("folder"));
                    groupItem->setExpanded(true);
                    QFont font = groupItem->font(0);
                    font.setBold(true);
                    groupItem->setFont(0, font);
                    groupItem->setForeground(0, QBrush(Qt::gray));
                    dateGroups[dateGroup] = groupItem;
                }

                QTreeWidgetItem *entryItem = new QTreeWidgetItem(dateGroups[dateGroup]);
                entryItem->setText(0, entry["title"].toString());
                entryItem->setText(1, entry["url"].toString());
                entryItem->setText(2, dateTime.toString("hh:mm:ss"));
                entryItem->setToolTip(0, entry["title"].toString());
                entryItem->setToolTip(1, entry["url"].toString());
                entryItem->setToolTip(2, dateTime.toString("dddd, MMMM d, yyyy h:mm:ss AP"));
                entryItem->setData(0, Qt::UserRole, entry["url"].toString());
                entryItem->setIcon(0, Icon_Man::instance().loadIcon(entry["iconPath"].toString()));
            }

            if (filter.isEmpty()) {
                statsLabel->setText(QString("%1 item%2").arg(localHistory.size()).arg(localHistory.size() == 1 ? "" : "s"));
            } else {
                statsLabel->setText(QString("Showing %1 of %2 items").arg(filtered.size()).arg(localHistory.size()));
            }
        };

        populateTree();

        connect(searchEdit, &QLineEdit::textChanged, [populateTree, sortCombo](const QString &text) {
            populateTree(text, sortCombo->currentIndex());
        });

        connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [populateTree, searchEdit](int index) {
                    populateTree(searchEdit->text(), index);
                });

        connect(tree, &QTreeWidget::customContextMenuRequested,
                [tree, parent, &localHistory, populateTree, searchEdit, sortCombo](const QPoint &pos) {
            QTreeWidgetItem *item = tree->itemAt(pos);
            if (!item || item->childCount() > 0) return;

            QMenu menu(parent);

            menu.addAction(QIcon::fromTheme("window-new"), "Open in New Tab", [item]() {
                QString url = item->data(0, Qt::UserRole).toString();
                 Browser_Backend::instance().openInNewTab(url);
            });

            menu.addAction(QIcon::fromTheme("edit-copy"), "Copy URL", [item]() {
                QString url = item->data(0, Qt::UserRole).toString();
                QApplication::clipboard()->setText(url);
            });

            menu.addAction(QIcon::fromTheme("edit-copy"), "Copy Title", [item]() {
                QApplication::clipboard()->setText(item->text(0));
            });

            menu.addSeparator();

            menu.addAction(QIcon::fromTheme("bookmark-new"), "Add to Favorites", [item]() {
                QString url = item->data(0, Qt::UserRole).toString();
                QString title = item->text(0);


                auto favs = Settings_Backend::instance().loadFavorites();
                QVariantMap entry;
                entry["url"] = url;
                entry["title"] = title;
                entry["iconPath"] = "";
                favs.prepend(entry);
                Settings_Backend::instance().saveFavorites(favs);
            });

            menu.addSeparator();

            menu.addAction(QIcon::fromTheme("edit-delete"), "Remove from History",
                          [item, &localHistory, populateTree, searchEdit, sortCombo]() {
                QString url = item->data(0, Qt::UserRole).toString();


                for (int i = localHistory.size() - 1; i >= 0; --i) {
                    if (localHistory[i].toMap()["url"].toString() == url) {
                        localHistory.removeAt(i);
                        break;
                    }
                }


                Settings_Backend::instance().saveHistory(localHistory);


                populateTree(searchEdit->text(), sortCombo->currentIndex());
            });

            menu.exec(tree->mapToGlobal(pos));
        });

        connect(tree, &QTreeWidget::itemDoubleClicked, [](QTreeWidgetItem *item, int) {
            if (item->childCount() > 0) return;

            QString url = item->data(0, Qt::UserRole).toString();
             Browser_Backend::instance().openInNewTab(url);
        });

        QHBoxLayout *btnLayout = new QHBoxLayout();

        QPushButton *removeBtn = new QPushButton("Remove Selected");
        removeBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
        removeBtn->setMinimumHeight(32);
        removeBtn->setEnabled(false);

        QPushButton *clearAllBtn = new QPushButton("Clear All");
        clearAllBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
        clearAllBtn->setMinimumHeight(32);
        clearAllBtn->setEnabled(!recentTabs.isEmpty());

        btnLayout->addWidget(removeBtn);
        btnLayout->addWidget(clearAllBtn);
        btnLayout->addStretch();

        QPushButton *closeBtn = new QPushButton("Close");
        closeBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::WindowClose));
        closeBtn->setMinimumHeight(32);
        closeBtn->setDefault(true);

        btnLayout->addWidget(closeBtn);
        layout->addLayout(btnLayout);

        connect(tree, &QTreeWidget::itemSelectionChanged, [tree, removeBtn]() {
            QList<QTreeWidgetItem*> selected = tree->selectedItems();
            bool hasNonGroupItems = false;
            for (QTreeWidgetItem *item : selected) {
                if (item->childCount() == 0) {
                    hasNonGroupItems = true;
                    break;
                }
            }
            removeBtn->setEnabled(hasNonGroupItems);
        });

        connect(removeBtn, &QPushButton::clicked,
               [tree, &localHistory, populateTree, searchEdit, sortCombo]() {
            QList<QTreeWidgetItem*> selected = tree->selectedItems();
            QStringList urlsToRemove;

            for (QTreeWidgetItem *item : selected) {
                if (item->childCount() == 0) {
                    urlsToRemove.append(item->data(0, Qt::UserRole).toString());
                }
            }

            if (urlsToRemove.isEmpty()) return;

            QString message = urlsToRemove.size() == 1
                ? "Delete this history entry?"
                : QString("Delete %1 history entries?").arg(urlsToRemove.size());

            if (QMessageBox::question(tree, "Confirm Delete", message,
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

                for (const QString &url : urlsToRemove) {
                    for (int i = localHistory.size() - 1; i >= 0; --i) {
                        if (localHistory[i].toMap()["url"].toString() == url) {
                            localHistory.removeAt(i);
                            break;
                        }
                    }
                }

                Settings_Backend::instance().saveHistory(localHistory);
                populateTree(searchEdit->text(), sortCombo->currentIndex());
            }
        });

        connect(clearAllBtn, &QPushButton::clicked,
               [parent, clearCallback, dialog = dialog.get()]() {
            if (QMessageBox::warning(parent,
                                    "Clear All History",
                                    "Delete all browsing history?\n\nThis action cannot be undone.",
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No) == QMessageBox::Yes) {
                if (clearCallback) clearCallback();
                dialog->accept();
            }
        });

        connect(closeBtn, &QPushButton::clicked, dialog.get(), &QDialog::accept);

        dialog->exec();
    }

    static void source(QWidget *parent, const QString &html, const QString &title) {
        auto dialog = std::make_unique<QDialog>(parent);
        dialog->setWindowTitle("Page Source - " + title);
        dialog->setWindowIcon(QIcon::fromTheme("text-html"));
        dialog->resize(1000, 700);
        dialog->setMinimumSize(700, 500);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        QVBoxLayout *layout = new QVBoxLayout(dialog.get());
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QToolBar *toolbar = new QToolBar();
        toolbar->setMovable(false);
        toolbar->setIconSize(QSize(16, 16));

        QAction *wrapAction = toolbar->addAction(QIcon::fromTheme("format-text-wrap"), "Wrap Lines");
        wrapAction->setCheckable(true);
        wrapAction->setChecked(false);

        toolbar->addSeparator();

        QAction *copyAllAction = toolbar->addAction(QIcon::fromTheme("edit-copy"), "Copy All");
        QAction *saveAction = toolbar->addAction(QIcon::fromTheme("document-save"), "Save As...");

        toolbar->addSeparator();

        QLineEdit *searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Find in source...");
        searchEdit->setClearButtonEnabled(true);
        searchEdit->setMaximumWidth(250);
        toolbar->addWidget(searchEdit);

        QAction *findNextAction = toolbar->addAction(QIcon::fromTheme("go-down"), "Next");
        QAction *findPrevAction = toolbar->addAction(QIcon::fromTheme("go-up"), "Previous");

        layout->addWidget(toolbar);

        QPlainTextEdit *sourceEdit = new QPlainTextEdit();
        sourceEdit->setPlainText(html);
        sourceEdit->setReadOnly(true);
        sourceEdit->setFont(QFont("Monospace", 10));
        sourceEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        sourceEdit->document()->setMaximumBlockCount(0);
        layout->addWidget(sourceEdit);

        QStatusBar *statusBar = new QStatusBar();
        QLabel *linesLabel = new QLabel();
        int lineCount = html.count('\n') + 1;
        int charCount = html.length();
        linesLabel->setText(QString("%1 lines, %2 characters").arg(lineCount).arg(charCount));
        statusBar->addPermanentWidget(linesLabel);
        layout->addWidget(statusBar);

        connect(wrapAction, &QAction::toggled, [sourceEdit](bool checked) {
            sourceEdit->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
        });

        connect(copyAllAction, &QAction::triggered, [sourceEdit]() {
            sourceEdit->selectAll();
            sourceEdit->copy();
            sourceEdit->moveCursor(QTextCursor::Start);
        });

        connect(saveAction, &QAction::triggered, [parent, sourceEdit, title]() {
            QString fileName = QFileDialog::getSaveFileName(parent,
                                                           "Save Source",
                                                           title + "_source.html",
                                                           "HTML Files (*.html);;All Files (*)");
            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.write(sourceEdit->toPlainText().toUtf8());
                    file.close();
                }
            }
        });

        connect(searchEdit, &QLineEdit::returnPressed, findNextAction, &QAction::trigger);

        connect(findNextAction, &QAction::triggered, [sourceEdit, searchEdit]() {
            if (!searchEdit->text().isEmpty()) {
                sourceEdit->find(searchEdit->text());
            }
        });

        connect(findPrevAction, &QAction::triggered, [sourceEdit, searchEdit]() {
            if (!searchEdit->text().isEmpty()) {
                sourceEdit->find(searchEdit->text(), QTextDocument::FindBackward);
            }
        });

        dialog->show();
        dialog.release();
    }
    static void inspect(QWebEnginePage *page, const QString &title) {
        auto devTools = new QWebEngineView();
        devTools->setWindowTitle("Inspect: " + title);
        devTools->setWindowIcon(QIcon::fromTheme("tools-report-bug"));
        devTools->setMinimumSize(800, 600);
        page->setDevToolsPage(devTools->page());
        devTools->setAttribute(Qt::WA_DeleteOnClose);
        devTools->resize(1200, 800);
        devTools->show();
    }
    static void keybinds(QWidget *parent, std::function<void()> onClosed = nullptr) {
        auto dialog = std::make_unique<QDialog>(parent);
        dialog->setWindowTitle("Keybinds Editor");
        dialog->setWindowIcon(dialog->style()->standardIcon(QStyle::SP_ComputerIcon));
        dialog->resize(750, 600);
        dialog->setMinimumSize(650, 450);
        dialog->setModal(true);
        dialog->setWindowModality(Qt::ApplicationModal);

        QVBoxLayout *mainLayout = new QVBoxLayout(dialog.get());
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(10, 10, 10, 10);

        QLineEdit *searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Search actions...");
        searchEdit->setClearButtonEnabled(true);
        searchEdit->setMinimumHeight(30);
        mainLayout->addWidget(searchEdit);

        QListWidget *listWidget = new QListWidget();
        listWidget->setSelectionMode(QAbstractItemView::NoSelection);
        listWidget->setSpacing(1);
        mainLayout->addWidget(listWidget);

        auto &s = Settings_Backend::instance();
        QMap<QString, QString> defaults = s.defaultKeybinds();
        QMap<QString, QString> current = s.getAllKeybinds();

        QList<QPushButton*> keyButtons;
        QList<QPushButton*> resetButtons;

        for (auto it = defaults.begin(); it != defaults.end(); ++it) {
            QString action = it.key();
            QString defaultKey = it.value();
            QString currentKey = current.contains(action) ? current[action] : defaultKey;

            QListWidgetItem *item = new QListWidgetItem(listWidget);
            item->setSizeHint(QSize(0, 40));

            QWidget *row = new QWidget();
            QHBoxLayout *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(10, 5, 10, 5);
            rowLayout->setSpacing(15);

            QLabel *actionLabel = new QLabel(action);
            actionLabel->setMinimumWidth(180);
            actionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            rowLayout->addWidget(actionLabel);

            rowLayout->addStretch();

            QPushButton *resetBtn = new QPushButton();
            resetBtn->setIcon(resetBtn->style()->standardIcon(QStyle::SP_BrowserReload));
            resetBtn->setFixedSize(28, 28);
            resetBtn->setVisible(currentKey != defaultKey);
            resetBtn->setProperty("action", action);
            resetBtn->setProperty("defaultKey", defaultKey);
            resetBtn->setToolTip("Reset to default");
            rowLayout->addWidget(resetBtn);
            resetButtons.append(resetBtn);

            QPushButton *keyBtn = new QPushButton(currentKey);
            keyBtn->setCheckable(true);
            keyBtn->setProperty("action", action);
            keyBtn->setProperty("defaultKey", defaultKey);
            keyBtn->setMinimumWidth(130);
            keyBtn->setMaximumWidth(150);
            keyBtn->setMinimumHeight(28);
            keyBtn->setToolTip("Click to change keybind");
            rowLayout->addWidget(keyBtn);
            keyButtons.append(keyBtn);

            listWidget->setItemWidget(item, row);
        }

        listWidget->setMinimumHeight(400);

        QObject::connect(searchEdit, &QLineEdit::textChanged, [listWidget](const QString &text) {
            for (int i = 0; i < listWidget->count(); ++i) {
                QListWidgetItem *item = listWidget->item(i);
                QWidget *row = listWidget->itemWidget(item);
                if (row) {
                    QLabel *label = row->findChild<QLabel*>();
                    if (label) {
                        bool hide = !text.isEmpty() && !label->text().contains(text, Qt::CaseInsensitive);
                        item->setHidden(hide);
                    }
                }
            }
        });

        for (QPushButton *keyBtn : keyButtons) {
            QObject::connect(keyBtn, &QPushButton::toggled, [keyBtn](bool checked) {
                if (checked) {
                    keyBtn->setText("...");
                    keyBtn->grabKeyboard();
                }
            });

            class KeyFilter : public QObject {
            public:
                KeyFilter(QPushButton *btn) : m_btn(btn) {}
            protected:
                bool eventFilter(QObject *obj, QEvent *event) override {
                    if (event->type() == QEvent::KeyPress && m_btn->isChecked()) {
                        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

                        int key = keyEvent->key();
                        Qt::KeyboardModifiers mods = keyEvent->modifiers();

                        if (key == Qt::Key_Control || key == Qt::Key_Shift ||
                            key == Qt::Key_Alt || key == Qt::Key_Meta) {
                            return true;
                        }

                        QStringList parts;
                        if (mods & Qt::ControlModifier) parts << "Ctrl";
                        if (mods & Qt::ShiftModifier) parts << "Shift";
                        if (mods & Qt::AltModifier) parts << "Alt";
                        if (mods & Qt::MetaModifier) parts << "Meta";

                        QString keyName = QKeySequence(key).toString();
                        if (!keyName.isEmpty()) {
                            parts << keyName;
                            QString newKey = parts.join("+");

                            if (newKey.isEmpty() || newKey == "+") {
                                QString defaultKey = m_btn->property("defaultKey").toString();
                                m_btn->setText(defaultKey);
                            } else {
                                m_btn->setText(newKey);
                            }

                            m_btn->setChecked(false);
                            m_btn->releaseKeyboard();
                        }
                        return true;
                    }
                    return false;
                }
            private:
                QPushButton *m_btn;
            };

            keyBtn->installEventFilter(new KeyFilter(keyBtn));
        }

        for (QPushButton *resetBtn : resetButtons) {
            QObject::connect(resetBtn, &QPushButton::clicked, [resetBtn, &keyButtons]() {
                QString action = resetBtn->property("action").toString();
                QString defaultKey = resetBtn->property("defaultKey").toString();

                for (QPushButton *btn : keyButtons) {
                    if (btn->property("action").toString() == action) {
                        btn->setText(defaultKey);
                        break;
                    }
                }
                resetBtn->setVisible(false);
            });
        }

        auto findConflicts = [&]() -> QMap<QString, QStringList> {
            QMap<QString, QStringList> conflicts;
            QMap<QString, QString> keybindMap;

            for (QPushButton *btn : keyButtons) {
                QString action = btn->property("action").toString();
                QString key = btn->text();
                keybindMap[action] = key;
            }

            for (auto it1 = keybindMap.begin(); it1 != keybindMap.end(); ++it1) {
                for (auto it2 = keybindMap.begin(); it2 != keybindMap.end(); ++it2) {
                    if (it1.key() != it2.key() && it1.value() == it2.value() && !it1.value().isEmpty()) {
                        if (!conflicts.contains(it1.value())) {
                            conflicts[it1.value()] = QStringList();
                        }
                        if (!conflicts[it1.value()].contains(it1.key())) {
                            conflicts[it1.value()].append(it1.key());
                        }
                        if (!conflicts[it1.value()].contains(it2.key())) {
                            conflicts[it1.value()].append(it2.key());
                        }
                    }
                }
            }
            return conflicts;
        };

        auto resetAllToDefault = [&]() {
            for (QPushButton *btn : keyButtons) {
                QString defaultKey = btn->property("defaultKey").toString();
                btn->setText(defaultKey);
            }
            for (QPushButton *btn : resetButtons) {
                btn->setVisible(false);
            }
        };

        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->setContentsMargins(0, 10, 0, 0);

        QPushButton *saveBtn = new QPushButton("Save");
        saveBtn->setIcon(saveBtn->style()->standardIcon(QStyle::SP_DialogApplyButton));
        saveBtn->setMinimumHeight(30);
        QObject::connect(saveBtn, &QPushButton::clicked, [&, dialog = dialog.get()]() {
            QMap<QString, QStringList> conflicts = findConflicts();

            if (!conflicts.isEmpty()) {
                QString message = "The following key conflicts were detected:\n\n";
                for (auto it = conflicts.begin(); it != conflicts.end(); ++it) {
                    message += "  " + it.key() + ": " + it.value().join(", ") + "\n";
                }
                message += "\nWhat would you like to do?";

                QMessageBox conflictBox(dialog);
                conflictBox.setWindowTitle("Keybind Conflicts");
                conflictBox.setText(message);
                conflictBox.setIcon(QMessageBox::Warning);

                QPushButton *restoreBtn = conflictBox.addButton("Restore All to Default", QMessageBox::ActionRole);
                QPushButton *cancelBtn = conflictBox.addButton("Cancel", QMessageBox::RejectRole);

                conflictBox.exec();

                if (conflictBox.clickedButton() == restoreBtn) {
                    resetAllToDefault();
                    return;

                } else if (conflictBox.clickedButton() == cancelBtn) {
                    return;

                }
            }

            for (QPushButton *btn : keyButtons) {
                QString action = btn->property("action").toString();
                QString key = btn->text();
                s.setKeybind(action, key);
            }

            dialog->accept();
        });
        btnLayout->addWidget(saveBtn);

        btnLayout->addStretch();

        QPushButton *cancelDialogBtn = new QPushButton("Cancel");
        cancelDialogBtn->setIcon(cancelDialogBtn->style()->standardIcon(QStyle::SP_DialogCancelButton));
        cancelDialogBtn->setMinimumHeight(30);
        QObject::connect(cancelDialogBtn, &QPushButton::clicked, dialog.get(), &QDialog::reject);
        btnLayout->addWidget(cancelDialogBtn);

        mainLayout->addLayout(btnLayout);

        dialog->exec();

        if (onClosed) {
            onClosed();
        }
    }
    static void ExtMan(QWidget* parent, std::function<void()> refreshCallback = nullptr) {
        auto dialog = std::make_unique<QDialog>(parent);
        dialog->setWindowTitle("Extension Manager");
        dialog->setWindowIcon(QIcon::fromTheme("application-x-addon"));
        dialog->resize(900, 650);
        dialog->setModal(true);

        QVBoxLayout* mainLayout = new QVBoxLayout(dialog.get());

        QHBoxLayout* headerLayout = new QHBoxLayout();
        QLabel* header = new QLabel("Extension Manager");
        QFont hFont = header->font();
        hFont.setPointSize(14);
        hFont.setBold(true);
        header->setFont(hFont);
        headerLayout->addWidget(header);

        QLabel* statsLabel = new QLabel();
        statsLabel->setStyleSheet("color: gray; padding: 5px;");
        headerLayout->addWidget(statsLabel);
        headerLayout->addStretch();
        mainLayout->addLayout(headerLayout);

        QHBoxLayout* searchLayout = new QHBoxLayout();
        QLabel* searchLabel = new QLabel("Search:");
        QLineEdit* searchBox = new QLineEdit();
        searchBox->setPlaceholderText("Filter extensions by name, maintainer, or description...");
        searchBox->setClearButtonEnabled(true);

        QComboBox* filterCombo = new QComboBox();
        filterCombo->addItems({"All", "Enabled", "Disabled", "Unspecified"});

        searchLayout->addWidget(searchLabel);
        searchLayout->addWidget(searchBox, 1);
        searchLayout->addWidget(filterCombo);
        mainLayout->addLayout(searchLayout);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setSpacing(10);

        QListWidget* listWidget = new QListWidget();
        listWidget->setIconSize(QSize(32, 32));
        listWidget->setMinimumWidth(250);
        listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

        QVBoxLayout* detailsLayout = new QVBoxLayout();
        detailsLayout->setSpacing(8);

        QLabel* iconPreview = new QLabel();
        iconPreview->setFixedSize(128, 128);
        iconPreview->setAlignment(Qt::AlignCenter);
        iconPreview->setStyleSheet("background-color: transparent;");
        detailsLayout->addWidget(iconPreview, 0, Qt::AlignCenter);

        QLabel* nameLabel = new QLabel("Select an extension");
        nameLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
        nameLabel->setAlignment(Qt::AlignCenter);
        detailsLayout->addWidget(nameLabel);

        QLabel* maintainerLabel = new QLabel();
        maintainerLabel->setAlignment(Qt::AlignCenter);
        detailsLayout->addWidget(maintainerLabel);

        QLabel* versionLabel = new QLabel();
        versionLabel->setAlignment(Qt::AlignCenter);
        detailsLayout->addWidget(versionLabel);

        QLabel* fileLabel = new QLabel();
        fileLabel->setAlignment(Qt::AlignCenter);
        fileLabel->setStyleSheet("color: gray; font-size: 10px;");
        fileLabel->setWordWrap(true);
        detailsLayout->addWidget(fileLabel);

        detailsLayout->addSpacing(10);

        QLabel* detailsTitle = new QLabel("Description:");
        detailsTitle->setStyleSheet("font-weight: bold;");
        detailsLayout->addWidget(detailsTitle);

        QTextEdit* descEdit = new QTextEdit();
        descEdit->setReadOnly(true);
        descEdit->setMinimumHeight(120);
        detailsLayout->addWidget(descEdit);

        QHBoxLayout* radioLayout = new QHBoxLayout();
        QRadioButton* unsureRadio = new QRadioButton("Unspecified");
        QRadioButton* disableRadio = new QRadioButton("Disable");
        QRadioButton* enableRadio = new QRadioButton("Enable");

        unsureRadio->setToolTip("Leave extension state undecided");
        disableRadio->setToolTip("Disable this extension");
        enableRadio->setToolTip("Enable this extension");

        radioLayout->addWidget(unsureRadio);
        radioLayout->addWidget(disableRadio);
        radioLayout->addWidget(enableRadio);
        radioLayout->addStretch();
        detailsLayout->addLayout(radioLayout);
        detailsLayout->addStretch();

        QWidget* detailsWidget = new QWidget();
        detailsWidget->setLayout(detailsLayout);
        detailsWidget->setMinimumWidth(300);
        detailsWidget->setMaximumWidth(400);

        hLayout->addWidget(listWidget);
        hLayout->addWidget(detailsWidget);
        mainLayout->addLayout(hLayout, 1);

        QHBoxLayout* btnLayout = new QHBoxLayout();

        QPushButton* extBtn = new QPushButton("Get extensions");
        extBtn->setIcon(QIcon::fromTheme("plugin"));

        QPushButton* addBtn = new QPushButton("Add Extension");
        addBtn->setIcon(QIcon::fromTheme("list-add"));

        QPushButton* removeBtn = new QPushButton("Remove");
        removeBtn->setIcon(QIcon::fromTheme("edit-delete"));
        removeBtn->setEnabled(false);

        QPushButton* openFolderBtn = new QPushButton("Open Folder");
        openFolderBtn->setIcon(QIcon::fromTheme("folder-open"));

        QPushButton* refreshBtn = new QPushButton("Refresh");
        refreshBtn->setIcon(QIcon::fromTheme("view-refresh"));


        btnLayout->addWidget(extBtn);
        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(removeBtn);
        btnLayout->addWidget(openFolderBtn);
        btnLayout->addWidget(refreshBtn);
        btnLayout->addStretch();

        QPushButton* closeBtn = new QPushButton("Close");
        closeBtn->setIcon(QIcon::fromTheme("window-close"));
        btnLayout->addWidget(closeBtn);

        mainLayout->addLayout(btnLayout);

        QMap<QListWidgetItem*, ExtInfo*> itemMap;
        ExtInfo* currentInfo = nullptr;
        QWidget* emptyStateWidget = nullptr;

        auto populate = [&]() {
            listWidget->clear();
            itemMap.clear();
            Extense::discover();

            QString searchText = searchBox->text().toLower();
            int filterMode = filterCombo->currentIndex();

            int enabledCount = 0, disabledCount = 0, unspecifiedCount = 0;
            int visibleCount = 0;

            for (auto& info : Extense::all()) {

                if (filterMode == 1 && info.state != 1) continue;
                if (filterMode == 2 && info.state != 2) continue;
                if (filterMode == 3 && info.state != 0) continue;

                if (!searchText.isEmpty()) {
                    bool match = info.meta.name.toLower().contains(searchText) ||
                                info.meta.maintainer.toLower().contains(searchText) ||
                                info.meta.description.toLower().contains(searchText) ||
                                info.meta.version.contains(searchText) ||
                                info.file.contains(searchText, Qt::CaseInsensitive);
                    if (!match) continue;
                }

                visibleCount++;

                if (info.state == 1) enabledCount++;
                else if (info.state == 2) disabledCount++;
                else unspecifiedCount++;

                QListWidgetItem* item = new QListWidgetItem();

                QIcon extIcon = info.meta.icon.isNull() ?
                               QIcon::fromTheme("application-x-addon") :
                               info.meta.icon;
                item->setIcon(extIcon);
                item->setText(info.meta.name);
                item->setToolTip(info.meta.description);

                listWidget->addItem(item);
                itemMap[item] = &info;
            }

            statsLabel->setText(QString("Total: %1 | Enabled: %2 | Disabled: %3 | Unspecified: %4")
                .arg(enabledCount + disabledCount + unspecifiedCount)
                .arg(enabledCount).arg(disabledCount).arg(unspecifiedCount));

            if (visibleCount == 0) {
                if (!emptyStateWidget) {
                    emptyStateWidget = new QWidget(listWidget);
                    QVBoxLayout* emptyLayout = new QVBoxLayout(emptyStateWidget);

                    QLabel* emptyIcon = new QLabel();
                    QIcon emptyExtIcon = QIcon::fromTheme("application-x-addon");
                    emptyIcon->setPixmap(emptyExtIcon.pixmap(64, 64, QIcon::Disabled));
                    emptyIcon->setAlignment(Qt::AlignCenter);

                    QLabel* emptyText = new QLabel();
                    if (searchText.isEmpty() && filterMode == 0) {
                        emptyText->setText("No extensions found\nClick 'Add Extension' to get started");
                    } else if (!searchText.isEmpty()) {
                        emptyText->setText("No extensions match your search");
                    } else {
                        emptyText->setText("No extensions match the selected filter");
                    }
                    emptyText->setAlignment(Qt::AlignCenter);
                    emptyText->setStyleSheet("color: gray; font-size: 14px;");

                    emptyLayout->addStretch();
                    emptyLayout->addWidget(emptyIcon);
                    emptyLayout->addWidget(emptyText);
                    emptyLayout->addStretch();

                    listWidget->setLayout(emptyLayout);
                }
                emptyStateWidget->show();

                iconPreview->clear();
                nameLabel->setText("No extension selected");
                maintainerLabel->clear();
                versionLabel->clear();
                fileLabel->clear();
                descEdit->clear();
                removeBtn->setEnabled(false);
            } else {
                if (emptyStateWidget) {
                    emptyStateWidget->hide();
                    listWidget->setLayout(nullptr);
                }
                listWidget->setCurrentRow(0);
            }
        };

        connect(listWidget, &QListWidget::currentItemChanged,
            [&](QListWidgetItem* current, QListWidgetItem*) {
                if (!current || !itemMap.contains(current)) {
                    removeBtn->setEnabled(false);
                    return;
                }

                currentInfo = itemMap[current];
                if (!currentInfo) return;

                removeBtn->setEnabled(true);

                QIcon icon = currentInfo->meta.icon.isNull() ?
                            QIcon::fromTheme("application-x-addon") :
                            currentInfo->meta.icon;
                QPixmap pixmap = icon.pixmap(128, 128);
                iconPreview->setPixmap(pixmap);

                nameLabel->setText(currentInfo->meta.name);
                maintainerLabel->setText(QString("Maintainer: %1").arg(currentInfo->meta.maintainer));
                versionLabel->setText(QString("Version: %1").arg(currentInfo->meta.version));
                fileLabel->setText(QString("File: %1").arg(currentInfo->file));
                descEdit->setText(currentInfo->meta.description);

                if (currentInfo->state == 1) enableRadio->setChecked(true);
                else if (currentInfo->state == 2) disableRadio->setChecked(true);
                else unsureRadio->setChecked(true);
            });

        connect(enableRadio, &QRadioButton::toggled, [&](bool checked) {
            if (checked && currentInfo && currentInfo->state != 1) {
                currentInfo->state = 1;
                Extense::setState(currentInfo->meta.name, currentInfo->meta.maintainer, 1);
                populate();
            }
        });

        connect(disableRadio, &QRadioButton::toggled, [&](bool checked) {
            if (checked && currentInfo && currentInfo->state != 2) {
                currentInfo->state = 2;
                Extense::setState(currentInfo->meta.name, currentInfo->meta.maintainer, 2);
                populate();
            }
        });

        connect(unsureRadio, &QRadioButton::toggled, [&](bool checked) {
            if (checked && currentInfo && currentInfo->state != 0) {
                currentInfo->state = 0;
                Extense::setState(currentInfo->meta.name, currentInfo->meta.maintainer, 0);
                populate();
            }
        });

        connect(addBtn, &QPushButton::clicked, [&]() {
            QStringList files = QFileDialog::getOpenFileNames(dialog.get(),
                "Add Extension(s)", Settings_Backend::instance().extensionsPath(),
                "Plugins (*.so *.dll *.dylib)");

            if (!files.isEmpty()) {
                QString extPath = Settings_Backend::instance().extensionsPath();
                for (const QString& file : files) {
                    QFileInfo info(file);
                    QString destPath = extPath + "/" + info.fileName();
                    if (QFile::exists(destPath)) {
                        if (QMessageBox::question(dialog.get(), "File Exists",
                                QString("%1 already exists. Overwrite?").arg(info.fileName()),
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                            continue;
                        }
                        QFile::remove(destPath);
                    }
                    QFile::copy(file, destPath);
                }
                populate();
            }
        });

        connect(removeBtn, &QPushButton::clicked, [&]() {
            if (!currentInfo) return;

            if (QMessageBox::warning(dialog.get(), "Remove Extension",
                    QString("Remove '%1'?\nFile: %2")
                    .arg(currentInfo->meta.name, currentInfo->file),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

                QFile::remove(currentInfo->path);
                Extense::setState(currentInfo->meta.name, currentInfo->meta.maintainer, 0);
                populate();
            }
        });


        connect(extBtn, &QPushButton::clicked, []() {
            Browser_Backend::instance().openInNewTab(
                QUrl("https://github.com/zynomon/onu/tree/assets/Extensions")
            );
        });

        connect(openFolderBtn, &QPushButton::clicked, []() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(
                Settings_Backend::instance().extensionsPath()));
        });

        connect(refreshBtn, &QPushButton::clicked, [&]() {
            populate();
        });

        connect(searchBox, &QLineEdit::textChanged, [&](const QString&) {
            populate();
        });

        connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) {
            populate();
        });

        connect(closeBtn, &QPushButton::clicked, dialog.get(), &QDialog::accept);

        populate();
        dialog->exec();

        if (refreshCallback) refreshCallback();
    }

    static void permissionPrompt(QWidget *parent, const QString &origin,
                                 const QString &permission,
                                 std::function<void(int, bool)> callback) {
        auto dialog = std::make_unique<QDialog>(parent);
        dialog->setWindowTitle("Permission Request");
        dialog->setWindowIcon(QIcon::fromTheme("dialog-question"));
        dialog->setModal(true);
        dialog->setMinimumSize(450, 300);
        dialog->resize(450, 320);

        QVBoxLayout *layout = new QVBoxLayout(dialog.get());
        layout->setSpacing(15);
        layout->setContentsMargins(25, 25, 25, 25);

        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(QIcon::fromTheme("dialog-question").pixmap(64, 64));
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);

        QLabel *titleLabel = new QLabel(QString("<b>%1</b> wants to:").arg(origin));
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setWordWrap(true);
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(12);
        titleLabel->setFont(titleFont);
        layout->addWidget(titleLabel);

        auto permMap = getPermissionMap();
        PermissionInfo info = permMap.value(permission, {permission, "dialog-question"});

        QHBoxLayout *permLayout = new QHBoxLayout();
        QLabel *permIconLabel = new QLabel();
        permIconLabel->setPixmap(QIcon::fromTheme(info.iconName).pixmap(32, 32));

        QLabel *permLabel = new QLabel(info.displayName);
        QFont permFont = permLabel->font();
        permFont.setPointSize(12);
        permFont.setBold(true);
        permLabel->setFont(permFont);

        permLayout->addStretch();
        permLayout->addWidget(permIconLabel);
        permLayout->addWidget(permLabel);
        permLayout->addStretch();
        layout->addLayout(permLayout);

        layout->addSpacing(10);
        QCheckBox *rememberCheck = new QCheckBox("Remember my choice for this site");
        rememberCheck->setChecked(true);
        layout->addWidget(rememberCheck);

        layout->addStretch();
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->setContentsMargins(0, 10, 0, 0);

        QPushButton *denyBtn = new QPushButton("Deny");
        denyBtn->setIcon(QIcon::fromTheme("dialog-cancel"));
        denyBtn->setMinimumHeight(32);

        QPushButton *allowBtn = new QPushButton("Allow");
        allowBtn->setIcon(QIcon::fromTheme("dialog-ok-apply"));
        allowBtn->setMinimumHeight(32);
        allowBtn->setDefault(true);

        btnLayout->addStretch();
        btnLayout->addWidget(denyBtn);
        btnLayout->addWidget(allowBtn);
        layout->addLayout(btnLayout);

        QObject::connect(denyBtn, &QPushButton::clicked, [&]() {
            if (callback) callback(2, rememberCheck->isChecked());
            dialog->accept();
        });

        QObject::connect(allowBtn, &QPushButton::clicked, [&]() {
            if (callback) callback(1, rememberCheck->isChecked());
            dialog->accept();
        });

        QObject::connect(dialog.get(), &QDialog::rejected, [&]() {
            if (callback) callback(2, false);
        });

        dialog->exec();
    }
    static void siteInfo(QWidget *parent, const QString &origin, bool isCurrentlyOpen = false) {
        auto &s = Settings_Backend::instance();

        QStringList storedSites = s.getPermissionSites();

        QString matchedOrigin = origin;
        QUrl originUrl = QUrl::fromUserInput(origin);
        QString originHost = originUrl.host();

        for (const QString &stored : storedSites) {
            QUrl storedUrl(stored);
            if (storedUrl.host() == originHost || stored == origin) {
                matchedOrigin = stored;
                break;
            }
        }

        QDialog dialog(parent);
        dialog.setWindowTitle("Site Information - " + origin);
        dialog.setWindowIcon(QIcon::fromTheme("dialog-information"));
        dialog.setModal(true);
        dialog.setMinimumSize(650, 600);
        dialog.resize(700, 650);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setSpacing(10);
        layout->setContentsMargins(15, 15, 15, 15);

        QHBoxLayout *headerLayout = new QHBoxLayout();

        QIcon siteIcon = QIcon::fromTheme("applications-internet");
        if (isCurrentlyOpen) {
            if (auto *view = qobject_cast<QWebEngineView*>(parent)) {
                QIcon favicon = view->icon();
                if (!favicon.isNull()) {
                    siteIcon = favicon;
                }
            }
        }

        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(siteIcon.pixmap(48, 48));
        headerLayout->addWidget(iconLabel);

        QLabel *titleLabel = new QLabel(QString("<b>%1</b>").arg(origin));
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(14);
        titleLabel->setFont(titleFont);
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        layout->addLayout(headerLayout);
        layout->addSpacing(10);

        QTabWidget *tabWidget = new QTabWidget();

        QWidget *permTab = new QWidget();
        QVBoxLayout *permTabLayout = new QVBoxLayout(permTab);

        QGroupBox *permGroup = new QGroupBox("Site-specific permissions");
        QVBoxLayout *permLayout = new QVBoxLayout(permGroup);

        QStringList permissions = s.allPermissionTypes();

        for (const QString &perm : permissions) {
            QWidget *permWidget = createPermissionWidget(matchedOrigin, perm, false);
            permLayout->addWidget(permWidget);
        }

        permTabLayout->addWidget(permGroup);
        permTabLayout->addStretch();
        tabWidget->addTab(permTab, QIcon::fromTheme("preferences-system"), "Permissions");

        if (isCurrentlyOpen) {
            QWidget *cookieTab = new QWidget();
            QVBoxLayout *cookieTabLayout = new QVBoxLayout(cookieTab);

            QGroupBox *cookieGroup = new QGroupBox("Cookies");
            QVBoxLayout *cookieLayout = new QVBoxLayout(cookieGroup);

            QListWidget *cookieList = new QListWidget();
            cookieList->setMaximumHeight(250);
            cookieLayout->addWidget(cookieList);

            if (auto *view = qobject_cast<QWebEngineView*>(parent)) {
                QWebEngineCookieStore *cookieStore = view->page()->profile()->cookieStore();

                auto *loadingItem = new QListWidgetItem("Loading cookies...");
                loadingItem->setForeground(Qt::gray);
                loadingItem->setFlags(loadingItem->flags() & ~Qt::ItemIsSelectable);
                cookieList->addItem(loadingItem);

                connect(cookieStore, &QWebEngineCookieStore::cookieAdded,
                        &dialog, [cookieList, matchedOrigin](const QNetworkCookie &cookie) {

                    QString cookieDomain = cookie.domain();
                    if (cookieDomain.startsWith('.')) {
                        cookieDomain = cookieDomain.mid(1);
                    }

                    QUrl matchedUrl(matchedOrigin);
                    QString matchedHost = matchedUrl.host();

                    if (cookieDomain.contains(matchedHost) || matchedHost.contains(cookieDomain)) {

                        if (cookieList->count() == 1 &&
                            cookieList->item(0)->text() == "Loading cookies...") {
                            cookieList->clear();
                        }

                        QString cookieText = QString("%1 = %2")
                            .arg(QString::fromUtf8(cookie.name()))
                            .arg(QString::fromUtf8(cookie.value()));

                        if (cookieText.length() > 60) {
                            cookieText = cookieText.left(57) + "...";
                        }

                        QString expiryStr = cookie.isSessionCookie() ?
                            "Session" : cookie.expirationDate().toString("yyyy-MM-dd hh:mm");

                        QString tooltip = QString("Name: %1\nValue: %2\nDomain: %3\nPath: %4\nExpires: %5\nSecure: %6\nHttpOnly: %7")
                            .arg(QString::fromUtf8(cookie.name()))
                            .arg(QString::fromUtf8(cookie.value()))
                            .arg(cookie.domain())
                            .arg(cookie.path())
                            .arg(expiryStr)
                            .arg(cookie.isSecure() ? "Yes" : "No")
                            .arg(cookie.isHttpOnly() ? "Yes" : "No");

                        auto *item = new QListWidgetItem(
                            QIcon::fromTheme("preferences-web-browser-cookies"),
                            cookieText
                        );
                        item->setToolTip(tooltip);
                        cookieList->addItem(item);
                    }
                });

                cookieStore->loadAllCookies();

                QTimer::singleShot(2000, &dialog, [cookieList]() {
                    if (cookieList->count() == 1 &&
                        cookieList->item(0)->text() == "Loading cookies...") {
                        cookieList->clear();
                        auto *item = new QListWidgetItem("No cookies found for this site");
                        item->setForeground(Qt::gray);
                        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                        cookieList->addItem(item);
                    }
                });

                QPushButton *clearCookiesBtn = new QPushButton("Clear All Cookies");
                clearCookiesBtn->setIcon(QIcon::fromTheme("edit-clear"));
                connect(clearCookiesBtn, &QPushButton::clicked, [cookieStore, cookieList]() {
                    cookieStore->deleteAllCookies();
                    cookieList->clear();
                    auto *item = new QListWidgetItem("All cookies cleared");
                    item->setForeground(Qt::gray);
                    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                    cookieList->addItem(item);
                });
                cookieLayout->addWidget(clearCookiesBtn);
            }

            cookieTabLayout->addWidget(cookieGroup);
            cookieTabLayout->addStretch();
            tabWidget->addTab(cookieTab, QIcon::fromTheme("preferences-web-browser-cookies"), "Cookies");
        }

        QWidget *notifTab = new QWidget();
        QVBoxLayout *notifTabLayout = new QVBoxLayout(notifTab);

        QGroupBox *notifGroup = new QGroupBox("Session Notifications");
        QVBoxLayout *notifLayout = new QVBoxLayout(notifGroup);

        QListWidget *notifList = new QListWidget();
        notifList->setMaximumHeight(250);
        notifLayout->addWidget(notifList);

        auto notifications = SessionNotifications::getForOrigin(matchedOrigin);

        if (notifications.isEmpty()) {
            auto *item = new QListWidgetItem("No notifications received this session");
            item->setForeground(Qt::gray);
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            notifList->addItem(item);
        } else {
            for (const auto &n : notifications) {
                QIcon notifIcon = n.iconPath.isEmpty() ?
                    QIcon::fromTheme("dialog-information") :
                    Icon_Man::instance().loadIcon(n.iconPath);

                QString displayText = QString("%1 - %2")
                    .arg(QDateTime::fromMSecsSinceEpoch(n.timestamp).toString("hh:mm:ss"))
                    .arg(n.title);

                if (displayText.length() > 60) {
                    displayText = displayText.left(57) + "...";
                }

                auto *item = new QListWidgetItem(notifIcon, displayText);
                item->setToolTip(QString("Title: %1\nBody: %2\nTime: %3")
                    .arg(n.title)
                    .arg(n.body)
                    .arg(QDateTime::fromMSecsSinceEpoch(n.timestamp).toString("yyyy-MM-dd hh:mm:ss")));
                notifList->addItem(item);
            }
        }

        QPushButton *clearNotifBtn = new QPushButton("Clear Notification History");
        clearNotifBtn->setIcon(QIcon::fromTheme("edit-clear"));
        connect(clearNotifBtn, &QPushButton::clicked, [notifList, matchedOrigin]() {
            SessionNotifications::clearForOrigin(matchedOrigin);
            notifList->clear();
            auto *item = new QListWidgetItem("Notification history cleared");
            item->setForeground(Qt::gray);
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            notifList->addItem(item);
        });
        notifLayout->addWidget(clearNotifBtn);

        notifTabLayout->addWidget(notifGroup);
        notifTabLayout->addStretch();
        tabWidget->addTab(notifTab, QIcon::fromTheme("preferences-desktop-notification"), "Notifications");

        layout->addWidget(tabWidget);

        QPushButton *resetBtn = new QPushButton("Reset all to global defaults");
        resetBtn->setIcon(QIcon::fromTheme("edit-clear"));
        connect(resetBtn, &QPushButton::clicked, [&dialog, matchedOrigin, parent, isCurrentlyOpen]() {
            if (QMessageBox::question(&dialog, "Reset Site",
                "Reset all permissions for this site to global defaults?") == QMessageBox::Yes) {
                Settings_Backend::instance().clearSitePermissions(matchedOrigin);
                dialog.accept();
                siteInfo(parent, matchedOrigin, isCurrentlyOpen);
            }
        });
        layout->addWidget(resetBtn);

        layout->addStretch();

        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        QPushButton *closeBtn = new QPushButton("Close");
        closeBtn->setMinimumHeight(32);
        closeBtn->setDefault(true);
        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        btnLayout->addWidget(closeBtn);
        layout->addLayout(btnLayout);

        dialog.exec();
    }
    static void sitesManager(QWidget *parent) {
        QDialog dialog(parent);
        dialog.setWindowTitle("Site Permissions Manager");
        dialog.setWindowIcon(QIcon::fromTheme("preferences-system"));
        dialog.resize(800, 600);
        dialog.setModal(true);

        QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(10, 10, 10, 10);

        QTabWidget *tabWidget = new QTabWidget();

        QWidget *globalTab = new QWidget();
        QVBoxLayout *globalLayout = new QVBoxLayout(globalTab);

        QLabel *globalDesc = new QLabel(
            "Default permissions for all sites. These apply when a site hasn't set a specific override."
        );
        globalDesc->setWordWrap(true);
        globalDesc->setStyleSheet("color: gray; padding: 5px;");
        globalLayout->addWidget(globalDesc);

        QGroupBox *globalGroup = new QGroupBox("Global Default Permissions");
        QVBoxLayout *globalPermLayout = new QVBoxLayout(globalGroup);

        auto &s = Settings_Backend::instance();
        QStringList permissions = s.allPermissionTypes();

        for (const QString &perm : permissions) {
            QWidget *permWidget = createPermissionWidget("", perm, true);

            globalPermLayout->addWidget(permWidget);
        }

        globalLayout->addWidget(globalGroup);
        globalLayout->addStretch();

        tabWidget->addTab(globalTab, QIcon::fromTheme("preferences-system"), "Global Defaults");

        QWidget *sitesTab = new QWidget();
        QVBoxLayout *sitesLayout = new QVBoxLayout(sitesTab);

        QHBoxLayout *searchLayout = new QHBoxLayout();
        QLineEdit *searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Search sites...");
        searchEdit->setClearButtonEnabled(true);
        searchLayout->addWidget(new QLabel("Search:"));
        searchLayout->addWidget(searchEdit);
        sitesLayout->addLayout(searchLayout);

        QSplitter *splitter = new QSplitter(Qt::Horizontal);

        QListWidget *sitesList = new QListWidget();
        sitesList->setMinimumWidth(200);
        sitesList->setSelectionMode(QAbstractItemView::SingleSelection);

        QStringList sites = s.getPermissionSites();
        for (const QString &site : std::as_const(sites)) {
            QListWidgetItem *item = new QListWidgetItem(site);
            item->setIcon(QIcon::fromTheme("applications-internet"));
            sitesList->addItem(item);
        }

        if (sites.isEmpty()) {
            QListWidgetItem *item = new QListWidgetItem("No sites with custom permissions");
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            sitesList->addItem(item);
        }

        splitter->addWidget(sitesList);

        QWidget *sitePermWidget = new QWidget();
        QVBoxLayout *sitePermLayout = new QVBoxLayout(sitePermWidget);

        QLabel *selectLabel = new QLabel("Select a site to view its permissions");
        selectLabel->setAlignment(Qt::AlignCenter);
        selectLabel->setStyleSheet("color: gray; padding: 20px;");
        sitePermLayout->addWidget(selectLabel);
        sitePermLayout->addStretch();

        splitter->addWidget(sitePermWidget);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 2);

        sitesLayout->addWidget(splitter);

        QTimer::singleShot(0, [&]() {
            connect(sitesList, &QListWidget::currentItemChanged,
                [&, sitePermWidget, sitePermLayout](QListWidgetItem *current, QListWidgetItem*) {
                if (!current || current->text() == "No sites with custom permissions") return;

                QLayoutItem *child;
                while ((child = sitePermLayout->takeAt(0)) != nullptr) {
                    delete child->widget();
                    delete child;
                }

                QString origin = current->text();

                QLabel *titleLabel = new QLabel(QString("<b>%1</b>").arg(origin));
                titleLabel->setStyleSheet("font-size: 14px; padding: 5px;");
                sitePermLayout->addWidget(titleLabel);

                QGroupBox *siteGroup = new QGroupBox("Site-specific overrides");
                QVBoxLayout *siteGroupLayout = new QVBoxLayout(siteGroup);

                QMap<QString, int> sitePerms = s.getSitePermissions(origin);

                for (const QString &perm : permissions) {
                    QWidget *permWidget = createPermissionWidget(origin, perm, false);
                    siteGroupLayout->addWidget(permWidget);
                }

                sitePermLayout->addWidget(siteGroup);

                QPushButton *clearBtn = new QPushButton("Reset all for this site");
                clearBtn->setIcon(QIcon::fromTheme("edit-clear"));
              connect(clearBtn, &QPushButton::clicked, [&dialog, origin, sitesList, &s]() {
                  if (QMessageBox::question(&dialog, "Reset Site",
                        "Reset all permissions for this site to global defaults?") == QMessageBox::Yes) {
                        s.clearSitePermissions(origin);

                        sitesList->clear();
                        QStringList updatedSites = s.getPermissionSites();
                        for (const QString &site : updatedSites) {
                            QListWidgetItem *item = new QListWidgetItem(site);
                            item->setIcon(QIcon::fromTheme("applications-internet"));
                            sitesList->addItem(item);
                        }
                        if (updatedSites.isEmpty()) {
                            QListWidgetItem *item = new QListWidgetItem("No sites with custom permissions");
                            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
                            sitesList->addItem(item);
                        }
                    }
                });
                sitePermLayout->addWidget(clearBtn);

                sitePermLayout->addStretch();
            });
        });

        connect(searchEdit, &QLineEdit::textChanged, [sitesList](const QString &text) {
            for (int i = 0; i < sitesList->count(); ++i) {
                QListWidgetItem *item = sitesList->item(i);
                bool hide = !text.isEmpty() && !item->text().contains(text, Qt::CaseInsensitive);
                item->setHidden(hide);
            }
        });

        tabWidget->addTab(sitesTab, QIcon::fromTheme("applications-internet"), "Sites");

        mainLayout->addWidget(tabWidget);

        QHBoxLayout *btnLayout = new QHBoxLayout();

        QPushButton *clearAllBtn = new QPushButton("Clear All Site Permissions");
        clearAllBtn->setIcon(QIcon::fromTheme("edit-delete"));
        connect(clearAllBtn, &QPushButton::clicked, [&dialog, parent]() {
            if (QMessageBox::warning(&dialog, "Clear All",
                "Remove ALL site-specific permissions?\n\nThis cannot be undone.",
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                Settings_Backend::instance().clearAllSitePermissions();
                dialog.accept();
                sitesManager(parent);

            }
        });
        btnLayout->addWidget(clearAllBtn);

        btnLayout->addStretch();

        QPushButton *closeBtn = new QPushButton("Close");
        closeBtn->setMinimumHeight(32);
        closeBtn->setDefault(true);
        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        btnLayout->addWidget(closeBtn);

        mainLayout->addLayout(btnLayout);

        dialog.exec();
    }


    static bool requestEncryptionKey(QWidget *parent, int &attemptCount) {
        constexpr int maxAttempts = 10;

        auto remainingText = [&](int count) {
            int remaining = std::clamp(maxAttempts - count, 0, maxAttempts);
            return QString("Attempts remaining: %1 / %2")
                   .arg(remaining)
                   .arg(maxAttempts);
        };

        if (attemptCount >= maxAttempts) {
            QMessageBox msgBox(parent);
            msgBox.setWindowTitle("Maximum Attempts Exceeded");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText("Too many failed login attempts.");
            msgBox.setInformativeText(
                "To reset and regain access:\n\n"
                "1. Close Onu completely\n"
                "2. Navigate to: " + Settings_Backend::instance().dataPath() + "\n"
                "3. Delete the file: encrypted.conf\n"
                "4. Restart Onu\n\n"
                "Warning: This will permanently delete all encrypted data "
                "(history, favorites, sessions,extension prefernces, permissions given to sites)."
            );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            return false;
        }

        QDialog dialog(parent);
        dialog.setWindowTitle("Onu - Encryption Key Required");
        dialog.setWindowIcon(QIcon::fromTheme("dialog-password"));
        dialog.setModal(true);
        dialog.resize(450, 200);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        QLabel *title = new QLabel("Enter Encryption Password");
        QFont tf = title->font(); tf.setPointSize(12); tf.setBold(true);
        title->setFont(tf);
        layout->addWidget(title);

        QLabel *info = new QLabel(
            "Your data is encrypted with a custom key.\n"
            "Please enter your password to continue:"
        );
        info->setWordWrap(true);
        layout->addWidget(info);

        QLineEdit *passwordEdit = new QLineEdit();
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setPlaceholderText("Enter password...");
        layout->addWidget(passwordEdit);

        QLabel *feedback = new QLabel(remainingText(attemptCount));
        feedback->setStyleSheet("color:#ff5555; font-weight:bold;");
        feedback->setWordWrap(true);
        layout->addWidget(feedback);

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel
        );
        QPushButton *okBtn = buttons->button(QDialogButtonBox::Ok);
        okBtn->setText("Unlock");
        okBtn->setEnabled(false);
        layout->addWidget(buttons);

        QObject::connect(passwordEdit, &QLineEdit::textChanged, [&](const QString &txt) {
            okBtn->setEnabled(!txt.isEmpty());
            feedback->setText(remainingText(attemptCount));
        });

        QObject::connect(buttons, &QDialogButtonBox::accepted, [&]() {
            QByteArray key = QCryptographicHash::hash(passwordEdit->text().toUtf8(),
                                                      QCryptographicHash::Sha256);
            Settings_Backend::instance().setSessionKey(key);

            if (!Settings_Backend::instance().verifyEncryptionKey()) {
                attemptCount++;
                feedback->setPixmap(QIcon::fromTheme("dialog-error").pixmap(16,16));
                feedback->setText("Incorrect password.\n" + remainingText(attemptCount));
                passwordEdit->clear();
                passwordEdit->setFocus();

                if (attemptCount >= maxAttempts) {
                    dialog.reject();
                }
                return;
            }

            feedback->setPixmap(QIcon::fromTheme("dialog-ok").pixmap(16,16));
            feedback->setText("Password correct! Unlocking...");
            QApplication::processEvents();
            QThread::msleep(500);
            dialog.accept();
        });

        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        QObject::connect(passwordEdit, &QLineEdit::returnPressed, [&]() {
            if (!passwordEdit->text().isEmpty())
                okBtn->click();
        });

        return dialog.exec() == QDialog::Accepted;
    }
static void settings(QWidget *parent, std::function<void()> applyCallback) {
        SettingsDialog dialog(parent, applyCallback);

        QPushButton *kbBtn = dialog.findChild<QPushButton*>("btnKeybinds");
        if (kbBtn) connect(kbBtn, &QPushButton::clicked, [&]() { keybinds(&dialog, applyCallback); });

        QPushButton *favBtn = dialog.findChild<QPushButton*>("btnFavorites");
        if (favBtn) connect(favBtn, &QPushButton::clicked, [&]() {
            QVariantList favs = Settings_Backend::instance().loadFavorites();
            fav(&dialog, favs, applyCallback);
            Settings_Backend::instance().saveFavorites(favs);
        });

        QPushButton *histBtn = dialog.findChild<QPushButton*>("btnHistory");
        if (histBtn) connect(histBtn, &QPushButton::clicked, [&]() {
            history(&dialog, Settings_Backend::instance().loadHistory(),
                   []() { Settings_Backend::instance().clearHistory(); });
        });

        QPushButton *extBtn = dialog.findChild<QPushButton*>("btnExtensions");
        if (extBtn) connect(extBtn, &QPushButton::clicked, [&]() { ExtMan(&dialog, applyCallback); });

        QPushButton *sitesBtn = dialog.findChild<QPushButton*>("btnSitePermissions");
        if (sitesBtn) connect(sitesBtn, &QPushButton::clicked, [&]() {
            sitesManager(&dialog);
        });

        dialog.exec();
    }

private:
struct PermissionInfo {
    QString displayName;
    QString iconName;
};

static QMap<QString, PermissionInfo> getPermissionMap() {
    static QMap<QString, PermissionInfo> permMap = {
        {"geolocation", {"Access your location", "find-location"}},
        {"microphone", {"Use your microphone", "audio-input-microphone"}},
        {"camera", {"Use your camera", "camera-video"}},
        {"camera_microphone", {"Use your camera and microphone", "camera-video"}},
        {"mouselock", {"Lock your mouse pointer", "input-mouse"}},
        {"notifications", {"Show desktop notifications", "preferences-desktop-notification"}},
        {"popups", {"Open popup windows", "window-new"}},
        {"clipboard", {"Read and write to clipboard", "edit-copy"}},
        {"localFonts", {"Access your local fonts", "preferences-desktop-font"}},
        {"desktopVideo", {"Share your screen", "video-display"}},
        {"desktopAudioVideo", {"Share your screen with audio", "video-display"}},
        {"fileAccess", {"Access files on your device", "document-open"}}
    };
    return permMap;
}
static QWidget* createPermissionRow(const QString &context, const QString &permission, bool isGlobal = false) {
    QWidget *rowWidget = new QWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(5, 2, 5, 2);
    rowLayout->setSpacing(10);

    auto permMap = getPermissionMap();
    PermissionInfo info = permMap.value(permission, {permission, "dialog-question"});

    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(QIcon::fromTheme(info.iconName).pixmap(16, 16));
    iconLabel->setFixedSize(20, 20);
    rowLayout->addWidget(iconLabel);

    QLabel *nameLabel = new QLabel(info.displayName);
    nameLabel->setMinimumWidth(200);
    rowLayout->addWidget(nameLabel);

    rowLayout->addStretch();

    QRadioButton *denyRadio = new QRadioButton();
    QRadioButton *allowRadio = new QRadioButton();
    QRadioButton *askRadio = new QRadioButton();

    denyRadio->setFixedWidth(70);
    allowRadio->setFixedWidth(70);
    askRadio->setFixedWidth(70);

    int currentState;
    if (isGlobal) {
        currentState = Settings_Backend::instance().getGlobalPermission(permission);
    } else {
        currentState = Settings_Backend::instance().getSitePermission(context, permission);
    }

    if (currentState == 1) allowRadio->setChecked(true);
    else if (currentState == 2) denyRadio->setChecked(true);
    else askRadio->setChecked(true);

    rowLayout->addWidget(denyRadio);
    rowLayout->addWidget(allowRadio);
    rowLayout->addWidget(askRadio);

    if (isGlobal) {
        QObject::connect(denyRadio, &QRadioButton::toggled, [permission](bool checked) {
            if (checked) Settings_Backend::instance().setGlobalPermission(permission, 2);
        });
        QObject::connect(allowRadio, &QRadioButton::toggled, [permission](bool checked) {
            if (checked) Settings_Backend::instance().setGlobalPermission(permission, 1);
        });
        QObject::connect(askRadio, &QRadioButton::toggled, [permission](bool checked) {
            if (checked) Settings_Backend::instance().setGlobalPermission(permission, 0);
        });
    } else {
        QObject::connect(denyRadio, &QRadioButton::toggled, [context, permission](bool checked) {
            if (checked) Settings_Backend::instance().setSitePermission(context, permission, 2);
        });
        QObject::connect(allowRadio, &QRadioButton::toggled, [context, permission](bool checked) {
            if (checked) Settings_Backend::instance().setSitePermission(context, permission, 1);
        });
        QObject::connect(askRadio, &QRadioButton::toggled, [context, permission](bool checked) {
            if (checked) Settings_Backend::instance().setSitePermission(context, permission, 0);
        });
    }

    return rowWidget;
}

static QWidget* createPermissionWidget(const QString &context, const QString &permission, bool isGlobal = false) {

    static QWidget *container = nullptr;
    static QVBoxLayout *containerLayout = nullptr;
    static QScrollArea *scrollArea = nullptr;
    static QWidget *contentWidget = nullptr;
    static QVBoxLayout *contentLayout = nullptr;
    static int callCount = 0;
    static QPointer<QWidget> lastContainer;

    if (lastContainer.isNull() || !lastContainer) {
        container = nullptr;
        containerLayout = nullptr;
        scrollArea = nullptr;
        contentWidget = nullptr;
        contentLayout = nullptr;
        callCount = 0;
    }

    if (callCount == 0) {
        container = new QWidget();
        lastContainer = container;

        containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);

        scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        contentWidget = new QWidget();
        contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(0);

        QHBoxLayout *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(5, 2, 5, 2);
        headerLayout->setSpacing(10);

        QLabel *permHeader = new QLabel("Permission");
        permHeader->setMinimumWidth(220);
        headerLayout->addWidget(permHeader);

        headerLayout->addStretch();

        QLabel *denyHeader = new QLabel("Deny");
        denyHeader->setFixedWidth(70);
        denyHeader->setAlignment(Qt::AlignCenter);
        headerLayout->addWidget(denyHeader);

        QLabel *allowHeader = new QLabel("Allow");
        allowHeader->setFixedWidth(70);
        allowHeader->setAlignment(Qt::AlignCenter);
        headerLayout->addWidget(allowHeader);

        QLabel *askHeader = new QLabel("Ask");
        askHeader->setFixedWidth(70);
        askHeader->setAlignment(Qt::AlignCenter);
        headerLayout->addWidget(askHeader);

        contentLayout->addLayout(headerLayout);

        QFrame *line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        contentLayout->addWidget(line);

        scrollArea->setWidget(contentWidget);
        containerLayout->addWidget(scrollArea);
    }

    QWidget *permissionRow = createPermissionRow(context, permission, isGlobal);
    contentLayout->addWidget(permissionRow);

    callCount++;

    if (callCount >= 12) {
        callCount = 0;
        container = nullptr;
        containerLayout = nullptr;
        scrollArea = nullptr;
        contentWidget = nullptr;
        contentLayout = nullptr;
    }

    return lastContainer;
}
static void showFavoriteDialog(QWidget *parent, QVariantList &favorites,
                                   std::function<void(const QString&)> populateList,
                                   const QString &title, const QString &url,
                                   const QString &iconPath, bool isEdit) {
        QDialog dialog(parent);
        dialog.setWindowTitle(isEdit ? "Edit Favorite" : "Add Favorite");
        dialog.setMinimumWidth(500);

        QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
        QFormLayout *form = new QFormLayout();
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

        QHBoxLayout *iconLayout = new QHBoxLayout();
        QLabel *iconPreview = new QLabel();
        iconPreview->setPixmap(Icon_Man::instance().loadIcon(iconPath).pixmap(48, 48));
        iconPreview->setFixedSize(52, 52);
        iconPreview->setStyleSheet("border: 1px solid palette(mid); border-radius: 4px; padding: 2px;");

        QPushButton *pickIconBtn = new QPushButton("Choose Icon...");
        pickIconBtn->setIcon(QIcon::fromTheme("image-x-generic"));

        QPushButton *clearIconBtn = new QPushButton("Default");
        clearIconBtn->setIcon(QIcon::fromTheme("edit-clear"));

        QString currentIconPath = iconPath;

        iconLayout->addWidget(iconPreview);
        iconLayout->addWidget(pickIconBtn);
        iconLayout->addWidget(clearIconBtn);
        iconLayout->addStretch();

        form->addRow("Icon:", iconLayout);

        QLineEdit *titleEdit = new QLineEdit(title);
        titleEdit->setPlaceholderText("My Favorite Site");
        titleEdit->setMinimumHeight(32);
        form->addRow("Title:", titleEdit);

        QLineEdit *urlEdit = new QLineEdit(url);
        urlEdit->setPlaceholderText("https://example.com");
        urlEdit->setMinimumHeight(32);
        form->addRow("URL:", urlEdit);

        mainLayout->addLayout(form);

        QLabel *validationLabel = new QLabel();
        validationLabel->setStyleSheet("color: red; font-size: 11px;");
        validationLabel->setWordWrap(true);
        validationLabel->hide();
        mainLayout->addWidget(validationLabel);

        mainLayout->addStretch();

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setText(isEdit ? "Save" : "Add");
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!title.isEmpty() && !url.isEmpty());
        mainLayout->addWidget(buttons);

        auto validateInputs = [titleEdit, urlEdit, buttons, validationLabel]() {
            QString titleText = titleEdit->text().trimmed();
            QString urlText = urlEdit->text().trimmed();

            if (titleText.isEmpty() && urlText.isEmpty()) {
                validationLabel->hide();
                buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
                return false;
            }

            if (titleText.isEmpty()) {
                validationLabel->setText("Please enter a title");
                validationLabel->show();
                buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
                return false;
            }

            if (urlText.isEmpty()) {
                validationLabel->setText("Please enter a URL");
                validationLabel->show();
                buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
                return false;
            }

            QUrl parsedUrl = QUrl::fromUserInput(urlText);
            if (!parsedUrl.isValid() || parsedUrl.scheme().isEmpty()) {
                validationLabel->setText("Please enter a valid URL (e.g., https://example.com)");
                validationLabel->show();
                buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
                return false;
            }

            validationLabel->hide();
            buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
            return true;
        };

        connect(titleEdit, &QLineEdit::textChanged, validateInputs);
        connect(urlEdit, &QLineEdit::textChanged, validateInputs);

        connect(pickIconBtn, &QPushButton::clicked, [&currentIconPath, iconPreview, &dialog]() {
            QString cachePath = Settings_Backend::instance().othersIconCache();
            QString fileName = QFileDialog::getOpenFileName(&dialog,
                                                            "Select Icon",
                                                            cachePath,
                                                            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.ico *.svg)");
            if (!fileName.isEmpty()) {
                QIcon icon(fileName);
                if (!icon.isNull()) {
                    currentIconPath = Icon_Man::instance().saveIcon(icon);
                    iconPreview->setPixmap(icon.pixmap(48, 48));
                }
            }
        });

        connect(clearIconBtn, &QPushButton::clicked, [&currentIconPath, iconPreview]() {
            currentIconPath = "";
            iconPreview->setPixmap(QIcon::fromTheme("applications-internet").pixmap(48, 48));
        });

        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            QString newTitle = titleEdit->text().trimmed();
            QString newUrl = urlEdit->text().trimmed();

            if (!newTitle.isEmpty() && !newUrl.isEmpty()) {

                QUrl parsedUrl = QUrl::fromUserInput(newUrl);
                newUrl = parsedUrl.toString();

                if (isEdit) {
                    for (int i = 0; i < favorites.size(); ++i) {
                        QVariantMap fav = favorites[i].toMap();
                        if (fav["url"].toString() == url) {
                            fav["title"] = newTitle;
                            fav["url"] = newUrl;
                            fav["iconPath"] = currentIconPath;
                            favorites[i] = fav;
                            break;
                        }
                    }
                } else {
                    favorites.append(QVariantMap{
                        {"title", newTitle},
                        {"url", newUrl},
                        {"iconPath", currentIconPath}
                    });
                }
                populateList("");
            }
        }
    }
};
class Content_Protector : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    explicit Content_Protector(QObject *parent = nullptr)
        : QWebEngineUrlRequestInterceptor(parent) {
        loadFilterRules();
    }

    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        QUrl url = info.requestUrl();
        QString urlStr = url.toString();
        QString host = url.host().toLower();

        if (Settings_Backend::instance().adblockEnabled()) {
            if (isHostBlocked(host) || isUrlPatternBlocked(urlStr)) {
                info.block(true);
                return;
            }
        }

        if (Settings_Backend::instance().doNotTrack()) {
            info.setHttpHeader("DNT", "1");
            info.setHttpHeader("Sec-GPC", "1");
        }
    }

    QString getInjectionScript() const {
        QString userScript = Settings_Backend::instance().userScript();
        QString collectorScript = dataCollectionScript();
        QString combined = collectorScript;
        if (!userScript.trimmed().isEmpty()) {
            combined += "\n// === User Script ===\n" + userScript;
        }
        return combined;
    }

    void reloadRules() {
        loadFilterRules();
    }

private:
    QSet<QString> m_blockedHosts;
    QList<QRegularExpression> m_blockedPatterns;

    void loadFilterRules() {
        m_blockedHosts.clear();
        m_blockedPatterns.clear();
        if (!Settings_Backend::instance().adblockEnabled()) return;

        QString filePath = Settings_Backend::instance().blockedHostsFile();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('!') || line.startsWith('[')) continue;
            if (line.startsWith("@@")) continue;

            if (line.startsWith("||")) {
                int endIdx = line.indexOf('^');
                QString domain = (endIdx > 2) ? line.mid(2, endIdx - 2) : line.mid(2);
                m_blockedHosts.insert(domain.toLower());
            } else if (!line.contains('*') && !line.contains('?')) {
                m_blockedHosts.insert(line.toLower());
            } else {
                QString pattern = QRegularExpression::escape(line);
                pattern.replace("\\*", ".*");
                m_blockedPatterns.append(QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption));
            }
        }
        file.close();
    }

    bool isHostBlocked(const QString &host) const {
        return m_blockedHosts.contains(host);
    }

    bool isUrlPatternBlocked(const QString &url) const {
        for (const QRegularExpression &re : m_blockedPatterns) {
            if (re.match(url).hasMatch()) return true;
        }
        return false;
    }

    static QString dataCollectionScript() {
        return R"JS(
(function() {
    'use strict';
    function collectMedia() {
        var results = [];
        document.querySelectorAll('img, video, audio, source').forEach(function(el) {
            var rect = el.getBoundingClientRect();
            results.push({
                tag: el.tagName.toLowerCase(),
                src: el.src || el.currentSrc || '',
                alt: el.alt || el.title || '',
                width: Math.round(rect.width),
                height: Math.round(rect.height)
            });
        });
        return results;
    }
    var debounceTimer;
    function scheduleCollection() {
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(function() {
            var origin = location.protocol + '//' + location.host;
            if (window.onuDataCollector) {
                window.onuDataCollector(origin, collectMedia());
            }
        }, 500);
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', scheduleCollection);
    } else {
        scheduleCollection();
    }
    if (typeof MutationObserver !== 'undefined') {
        var observer = new MutationObserver(scheduleCollection);
        observer.observe(document.body, { childList: true, subtree: true });
    }
})();
)JS";
    }
};
#include "Qtdl.H"
class Create_Toolbars : public QObject {
    Q_OBJECT

public:
    explicit Create_Toolbars(QMainWindow *mainWindow, QObject *parent = nullptr)
        : QObject(parent), m_mainWindow(mainWindow), m_currentZoomLevel(100) {

        m_networkManager = new QNetworkAccessManager(this);

        navbar();
        urlbar();
        tabbar();
        recbar();
        favbar();
        createFindBar();

        applyToolbarSettings();
        installToolbarEventFilters();
        setupZoomShortcuts();
    }

    QToolBar* navToolbar() { return m_navToolbar; }
    QToolBar* recToolbar() { return m_recToolbar; }
    QToolBar* urlToolbar() { return m_urlToolbar; }
    QToolBar* favToolbar() { return m_favToolbar; }
    QToolBar* tabToolbar() { return m_tabToolbar; }
    QWidget* findBar() { return m_findBar; }
    QLineEdit* findEdit() { return m_findEdit; }
    QByteArray saveToolbarGeometry() { return m_mainWindow->saveState(); }
    QTabBar* tabBar() { return m_tabBar; }
    QLineEdit* addressBar() { return m_addressBar; }
    QComboBox* searchEngineCombo() { return m_searchEngineCombo; }
    QAction* reloadAction() { return m_reloadAction; }
    QAction* stopAction() { return m_stopAction; }
    QAction* findAction() { return m_findAction; }
    QAction* zoomAction() { return m_zoomAction; }

    void updateNavActions(bool back, bool forward) {
        if (m_backAction) m_backAction->setEnabled(back);
        if (m_forwardAction) m_forwardAction->setEnabled(forward);
    }

    void updateSuggestions(const QVariantList &history, const QVariantList &favorites) {
        m_historyData = history;
        m_favoritesData = favorites;
    }


    void updateVerticalTabs() {
        Qt::ToolBarArea tabArea = m_mainWindow->toolBarArea(m_tabToolbar);
        bool isVertical = (tabArea == Qt::LeftToolBarArea || tabArea == Qt::RightToolBarArea);

        if (isVertical && m_verticalTabWidget && m_verticalTabWidgetAction && m_verticalTabWidgetAction->isVisible()) {
            syncTabsToVertical();
        }
    }


    void updateVerticalTab(int index, const QIcon &icon, const QString &text) {
        if (m_verticalTabWidget && index >= 0 && index < m_verticalTabWidget->count()) {
            m_verticalTabWidget->blockSignals(true);
            m_verticalTabWidget->setTabIcon(index, icon);
            m_verticalTabWidget->setTabText(index, text);
            m_verticalTabWidget->blockSignals(false);
        }
    }


    void updateTab(int index, const QIcon &icon, const QString &text) {
        if (m_tabBar && index >= 0 && index < m_tabBar->count()) {
            m_tabBar->setTabIcon(index, icon);
            m_tabBar->setTabText(index, text);
        }
        updateVerticalTab(index, icon, text);
    }


    void updateTabToolTip(int index, const QString &tooltip) {
        if (m_tabBar && index >= 0 && index < m_tabBar->count()) {
            m_tabBar->setTabToolTip(index, tooltip);
        }
        if (m_verticalTabWidget && index >= 0 && index < m_verticalTabWidget->count()) {
            m_verticalTabWidget->setTabToolTip(index, tooltip);
        }
    }


    void addVerticalTab(const QIcon &icon, const QString &text) {
        Qt::ToolBarArea tabArea = m_mainWindow->toolBarArea(m_tabToolbar);
        bool isVertical = (tabArea == Qt::LeftToolBarArea || tabArea == Qt::RightToolBarArea);

        if (isVertical && m_verticalTabWidget && m_verticalTabWidgetAction && m_verticalTabWidgetAction->isVisible()) {
            QWidget *placeholder = new QWidget();
            m_verticalTabWidget->addTab(placeholder, icon, text);
        }
    }


    void removeVerticalTab(int index) {
        if (m_verticalTabWidget && index >= 0 && index < m_verticalTabWidget->count()) {
            m_verticalTabWidget->removeTab(index);
        }
    }


    void setVerticalCurrentTab(int index) {
        if (m_verticalTabWidget && index >= 0 && index < m_verticalTabWidget->count()) {
            m_verticalTabWidget->blockSignals(true);
            m_verticalTabWidget->setCurrentIndex(index);
            m_verticalTabWidget->blockSignals(false);
        }
    }

    void applyToolbarSettings() {
        auto &s = Settings_Backend::instance();

        m_navToolbar->setVisible(s.toolbarVisible("Navigation"));
        m_recToolbar->setVisible(s.toolbarVisible("Recent"));
        m_urlToolbar->setVisible(s.toolbarVisible("Search"));
        m_favToolbar->setVisible(s.toolbarVisible("Favorites"));
        m_tabToolbar->setVisible(s.toolbarVisible("Tab"));

        m_navToolbar->setMovable(!s.toolbarLockDragging("Navigation"));
        m_recToolbar->setMovable(!s.toolbarLockDragging("Recent"));
        m_urlToolbar->setMovable(!s.toolbarLockDragging("Search"));
        m_favToolbar->setMovable(!s.toolbarLockDragging("Favorites"));
        m_tabToolbar->setMovable(!s.toolbarLockDragging("Tab"));

        m_navToolbar->setIconSize(QSize(s.navigationIconSize(), s.navigationIconSize()));
        m_recToolbar->setIconSize(QSize(24, 24));
        m_favToolbar->setIconSize(QSize(24, 24));

        if (m_searchEngineComboAction) {
            m_searchEngineComboAction->setVisible(s.showSearchEnginePicker());
        }
        updateSearchEngineCombo();
        applyNavToolbarVisibility();
    }

    void refreshRecentToolbar(const QVariantList &recent) {
        m_historyData = recent;
        m_recToolbar->clear();
        int limit = Settings_Backend::instance().historySidebarLimit();
        for(int i=0; i < qMin(limit, (int)recent.size()); ++i) {
            auto m = recent[i].toMap();
            auto a = m_recToolbar->addAction(Icon_Man::instance().loadIcon(m["iconPath"].toString()), "");
            a->setData(QUrl(m["url"].toString()));
            a->setToolTip(m["title"].toString());
        }
        m_recToolbar->addSeparator();
        auto a = m_recToolbar->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties), "");
        a->setData("manage");
    }

    void refreshFavToolbar(const QVariantList &favs) {
        m_favoritesData = favs;
        m_favToolbar->clear();
        int limit = Settings_Backend::instance().favSidebarLimit();
        for(int i=0; i < qMin(limit, (int)favs.size()); ++i) {
            auto m = favs[i].toMap();
            auto a = m_favToolbar->addAction(Icon_Man::instance().loadIcon(m["iconPath"].toString()), "");
            a->setData(QUrl(m["url"].toString()));
            a->setToolTip(m["title"].toString());
        }
        m_favToolbar->addSeparator();
        auto a = m_favToolbar->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties), "");
        a->setData("customize");
    }
    void restoreToolbarGeometry(const QByteArray &state) {
        if (!state.isEmpty()) {
            m_mainWindow->restoreState(state);
        }
    }
signals:
    void backRequested();
    void forwardRequested();
    void reloadRequested();
    void stopRequested();
    void findRequested();
    void navigateRequested(const QUrl &url);
    void recentActionTriggered(const QUrl &url, bool isManage);
    void Open_InNewTab_tb(const QUrl &url, bool isCustomize);
    void tabCloseRequested(int index);
    void tabChanged(int index);
    void tabMoved(int from, int to);
    void newTabRequested();
    void volumeChanged(int volume);
    void showHistoryRequested();
    void showSettingsRequested();
    void showDownloadsRequested();
    void quitRequested();
    void populateBackMenu(QMenu *menu);
    void populateForwardMenu(QMenu *menu);
    void findTextChanged(const QString &text);
    void findNext();
    void findPrevious();
    void clearHistoryRequested();
    void tabContextMenuRequested(int index, const QPoint &pos);
    void addTabToFavorites(int tabIndex);
    void addUrlToFavorites(const QUrl &url, const QString &title);
    void zoomLevelChanged(int percent);
    void zoomIn();
    void zoomOut();
    void zoomReset();

private:
    QMainWindow *m_mainWindow;
    QToolBar *m_navToolbar = nullptr;
    QToolBar *m_recToolbar = nullptr;
    QToolBar *m_urlToolbar = nullptr;
    QToolBar *m_favToolbar = nullptr;
    QToolBar *m_tabToolbar = nullptr;
    QWidget *m_findBar = nullptr;
    QLineEdit *m_findEdit = nullptr;
    QLineEdit *m_addressBar = nullptr;
    QTabBar *m_tabBar = nullptr;
    QTabWidget *m_verticalTabWidget = nullptr;
    QComboBox *m_searchEngineCombo = nullptr;
    QAction *m_backAction = nullptr;
    QAction *m_forwardAction = nullptr;
    QAction *m_reloadAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_findAction = nullptr;
    QAction *m_homeAction = nullptr;
    QAction *m_searchEngineComboAction = nullptr;
    QAction *m_volumeAction = nullptr;
    QAction *m_zoomAction = nullptr;
    QAction *m_searchIconAction = nullptr;
    QAction *m_addressBarAction = nullptr;
    QAction *m_tabBarAction = nullptr;
    QAction *m_verticalTabWidgetAction = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QToolButton *m_menuButton = nullptr;
    QToolButton *m_forwardButton = nullptr;
    QToolButton *m_zoomButton = nullptr;
    QToolButton *m_searchIconButton = nullptr;


    QComboBox *m_verticalSearchEngineCombo = nullptr;
    QLineEdit *m_verticalAddressBar = nullptr;

    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_currentSuggestionReply = nullptr;
    QStandardItemModel *m_suggestionModel = nullptr;
    QCompleter *m_suggestionCompleter = nullptr;

    QVariantList m_historyData;
    QVariantList m_favoritesData;
    int m_currentZoomLevel;


    void applyNavToolbarVisibility() {
        auto &s = Settings_Backend::instance();

        if (m_reloadAction) {
            m_reloadAction->setVisible(s.showReload());
        }

        if (m_forwardAction) {
            m_forwardAction->setVisible(s.showForward());
        }

        if (m_menuButton) {
            m_menuButton->setVisible(s.showMenu());
        }

        if (m_volumeAction) {
            m_volumeAction->setVisible(s.showVolumeSlider());
        }
    }

    void setupZoomShortcuts() {
        QShortcut *zoomInShortcut = new QShortcut(QKeySequence("Ctrl++"), m_mainWindow);
        connect(zoomInShortcut, &QShortcut::activated, this, [this]() {
            int newZoom = qMin(500, m_currentZoomLevel + 10);
            m_currentZoomLevel = newZoom;
            emit zoomLevelChanged(newZoom);
        });

        QShortcut *zoomOutShortcut = new QShortcut(QKeySequence("Ctrl+-"), m_mainWindow);
        connect(zoomOutShortcut, &QShortcut::activated, this, [this]() {
            int newZoom = qMax(25, m_currentZoomLevel - 10);
            m_currentZoomLevel = newZoom;
            emit zoomLevelChanged(newZoom);
        });

        QShortcut *zoomResetShortcut = new QShortcut(QKeySequence("Ctrl+0"), m_mainWindow);
        connect(zoomResetShortcut, &QShortcut::activated, this, [this]() {
            m_currentZoomLevel = 100;
            emit zoomLevelChanged(100);
        });
    }


    void installToolbarEventFilters() {
        m_urlToolbar->installEventFilter(this);
        m_tabToolbar->installEventFilter(this);

        QTimer::singleShot(100, this, [this]() {
            checkToolbarOrientation();
            checkTabToolbarOrientation();
        });
    }

    void checkToolbarOrientation() {
        Qt::ToolBarArea urlArea = m_mainWindow->toolBarArea(m_urlToolbar);
        handleSearchBarOrientation(urlArea);
    }


    void checkTabToolbarOrientation() {
        Qt::ToolBarArea tabArea = m_mainWindow->toolBarArea(m_tabToolbar);
        handleTabBarOrientation(tabArea);
    }

    void handleSearchBarOrientation(Qt::ToolBarArea area) {
        if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea) {

            if (m_addressBarAction) m_addressBarAction->setVisible(false);
            if (m_searchEngineComboAction) m_searchEngineComboAction->setVisible(false);
            if (m_searchIconAction) m_searchIconAction->setVisible(true);
        } else {

            if (m_searchIconAction) m_searchIconAction->setVisible(false);
            if (m_addressBarAction) m_addressBarAction->setVisible(true);
            if (m_searchEngineComboAction) {
                m_searchEngineComboAction->setVisible(Settings_Backend::instance().showSearchEnginePicker());
            }
        }
    }


    void handleTabBarOrientation(Qt::ToolBarArea area) {
        bool isVertical = (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea);

        if (isVertical) {

            if (m_tabBarAction) m_tabBarAction->setVisible(false);


            if (!m_verticalTabWidget) {

                m_verticalTabWidget = new QTabWidget();
                m_verticalTabWidget->setDocumentMode(true);
                m_verticalTabWidget->setTabsClosable(true);
                m_verticalTabWidget->setMovable(true);
                m_verticalTabWidget->setUsesScrollButtons(true);


                connect(m_verticalTabWidget, &QTabWidget::tabCloseRequested, this, &Create_Toolbars::tabCloseRequested);
                connect(m_verticalTabWidget, &QTabWidget::currentChanged, this, &Create_Toolbars::tabChanged);


                connect(m_verticalTabWidget->tabBar(), &QTabBar::tabMoved, this, [this](int from, int to) {
                    emit tabMoved(from, to);
                });

                m_verticalTabWidgetAction = m_tabToolbar->addWidget(m_verticalTabWidget);
            }

            if (m_verticalTabWidgetAction) m_verticalTabWidgetAction->setVisible(true);


            if (area == Qt::LeftToolBarArea) {
                m_verticalTabWidget->setTabPosition(QTabWidget::West);
            } else {
                m_verticalTabWidget->setTabPosition(QTabWidget::East);
            }


            syncTabsToVertical();
        } else {

            if (m_tabBarAction) m_tabBarAction->setVisible(true);


            if (m_verticalTabWidgetAction) m_verticalTabWidgetAction->setVisible(false);


            if (m_verticalTabWidget && m_tabBar) {
                int verticalIndex = m_verticalTabWidget->currentIndex();
                if (verticalIndex >= 0 && verticalIndex < m_tabBar->count()) {
                    m_tabBar->setCurrentIndex(verticalIndex);
                }
            }
        }
    }


    void syncTabsToVertical() {
        if (!m_verticalTabWidget || !m_tabBar) return;


        m_verticalTabWidget->blockSignals(true);


        while (m_verticalTabWidget->count() > 0) {
            m_verticalTabWidget->removeTab(0);
        }


        for (int i = 0; i < m_tabBar->count(); ++i) {

            QWidget *placeholder = new QWidget();
            m_verticalTabWidget->addTab(placeholder, m_tabBar->tabIcon(i), m_tabBar->tabText(i));
            m_verticalTabWidget->setTabToolTip(i, m_tabBar->tabToolTip(i));
        }


        m_verticalTabWidget->setCurrentIndex(m_tabBar->currentIndex());

        m_verticalTabWidget->blockSignals(false);
    }


    void setupTabSyncing() {
        if (!m_tabBar) return;


        connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
            setVerticalCurrentTab(index);
        });


        if (m_verticalTabWidget) {
            connect(m_verticalTabWidget, &QTabWidget::currentChanged, this, [this](int index) {
                if (m_tabBar && index >= 0 && index < m_tabBar->count()) {
                    m_tabBar->blockSignals(true);
                    m_tabBar->setCurrentIndex(index);
                    m_tabBar->blockSignals(false);
                }
            });
        }
    }

    void createZoomControl() {
        m_zoomButton = new QToolButton();
        m_zoomButton->setIcon(QIcon::fromTheme("zoom-in"));
        m_zoomButton->setPopupMode(QToolButton::InstantPopup);
        m_zoomButton->setToolTip("Zoom (Ctrl +/- to adjust)");

        QWidget *zoomPopup = new QWidget();
        QVBoxLayout *popupLayout = new QVBoxLayout(zoomPopup);
        popupLayout->setContentsMargins(10, 10, 10, 10);

        QLabel *titleLabel = new QLabel("Zoom Level");
        titleLabel->setStyleSheet("font-weight: bold;");
        popupLayout->addWidget(titleLabel);

        QSlider *zoomSlider = new QSlider(Qt::Horizontal);
        zoomSlider->setRange(25, 500);
        zoomSlider->setValue(100);
        zoomSlider->setTickPosition(QSlider::TicksBelow);
        zoomSlider->setTickInterval(25);
        zoomSlider->setMinimumWidth(200);

        QLabel *sliderLabel = new QLabel("100%");
        sliderLabel->setAlignment(Qt::AlignCenter);
        sliderLabel->setStyleSheet("font-size: 14px;");

        QPushButton *resetButton = new QPushButton("Reset to 100%");
        resetButton->setIcon(QIcon::fromTheme("view-refresh"));

        popupLayout->addWidget(sliderLabel);
        popupLayout->addWidget(zoomSlider);
        popupLayout->addSpacing(10);
        popupLayout->addWidget(resetButton);

        connect(zoomSlider, &QSlider::valueChanged, this, [this, sliderLabel](int value) {
            sliderLabel->setText(QString("%1%").arg(value));
            m_currentZoomLevel = value;
            emit zoomLevelChanged(value);
        });

        connect(resetButton, &QPushButton::clicked, this, [zoomSlider]() {
            zoomSlider->setValue(100);
        });

        connect(this, &Create_Toolbars::zoomLevelChanged, zoomSlider, [zoomSlider](int value) {
            zoomSlider->blockSignals(true);
            zoomSlider->setValue(value);
            zoomSlider->blockSignals(false);
        });

        QMenu *zoomMenu = new QMenu(m_zoomButton);
        QWidgetAction *widgetAction = new QWidgetAction(zoomMenu);
        widgetAction->setDefaultWidget(zoomPopup);
        zoomMenu->addAction(widgetAction);

        m_zoomButton->setMenu(zoomMenu);
        m_zoomAction = m_navToolbar->addWidget(m_zoomButton);
    }

    void createVolumeControl() {
        QToolButton *volumeButton = new QToolButton();
        volumeButton->setIcon(QIcon::fromTheme("audio-volume-high"));
        volumeButton->setPopupMode(QToolButton::InstantPopup);
        volumeButton->setToolTip("Volume");

        QWidget *volumePopup = new QWidget();
        QVBoxLayout *popupLayout = new QVBoxLayout(volumePopup);
        popupLayout->setContentsMargins(10, 10, 10, 10);

        QLabel *titleLabel = new QLabel("Volume");
        titleLabel->setStyleSheet("font-weight: bold;");
        popupLayout->addWidget(titleLabel);

        m_volumeSlider = new QSlider(Qt::Horizontal);
        m_volumeSlider->setRange(0, 100);
        m_volumeSlider->setValue(Settings_Backend::instance().audioVolume());
        m_volumeSlider->setTickPosition(QSlider::TicksBelow);
        m_volumeSlider->setTickInterval(10);
        m_volumeSlider->setMinimumWidth(150);

        QLabel *volumeLabel = new QLabel(QString("%1%").arg(m_volumeSlider->value()));
        volumeLabel->setAlignment(Qt::AlignCenter);
        volumeLabel->setStyleSheet("font-size: 14px;");

        popupLayout->addWidget(volumeLabel);
        popupLayout->addWidget(m_volumeSlider);

        connect(m_volumeSlider, &QSlider::valueChanged, this, [this, volumeLabel, volumeButton](int value) {
            volumeLabel->setText(QString("%1%").arg(value));

            QIcon icon;
            if (value == 0) {
                icon = QIcon::fromTheme("audio-volume-muted");
            } else if (value < 33) {
                icon = QIcon::fromTheme("audio-volume-low");
            } else if (value < 66) {
                icon = QIcon::fromTheme("audio-volume-medium");
            } else {
                icon = QIcon::fromTheme("audio-volume-high");
            }
            volumeButton->setIcon(icon);
            volumeButton->setToolTip(QString("Volume: %1%").arg(value));

            emit volumeChanged(value);
        });

        QMenu *volumeMenu = new QMenu(volumeButton);
        QWidgetAction *widgetAction = new QWidgetAction(volumeMenu);
        widgetAction->setDefaultWidget(volumePopup);
        volumeMenu->addAction(widgetAction);

        volumeButton->setMenu(volumeMenu);
        m_volumeAction = m_navToolbar->addWidget(volumeButton);
    }

    void createSearchIconButton() {
        m_searchIconButton = new QToolButton();
        m_searchIconButton->setIcon(QIcon::fromTheme("edit-find"));
        m_searchIconButton->setToolTip("Search");
        m_searchIconButton->setPopupMode(QToolButton::InstantPopup);

        QMenu *searchMenu = new QMenu(m_searchIconButton);

        QWidget *searchWidget = new QWidget();
        QVBoxLayout *searchLayout = new QVBoxLayout(searchWidget);
        searchLayout->setContentsMargins(10, 10, 10, 10);

        QLabel *engineLabel = new QLabel("Search Engine:");
        engineLabel->setStyleSheet("font-weight: bold;");


        m_verticalSearchEngineCombo = new QComboBox();
        auto engines = Settings_Backend::instance().searchEngines();
        for (const auto &v : engines) {
            auto m = v.toMap();
            m_verticalSearchEngineCombo->addItem(
                QIcon::fromTheme(m["icon"].toString(), QIcon::fromTheme("edit-find")),
                m["name"].toString(),
                m["url"].toString()
            );
        }
        m_verticalSearchEngineCombo->setCurrentIndex(Settings_Backend::instance().currentSearchEngineIndex());
        m_verticalSearchEngineCombo->setMinimumWidth(200);

        m_verticalAddressBar = new QLineEdit();
        m_verticalAddressBar->setPlaceholderText("Search or enter address...");
        m_verticalAddressBar->setClearButtonEnabled(true);
        m_verticalAddressBar->setMinimumWidth(250);

        searchLayout->addWidget(engineLabel);
        searchLayout->addWidget(m_verticalSearchEngineCombo);
        searchLayout->addSpacing(5);
        searchLayout->addWidget(new QLabel("Query:"));
        searchLayout->addWidget(m_verticalAddressBar);


        connect(m_verticalAddressBar, &QLineEdit::returnPressed, this, [this, searchMenu]() {
            QString t = m_verticalAddressBar->text().trimmed();
            if (t.isEmpty()) return;
            if (t.contains('.') || t.contains("://"))
                emit navigateRequested(QUrl::fromUserInput(t));
            else
                emit navigateRequested(QUrl(m_verticalSearchEngineCombo->currentData().toString()
                                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(t)))));
            searchMenu->hide();
            m_verticalAddressBar->clear();
        });


        connect(m_verticalSearchEngineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index) {
                    Settings_Backend::instance().setCurrentSearchEngineIndex(index);
                    if (m_searchEngineCombo) {
                        m_searchEngineCombo->setCurrentIndex(index);
                    }
                });


        connect(m_searchEngineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index) {
                    if (m_verticalSearchEngineCombo) {
                        m_verticalSearchEngineCombo->blockSignals(true);
                        m_verticalSearchEngineCombo->setCurrentIndex(index);
                        m_verticalSearchEngineCombo->blockSignals(false);
                    }
                });

        QWidgetAction *widgetAction = new QWidgetAction(searchMenu);
        widgetAction->setDefaultWidget(searchWidget);
        searchMenu->addAction(widgetAction);

        m_searchIconButton->setMenu(searchMenu);
        m_searchIconAction = m_urlToolbar->addWidget(m_searchIconButton);
        m_searchIconAction->setVisible(false);
    }

    void createExtendedHamburgerMenu() {
        if (!m_menuButton) return;

        QMenu *mainMenu = new QMenu(m_menuButton);

        mainMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::WindowNew), "New Tab",
                            this, &Create_Toolbars::newTabRequested);
        mainMenu->addSeparator();

        QMenu *onuSitesMenu = mainMenu->addMenu(QIcon::fromTheme("folder-symbolic"), "Onu Sites");
        onuSitesMenu->addAction(QIcon::fromTheme("go-home"), "Home", this, [this]() {
            emit Open_InNewTab_tb(QUrl("onu://home"), false);
        });
        onuSitesMenu->addAction(QIcon::fromTheme("input-gaming"), "Game", this, [this]() {
            emit Open_InNewTab_tb(QUrl("onu://game"), false);
        });
        onuSitesMenu->addAction(QIcon::fromTheme("dialog-information"), "About", this, [this]() {
            emit Open_InNewTab_tb(QUrl("onu://about"), false);
        });
        #ifdef CMAKE_DEBUG
        onuSitesMenu->addAction(QIcon::fromTheme("applications-science"), "Test", this, [this]() {
            emit Open_InNewTab_tb(QUrl("onu://test"), false);
        });
        #endif

        mainMenu->addSeparator();

        QMenu *toolbarsMenu = mainMenu->addMenu(QIcon::fromTheme("view-list-icons"), "Toolbars");

        auto addToolbarToggle = [this, toolbarsMenu](const QString &label, const QString &toolbarName, QToolBar *toolbar) {
            QAction *act = toolbarsMenu->addAction(label);
            act->setCheckable(true);
            act->setChecked(Settings_Backend::instance().toolbarVisible(toolbarName));
            connect(act, &QAction::toggled, this, [this, toolbarName, toolbar](bool checked) {
                Settings_Backend::instance().setToolbarVisible(toolbarName, checked);
                if (toolbar) toolbar->setVisible(checked);
            });
        };

        addToolbarToggle("Navigation Toolbar", "Navigation", m_navToolbar);
        addToolbarToggle("Search Toolbar", "Search", m_urlToolbar);
        addToolbarToggle("Favorites Toolbar", "Favorites", m_favToolbar);
        addToolbarToggle("Recent Toolbar", "Recent", m_recToolbar);
        addToolbarToggle("Tab Bar", "Tab", m_tabToolbar);

        mainMenu->addSeparator();

        QMenu *favsMenu = mainMenu->addMenu(QIcon::fromTheme("bookmark-new"), "Favorites");
        connect(favsMenu, &QMenu::aboutToShow, this, [this, favsMenu]() {
            favsMenu->clear();
            for (const QVariant &v : m_favoritesData) {
                QVariantMap fav = v.toMap();
                QAction *act = favsMenu->addAction(
                    Icon_Man::instance().loadIcon(fav["iconPath"].toString()),
                    fav["title"].toString()
                    );
                QUrl url = fav["url"].toUrl();
                connect(act, &QAction::triggered, this, [this, url]() {
                    emit Open_InNewTab_tb(url, false);
                });
            }
            if (m_favoritesData.isEmpty()) {
                QAction *emptyAct = favsMenu->addAction("No favorites");
                emptyAct->setEnabled(false);
            }
            favsMenu->addSeparator();
            favsMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties),
                                "Manage Favorites...", this, [this]() {
                                    emit Open_InNewTab_tb(QUrl(), true);
                                });
        });

        QMenu *recentsMenu = mainMenu->addMenu(QIcon::fromTheme("document-open-recent"), "Recent");
        connect(recentsMenu, &QMenu::aboutToShow, this, [this, recentsMenu]() {
            recentsMenu->clear();
            int count = qMin(10, (int)m_historyData.size());
            for (int i = 0; i < count; ++i) {
                QVariantMap item = m_historyData[i].toMap();
                QAction *act = recentsMenu->addAction(
                    Icon_Man::instance().loadIcon(item["iconPath"].toString()),
                    item["title"].toString()
                    );
                QUrl url = item["url"].toUrl();
                connect(act, &QAction::triggered, this, [this, url]() {
                    emit Open_InNewTab_tb(url, false);
                });
            }
            if (m_historyData.isEmpty()) {
                QAction *emptyAct = recentsMenu->addAction("No recent history");
                emptyAct->setEnabled(false);
            }
            recentsMenu->addSeparator();
            recentsMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpenRecent),
                                   "View History...", this, &Create_Toolbars::showHistoryRequested);
        });

        mainMenu->addSeparator();
        mainMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen), "Downloads",
                            this, &Create_Toolbars::showDownloadsRequested);
        mainMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties), "Settings",
                            this, &Create_Toolbars::showSettingsRequested);
        mainMenu->addSeparator();
        mainMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::ApplicationExit), "Quit",
                            this, [this]() {
                                emit quitRequested();
                            });

        m_menuButton->setMenu(mainMenu);
    }

    void showToolbarContextMenu(QToolBar* toolbar, const QPoint &pos) {
        auto &s = Settings_Backend::instance();
        QMenu menu;
        QString tName = toolbar->objectName().remove("Toolbar");

        QAction *vAct = menu.addAction("Visible");
        vAct->setCheckable(true);
        vAct->setChecked(toolbar->isVisible());
        connect(vAct, &QAction::toggled, this, [toolbar, tName](bool checked) {
            toolbar->setVisible(checked);
            Settings_Backend::instance().setToolbarVisible(tName, checked);
        });

        QAction *lAct = menu.addAction("Lock Position");
        lAct->setCheckable(true);
        lAct->setChecked(!toolbar->isMovable());
        connect(lAct, &QAction::toggled, this, [toolbar, tName](bool checked) {
            toolbar->setMovable(!checked);
            Settings_Backend::instance().setToolbarLockDragging(tName, checked);
        });

        menu.addSeparator();

        if (toolbar == m_navToolbar) {
            QMenu *navMenu = menu.addMenu("Button Visibility");

            QAction *showReloadAct = navMenu->addAction("Show Reload Button");
            showReloadAct->setCheckable(true);
            showReloadAct->setChecked(s.showReload());
            connect(showReloadAct, &QAction::toggled, this, [this](bool checked) {
                Settings_Backend::instance().setPublicValue("navigation/showReload", checked);
                applyNavToolbarVisibility();
            });

            QAction *showForwardAct = navMenu->addAction("Show Forward Button");
            showForwardAct->setCheckable(true);
            showForwardAct->setChecked(s.showForward());
            connect(showForwardAct, &QAction::toggled, this, [this](bool checked) {
                Settings_Backend::instance().setPublicValue("navigation/showForward", checked);
                applyNavToolbarVisibility();
            });

            QAction *showMenuAct = navMenu->addAction("Show Menu Button");
            showMenuAct->setCheckable(true);
            showMenuAct->setChecked(s.showMenu());
            connect(showMenuAct, &QAction::toggled, this, [this](bool checked) {
                Settings_Backend::instance().setPublicValue("navigation/showMenu", checked);
                applyNavToolbarVisibility();
            });

            QAction *showVolumeAct = navMenu->addAction("Show Volume Button");
            showVolumeAct->setCheckable(true);
            showVolumeAct->setChecked(s.showVolumeSlider());
            connect(showVolumeAct, &QAction::toggled, this, [this](bool checked) {
                Settings_Backend::instance().setPublicValue("navigation/showVolume", checked);
                applyNavToolbarVisibility();
            });

            menu.addAction("Set Icon Size...", [this](){
                bool ok;
                int size = QInputDialog::getInt(m_mainWindow, "Icon Size", "Pixels:",
                                                Settings_Backend::instance().navigationIconSize(),
                                                16, 64, 1, &ok);
                if(ok) {
                    Settings_Backend::instance().setPublicValue("navigation/iconSize", size);
                    m_navToolbar->setIconSize(QSize(size, size));
                }
            });
        }
        else if (toolbar == m_urlToolbar) {
            bool pickerVis = s.showSearchEnginePicker();

            QAction *pickerAction = menu.addAction("Show Search Engine Picker");
            pickerAction->setCheckable(true);
            pickerAction->setChecked(pickerVis);

            connect(pickerAction, &QAction::toggled, this, [this](bool checked) {
                Settings_Backend::instance().setShowSearchEnginePicker(checked);
                if (m_searchEngineComboAction) {
                    m_searchEngineComboAction->setVisible(checked);
                }
            });

            QMenu *engMenu = menu.addMenu("Default Search Engine");
            auto engines = s.searchEngines();
            int currentIndex = s.currentSearchEngineIndex();

            for(int i=0; i<engines.size(); ++i) {
                QAction *act = engMenu->addAction(engines[i].toMap()["name"].toString(), [this, i](){
                    m_searchEngineCombo->setCurrentIndex(i);
                    Settings_Backend::instance().setCurrentSearchEngineIndex(i);
                });
                act->setCheckable(true);
                act->setChecked(i == currentIndex);
            }
        }
        else if (toolbar == m_recToolbar || toolbar == m_favToolbar) {
            bool isRec = (toolbar == m_recToolbar);

            QMenu *limitMenu = menu.addMenu("Items Limit");
            QList<int> limits = {3, 5, 10, 15, 20, 30};
            int currentLimit = isRec ? s.historySidebarLimit() : s.favSidebarLimit();

            for(int l : limits) {
                QAction *act = limitMenu->addAction(QString::number(l), [this, isRec, l](){
                    if(isRec) Settings_Backend::instance().setHistorySidebarLimit(l);
                    else Settings_Backend::instance().setFavSidebarLimit(l);
                    isRec ? refreshRecentToolbar(m_historyData) : refreshFavToolbar(m_favoritesData);
                });
                act->setCheckable(true);
                act->setChecked(l == currentLimit);
            }

            menu.addAction("Set Icon Size...", [this, toolbar](){
                bool ok;
                int size = QInputDialog::getInt(m_mainWindow, "Icon Size", "Pixels:",
                                                toolbar->iconSize().width(), 16, 64, 1, &ok);
                if(ok) toolbar->setIconSize(QSize(size, size));
            });
        }
        else if (toolbar == m_tabToolbar) {
            menu.addAction("Set Icon Size...", [this](){
                bool ok;
                int size = QInputDialog::getInt(m_mainWindow, "Tab Icon Size", "Pixels:",
                                                m_tabToolbar->iconSize().width(), 16, 48, 1, &ok);
                if(ok) m_tabToolbar->setIconSize(QSize(size, size));
            });
        }

        menu.exec(pos);
    }

    void setupSuggestionCompleter() {
        m_suggestionModel = new QStandardItemModel(this);
        m_suggestionCompleter = new QCompleter(m_suggestionModel, this);
        m_suggestionCompleter->setFilterMode(Qt::MatchContains);
        m_suggestionCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        m_suggestionCompleter->setCompletionMode(QCompleter::PopupCompletion);

        m_addressBar->setCompleter(m_suggestionCompleter);

        connect(m_addressBar, &QLineEdit::textEdited, this, [this](const QString &text) {
            if (text.length() >= 2) refreshSuggestions();
            else m_suggestionModel->clear();
        });

        connect(m_suggestionCompleter, QOverload<const QModelIndex &>::of(&QCompleter::activated),
                this, [this](const QModelIndex &index) {
                    QString url = index.data(Qt::DisplayRole).toString();
                    if (url.contains("://"))
                        emit navigateRequested(QUrl(url));
                    else
                        emit navigateRequested(QUrl(m_searchEngineCombo->currentData().toString()
                                                        .arg(QString::fromUtf8(QUrl::toPercentEncoding(url)))));
                });
    }

    void refreshSuggestions() {
        QString query = m_addressBar->text().trimmed();
        m_suggestionModel->clear();
        auto &s = Settings_Backend::instance();

        auto addBatch = [&](const QVariantList &data, QIcon::ThemeIcon themeIco) {
            int found = 0;
            for (const QVariant &v : data) {
                QVariantMap m = v.toMap();
                QString url = m["url"].toString();
                QString title = m["title"].toString();
                if (url.contains(query, Qt::CaseInsensitive) || title.contains(query, Qt::CaseInsensitive)) {
                    QStandardItem *item = new QStandardItem(QIcon::fromTheme(themeIco), url);
                    m_suggestionModel->appendRow(item);
                    if (++found > 5) break;
                }
            }
        };

        if (s.suggestionsFromHistory())
            addBatch(m_historyData, QIcon::ThemeIcon::DocumentOpenRecent);
        if (s.suggestionsFromFavorites())
            addBatch(m_favoritesData, QIcon::ThemeIcon::AudioCard);

        if (s.suggestionsFromRemote() && !query.contains("://"))
            fetchRemoteSuggestions(query);
    }

    void fetchRemoteSuggestions(const QString &query) {
        if (m_currentSuggestionReply) {
            m_currentSuggestionReply->abort();
            m_currentSuggestionReply->deleteLater();
            m_currentSuggestionReply = nullptr;
        }

        QUrl url(Settings_Backend::instance().suggestionApiUrl().arg(QString::fromUtf8(QUrl::toPercentEncoding(query))));
        QNetworkRequest req(url);
        req.setRawHeader("User-Agent", "OnuBrowser/1.1");
        req.setTransferTimeout(3000);

        m_currentSuggestionReply = m_networkManager->get(req);

        connect(m_currentSuggestionReply, &QNetworkReply::finished, this, [this]() {
            if (!m_currentSuggestionReply) return;

            if (m_currentSuggestionReply->error() != QNetworkReply::NoError) {
                m_currentSuggestionReply->deleteLater();
                m_currentSuggestionReply = nullptr;
                return;
            }

            QString response = QString::fromUtf8(m_currentSuggestionReply->readAll());

            QJSEngine engine;
            engine.globalObject().setProperty("response", response);
            engine.globalObject().setProperty("query", m_addressBar->text().trimmed());
            QJSValue res = engine.evaluate(Settings_Backend::instance().suggestionParserScript());

            if (res.isArray()) {
                int len = res.property("length").toInt();
                for (int i=0; i<len && i<8; ++i) {
                    QString suggestion = res.property(i).toString();
                    if (!suggestion.isEmpty()) {
                        QStandardItem *item = new QStandardItem(
                            QIcon::fromTheme("applications-internet"),
                            suggestion
                            );
                        m_suggestionModel->appendRow(item);
                    }
                }
            }

            m_currentSuggestionReply->deleteLater();
            m_currentSuggestionReply = nullptr;
        });
    }

    void navbar() {
        m_navToolbar = m_mainWindow->addToolBar("Navigation");
        m_navToolbar->setObjectName("NavigationToolbar");
        m_navToolbar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_navToolbar, &QToolBar::customContextMenuRequested, this, [this](const QPoint &p){
            showToolbarContextMenu(m_navToolbar, m_navToolbar->mapToGlobal(p));
        });

        auto makeBtn = [&](QIcon::ThemeIcon ico, const QString &tip, bool back) {
            QToolButton *b = new QToolButton();
            b->setIcon(QIcon::fromTheme(ico));
            b->setToolTip(tip);
            b->setPopupMode(QToolButton::DelayedPopup);
            QMenu *m = new QMenu(b);
            b->setMenu(m);
            connect(b, &QToolButton::clicked, this, back ? &Create_Toolbars::backRequested : &Create_Toolbars::forwardRequested);
            connect(m, &QMenu::aboutToShow, [this, m, back]{
                m->clear();
                back ? emit populateBackMenu(m) : emit populateForwardMenu(m);
            });
            return b;
        };

        m_backAction = m_navToolbar->addWidget(makeBtn(QIcon::ThemeIcon::GoPrevious, "Back", true));

        m_forwardButton = makeBtn(QIcon::ThemeIcon::GoNext, "Forward", false);
        m_forwardAction = m_navToolbar->addWidget(m_forwardButton);

        m_reloadAction = m_navToolbar->addAction(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh), "Reload", this, &Create_Toolbars::reloadRequested);
        m_stopAction = m_navToolbar->addAction(QIcon::fromTheme(QIcon::ThemeIcon::ProcessStop), "Stop", this, &Create_Toolbars::stopRequested);
        m_stopAction->setVisible(false);

        m_findAction = m_navToolbar->addAction(QIcon::fromTheme("edit-find-replace"), "Find", [this]{
            m_findBar->show();
            m_findEdit->setFocus();
        });
        createZoomControl();
        createVolumeControl();
        m_menuButton = new QToolButton();
        m_menuButton->setIcon(QIcon::fromTheme("open-menu-symbolic", QIcon::fromTheme("application-menu")));
        m_menuButton->setToolTip("Menu");
        m_menuButton->setPopupMode(QToolButton::InstantPopup);
        createExtendedHamburgerMenu();

        m_navToolbar->addWidget(m_menuButton);
    }

    void urlbar() {
        m_urlToolbar = m_mainWindow->addToolBar("Search");
        m_urlToolbar->setObjectName("SearchToolbar");
        m_urlToolbar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_urlToolbar, &QToolBar::customContextMenuRequested, this, [this](const QPoint &p){
            showToolbarContextMenu(m_urlToolbar, m_urlToolbar->mapToGlobal(p));
        });

        m_searchEngineCombo = new QComboBox();
        m_searchEngineCombo->setFixedWidth(130);
        m_searchEngineComboAction = m_urlToolbar->addWidget(m_searchEngineCombo);

        m_addressBar = new QLineEdit();
        m_addressBar->setPlaceholderText("Search or enter address...");
        m_addressBar->setClearButtonEnabled(true);
        m_addressBarAction = m_urlToolbar->addWidget(m_addressBar);

        setupSuggestionCompleter();
        createSearchIconButton();

        connect(m_searchEngineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index) {
                    Settings_Backend::instance().setCurrentSearchEngineIndex(index);
                });

        connect(m_addressBar, &QLineEdit::returnPressed, [this]{
            QString t = m_addressBar->text().trimmed();
            if (t.isEmpty()) return;
            if (t.contains('.') || t.contains("://"))
                emit navigateRequested(QUrl::fromUserInput(t));
            else
                emit navigateRequested(QUrl(m_searchEngineCombo->currentData().toString()
                                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(t)))));
        });
    }

    void tabbar() {
        m_tabToolbar = m_mainWindow->addToolBar("Tabs");
        m_tabToolbar->setObjectName("TabToolbar");
        m_tabToolbar->setContextMenuPolicy(Qt::CustomContextMenu);
        m_tabToolbar->setAcceptDrops(true);
        m_tabToolbar->installEventFilter(this);

        connect(m_tabToolbar, &QToolBar::customContextMenuRequested, this, [this](const QPoint &p){
            showToolbarContextMenu(m_tabToolbar, m_tabToolbar->mapToGlobal(p));
        });

        m_tabBar = new QTabBar();
        m_tabBar->setTabsClosable(true);
        m_tabBar->setMovable(true);
        m_tabBar->setExpanding(true);
        m_tabBar->setElideMode(Qt::ElideMiddle);
        m_tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
        m_tabBar->setDrawBase(false);
        m_tabBar->setAcceptDrops(true);
        m_tabBar->installEventFilter(this);
        m_tabBarAction = m_tabToolbar->addWidget(m_tabBar);

        connect(m_tabBar, &QTabBar::tabCloseRequested, this, &Create_Toolbars::tabCloseRequested);
        connect(m_tabBar, &QTabBar::currentChanged, this, &Create_Toolbars::tabChanged);

        connect(m_tabBar, &QTabBar::tabMoved, this, [this](int from, int to) {
            emit tabMoved(from, to);
        });

        connect(m_tabBar, &QTabBar::customContextMenuRequested, [this](const QPoint &p){
            int i = m_tabBar->tabAt(p);
            if (i >= 0) emit tabContextMenuRequested(i, m_tabBar->mapToGlobal(p));
        });


        connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
            setVerticalCurrentTab(index);
        });

        m_tabToolbar->addAction(QIcon::fromTheme("list-add"), "", this, &Create_Toolbars::newTabRequested);
    }

    void recbar() {
        m_recToolbar = new QToolBar("Recent");
        m_recToolbar->setObjectName("RecentToolbar");
        m_recToolbar->setContextMenuPolicy(Qt::CustomContextMenu);
        m_mainWindow->addToolBar(Qt::LeftToolBarArea, m_recToolbar);

        connect(m_recToolbar, &QToolBar::customContextMenuRequested, this, [this](const QPoint &p){
            showToolbarContextMenu(m_recToolbar, m_recToolbar->mapToGlobal(p));
        });

        connect(m_recToolbar, &QToolBar::actionTriggered, [this](QAction *a){
            if(a->data().toString() == "manage")
                emit showHistoryRequested();
            else
                emit recentActionTriggered(a->data().toUrl(), false);
        });
    }

    void favbar() {
        m_favToolbar = new QToolBar("Favorites");
        m_favToolbar->setObjectName("FavoritesToolbar");
        m_favToolbar->setContextMenuPolicy(Qt::CustomContextMenu);
        m_favToolbar->setAcceptDrops(true);
        m_favToolbar->installEventFilter(this);
        m_mainWindow->addToolBar(Qt::LeftToolBarArea, m_favToolbar);

        connect(m_favToolbar, &QToolBar::customContextMenuRequested, this, [this](const QPoint &p){
            showToolbarContextMenu(m_favToolbar, m_favToolbar->mapToGlobal(p));
        });

        connect(m_favToolbar, &QToolBar::actionTriggered, [this](QAction *a){
            if(a->data().toString() == "customize")
                emit Open_InNewTab_tb(QUrl(), true);
            else
                emit Open_InNewTab_tb(a->data().toUrl(), false);
        });
    }
    void createFindBar() {
        m_findBar = new QWidget();
        m_findBar->setFixedHeight(35);
        QHBoxLayout *l = new QHBoxLayout(m_findBar);
        l->setContentsMargins(5,2,5,2);

        m_findEdit = new QLineEdit();
        m_findEdit->setPlaceholderText("Find...");
        l->addWidget(m_findEdit);

        connect(m_findEdit, &QLineEdit::textChanged, this, &Create_Toolbars::findTextChanged);
        connect(m_findEdit, &QLineEdit::returnPressed, this, &Create_Toolbars::findNext);

        QPushButton *prevBtn = new QPushButton();
        prevBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoPrevious));
        prevBtn->setFixedWidth(25);
        connect(prevBtn, &QPushButton::clicked, this, &Create_Toolbars::findPrevious);
        l->addWidget(prevBtn);

        QPushButton *nextBtn = new QPushButton();
        nextBtn->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoNext));
        nextBtn->setFixedWidth(25);
        connect(nextBtn, &QPushButton::clicked, this, &Create_Toolbars::findNext);
        l->addWidget(nextBtn);

        QPushButton *c = new QPushButton();
        c->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::WindowClose));
        c->setFixedWidth(25);
        l->addWidget(c);
        connect(c, &QPushButton::clicked, m_findBar, &QWidget::hide);

        m_findBar->hide();
    }
    void updateSearchEngineCombo() {
        if (!m_searchEngineCombo) return;
        m_searchEngineCombo->blockSignals(true);
        m_searchEngineCombo->clear();

        auto engines = Settings_Backend::instance().searchEngines();
        for (const auto &v : engines) {
            auto m = v.toMap();
            m_searchEngineCombo->addItem(
                QIcon::fromTheme(m["icon"].toString(), QIcon::fromTheme("edit-find")),
                m["name"].toString(),
                m["url"].toString()
                );
        }

        m_searchEngineCombo->setCurrentIndex(Settings_Backend::instance().currentSearchEngineIndex());
        m_searchEngineCombo->blockSignals(false);
    }


    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == m_urlToolbar) {
            if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
                QTimer::singleShot(50, this, [this]() {
                    checkToolbarOrientation();
                });
            }
        }


        if (obj == m_tabToolbar) {
            if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
                QTimer::singleShot(50, this, [this]() {
                    checkTabToolbarOrientation();
                });
            }
        }

        if ((obj == m_tabToolbar || obj == m_tabBar) && event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasText() || dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        else if ((obj == m_tabToolbar || obj == m_tabBar) && event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);

            QUrl url;
            if (dropEvent->mimeData()->hasUrls()) {
                QList<QUrl> urls = dropEvent->mimeData()->urls();
                if (!urls.isEmpty()) {
                    url = urls.first();
                }
            }
            else if (dropEvent->mimeData()->hasText()) {
                QString text = dropEvent->mimeData()->text().trimmed();
                if (!text.isEmpty()) {
                    if (text.contains('.') || text.contains("://")) {
                        url = QUrl::fromUserInput(text);
                    } else {
                        url = QUrl(m_searchEngineCombo->currentData().toString()
                                       .arg(QString::fromUtf8(QUrl::toPercentEncoding(text))));
                    }
                }
            }

            if (url.isValid()) {

                emit Open_InNewTab_tb(url, false);
                dropEvent->acceptProposedAction();
                return true;
            }
        }
        else if (obj == m_favToolbar && event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasText() || dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        else if (obj == m_favToolbar && event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);

            QString title = "Bookmark";
            QUrl url;

            if (dropEvent->mimeData()->hasUrls()) {
                QList<QUrl> urls = dropEvent->mimeData()->urls();
                if (!urls.isEmpty()) {
                    url = urls.first();
                    title = url.host();
                    if (title.isEmpty()) title = url.toString();
                }
            }
            else if (dropEvent->mimeData()->hasText()) {
                QString text = dropEvent->mimeData()->text().trimmed();
                if (!text.isEmpty()) {
                    if (text.contains('.') || text.contains("://")) {
                        url = QUrl::fromUserInput(text);
                        title = url.host();
                        if (title.isEmpty()) title = text;
                    } else {
                        url = QUrl(m_searchEngineCombo->currentData().toString()
                                       .arg(QString::fromUtf8(QUrl::toPercentEncoding(text))));
                        title = text;
                    }
                }
            }

            if (url.isValid()) {
                emit addUrlToFavorites(url, title);
            }

            dropEvent->acceptProposedAction();
            return true;
        }

        return QObject::eventFilter(obj, event);
    }
};


class Onu_Web : public QWebEnginePage {
    Q_OBJECT
public:
    explicit Onu_Web(QWebEngineProfile *profile, QObject *parent = nullptr)
        : QWebEnginePage(profile, parent), m_menuActive(false)
    {
        connect(this, &QWebEnginePage::permissionRequested,
                this, &Onu_Web::onPermissionRequested);

        connect(this, &QWebEnginePage::fileSystemAccessRequested,
                this, &Onu_Web::onFileSystemAccessRequested);
    }

    static QWebEngineProfile* defaultProfile() {
        static QWebEngineProfile* profile = nullptr;
        if (!profile) {
            profile = new QWebEngineProfile("OnuBrowser");
            configureProfile(profile);
        }
        return profile;
    }

    static void configureProfile(QWebEngineProfile *profile) {
        if (!profile) return;

        auto &s = Settings_Backend::instance();

        QString storagePath = s.dataPath() + "/Webengine";
        QString cachePath = s.dataPath() + "/Cache";

        profile->setPersistentStoragePath(storagePath);
        profile->setCachePath(cachePath);

        if (s.deleteCookies()) {
            profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
            profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
        } else {
            profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
            profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
        }

        QString userAgent = profile->httpUserAgent();
        if (!userAgent.contains("Onu")) {
            userAgent += " Onu/" + QApplication::applicationVersion();
            profile->setHttpUserAgent(userAgent);
        }

        QWebEngineSettings *ws = profile->settings();
        if (!ws) return;

        ws->setAttribute(QWebEngineSettings::LocalStorageEnabled, !s.deleteCookies());
        ws->setAttribute(QWebEngineSettings::JavascriptEnabled, s.javascriptEnabled());
        ws->setAttribute(QWebEngineSettings::AutoLoadImages, s.imagesEnabled());

        /* ws->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true); soon will be added after adding its own logic as a permission api */
        ws->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
        ws->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

 }
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override {
        if (type == NavigationTypeLinkClicked && !isMainFrame) {
            QString origin = url.scheme() + "://" + url.host();
            int state = Settings_Backend::instance().getSitePermission(origin, "popups");

            if (state == 2) {
                return false;
            } else if (state == 1) {
                emit openInNewTabRequested(url);
                return false;
            } else {
                int requestId = m_nextRequestId++;
                m_pendingPopups[requestId] = url;
                m_pendingOrigins[requestId] = origin;
                m_pendingTypes[requestId] = "popups";
                QWidget *parentWidget = qobject_cast<QWidget*>(parent());

                auto callback = [this, requestId](int decision, bool remember) {
                    handlePopupResponse(requestId, decision, remember);
                };

                emit permissionPromptRequested(parentWidget, origin, "popups", requestId, callback);
                return false;
            }
        }

        if (isMainFrame && type == NavigationTypeLinkClicked) {
            emit navigationRequestAccepted(url);
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

    QWebEnginePage* createWindow(WebWindowType type) override {
        if (type != WebBrowserTab) {
            return QWebEnginePage::createWindow(type);
        }

        QString origin = this->url().scheme() + "://" + this->url().host();
        int state = Settings_Backend::instance().getSitePermission(origin, "popups");

        if (state == 2) {
            return nullptr;
        } else if (state == 1) {
            auto *tempPage = new QWebEnginePage(this);
            connect(tempPage, &QWebEnginePage::urlChanged, this, [this, tempPage](const QUrl &url) {
                emit newTabRequested(url);
                tempPage->deleteLater();
            });
            return tempPage;
        } else {
            int requestId = m_nextRequestId++;
            m_pendingPopups[requestId] = QUrl();
            m_pendingOrigins[requestId] = origin;
            m_pendingTypes[requestId] = "popups";
            QWidget *parentWidget = qobject_cast<QWidget*>(parent());

            auto callback = [this, requestId](int decision, bool remember) {
                handlePopupResponse(requestId, decision, remember);
            };

            emit permissionPromptRequested(parentWidget, origin, "popups", requestId, callback);
            return nullptr;
        }
    }

    void onPermissionRequested(QWebEnginePermission permission) {
        QUrl originUrl = permission.origin();
        QString origin = originUrl.scheme() + "://" + originUrl.host();

        QString permTypeStr;

        switch (permission.permissionType()) {
            case QWebEnginePermission::PermissionType::Geolocation:
                permTypeStr = "geolocation";
                break;
            case QWebEnginePermission::PermissionType::MediaAudioCapture:
                permTypeStr = "microphone";
                break;
            case QWebEnginePermission::PermissionType::MediaVideoCapture:
                permTypeStr = "camera";
                break;
            case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
                permTypeStr = "camera_microphone";
                break;
            case QWebEnginePermission::PermissionType::MouseLock:
                permTypeStr = "mouselock";
                break;
            case QWebEnginePermission::PermissionType::Notifications:
                permTypeStr = "notifications";
                break;
            case QWebEnginePermission::PermissionType::DesktopVideoCapture:
                permTypeStr = "desktopVideo";
                break;
            case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
                permTypeStr = "desktopAudioVideo";
                break;
            case QWebEnginePermission::PermissionType::ClipboardReadWrite:
                permTypeStr = "clipboard";
                break;
            case QWebEnginePermission::PermissionType::LocalFontsAccess:
                permTypeStr = "localFonts";
                break;
            default:
                permission.deny();
                return;
        }

        auto &s = Settings_Backend::instance();
        int state = s.getSitePermission(origin, permTypeStr);

        if (state == 1) {
            permission.grant();
            return;
        }
        if (state == 2) {
            permission.deny();
            return;
        }

        int requestId = m_nextRequestId++;
        m_pendingPermissions[requestId] = permission;
        m_pendingOrigins[requestId] = origin;
        m_pendingTypes[requestId] = permTypeStr;

        QWidget *parentWidget = qobject_cast<QWidget*>(parent());

        auto callback = [this, requestId](int decision, bool remember) {
            handlePermissionResponse(requestId, decision, remember);
        };

        emit permissionPromptRequested(parentWidget, origin, permTypeStr, requestId, callback);
    }

    void onFileSystemAccessRequested(QWebEngineFileSystemAccessRequest request) {
        QUrl originUrl = request.origin();
        QString origin = originUrl.scheme() + "://" + originUrl.host();
        QString permTypeStr = "fileAccess";

        auto &s = Settings_Backend::instance();
        int state = s.getSitePermission(origin, permTypeStr);

        QString accessType = (request.accessFlags() & QWebEngineFileSystemAccessRequest::Write) ? "write" : "read";
        QString handleType = request.handleType() == QWebEngineFileSystemAccessRequest::File ? "file" : "directory";
        QString description = QString("%1 %2 access to: %3")
            .arg(handleType)
            .arg(accessType)
            .arg(request.filePath().toString());

        if (state == 1) {
            request.accept();
            return;
        }
        if (state == 2) {
            request.reject();
            return;
        }

        int requestId = m_nextRequestId++;

        m_pendingFileAccessRequests.append({requestId, std::move(request)});
        m_pendingOrigins[requestId] = origin;
        m_pendingTypes[requestId] = permTypeStr;
        m_pendingDescriptions[requestId] = description;

        QWidget *parentWidget = qobject_cast<QWidget*>(parent());

        auto callback = [this, requestId](int decision, bool remember) {
            handleFileAccessResponse(requestId, decision, remember);
        };

        emit fileAccessPromptRequested(parentWidget, origin, description, requestId, callback);
    }

    void notification(std::unique_ptr<QWebEngineNotification> notification) {
        QUrl originUrl = notification->origin();
        QString origin = originUrl.scheme() + "://" + originUrl.host();

        QString title = notification->title();
        QString body = notification->message();
        QString iconPath;

        if (!notification->icon().isNull()) {
            iconPath = Icon_Man::instance().saveIcon(QIcon(QPixmap::fromImage(notification->icon())));
        }

        SessionNotifications::add(origin, title, body, iconPath);
        emit notificationReceived(origin, title, body, iconPath);

        m_activeNotifications[notification.get()] = origin;
        notification->show();

        connect(notification.get(), &QWebEngineNotification::closed, this, [this, notificationPtr = notification.get()]() {
            m_activeNotifications.remove(notificationPtr);
        });
    }

    void showContextMenu(QWebEngineView *view, const QPoint &globalPos) {
        if (m_menuActive) return;
        m_menuActive = true;
        auto *contextData = view->lastContextMenuRequest();
        if (!contextData) { m_menuActive = false; return; }

        QMenu menu(view);
        bool isLink = !contextData->linkUrl().isEmpty();
        bool isImage = contextData->mediaType() == QWebEngineContextMenuRequest::MediaTypeImage;
        bool isMedia = contextData->mediaType() == QWebEngineContextMenuRequest::MediaTypeVideo ||
                       contextData->mediaType() == QWebEngineContextMenuRequest::MediaTypeAudio;
        bool hasSelection = !contextData->selectedText().isEmpty();

        menu.addAction(QIcon::fromTheme("go-previous"), "Back", view, &QWebEngineView::back)
            ->setEnabled(view->history()->canGoBack());
        menu.addAction(QIcon::fromTheme("go-next"), "Forward", view, &QWebEngineView::forward)
            ->setEnabled(view->history()->canGoForward());
        menu.addAction(QIcon::fromTheme("view-refresh"), "Reload", view, &QWebEngineView::reload);
        menu.addSeparator();

        menu.addAction(QIcon::fromTheme("bookmark-new"), "Add to Favorites", [this, view]() {
            emit addToFavoritesRequested(view->title(), view->url(), view->icon());
        });

        menu.addAction(QIcon::fromTheme("text-html"), "View Page Source", [view]() {
            view->page()->toHtml([view](const QString &html) {
                QWidget *parentWidget = view->parentWidget();
                Dialogs::source(parentWidget, html, view->title());
            });
        });

        menu.addAction(QIcon::fromTheme("dialog-information"), "Site Information", [view]() {
            QString origin = view->url().scheme() + "://" + view->url().host();
            QWidget *parentWidget = view->parentWidget();
            Dialogs::siteInfo(parentWidget, origin, true);
        });
        menu.addSeparator();

        auto *copyMenu = menu.addMenu(QIcon::fromTheme("edit-copy"), "Copy");
        copyMenu->addAction("Copy URL", [view, this]() {
            QApplication::clipboard()->setText(view->url().toString());
            emit copyStatusRequested("Copied: " + view->url().toString());
        });
        copyMenu->addAction("Copy Title", [view, this]() {
            QApplication::clipboard()->setText(view->title());
            emit copyStatusRequested("Copied title: " + view->title());
        });

        if (hasSelection) {
            QString text = contextData->selectedText();
            copyMenu->addAction("Copy Selection", [text, this]() {
                QApplication::clipboard()->setText(text);
                emit copyStatusRequested("Copied: " + text.left(50) + (text.length() > 50 ? "..." : ""));
            });
        }

        if (isLink) {
            menu.addSeparator();
            QMenu *linkMenu = menu.addMenu(QIcon::fromTheme("insert-link"), "Link");
            linkMenu->addAction(QIcon::fromTheme("window-new"), "Open in New Tab", [this, contextData]() {
                emit openInNewTabRequested(contextData->linkUrl());
            });
            linkMenu->addAction("Copy Link Address", [contextData, this]() {
                QApplication::clipboard()->setText(contextData->linkUrl().toString());
                emit copyStatusRequested("Copied link: " + contextData->linkUrl().toString());
            });
        }

        if (isImage) {
            menu.addSeparator();
            QMenu *imageMenu = menu.addMenu(QIcon::fromTheme("image-x-generic"), "Image");
            imageMenu->addAction(QIcon::fromTheme("document-save"), "Save Image", [this, contextData]() {
                emit saveImageRequested(contextData->mediaUrl());
            });
            imageMenu->addAction("Copy Image Address", [contextData, this]() {
                QApplication::clipboard()->setText(contextData->mediaUrl().toString());
                emit copyStatusRequested("Copied image URL");
            });
        }

        if (isMedia) {
            menu.addSeparator();
            QMenu *mediaMenu = menu.addMenu(QIcon::fromTheme("video-x-generic"), "Media");
            mediaMenu->addAction(QIcon::fromTheme("document-save"), "Save Media", [this, contextData]() {
                emit saveMediaRequested(contextData->mediaUrl(),
                    contextData->mediaType() == QWebEngineContextMenuRequest::MediaTypeVideo ? "video" : "audio");
            });
        }

        menu.addSeparator();

        menu.addAction(QIcon::fromTheme("tools-report-bug"), "Inspect Element", [view]() {
            Dialogs::inspect(view->page(), view->title());
        });

        menu.exec(globalPos);
        m_menuActive = false;
    }

signals:
    void newTabRequested(const QUrl &url);
    void navigationRequestAccepted(const QUrl &url);
    void openInNewTabRequested(const QUrl &url);
    void addToFavoritesRequested(const QString &title, const QUrl &url, const QIcon &icon);
    void saveImageRequested(const QUrl &imageUrl);
    void saveMediaRequested(const QUrl &mediaUrl, const QString &mediaType);
    void copyStatusRequested(const QString &message);
    void permissionPromptRequested(QWidget *parent, const QString &origin,
                                   const QString &permission, int requestId,
                                   std::function<void(int, bool)> callback);
    void fileAccessPromptRequested(QWidget *parent, const QString &origin,
                                   const QString &description, int requestId,
                                   std::function<void(int, bool)> callback);
    void notificationReceived(const QString &origin, const QString &title,
                              const QString &body, const QString &iconPath);
    void siteInfoRequested(const QString &origin, bool isCurrentlyOpen);
    void viewPageSourceRequested(const QString &html, const QString &title);

private slots:
    void handlePermissionResponse(int requestId, int decision, bool remember) {
        if (!m_pendingPermissions.contains(requestId)) return;

        QWebEnginePermission permission = m_pendingPermissions.take(requestId);
        QString origin = m_pendingOrigins.take(requestId);
        QString permType = m_pendingTypes.take(requestId);

        if (decision == 1) {
            permission.grant();
            if (remember) {
                Settings_Backend::instance().setSitePermission(origin, permType, 1);
            }
        } else {
            permission.deny();
            if (remember) {
                Settings_Backend::instance().setSitePermission(origin, permType, 2);
            }
        }
    }

    void handleFileAccessResponse(int requestId, int decision, bool remember) {
        auto it = std::find_if(m_pendingFileAccessRequests.begin(),
                               m_pendingFileAccessRequests.end(),
                               [requestId](const FileAccessRequest& far) {
                                   return far.requestId == requestId;
                               });

        if (it == m_pendingFileAccessRequests.end()) return;

        QWebEngineFileSystemAccessRequest request = std::move(it->request);
        m_pendingFileAccessRequests.erase(it);

        QString origin = m_pendingOrigins.value(requestId);
        QString permType = m_pendingTypes.value(requestId);

        if (decision == 1) {
            request.accept();
            if (remember) {
                Settings_Backend::instance().setSitePermission(origin, permType, 1);
            }
        } else {
            request.reject();
            if (remember) {
                Settings_Backend::instance().setSitePermission(origin, permType, 2);
            }
        }

        m_pendingOrigins.remove(requestId);
        m_pendingTypes.remove(requestId);
        m_pendingDescriptions.remove(requestId);
    }

    void handlePopupResponse(int requestId, int decision, bool remember) {
        if (!m_pendingPopups.contains(requestId)) return;

        QUrl url = m_pendingPopups.take(requestId);
        QString origin = m_pendingOrigins.take(requestId);
        QString permType = m_pendingTypes.take(requestId);

        if (decision == 1) {
            if (url.isValid()) {
                emit newTabRequested(url);
            }
            if (remember) {
                Settings_Backend::instance().setSitePermission(origin, permType, 1);
            }
        } else if (decision == 2) {
            if (remember) {
                Settings_Backend::instance().setSitePermission(origin, permType, 2);
            }
        }
    }

private:
    struct FileAccessRequest {
        int requestId;
        QWebEngineFileSystemAccessRequest request;
    };

    bool m_menuActive;
    QMap<int, QWebEnginePermission> m_pendingPermissions;
    QList<FileAccessRequest> m_pendingFileAccessRequests;
    QMap<int, QUrl> m_pendingPopups;
    QMap<int, QString> m_pendingOrigins;
    QMap<int, QString> m_pendingTypes;
    QMap<int, QString> m_pendingDescriptions;
    QMap<QWebEngineNotification*, QString> m_activeNotifications;
    int m_nextRequestId = 0;
};


class Onu_window : public QMainWindow {
    Q_OBJECT
public:
    static bool initializeEncryption(QWidget *parent = nullptr) {
        auto &settings = Settings_Backend::instance();

        if (!settings.useCustomEncryption()) {
           return true;
        }

      int attemptCount = 0;

        if (Dialogs::requestEncryptionKey(parent, attemptCount)) {
          return true;
        }
      return false;
    }

    Onu_window()
        : m_audioOutput(std::make_unique<QAudioOutput>(this))
    {
        setWindowTitle("Onu.");
        setWindowIcon(QIcon::fromTheme("onu"));
        resize(1280, 800);
        REG_CRASH();

        setupContentProtector();

        setupUI();
        setupToolbars();
        setupKeybinds();
        applyTheme();
        loadExtensions();

        if (Settings_Backend::instance().restoreSessions()) {
            QList<QUrl> sessionTabs = Browser_Backend::instance().restoreSession();
            if (sessionTabs.isEmpty()) {
                addTab();
            } else {
                for (const QUrl &url : sessionTabs) {
                    addTab(url);
                }
            }
        } else {
            addTab();
        }

        connect(Onu_Web::defaultProfile(), &QWebEngineProfile::downloadRequested,
                this, &Onu_window::handleDownload);

        m_audioOutput->setVolume(Settings_Backend::instance().audioVolume() / 100.0);
    }

    ~Onu_window() {

        if (m_contentProtector) {
            Onu_Web::defaultProfile()->setUrlRequestInterceptor(nullptr);

            m_contentProtector = nullptr;
        }

        qDeleteAll(m_shortcuts);
        m_shortcuts.clear();
    }


    void openUrls(const QList<QUrl> &urls) {
        for (const QUrl &url : urls) {
            addTab(url);
        }
    }

    Browser_Backend* backend() const { return &Browser_Backend::instance(); }

    void viewPageSource() {
        QWebEngineView *view = currentWebView();
        if (!view) return;

        view->page()->toHtml([this, view](const QString &html) {
            Dialogs::source(this, html, view->title());
        });
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        QList<QUrl> tabs;
        for (const auto &view : m_contents) {
            if (view && view->url().scheme() != "onu") {
                tabs.append(view->url());
            }
        }

        QByteArray toolbarState = m_toolbars->saveToolbarGeometry();
        Settings_Backend::instance().saveToolbarState(toolbarState);

        Browser_Backend::instance().close(this, tabs);
        event->ignore();
    }

private slots:
    void addTab(const QUrl &url = QUrl()) {
        auto& s = Settings_Backend::instance();
        QUrl finalUrl = url.isEmpty() ? QUrl(s.homepage()) : url;
        if (!finalUrl.isValid()) finalUrl = QUrl(s.homepage());

        auto view = std::make_unique<QWebEngineView>();
        if (!view) return;

     Onu_Web *page = new Onu_Web(Onu_Web::defaultProfile(), view.get());
     if (!page) return;

        view->setPage(page);

        page->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, s.javascriptEnabled());
        page->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, s.imagesEnabled());

        setupScriptInjection(page);
        Extense::notifyPage(page);

        int index = m_toolbars->tabBar()->addTab("New Tab");
        if (index < 0) return;

        while (m_fullTitles.size() <= index) {
            m_fullTitles.append("New Tab");
        }
        m_fullTitles[index] = "New Tab";

        m_mainLayout->insertWidget(m_mainLayout->count() - 1, view.get());
        view->hide();

        QWebEngineView *viewPtr = view.get();
        if (!viewPtr) return;

        setupTabCallbacks(viewPtr, page, index);

        view->load(finalUrl);
        m_contents.push_back(std::move(view));
        m_toolbars->tabBar()->setCurrentIndex(index);
        tabChanged(index);
    }

    void setupTabCallbacks(QWebEngineView *viewPtr, Onu_Web *page, int index) {
        QPointer<QWebEngineView> safeView(viewPtr);
        QPointer<Onu_Web> safePage(page);
        QPointer<Create_Toolbars> safeToolbars(m_toolbars);
        QPointer<Onu_window> safeThis(this);
        int tabIndex = index;

        const auto getOnuIcon = [](const QString &host) -> QIcon {
            if (host == "home")  return QIcon::fromTheme("go-home");
            if (host == "game")  return QIcon::fromTheme("input-gaming");
            if (host == "about") return QIcon::fromTheme("dialog-information");
            return QIcon::fromTheme("applications-internet");
        };

        const auto validateTab = [safeToolbars, tabIndex]() -> bool {
            return safeToolbars &&
                   safeToolbars->tabBar() &&
                   tabIndex >= 0 &&
                   tabIndex < safeToolbars->tabBar()->count();
        };

        const auto updateTabIcon = [safeThis, safeView, safeToolbars, tabIndex, &getOnuIcon, validateTab]() {
            if (!safeThis || !safeView || !validateTab()) return;

            const QUrl url = safeView->url();
            if (url.scheme() == "onu") {
                safeToolbars->tabBar()->setTabIcon(tabIndex, getOnuIcon(url.host()));
            } else {
                const QIcon icon = safeView->icon();
                safeToolbars->tabBar()->setTabIcon(tabIndex,
                    icon.isNull() ? QIcon::fromTheme("applications-internet") : icon);
            }
        };

        const auto updateTabTitle = [safeThis, safeView, safeToolbars, tabIndex, validateTab]() {
            if (!safeThis || !safeView || !validateTab()) return;

            QString title = safeView->title();
            if (title.isEmpty()) {
                const QUrl url = safeView->url();
                title = (url.scheme() == "onu") ?
                        (url.host().isEmpty() ? "onu" : url.host()) :
                        "New Tab";
            }

            if (tabIndex < safeThis->m_fullTitles.size()) {
                safeThis->m_fullTitles[tabIndex] = title;
            }

            const bool vertical = (safeToolbars->tabToolbar()->orientation() == Qt::Vertical);
            const int maxChars = vertical ? 15 : 30;
            const QString displayTitle = (title.length() > maxChars) ?
                                          title.left(maxChars) + "…" :
                                          title;

            safeToolbars->tabBar()->setTabText(tabIndex, displayTitle);
            safeToolbars->tabBar()->setTabToolTip(tabIndex, title + "\n" + safeView->url().toString());
        };

        connect(safePage, &Onu_Web::permissionPromptRequested, safeThis,
            [safeThis](QWidget *parent, const QString &origin, const QString &permission,
                       int requestId, std::function<void(int, bool)> callback) {
                if (!safeThis) return;
                Q_UNUSED(requestId);
                Dialogs::permissionPrompt(parent, origin, permission, std::move(callback));
            });

    connect(safePage, &Onu_Web::fileAccessPromptRequested, safeThis,
            [safeThis](QWidget *parent, const QString &origin, const QString &description,
                       int requestId, std::function<void(int, bool)> callback) {
                if (!safeThis) return;
                Q_UNUSED(requestId);
                Dialogs::permissionPrompt(parent, origin, description, std::move(callback));
            });

    connect(safePage, &Onu_Web::notificationReceived, safeThis,
        [safeThis](const QString &origin, const QString &title,
                   const QString &body, const QString &iconPath) {
            if (!safeThis) return;
            safeThis->showNotification(origin, title, body, iconPath);
        });


        connect(safePage, &Onu_Web::siteInfoRequested, safeThis,
            [safeThis](const QString &origin, bool isCurrentlyOpen) {
                if (!safeThis) return;
                Dialogs::siteInfo(safeThis, origin, isCurrentlyOpen);
            });

        connect(safePage, &Onu_Web::newTabRequested, safeThis,
            [safeThis](const QUrl &newUrl) {
                if (!safeThis) return;
                safeThis->addTab(newUrl);
            });

        connect(safePage, &Onu_Web::openInNewTabRequested, safeThis,
            [safeThis](const QUrl &url) {
                if (!safeThis) return;
                safeThis->addTab(url);
            });

        if (safeView) {
            safeView->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(safeView, &QWidget::customContextMenuRequested, safeThis,
                [safeThis, safePage, safeView](const QPoint &pos) {
                    if (!safeThis || !safePage || !safeView) return;
                    safePage->showContextMenu(safeView, safeView->mapToGlobal(pos));
                });
        }

        connect(safeView, &QWebEngineView::titleChanged, safeThis,
            [updateTabTitle](const QString&) {
                QMetaObject::invokeMethod(qApp, [updateTabTitle]() {
                    updateTabTitle();
                }, Qt::QueuedConnection);
            });

        connect(safeView, &QWebEngineView::iconChanged, safeThis,
            [safeThis, safeView, safeToolbars, tabIndex, updateTabIcon, validateTab](const QIcon &icon) {
                if (!safeThis || !safeView || !validateTab()) return;
                if (safeView->url().scheme() != "onu" && !icon.isNull()) {
                    QMetaObject::invokeMethod(qApp, [updateTabIcon]() {
                        updateTabIcon();
                    }, Qt::QueuedConnection);
                }
            });

        connect(safePage, &Onu_Web::navigationRequestAccepted, safeThis,
            [safeThis, safeToolbars](const QUrl &url) {
                if (!safeThis || !safeToolbars) return;
                if (auto* view = safeThis->currentWebView()) {
                    safeToolbars->addressBar()->setText(url.toString());
                    safeToolbars->addressBar()->setCursorPosition(0);
                }
            });

        connect(safePage, &QWebEnginePage::linkHovered, safeThis,
            [safeThis](const QString &url) {
                if (!safeThis) return;
                if (url.isEmpty()) {
                    safeThis->hideStatusBubble();
                } else {
                    safeThis->showStatusBubble("Referencing: " + url);
                }
            }, Qt::QueuedConnection);

        connect(safePage, &Onu_Web::copyStatusRequested, safeThis,
            [safeThis](const QString &message) {
                if (!safeThis) return;
                safeThis->showStatusMessage(message, 2000);
            });

        connect(safePage, &Onu_Web::viewPageSourceRequested, safeThis,
            [safeThis](const QString &html, const QString &title) {
                if (!safeThis) return;
                Dialogs::source(safeThis, html, title);
            });

        connect(safePage, &Onu_Web::saveImageRequested, safeThis,
            [safeThis](const QUrl &imageUrl) {
                if (!safeThis) return;
                if (!safeThis->m_downloadManager) {
                    safeThis->m_downloadManager = new DL_Man(safeThis);
                }
                safeThis->m_downloadManager->addDownload(imageUrl, imageUrl.fileName());
                safeThis->m_downloadManager->show();
                safeThis->showStatusMessage("Downloading image...", 2000);
            });

        connect(safePage, &Onu_Web::saveMediaRequested, safeThis,
            [safeThis](const QUrl &mediaUrl, const QString &mediaType) {
                if (!safeThis) return;
                if (!safeThis->m_downloadManager) {
                    safeThis->m_downloadManager = new DL_Man(safeThis);
                }
                QString suggestedName = QFileInfo(mediaUrl.path()).fileName();
                if (suggestedName.isEmpty() || suggestedName.contains('?')) {
                    suggestedName = QString("%1_%2.%3")
                        .arg(mediaType)
                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
                        .arg(mediaType == "video" ? "mp4" : "mp3");
                }
                safeThis->m_downloadManager->addDownload(mediaUrl, suggestedName);
                safeThis->m_downloadManager->show();
                safeThis->showStatusMessage(QString("Downloading %1...").arg(mediaType), 3000);
            });

        connect(safePage, &Onu_Web::addToFavoritesRequested, safeThis,
            [safeThis, safeToolbars](const QString &title, const QUrl &url, const QIcon &icon) {
                if (!safeThis || !safeToolbars) return;
                Browser_Backend::instance().addToFavorites(title, url, icon);
                safeToolbars->refreshFavToolbar(Browser_Backend::instance().favorites());
                safeToolbars->updateSuggestions(Browser_Backend::instance().recentTabs(), Browser_Backend::instance().favorites());
                safeThis->showStatusMessage("Added to favorites: " + title, 2000);
            });

        connect(safeView, &QWebEngineView::urlChanged, safeThis,
            [safeThis, safeView, safePage, safeToolbars, tabIndex,
             updateTabIcon, updateTabTitle, validateTab, getOnuIcon](const QUrl &newUrl) {
                if (!safeThis || !safeView || !safeToolbars || !validateTab()) return;

                QMetaObject::invokeMethod(qApp, [=]() {
                    if (!safeThis || !safeView || !safeToolbars || !validateTab()) return;

                    if (safeToolbars->tabBar()->currentIndex() == tabIndex) {
                        safeToolbars->addressBar()->setText(newUrl.toString());
                        safeToolbars->addressBar()->setCursorPosition(0);
                    }

                    if (newUrl.scheme() == "onu") {
                        const QString host = newUrl.host();
                        safeToolbars->tabBar()->setTabIcon(tabIndex, getOnuIcon(host));
                        const QString title = host.isEmpty() ? "onu" : host;
                        safeToolbars->tabBar()->setTabText(tabIndex, title);
                        safeToolbars->tabBar()->setTabToolTip(tabIndex, title + "\n" + newUrl.toString());
                        if (tabIndex < safeThis->m_fullTitles.size()) {
                            safeThis->m_fullTitles[tabIndex] = title;
                        }
                    } else {
                        updateTabIcon();
                    }

                    if (newUrl.scheme() != "onu" && !newUrl.isEmpty()) {
                        QTimer::singleShot(500, safeThis, [=]() {
                            if (safeView && safeToolbars) {
                                Browser_Backend::instance().addToHistory(newUrl, safeView->title(), safeView->icon());
                                safeToolbars->updateSuggestions(Browser_Backend::instance().recentTabs(),
                                                              Browser_Backend::instance().favorites());
                            }
                        });
                    }

                    safeThis->setupScriptInjection(safePage);

                    if (safeToolbars->tabBar()->currentIndex() == tabIndex && safeView) {
                        safeToolbars->updateNavActions(safeView->history()->canGoBack(),
                                                      safeView->history()->canGoForward());
                    }
                }, Qt::QueuedConnection);
            });

        connect(safeView, &QWebEngineView::loadStarted, safeThis,
            [safeThis, safeView, safeToolbars, tabIndex, validateTab]() {
                if (!safeThis || !safeView || !safeToolbars || !validateTab()) return;

                QMetaObject::invokeMethod(qApp, [=]() {
                    if (!safeThis || !safeView || !safeToolbars || !validateTab()) return;

                    safeToolbars->tabBar()->setTabIcon(tabIndex, QIcon::fromTheme("image-loading"));
                    if (safeToolbars->tabBar()->currentIndex() == tabIndex) {
                        safeToolbars->reloadAction()->setVisible(false);
                        safeToolbars->stopAction()->setVisible(true);
                    }
                    safeThis->showStatusMessage("Loading: " + safeView->url().host(), 0);
                }, Qt::QueuedConnection);
            });

        connect(safeView, &QWebEngineView::loadProgress, safeThis,
            [safeThis, safeToolbars, tabIndex, validateTab](int progress) {
                if (!safeThis || !safeToolbars || !validateTab()) return;

                if (safeToolbars->tabBar()->currentIndex() == tabIndex &&
                    progress > 0 && progress < 100) {
                    safeThis->showStatusMessage(QString("Loading... %1%").arg(progress), 0);
                }
            });

        connect(safeView, &QWebEngineView::loadFinished, safeThis,
            [safeThis, safeView, safeToolbars, tabIndex, updateTabIcon, updateTabTitle, validateTab]
            (bool ok) {
                if (!safeThis || !safeView || !safeToolbars || !validateTab()) return;

                QMetaObject::invokeMethod(qApp, [=]() {
                    if (!safeThis || !safeView || !safeToolbars || !validateTab()) return;

                    if (safeView->url().scheme() == "onu") {
                        const QString host = safeView->url().host();
                        const QString title = host.isEmpty() ? "onu" : host;
                        safeToolbars->tabBar()->setTabText(tabIndex, title);
                        if (tabIndex < safeThis->m_fullTitles.size()) {
                            safeThis->m_fullTitles[tabIndex] = title;
                        }
                    } else {
                        updateTabIcon();
                        updateTabTitle();
                    }

                    if (!ok) {
                        safeThis->showStatusMessage("Failed to load: " + safeView->url().toString(), 3000);
                    } else {
                        safeThis->hideStatusBubble();
                    }

                    if (safeToolbars->tabBar()->currentIndex() == tabIndex) {
                        safeToolbars->reloadAction()->setVisible(true);
                        safeToolbars->stopAction()->setVisible(false);
                        safeToolbars->updateNavActions(safeView->history()->canGoBack(),
                                                      safeView->history()->canGoForward());
                    }
                }, Qt::QueuedConnection);
            });
    }
    void closeTab(int index) {
        if (index < 0 || index >= (int)m_contents.size()) return;

        if (m_contents[index]) {
            m_closedTabs.append(m_contents[index]->url());
            if (m_closedTabs.size() > 10) m_closedTabs.removeFirst();
        }

        if (m_contents.size() <= 1) {
            close();
            return;
        }

        m_contents.erase(m_contents.begin() + index);
        m_fullTitles.removeAt(index);
        m_toolbars->tabBar()->removeTab(index);

        if (!m_contents.empty()) {
            int newIdx = qMin(index, (int)m_contents.size() - 1);
            m_toolbars->tabBar()->setCurrentIndex(newIdx);
        }

        updateTabTitles();
    }

    void tabChanged(int index) {
        for (size_t i = 0; i < m_contents.size(); ++i) {
            if (m_contents[i]) m_contents[i]->setVisible((int)i == index);
        }

        if (auto *view = currentWebView()) {
            m_toolbars->addressBar()->setText(view->url().toString());
            m_toolbars->addressBar()->setCursorPosition(0);
            m_toolbars->updateNavActions(view->history()->canGoBack(),
                                         view->history()->canGoForward());

            if (view->page()->isLoading()) {
                m_toolbars->reloadAction()->setVisible(false);
                m_toolbars->stopAction()->setVisible(true);
            } else {
                m_toolbars->reloadAction()->setVisible(true);
                m_toolbars->stopAction()->setVisible(false);
            }
        }

        updateTabTitles();
    }

    void handleDownload(QWebEngineDownloadRequest *download) {
        if (!download) return;

        if (Settings_Backend::instance().askDownloadConfirm()) {
            QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                      "Download",
                                                                      "Download " + download->downloadFileName() + "?",
                                                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No) {
                download->cancel();
                return;
            }
        }

        download->cancel();
        if (!m_downloadManager) m_downloadManager = new DL_Man(this);
        m_downloadManager->addDownload(download->url(), download->downloadFileName());
        showStatusMessage(QString("Downloading to %1").arg(Settings_Backend::instance().downloadPath()), 3000);
    }

    void showTabContextMenu(int index, const QPoint &pos) {
        if (index < 0 || index >= (int)m_contents.size()) return;

        QMenu menu(this);

        menu.addAction("New Tab", [this]() { addTab(); });
        menu.addAction("Duplicate Tab", [this, index]() {
            if (m_contents[index]) addTab(m_contents[index]->url());
        });
        menu.addSeparator();
        menu.addAction("Reload Tab", [this, index]() {
            if (m_contents[index]) m_contents[index]->reload();
        });
        menu.addAction("View Page Source", this, &Onu_window::viewPageSource);
        menu.addAction("Add to Favorites", [this, index]() {
            if (m_contents[index]) {
                Browser_Backend::instance().addToFavorites(m_contents[index]->title(),
                                          m_contents[index]->url(),
                                          m_contents[index]->icon());
            }
        });
        menu.addAction(QIcon::fromTheme("dialog-information"), "Site Information", [this, index]() {
            if (m_contents[index]) {
                QString origin = m_contents[index]->url().scheme() + "://" +
                                m_contents[index]->url().host();
                Dialogs::siteInfo(this, origin);
            }
        });

        menu.addSeparator();

        if (m_contents.size() > 1) {
            menu.addAction("Close Tab", [this, index]() { closeTab(index); });
            menu.addAction("Close Other Tabs", [this, index]() {
                for (int i = m_contents.size() - 1; i >= 0; --i) {
                    if (i != index) closeTab(i);
                }
            });
        }

        menu.exec(pos);
    }

    void clearHistory() {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "Clear History",
                                                                  "Are you sure you want to clear all browsing history?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            Browser_Backend::instance().clearHistory();
            m_toolbars->refreshRecentToolbar(Browser_Backend::instance().recentTabs());
            m_toolbars->updateSuggestions(Browser_Backend::instance().recentTabs(), Browser_Backend::instance().favorites());
            showStatusMessage("History cleared", 2000);
        }
    }

    void reopenClosedTab() {
        if (!m_closedTabs.isEmpty()) {
            QUrl url = m_closedTabs.takeLast();
            addTab(url);
        }
    }

    void showStatusMessage(const QString &message, int timeout)
    {
        showStatusBubble(message);

           if (timeout > 0) {
               QTimer::singleShot(timeout, this, [this]() {
                   hideStatusBubble();
               });
           }
    }

    void showStatusBubble(const QString &text)
      {
          QWidget *parent = centralWidget() ? centralWidget() : this;

          const int margin = 10;
          QPoint pos = parent->mapToGlobal(
              QPoint(margin, parent->height() - margin)
          );

          QToolTip::showText(pos, text, parent);
      }



    void hideStatusBubble() {
        if (m_statusBubble) {
            m_statusBubble->hide();
        }
    }

    void applyTheme() {
        auto& s = Settings_Backend::instance();
        QString themeName = s.currentTheme();
        QString qss = s.loadThemeQSS(themeName);
        qApp->setStyleSheet(qss);

        QString qtStyle = s.qtStyle();
        if (!qtStyle.isEmpty() && qtStyle != "System") {
            qApp->setStyle(QStyleFactory::create(qtStyle));
        }

        QString fontFamily = s.fontFamily();
        if (!fontFamily.isEmpty()) {
            QFont appFont(fontFamily, s.fontSize());
            qApp->setFont(appFont);
        }

        QString iconTheme = s.iconTheme();
        if (iconTheme != "System Default") {
            QIcon::setThemeName(iconTheme);
        }

        m_toolbars->applyToolbarSettings();

        if (m_contentProtector) {
            m_contentProtector->reloadRules();
        }
    }

    QWebEngineView* currentWebView() const {
        int idx = m_toolbars->tabBar()->currentIndex();
        if (idx >= 0 && idx < (int)m_contents.size()) return m_contents[idx].get();
        return nullptr;
    }

private:
    Create_Toolbars *m_toolbars = nullptr;

    Content_Protector *m_contentProtector;
    std::vector<std::unique_ptr<QWebEngineView>> m_contents;
    QVector<QString> m_fullTitles;
    QVector<QUrl> m_closedTabs;
    QWidget *m_central = nullptr;
    QVBoxLayout *m_mainLayout = nullptr;
    QLabel *m_statusBubble = nullptr;
    DL_Man *m_downloadManager = nullptr;
    std::unique_ptr<QAudioOutput> m_audioOutput;
    QList<QShortcut*> m_shortcuts;

    void setupContentProtector() {

        m_contentProtector = new Content_Protector(this);
        Onu_Web::defaultProfile()->setUrlRequestInterceptor(m_contentProtector);
    }
    void setupScriptInjection(QWebEnginePage *page) {
        QString userScript = Settings_Backend::instance().userScript();
        QString extScripts = Extense::collectScripts();

        QString combined = userScript;
        if (!extScripts.isEmpty()) {
            combined += "\n" + extScripts;
        }

        if (!combined.isEmpty()) {
            page->runJavaScript(combined);
        }
    }

    void setupKeybinds() {
        qDeleteAll(m_shortcuts);
        m_shortcuts.clear();

        auto &s = Settings_Backend::instance();
        QMap<QString, QString> defaults = s.defaultKeybinds();

        auto addShortcut = [this, &s](const QString &action, auto slot) {
            QString keybind = s.getKeybind(action);
            if (!keybind.isEmpty()) {
                QShortcut *shortcut = new QShortcut(QKeySequence(keybind), this);
                connect(shortcut, &QShortcut::activated, this, slot);
                m_shortcuts.append(shortcut);
            }
        };

        addShortcut("New Tab", [this]() { addTab(); });
        addShortcut("Close Tab", [this]() { closeTab(m_toolbars->tabBar()->currentIndex()); });
        addShortcut("Reopen Closed Tab", [this]() { reopenClosedTab(); });
        addShortcut("Next Tab", [this]() {
            int next = (m_toolbars->tabBar()->currentIndex() + 1) % m_toolbars->tabBar()->count();
            m_toolbars->tabBar()->setCurrentIndex(next);
        });
        addShortcut("Previous Tab", [this]() {
            int prev = (m_toolbars->tabBar()->currentIndex() - 1 + m_toolbars->tabBar()->count())
            % m_toolbars->tabBar()->count();
            m_toolbars->tabBar()->setCurrentIndex(prev);
        });
        addShortcut("Reload Page", [this]() {
            if (auto *v = currentWebView()) v->reload();
        });
        addShortcut("Force Reload", [this]() {
            if (auto *v = currentWebView())
                v->page()->triggerAction(QWebEnginePage::ReloadAndBypassCache);
        });
        addShortcut("Stop Loading", [this]() {
            if (auto *v = currentWebView()) v->stop();
        });
        addShortcut("Find in Page", [this]() {
            m_toolbars->findBar()->show();
            m_toolbars->findEdit()->setFocus();
        });
        addShortcut("Go Back", [this]() {
            if (auto *v = currentWebView()) v->back();
        });
        addShortcut("Go Forward", [this]() {
            if (auto *v = currentWebView()) v->forward();
        });
        addShortcut("Go Home", [this]() {
            if (auto *v = currentWebView())
                v->load(QUrl(Settings_Backend::instance().homepage()));
        });
        addShortcut("Focus Address Bar", [this]() {
            m_toolbars->addressBar()->setFocus();
            m_toolbars->addressBar()->selectAll();
        });
        addShortcut("Open Downloads", [this]() {
            if (!m_downloadManager) m_downloadManager = new DL_Man(this);
            m_downloadManager->show();
        });
        addShortcut("Open History", [this]() {
            Dialogs::history(this, Browser_Backend::instance().recentTabs(), [this]() { clearHistory(); });
        });
        addShortcut("Open Settings", [this]() {
            Dialogs::settings(this, [this]() {
                applyTheme();
                m_toolbars->applyToolbarSettings();
            });
        });
        addShortcut("Open Keybinds", [this]() {
            Dialogs::keybinds(this, [this]() {
                setupKeybinds();
            });
        });
        addShortcut("Open Extensions", [this]() {
            Dialogs::ExtMan(this);
        });
        addShortcut("Quit Browser", [this]() { close(); });
        addShortcut("View Page Source", [this]() { viewPageSource(); });
        addShortcut("Toggle Developer Tools", [this]() {
            if (auto *v = currentWebView()) {
                if (Settings_Backend::instance().developerMode()) {
                    Dialogs::inspect(v->page(), v->title());
                }
            }
        });
        addShortcut("Toggle Fullscreen", [this]() {
            if (isFullScreen()) showNormal();
            else showFullScreen();
        });
        addShortcut("Bookmark Page", [this]() {
            if (auto *v = currentWebView()) {
                Browser_Backend::instance().addToFavorites(v->title(), v->url(), v->icon());
                m_toolbars->refreshFavToolbar(Browser_Backend::instance().favorites());
                showStatusMessage("Added to favorites", 2000);
            }
        });
        addShortcut("Site Info", [this]() {
            if (auto *v = currentWebView()) {
                QString origin = v->url().scheme() + "://" + v->url().host();
                Dialogs::siteInfo(this, origin, true);
            }
        });

        addShortcut("Sites Manager", [this]() {
            Dialogs::sitesManager(this);
        });
    }
    void setupUI() {
        m_central = new QWidget(this);
        m_mainLayout = new QVBoxLayout(m_central);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        setCentralWidget(m_central);

        m_audioOutput = std::make_unique<QAudioOutput>(this);
        m_audioOutput->setVolume(Settings_Backend::instance().audioVolume() / 100.0);

       connect(&Browser_Backend::instance(), &Browser_Backend::newTabRequested,
                this, [this](const QUrl &url) {
                    addTab(url);
                });
    }

    void setupToolbars() {
        m_toolbars = new Create_Toolbars(this, this);

        m_mainLayout->insertWidget(0, m_toolbars->findBar());

        connect(m_toolbars, &Create_Toolbars::backRequested, this, [this]() {
            if (auto *v = currentWebView()) v->back();
        });

        connect(m_toolbars, &Create_Toolbars::forwardRequested, this, [this]() {
            if (auto *v = currentWebView()) v->forward();
        });

        connect(m_toolbars, &Create_Toolbars::reloadRequested, this, [this]() {
            if (auto *v = currentWebView()) {
                v->reload();
                if (m_contentProtector) {
                    m_contentProtector->reloadRules();
                }
            }
        });

        connect(m_toolbars, &Create_Toolbars::stopRequested, this, [this]() {
            if (auto *v = currentWebView()) v->stop();
        });

        connect(m_toolbars, &Create_Toolbars::zoomLevelChanged, this, [this](int percent) {
            if (auto *view = currentWebView()) {
                view->setZoomFactor(percent / 100.0);
            }
        });

        connect(m_toolbars, &Create_Toolbars::navigateRequested, this, [this](const QUrl &url) {
            if (auto *v = currentWebView()) v->load(url);
        });

        connect(m_toolbars, &Create_Toolbars::newTabRequested, this, [this]() {
            addTab();
        });

        connect(m_toolbars, &Create_Toolbars::tabCloseRequested, this, &Onu_window::closeTab);
        connect(m_toolbars, &Create_Toolbars::tabChanged, this, &Onu_window::tabChanged);

        connect(m_toolbars, &Create_Toolbars::tabMoved, this, [this](int from, int to) {
            if (from >= 0 && from < (int)m_contents.size() &&
                to >= 0 && to < (int)m_contents.size()) {
                std::swap(m_contents[from], m_contents[to]);
                if (from < m_fullTitles.size() && to < m_fullTitles.size()) {
                    m_fullTitles.swapItemsAt(from, to);
                }
            }
        });

        connect(m_toolbars, &Create_Toolbars::volumeChanged, this, [this](int volume) {
            if (m_audioOutput) {
                m_audioOutput->setVolume(volume / 100.0);
                Settings_Backend::instance().setPublicValue("audio/volume", volume);
            }
        });

        connect(m_toolbars, &Create_Toolbars::recentActionTriggered, this, [this](const QUrl &url, bool manage) {
            if (manage)
                Dialogs::history(this, Browser_Backend::instance().recentTabs(), [this]() { clearHistory(); });
            else if (url.isValid())
                addTab(url);
        });

        connect(m_toolbars, &Create_Toolbars::Open_InNewTab_tb, this, [this](const QUrl &url, bool customize) {
            if (customize) {
                QVariantList favs = Browser_Backend::instance().favorites();
                Dialogs::fav(this, favs, [this]() {
                    m_toolbars->refreshFavToolbar(Browser_Backend::instance().favorites());
                    m_toolbars->updateSuggestions(Browser_Backend::instance().recentTabs(), Browser_Backend::instance().favorites());
                });
                Browser_Backend::instance().setFavorites(favs);
            } else if (url.isValid()) {
                addTab(url);
            }
        });

        connect(&Browser_Backend::instance(), &Browser_Backend::recentTabsChanged, this, [this]() {
            m_toolbars->refreshRecentToolbar(Browser_Backend::instance().recentTabs());
            m_toolbars->updateSuggestions(Browser_Backend::instance().recentTabs(), Browser_Backend::instance().favorites());
        });

        connect(&Browser_Backend::instance(), &Browser_Backend::favoritesChanged, this, [this]() {
            m_toolbars->refreshFavToolbar(Browser_Backend::instance().favorites());
            m_toolbars->updateSuggestions(Browser_Backend::instance().recentTabs(), Browser_Backend::instance().favorites());
        });

        connect(m_toolbars, &Create_Toolbars::showHistoryRequested, this, [this]() {
            Dialogs::history(this, Browser_Backend::instance().recentTabs(), [this]() { clearHistory(); });
        });

        connect(m_toolbars, &Create_Toolbars::showSettingsRequested, this, [this]() {
            Dialogs::settings(this, [this]() {
                QTimer::singleShot(100, this, [this]() {
                    if (!this || !m_toolbars) return;
                    applyTheme();
                    m_toolbars->applyToolbarSettings();
                });
            });
        });

        connect(m_toolbars, &Create_Toolbars::showDownloadsRequested, this, [this]() {
            if (!m_downloadManager) m_downloadManager = new DL_Man(this);
            m_downloadManager->show();
        });

        connect(m_toolbars, &Create_Toolbars::quitRequested, this, [this]() {
            QList<QUrl> tabs;
            for (const auto &view : m_contents) {
                if (view && view->url().scheme() != "onu") {
                    tabs.append(view->url());
                }
            }
            Browser_Backend::instance().close(this, tabs);
        });

        connect(m_toolbars, &Create_Toolbars::clearHistoryRequested, this, &Onu_window::clearHistory);
        connect(m_toolbars, &Create_Toolbars::tabContextMenuRequested, this, &Onu_window::showTabContextMenu);

        connect(m_toolbars, &Create_Toolbars::populateBackMenu, this, [this](QMenu *menu) {
            if (!menu) return;
            auto *view = currentWebView();
            if (!view || !view->history()) {
                menu->addAction("No history")->setEnabled(false);
                return;
            }
            QList<QWebEngineHistoryItem> items = view->history()->backItems(10);
            if (items.isEmpty()) {
                menu->addAction("No back history")->setEnabled(false);
                return;
            }
            int count = qMin(5, items.count());
            for (int i = 0; i < count; ++i) {
                const QWebEngineHistoryItem &item = items[i];
                QUrl itemUrl = item.url();
                if (!itemUrl.isValid()) continue;
                QString title = item.title().trimmed();
                if (title.isEmpty()) title = itemUrl.toString();
                QIcon icon = QIcon::fromTheme("applications-internet");
                QAction *action = menu->addAction(icon, title);
                action->setData(itemUrl);
                action->setToolTip(itemUrl.toString());
                connect(action, &QAction::triggered, this, [view, item]() {
                    if (view && view->history()) {
                        view->history()->goToItem(item);
                    }
                });
            }
        });

        connect(m_toolbars, &Create_Toolbars::populateForwardMenu, this, [this](QMenu *menu) {
            if (!menu) return;
            auto *view = currentWebView();
            if (!view || !view->history()) {
                menu->addAction("No history")->setEnabled(false);
                return;
            }
            QList<QWebEngineHistoryItem> items = view->history()->forwardItems(10);
            if (items.isEmpty()) {
                menu->addAction("No forward history")->setEnabled(false);
                return;
            }
            int count = qMin(5, items.count());
            for (int i = 0; i < count; ++i) {
                const QWebEngineHistoryItem &item = items[i];
                QUrl itemUrl = item.url();
                if (!itemUrl.isValid()) continue;
                QString title = item.title().trimmed();
                if (title.isEmpty()) title = itemUrl.toString();
                QIcon icon = QIcon::fromTheme("applications-internet");
                QAction *action = menu->addAction(icon, title);
                action->setData(itemUrl);
                action->setToolTip(itemUrl.toString());
                connect(action, &QAction::triggered, this, [view, item]() {
                    if (view && view->history()) {
                        view->history()->goToItem(item);
                    }
                });
            }
        });

        connect(m_toolbars, &Create_Toolbars::findTextChanged, this, [this](const QString &text) {
            if (auto *view = currentWebView()) {
                view->findText(text);
            }
        });

        connect(m_toolbars, &Create_Toolbars::findNext, this, [this]() {
            if (auto *view = currentWebView()) {
                view->findText(m_toolbars->findEdit()->text());
            }
        });

        connect(m_toolbars, &Create_Toolbars::findPrevious, this, [this]() {
            if (auto *view = currentWebView()) {
                view->findText(m_toolbars->findEdit()->text(), QWebEnginePage::FindBackward);
            }
        });

        connect(m_toolbars, &Create_Toolbars::addUrlToFavorites, this, [this](const QUrl &url, const QString &title) {
            QIcon icon = QIcon::fromTheme("applications-internet");
            if (auto *view = currentWebView()) {
                if (!view->icon().isNull()) {
                    icon = view->icon();
                }
            }

            Browser_Backend::instance().addToFavorites(title, url, icon);
            m_toolbars->refreshFavToolbar(Browser_Backend::instance().favorites());
            m_toolbars->updateSuggestions(Browser_Backend::instance().recentTabs(), Browser_Backend::instance().favorites());
            showStatusMessage("Added to favorites: " + title, 2000);
        });

        m_toolbars->refreshRecentToolbar(Browser_Backend::instance().recentTabs());
        m_toolbars->refreshFavToolbar(Browser_Backend::instance().favorites());
        m_toolbars->updateSuggestions(Browser_Backend::instance().recentTabs(), Browser_Backend::instance().favorites());

        QByteArray toolbarState = Settings_Backend::instance().loadToolbarState();
        if (!toolbarState.isEmpty()) {
            m_toolbars->restoreToolbarGeometry(toolbarState);
        }
    }
    void updateTabTitles() {
        bool vertical = (m_toolbars->tabToolbar()->orientation() == Qt::Vertical);
        int maxWords = vertical ? 3 : 8;
        for (int i = 0; i < m_toolbars->tabBar()->count() && i < m_fullTitles.size(); ++i) {
            QStringList words = m_fullTitles[i].split(' ', Qt::SkipEmptyParts);
            QString displayTitle = (words.size() <= maxWords) ? m_fullTitles[i] : words.mid(0, maxWords).join(' ') + " ..";
            m_toolbars->tabBar()->setTabText(i, displayTitle);
        }

        m_toolbars->updateVerticalTabs();
    }
    void showNotification(const QString &origin, const QString &title,
                         const QString &body, const QString &iconPath)
    {
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            QSystemTrayIcon *trayIcon = new QSystemTrayIcon(this);

            QIcon icon = !iconPath.isEmpty() ? Icon_Man::instance().loadIcon(iconPath) : QIcon::fromTheme("onu");

            trayIcon->setIcon(icon);

            trayIcon->show();
            trayIcon->showMessage(
                title.isEmpty() ? "Notification from " + origin : title,
                body,
                QSystemTrayIcon::Information,
                5000
            );

            connect(trayIcon, &QSystemTrayIcon::messageClicked, this, [this, origin]() {
                if (!origin.isEmpty()) {
                    QUrl url(origin);
                    if (url.isValid()) {
                        addTab(url);
                    }
                }
            });

            QTimer::singleShot(6000, trayIcon, &QSystemTrayIcon::deleteLater);
            return;
        }

        QWidget *parent = centralWidget() ? centralWidget() : this;

        QWidget *notification = new QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint);
        notification->setAttribute(Qt::WA_DeleteOnClose);
        notification->setStyleSheet(
            "QWidget {"
            "   background-color: palette(window);"
            "   border: 1px solid palette(mid);"
            "   border-radius: 4px;"
            "   padding: 8px;"
            "}"
        );

        QVBoxLayout *mainLayout = new QVBoxLayout(notification);
        QHBoxLayout *contentLayout = new QHBoxLayout();

        if (!iconPath.isEmpty()) {
            QIcon icon = Icon_Man::instance().loadIcon(iconPath);
            if (!icon.isNull()) {
                QLabel *iconLabel = new QLabel();
                iconLabel->setPixmap(icon.pixmap(32, 32));
                iconLabel->setFixedSize(32, 32);
                contentLayout->addWidget(iconLabel);
            }
        }

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);

        if (!title.isEmpty()) {
            QLabel *titleLabel = new QLabel(title);
            QFont titleFont = titleLabel->font();
            titleFont.setBold(true);
            titleLabel->setFont(titleFont);
            textLayout->addWidget(titleLabel);
        }

        QLabel *bodyLabel = new QLabel(body);
        bodyLabel->setWordWrap(true);
        textLayout->addWidget(bodyLabel);

        if (!origin.isEmpty()) {
            QLabel *originLabel = new QLabel(origin);
            originLabel->setStyleSheet("color: gray; font-size: 9px;");
            originLabel->setAlignment(Qt::AlignRight);
            textLayout->addWidget(originLabel);
        }

        contentLayout->addLayout(textLayout);
        contentLayout->addStretch();
        mainLayout->addLayout(contentLayout);

        QPushButton *closeButton = new QPushButton("×");
        closeButton->setFixedSize(24, 24);
        closeButton->setStyleSheet(
            "QPushButton {"
            "   border: none;"
            "   font-size: 16px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: palette(highlight);"
            "   color: palette(highlighted-text);"
            "   border-radius: 12px;"
            "}"
        );
        connect(closeButton, &QPushButton::clicked, notification, &QWidget::deleteLater);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        mainLayout->addLayout(buttonLayout);

        class NotificationClickFilter : public QObject {
        public:
            NotificationClickFilter(std::function<void()> callback, QObject *parent = nullptr)
                : QObject(parent), m_callback(callback) {}

        protected:
            bool eventFilter(QObject *obj, QEvent *event) override {
                if (event->type() == QEvent::MouseButtonPress) {
                    m_callback();
                    return true;
                }
                return false;
            }

        private:
            std::function<void()> m_callback;
        };

        notification->setCursor(Qt::PointingHandCursor);
        notification->installEventFilter(new NotificationClickFilter(
            [this, origin, notification]() {
                if (!origin.isEmpty()) {
                    QUrl url(origin);
                    if (url.isValid()) {
                        addTab(url);
                    }
                }
                notification->deleteLater();
            },
            notification
        ));

        notification->adjustSize();

        const int margin = 10;
        QPoint pos = parent->mapToGlobal(
            QPoint(parent->width() - notification->width() - margin,
                   parent->height() - notification->height() - margin)
        );
        notification->move(pos);

        notification->show();

        QTimer::singleShot(8000, notification, &QWidget::deleteLater);
    }
    void loadExtensions() {
        Extense::discover();
        Extense::applyEnabled(this, &Browser_Backend::instance());

        QTimer::singleShot(500, this, [this]() {
            checkUnspecifiedExtensions();
        });
    }

    void checkUnspecifiedExtensions() {
        QList<ExtInfo*> unspec;

        for (auto& i : Extense::all()) {
            if (i.state == 0) {
                unspec.append(&i);
            }
        }

        if (unspec.isEmpty()) return;

        QDialog dlg(this);
        dlg.setWindowTitle("Unspecified Extensions");
        dlg.setModal(true);
        dlg.resize(650, 500);

        QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);

        QLabel* title = new QLabel(QString("An unassigned Extension is captured"));
        title->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;");
        mainLayout->addWidget(title);

        QHBoxLayout* hLayout = new QHBoxLayout();

        QListWidget* listWidget = new QListWidget();
        listWidget->setIconSize(QSize(32, 32));
        listWidget->setMinimumWidth(200);

        QMap<QListWidgetItem*, ExtInfo*> itemMap;

        for (auto* info : unspec) {
            QListWidgetItem* item = new QListWidgetItem();

            QIcon extIcon = info->meta.icon.isNull() ?
                           QIcon::fromTheme("application-x-addon") :
                           info->meta.icon;

            QPixmap combined(48, 48);
            combined.fill(Qt::transparent);
            QPainter painter(&combined);
            painter.drawPixmap(0, 0, 32, 32, extIcon.pixmap(32, 32));
            painter.drawPixmap(24, 24, 24, 24, QIcon::fromTheme("dialog-warning").pixmap(24, 24));
            painter.end();

            item->setIcon(QIcon(combined));
            item->setText(info->meta.name);
            item->setToolTip(info->meta.description);

            listWidget->addItem(item);
            itemMap[item] = info;
        }

        QVBoxLayout* detailsLayout = new QVBoxLayout();

        QLabel* iconPreview = new QLabel();
        iconPreview->setFixedSize(128, 128);
        iconPreview->setAlignment(Qt::AlignCenter);
        iconPreview->setStyleSheet("background-color: transparent;");
        detailsLayout->addWidget(iconPreview, 0, Qt::AlignCenter);

        QLabel* nameLabel = new QLabel("Extension name");
        nameLabel->setStyleSheet("font-weight: bold;");
        detailsLayout->addWidget(nameLabel);

        QLabel* maintainerLabel = new QLabel("Maintainer");
        detailsLayout->addWidget(maintainerLabel);

        QLabel* versionLabel = new QLabel("Version");
        detailsLayout->addWidget(versionLabel);

        detailsLayout->addSpacing(10);

        QLabel* detailsTitle = new QLabel("Details:");
        detailsTitle->setStyleSheet("font-weight: bold;");
        detailsLayout->addWidget(detailsTitle);

        QTextEdit* descEdit = new QTextEdit();
        descEdit->setReadOnly(true);
        descEdit->setMinimumHeight(120);
        detailsLayout->addWidget(descEdit);

        QHBoxLayout* radioLayout = new QHBoxLayout();
        QRadioButton* unsureRadio = new QRadioButton("N/A (not sure)");
        QRadioButton* disableRadio = new QRadioButton("Disable");
        QRadioButton* enableRadio = new QRadioButton("Enable");

        unsureRadio->setChecked(true);

        radioLayout->addWidget(unsureRadio);
        radioLayout->addWidget(disableRadio);
        radioLayout->addWidget(enableRadio);
        radioLayout->addStretch();
        detailsLayout->addLayout(radioLayout);

        QWidget* detailsWidget = new QWidget();
        detailsWidget->setLayout(detailsLayout);
        detailsWidget->setMinimumWidth(300);

        hLayout->addWidget(listWidget);
        hLayout->addWidget(detailsWidget);
        mainLayout->addLayout(hLayout);

        QLabel* warn = new QLabel("Extensions have full access to the browser.");
        warn->setStyleSheet("color: red; font-weight: bold; padding: 5px;");
        mainLayout->addWidget(warn);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        QPushButton* enableAllBtn = new QPushButton("enable all");
        QPushButton* disableAllBtn = new QPushButton("Disable all");
        QPushButton* keepBtn = new QPushButton("Keep unspecified");
        QPushButton* doneBtn = new QPushButton("Done");

        btnLayout->addWidget(enableAllBtn);
        btnLayout->addWidget(disableAllBtn);
        btnLayout->addWidget(keepBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(doneBtn);
        mainLayout->addLayout(btnLayout);

        ExtInfo* currentInfo = nullptr;

        connect(listWidget, &QListWidget::currentItemChanged,
            [&](QListWidgetItem* current, QListWidgetItem*) {
                if (!current) return;

                currentInfo = itemMap[current];
                if (!currentInfo) return;

                QIcon icon = currentInfo->meta.icon.isNull() ?
                            QIcon::fromTheme("application-x-addon") :
                            currentInfo->meta.icon;
                QPixmap pixmap = icon.pixmap(128, 128);
                pixmap.setMask(pixmap.createMaskFromColor(Qt::transparent));
                iconPreview->setPixmap(pixmap);

                nameLabel->setText(currentInfo->meta.name);
                maintainerLabel->setText(currentInfo->meta.maintainer);
                versionLabel->setText(currentInfo->meta.version);
                descEdit->setText(currentInfo->meta.description);
            });

        listWidget->setCurrentRow(0);

        connect(enableRadio, &QRadioButton::toggled, [&](bool checked) {
            if (checked && currentInfo) {
                currentInfo->state = 1;
            }
        });

        connect(disableRadio, &QRadioButton::toggled, [&](bool checked) {
            if (checked && currentInfo) {
                currentInfo->state = 2;
            }
        });

        connect(unsureRadio, &QRadioButton::toggled, [&](bool checked) {
            if (checked && currentInfo) {
                currentInfo->state = 0;
            }
        });

        connect(enableAllBtn, &QPushButton::clicked, [&]() {
            for (auto* info : unspec) {
                info->state = 1;
            }
            if (currentInfo && currentInfo->state == 1) {
                enableRadio->setChecked(true);
            }
        });

        connect(disableAllBtn, &QPushButton::clicked, [&]() {
            for (auto* info : unspec) {
                info->state = 2;
            }
            if (currentInfo && currentInfo->state == 2) {
                disableRadio->setChecked(true);
            }
        });

        connect(keepBtn, &QPushButton::clicked, [&]() {
            for (auto* info : unspec) {
                info->state = 0;
            }
            if (currentInfo) {
                unsureRadio->setChecked(true);
            }
        });

        connect(doneBtn, &QPushButton::clicked, [&]() {
            for (auto* info : unspec) {
                if (info->state != 0) {
                    Extense::setState(info->meta.name, info->meta.maintainer, info->state);
                    if (info->state == 1 && info->inst) {
                        info->inst->apply(this, &Browser_Backend::instance());
                    }
                }
            }
            dlg.accept();
        });

        dlg.exec();
    }
};
int main(int argc, char *argv[]) {

    QWebEngineUrlScheme onuScheme("onu");
    onuScheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    onuScheme.setFlags(QWebEngineUrlScheme::SecureScheme |
                       QWebEngineUrlScheme::LocalAccessAllowed |
                       QWebEngineUrlScheme::ContentSecurityPolicyIgnored);
    QWebEngineUrlScheme::registerScheme(onuScheme);

    auto& backend = Settings_Backend::instance();
    QString flags = backend.chromiumFlags();
    if (!flags.isEmpty()) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.toUtf8());
    }

    QApplication app(argc, argv);
    app.setApplicationName("Onu");
    app.setApplicationVersion("0.6");

    if (!Onu_window::initializeEncryption()) {
        return 0;

    }

    QWebEngineProfile *profile = Onu_Web::defaultProfile();

    auto handler = new Url_Scheme(&Browser_Backend::instance(), profile);
    profile->installUrlSchemeHandler("onu", handler);

    Onu_window window;

    QCommandLineParser parser;
    parser.setApplicationDescription("\033[1m\033[1;37mOnu.\033[0m\n---------------------------------------------------\nOnu is a Qt C++ based web browser, \nProject url: https://github.com/zynomon/onu\nLicense: Apache 2.0\nVersion:" + app.applicationVersion() + "\n\ndefine url to open them");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("urls", "URLs to open");
    parser.process(app);

    window.show();

    QList<QUrl> startupUrls;
    for (const QString &arg : parser.positionalArguments()) {
        startupUrls << QUrl::fromUserInput(arg);
    }

    if (!startupUrls.isEmpty()) {
        window.openUrls(startupUrls);
    }

    return app.exec();
}
#include "main.moc"
/*--------------------------

                 Zynomon aelius <zynomon@proton.me> ©️2026, Apache 2.0
                                                     project : Onu 0.6

                                                      --------------------*/
