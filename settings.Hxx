#include <sys/utsname.h>
#include "onu.H"

class Settings_Backend {
public:
    static Settings_Backend& instance() {
        static Settings_Backend inst;
        return inst;
    }

    QString Stngs_BasePath() const {
        QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString lastFolder = QFileInfo(base).fileName();
        if (lastFolder.compare("Onu", Qt::CaseInsensitive) == 0) {
            return base;
        } else {
            return base + "/Onu";
        }
    }

    QString confPath() const { return Stngs_BasePath() + "/Conf"; }
    QString dataPath() const { return Stngs_BasePath() + "/Data"; }
    QString themesPath() const { return confPath() + "/themes"; }
    QString extensionsPath() const { return confPath() + "/extensions"; }
    QString iconCachePath() const { return dataPath() + "/icon"; }
    QString themeIconCache() const { return iconCachePath() + "/ThemeCache"; }
    QString othersIconCache() const { return iconCachePath() + "/others"; }
    QString publicConfFile() const { return confPath() + "/onu.conf"; }
    QString privateConfFile() const { return dataPath() + "/encrypted.conf"; }
    QString blockedHostsFile() const { return confPath() + "/blocked_hosts.txt"; }

    void initializePaths() {
        QDir().mkpath(confPath());
        QDir().mkpath(dataPath());
        QDir().mkpath(themesPath());
        QDir().mkpath(extensionsPath());
        QDir().mkpath(themeIconCache());
        QDir().mkpath(othersIconCache());

        if (!QFile::exists(blockedHostsFile())) {
            QFile file(blockedHostsFile());
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write("# If you dont know what to do then copy paste whatever is on this page, to make adblocker work but to be honest you will feel slower page loading so be careful,\n# https://github.com/KDE/falkon/blob/master/tests/benchmarks/files/easylist.txt\n\n# Add one host per line\n");
                file.close();
            }
        }
    }

    void setPublicValue(const QString &key, const QVariant &val) {
        m_public->setValue(key, val);
        m_public->sync();
    }

    QVariant publicValue(const QString &key, const QVariant &def = QVariant()) const {
        return m_public->value(key, def);
    }

    QMap<QString, QString> defaultKeybinds() const {
        static QMap<QString, QString> defaults = {
            {"New Tab", "Ctrl+T"},
            {"Close Tab", "Ctrl+W"},
            {"Reopen Closed Tab", "Ctrl+Shift+T"},
            {"Next Tab", "Ctrl+Tab"},
            {"Previous Tab", "Ctrl+Shift+Tab"},
            {"Reload Page", "F5"},
            {"Force Reload", "Ctrl+F5"},
            {"Stop Loading", "Esc"},
            {"Find in Page", "Ctrl+F"},
            {"Find Next", "F3"},
            {"Find Previous", "Shift+F3"},
            {"Zoom In", "Ctrl++"},
            {"Zoom Out", "Ctrl+-"},
            {"Reset Zoom", "Ctrl+0"},
            {"Go Back", "Alt+Left"},
            {"Go Forward", "Alt+Right"},
            {"Go Home", "Alt+Home"},
            {"Focus Address Bar", "Ctrl+L"},
            {"Open Downloads", "Ctrl+J"},
            {"Open History", "Ctrl+H"},
            {"Open Settings", "Ctrl+,"},
            {"Open Keybinds", "Ctrl+K"},
            {"Open Extensions", "Ctrl+E"},
            {"Quit Browser", "Ctrl+Q"},
            {"Save Page", "Ctrl+S"},
            {"View Page Source", "Ctrl+U"},
            {"Toggle Developer Tools", "F12"},
            {"Toggle Fullscreen", "F11"},
            {"Bookmark Page", "Ctrl+D"},
            {"Show Bookmarks", "Ctrl+B"},
            {"Select All", "Ctrl+A"},
            {"Copy", "Ctrl+C"},
            {"Cut", "Ctrl+X"},
            {"Paste", "Ctrl+V"},
            {"Site Info", "Ctrl+I"},
            {"Sites Manager", "Ctrl+P"}
        };
        return defaults;
    }

    QString getKeybind(const QString &action) const {
        QString stored = publicValue("keybind/" + action).toString();
        if (!stored.isEmpty()) {
            return stored;
        }
        return defaultKeybinds().value(action, "");
    }

    void setKeybind(const QString &action, const QString &keybind) {
        setPublicValue("keybind/" + action, keybind);
    }

    void resetKeybind(const QString &action) {
        m_public->remove("keybind/" + action);
        m_public->sync();
    }

    void resetAllKeybinds() {
        QMap<QString, QString> defaults = defaultKeybinds();
        for (auto it = defaults.begin(); it != defaults.end(); ++it) {
            m_public->remove("keybind/" + it.key());
        }
        m_public->sync();
    }

    QMap<QString, QString> getAllKeybinds() const {
        QMap<QString, QString> defaults = defaultKeybinds();
        QMap<QString, QString> result;
        for (auto it = defaults.begin(); it != defaults.end(); ++it) {
            result[it.key()] = getKeybind(it.key());
        }
        return result;
    }

    void importKeybinds(const QMap<QString, QString> &keybinds) {
        for (auto it = keybinds.begin(); it != keybinds.end(); ++it) {
            setKeybind(it.key(), it.value());
        }
    }

    QStringList availableThemes() const {
        QStringList themes = {"System", "Onu-Dark", "Onu-Light"};
        QDir themesDir(themesPath());
        QStringList themeFiles = themesDir.entryList(QStringList() << "*.qss", QDir::Files);
        for (const QString &file : themeFiles) {
            QString baseName = QFileInfo(file).baseName();
            if (baseName != "Onu-Dark" && baseName != "Onu-Light") {
                themes.append(baseName);
            }
        }
        return themes;
    }

    bool isBuiltInTheme(const QString &themeName) const {
        return themeName == "Onu-Dark" || themeName == "Onu-Light";
    }

    QString loadThemeQSS(const QString &themeName) const {
        if (themeName == "System") {
            return QString();
        }

        QString resourcePath;

        if (themeName == "Onu-Dark") {
            resourcePath = ":/dark.qss";
        } else if (themeName == "Onu-Light") {
            resourcePath = ":/light.qss";
        } else {
            QString themeFile = themesPath() + "/" + themeName + ".qss";
            if (!QFile::exists(themeFile)) {
                qWarning() << "Custom theme file not found:" << themeFile;
                return QString();
            }
            resourcePath = themeFile;
        }

        QFile file(resourcePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to load theme:" << resourcePath
                       << "Error:" << file.errorString();
            return QString();
        }

        QByteArray content = file.readAll();
        file.close();

        QString qss = QString::fromUtf8(content);
        if (qss.isEmpty()) {
            qWarning() << "Theme loaded but content is empty:" << resourcePath;
        }
        return qss;
    }

    void saveThemeQSS(const QString &themeName, const QString &qss) {
        if (themeName == "System") return;
        if (isBuiltInTheme(themeName)) return;

        QString themeFile = themesPath() + "/" + themeName + ".qss";
        QFile file(themeFile);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(qss.toUtf8());
            file.close();
        }
    }

    void deleteTheme(const QString &themeName) {
        if (themeName == "System" || isBuiltInTheme(themeName)) return;
        QString themeFile = themesPath() + "/" + themeName + ".qss";
        QFile::remove(themeFile);
    }

    QVariantList searchEngines() const {
        QVariantList engines = publicValue("search/engines").toList();
        if (engines.isEmpty()) {
            engines.append(QVariantMap{{"name", "DuckDuckGo"}, {"url", "https://duckduckgo.com/?q=%1"}, {"icon", "duckduckgo"}});
            engines.append(QVariantMap{{"name", "Google"}, {"url", "https://www.google.com/search?q=%1"}, {"icon", "google"}});
            engines.append(QVariantMap{{"name", "Ecosia"}, {"url", "https://www.ecosia.org/search?q=%1"}, {"icon", "eco"}});
        }
        return engines;
    }

    bool showSearchEnginePicker() const { return publicValue("search/showEnginePicker", true).toBool(); }
    void setShowSearchEnginePicker(bool show) { setPublicValue("search/showEnginePicker", show); }
    int historySidebarLimit() const { return publicValue("sidebar/historyLimit", 5).toInt(); }
    void setHistorySidebarLimit(int limit) { setPublicValue("sidebar/historyLimit", limit); }
    int favSidebarLimit() const { return publicValue("sidebar/favLimit", 10).toInt(); }
    void setFavSidebarLimit(int limit) { setPublicValue("sidebar/favLimit", limit); }
    void setSearchEngines(const QVariantList &engines) { setPublicValue("search/engines", engines); }

    void addSearchEngine(const QString &name, const QString &url, const QString &icon = "") {
        QVariantList engines = searchEngines();
        engines.append(QVariantMap{{"name", name}, {"url", url}, {"icon", icon}});
        setSearchEngines(engines);
    }

    void removeSearchEngine(int index) {
        QVariantList engines = searchEngines();
        if (index >= 0 && index < engines.size()) {
            engines.removeAt(index);
            setSearchEngines(engines);
        }
    }

    int currentSearchEngineIndex() const { return publicValue("search/currentIndex", 0).toInt(); }
    void setCurrentSearchEngineIndex(int index) { setPublicValue("search/currentIndex", index); }

    QString currentSearchEngineUrl() const {
        QVariantList engines = searchEngines();
        int index = currentSearchEngineIndex();
        if (index >= 0 && index < engines.size()) {
            return engines[index].toMap()["url"].toString();
        }
        return "https://www.ecosia.org/search?q=%1";
    }

    bool suggestionsFromHistory() const { return publicValue("search/suggestHistory", true).toBool(); }
    void setSuggestionsFromHistory(bool enabled) { setPublicValue("search/suggestHistory", enabled); }
    bool suggestionsFromFavorites() const { return publicValue("search/suggestFavorites", true).toBool(); }
    void setSuggestionsFromFavorites(bool enabled) { setPublicValue("search/suggestFavorites", enabled); }
    bool suggestionsFromRemote() const { return publicValue("search/suggestRemote", false).toBool(); }
    void setSuggestionsFromRemote(bool enabled) { setPublicValue("search/suggestRemote", enabled); }
    QString suggestionApiUrl() const { return publicValue("search/apiUrl", "https://duckduckgo.com/ac/?q=%1").toString(); }
    void setSuggestionApiUrl(const QString &url) { setPublicValue("search/apiUrl", url); }

    QString suggestionParserScript() const {
        QString defaultParser =
            "// Parse API response and return array of suggestions\n"
            "// Variables: response (string), query (string)\n"
            "// Return: array of strings\n\n"
            "try {\n"
            "    var data = JSON.parse(response);\n"
            "    if (Array.isArray(data)) {\n"
            "        return data.map(function(item) {\n"
            "            return item.phrase || item;\n"
            "        });\n"
            "    }\n"
            "    return [];\n"
            "} catch(e) {\n"
            "    return [];\n"
            "}";
        return publicValue("search/parserScript", defaultParser).toString();
    }

    void setSuggestionParserScript(const QString &script) { setPublicValue("search/parserScript", script); }
    bool showSearchEngineNames() const { return publicValue("search/showEngineNames", false).toBool(); }
    void setShowSearchEngineNames(bool show) { setPublicValue("search/showEngineNames", show); }

    QStringList availableIconThemes() const {
        QStringList themes = {"System Default"};
        QStringList themePaths;
        themePaths << "/usr/share/icons" << QDir::homePath() + "/.local/share/icons" << QDir::homePath() + "/.icons";
        QSet<QString> foundThemes;
        for (const QString &basePath : themePaths) {
            QDir dir(basePath);
            if (dir.exists()) {
                QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString &entry : entries) {
                    if (QFile::exists(basePath + "/" + entry + "/index.theme")) {
                        foundThemes.insert(entry);
                    }
                }
            }
        }
        themes.append(foundThemes.values());
        themes.sort();
        return themes;
    }

    QString iconTheme() const { return publicValue("appearance/iconTheme", "System Default").toString(); }
    void setIconTheme(const QString &theme) {
        setPublicValue("appearance/iconTheme", theme);
        if (theme != "System Default") QIcon::setThemeName(theme);
    }

    QString homepage() const { return publicValue("homepage", "onu://home").toString(); }
    bool adblockEnabled() const { return publicValue("adblock/enabled", true).toBool(); }
    QString currentTheme() const { return publicValue("theme/current", "System").toString(); }
    void setCurrentTheme(const QString &theme) { setPublicValue("theme/current", theme); }
    QString chromiumFlags() const { return publicValue("flags/chromium", "").toString(); }
    bool devMode() const { return publicValue("developer/enabled", false).toBool(); }
    bool developerMode() const { return devMode(); }
    bool javascriptEnabled() const { return publicValue("privacy/javascript", true).toBool(); }
    bool imagesEnabled() const { return publicValue("privacy/images", true).toBool(); }
    int audioVolume() const { return publicValue("audio/volume", 50).toInt(); }
    QString fontFamily() const { return publicValue("appearance/font", "").toString(); }
    int fontSize() const { return publicValue("appearance/fontSize", 10).toInt(); }
    QString qtStyle() const { return publicValue("appearance/qtStyle", "System").toString(); }
    bool toolbarVisible(const QString &name) const { return publicValue("toolbar/" + name + "/visible", true).toBool(); }
    bool toolbarLockDragging(const QString &name) const { return publicValue("toolbar/" + name + "/lockDrag", false).toBool(); }
    bool toolbarLockFloating(const QString &name) const { return publicValue("toolbar/" + name + "/lockFloat", false).toBool(); }
    void setToolbarVisible(const QString &name, bool val) { setPublicValue("toolbar/" + name + "/visible", val); }
    void setToolbarLockDragging(const QString &name, bool val) { setPublicValue("toolbar/" + name + "/lockDrag", val); }
    void setToolbarLockFloating(const QString &name, bool val) { setPublicValue("toolbar/" + name + "/lockFloat", val); }
    bool showVolumeSlider() const { return publicValue("navigation/showVolume", true).toBool(); }
    bool showReload() const { return publicValue("navigation/showReload", true).toBool(); }
    bool showForward() const { return publicValue("navigation/showForward", true).toBool(); }
    bool showMenu() const { return publicValue("navigation/showMenu", true).toBool(); }
    int navigationIconSize() const { return publicValue("navigation/iconSize", 20).toInt(); }
    bool deleteCookies() const { return publicValue("privacy/deleteCookies", false).toBool(); }
    bool doNotTrack() const { return publicValue("privacy/doNotTrack", true).toBool(); }
    bool restoreSessions() const { return publicValue("privacy/restoreSessions", true).toBool(); }
    bool confirmClose() const { return publicValue("privacy/confirmClose", false).toBool(); }

    bool useCustomEncryption() const {
        return m_private->value("encryption/method", "default").toString() == "custom";
    }

    void setEncryptionMethod(const QString &method) {
        m_private->setValue("encryption/method", method);
        m_private->sync();
    }

    void setSessionKey(const QByteArray &key) {
        m_secretKey = key;
    }

    void setEncryptionVerificationValue() {
        if (!useCustomEncryption()) return;

        QByteArray data = "encryption";
        for (int i = 0; i < data.size(); ++i)
            data[i] = data[i] ^ m_secretKey[i % m_secretKey.size()];
        m_private->setValue("encryption/verify", data.toBase64());
        m_private->sync();
    }

    bool verifyEncryptionKey() {
        if (!useCustomEncryption()) return true;

        QString base64Str = m_private->value("encryption/verify").toString();
        if (base64Str.isEmpty()) {
            setEncryptionVerificationValue();
            return true;
        }

        QByteArray data = QByteArray::fromBase64(base64Str.toUtf8());
        for (int i = 0; i < data.size(); ++i)
            data[i] = data[i] ^ m_secretKey[i % m_secretKey.size()];
        return data == "encryption";
    }

    QString downloadPath() const {
        return m_private->value("path/downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
    }

    void setDownloadPath(const QString &path) {
        m_private->setValue("path/downloads", path);
        m_private->sync();
    }

    bool askDownloadConfirm() const { return publicValue("downloads/askConfirm", false).toBool(); }
    bool allowDuplicates() const { return publicValue("downloads/allowDuplicates", false).toBool(); }
    bool smartPath() const { return publicValue("downloads/smartPath", false).toBool(); }
    bool openWhenComplete() const { return publicValue("downloads/openComplete", false).toBool(); }

    QString userScript() const { return publicValue("extensions/userScript", "").toString(); }
    void setUserScript(const QString &script) { setPublicValue("extensions/userScript", script); }

    void saveHistory(const QVariantList &list) {
        m_private->setValue("user/history", encrypt(list));
        m_private->sync();
    }

    QVariantList loadHistory() {
        return decrypt(m_private->value("user/history").toString());
    }

    void saveFavorites(const QVariantList &list) {
        m_private->setValue("user/favorites", encrypt(list));
        m_private->sync();
    }

    QVariantList loadFavorites() {
        return decrypt(m_private->value("user/favorites").toString());
    }

    void saveSession(const QVariantList &urls) {
        m_private->setValue("user/session", encrypt(urls));
        m_private->sync();
    }

    QVariantList loadSession() {
        return decrypt(m_private->value("user/session").toString());
    }

    void saveToolbarState(const QByteArray &state) {
        m_private->setValue("ui/toolbarState", state.toBase64());
        m_private->sync();
    }

    QByteArray loadToolbarState() {
        return QByteArray::fromBase64(m_private->value("ui/toolbarState").toByteArray());
    }

    void clearHistory() {
        m_private->setValue("user/history", "");
        m_private->sync();
    }

    void openConfigFolder() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(confPath()));
    }

    void clearIconCache() {
        QDir(themeIconCache()).removeRecursively();
        QDir(othersIconCache()).removeRecursively();
        initializePaths();
    }

    void resetAllData() {
        setPublicValue("privacy/restoreSessions", false);
        m_public->sync();
        QDir(Stngs_BasePath()).removeRecursively();
        initializePaths();
    }

    struct ExtState {
        QString id;
        QString maintainer;
        int state;
    };
    QList<ExtState> loadExtensionStates() const {
        QList<ExtState> list;
        int count = m_private->value("ext/count", 0).toInt();
        for (int i = 0; i < count; ++i) {
            QString prefix = QString("ext/%1/").arg(i);
            ExtState s;
            s.id = m_private->value(prefix + "id").toString();
            s.maintainer = m_private->value(prefix + "maintainer").toString();
            s.state = m_private->value(prefix + "state", 0).toInt();
            list.append(s);
        }
        return list;
    }

    void saveExtensionStates(const QList<ExtState>& list) {
        m_private->setValue("ext/count", list.size());
        for (int i = 0; i < list.size(); ++i) {
            QString prefix = QString("ext/%1/").arg(i);
            m_private->setValue(prefix + "id", list[i].id);
            m_private->setValue(prefix + "maintainer", list[i].maintainer);
            m_private->setValue(prefix + "state", list[i].state);
        }
        m_private->sync();
    }

    int getExtensionState(const QString& id, const QString& maintainer) const {
        auto list = loadExtensionStates();
        for (const auto& s : list) {
            if (s.id == id && s.maintainer == maintainer) {
                return s.state;
            }
        }

        if (id == "Onboarding" && maintainer == "Onu Team") {
            return 1;
        }

        return 0;
    }

    void setExtensionState(const QString& id, const QString& maintainer, int state) {
        auto list = loadExtensionStates();
        bool found = false;
        for (auto& s : list) {
            if (s.id == id && s.maintainer == maintainer) {
                s.state = state;
                found = true;
                break;
            }
        }
        if (!found) {
            ExtState s;
            s.id = id;
            s.maintainer = maintainer;
            s.state = state;
            list.append(s);
        }
        saveExtensionStates(list);
    }

    QVariant privateValue(const QString &key, const QVariant &def = QVariant()) const {
        return m_private->value(key, def);
    }

    void setPrivateValue(const QString &key, const QVariant &val) {
        m_private->setValue(key, val);
        m_private->sync();
    }


    QString normalizeOrigin(const QString &origin) const {
        // If it's already in the right format, return as-is
        if (origin.contains("://")) {
            QUrl url(origin);
            if (url.isValid() && !url.host().isEmpty()) {
                return url.scheme() + "://" + url.host();
            }
        }

        QUrl url = QUrl::fromUserInput(origin);
        if (url.isValid() && !url.host().isEmpty()) {
            return url.scheme() + "://" + url.host();
        }

        return origin;
    }

    int getGlobalPermission(const QString &permission) const {
        return privateValue("permissions/global/" + permission, 0).toInt();
    }

    void setGlobalPermission(const QString &permission, int state) {
        setPrivateValue("permissions/global/" + permission, state);
    }

    int getSitePermission(const QString &origin, const QString &permission) const {
        QString normalized = normalizeOrigin(origin);
        QString obfuscated = obfuscateOrigin(normalized);
        QString key = QString("permissions/sites/%1/%2").arg(obfuscated, permission);
        QVariant v = privateValue(key);
        if (!v.isValid()) {
            return getGlobalPermission(permission);
        }
        return v.toInt();
    }

    void setSitePermission(const QString &origin, const QString &permission, int state) {
        QString normalized = normalizeOrigin(origin);
        QString obfuscated = obfuscateOrigin(normalized);
        QString key = QString("permissions/sites/%1/%2").arg(obfuscated, permission);
        setPrivateValue(key, state);
        storeOriginMapping(normalized);
    }

    void storeOriginMapping(const QString &origin) {
        QString normalized = normalizeOrigin(origin);
        QString obfuscated = obfuscateOrigin(normalized);
        setPrivateValue("permissions/origins/" + obfuscated, normalized);
    }

    QStringList getPermissionSites() const {
        QSet<QString> sites;
        m_private->beginGroup("permissions/origins");
        QStringList hashes = m_private->childKeys();
        for (const QString &hash : hashes) {
            QString origin = m_private->value(hash).toString();
            if (!origin.isEmpty()) {
                sites.insert(origin);
            }
        }
        m_private->endGroup();
        return QStringList(sites.begin(), sites.end());
    }

    QMap<QString, int> getSitePermissions(const QString &origin) const {
        QMap<QString, int> perms;
        QString normalized = normalizeOrigin(origin);
        QString obfuscated = obfuscateOrigin(normalized);
        m_private->beginGroup("permissions/sites/" + obfuscated);
        QStringList keys = m_private->childKeys();
        for (const QString &key : keys) {
            perms[key] = m_private->value(key).toInt();
        }
        m_private->endGroup();
        return perms;
    }

    void clearSitePermissions(const QString &origin) {
        QString normalized = normalizeOrigin(origin);
        QString obfuscated = obfuscateOrigin(normalized);
        m_private->beginGroup("permissions/sites/" + obfuscated);
        m_private->remove("");
        m_private->endGroup();
        m_private->remove("permissions/origins/" + obfuscated);
        m_private->sync();
    }

    void clearAllSitePermissions() {
        m_private->beginGroup("permissions/sites");
        m_private->remove("");
        m_private->endGroup();
        m_private->beginGroup("permissions/origins");
        m_private->remove("");
        m_private->endGroup();
        m_private->sync();
    }

    static QStringList allPermissionTypes() {
        return {
            "geolocation",
            "camera",
            "microphone",
            "camera_microphone",
            "mouselock",
            "notifications",
            "popups",
            "clipboard",
            "localFonts",
            "desktopVideo",
            "desktopAudioVideo",
            "fileAccess"
        };
    }

private:
    Settings_Backend() {
        initializePaths();
        m_public = std::make_unique<QSettings>(publicConfFile(), QSettings::IniFormat);
        m_private = std::make_unique<QSettings>(privateConfFile(), QSettings::IniFormat);
        struct utsname b;
        QByteArray key;
        if (uname(&b) == 0) {
            key.append(b.sysname);
            key.append(b.machine);
        }
        m_secretKey = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }

    QString encrypt(const QVariantList &list) {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << list;
        for (int i = 0; i < data.size(); ++i)
            data[i] = data[i] ^ m_secretKey[i % m_secretKey.size()];
        return data.toBase64();
    }

    QVariantList decrypt(const QString &base64Str) {
        if (base64Str.isEmpty()) return QVariantList();
        QByteArray data = QByteArray::fromBase64(base64Str.toUtf8());
        for (int i = 0; i < data.size(); ++i)
            data[i] = data[i] ^ m_secretKey[i % m_secretKey.size()];
        QVariantList list;
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> list;
        return list;
    }

    QString obfuscateOrigin(const QString &origin) const {
        QByteArray data = origin.toUtf8();
        QByteArray result = data;
        for (int i = 0; i < result.size(); ++i) {
            result[i] = result[i] ^ m_secretKey[i % m_secretKey.size()];
        }
        return QString::fromLatin1(result.toHex());
    }

    QByteArray m_secretKey;
    std::unique_ptr<QSettings> m_public;
    std::unique_ptr<QSettings> m_private;
};

/*--------------------------

                 Zynomon aelius <zynomon@proton.me> ©️2026, Apache 2.0
                                                     project : Onu 0.6

                     */
