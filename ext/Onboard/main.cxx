#include <QObject>
#include <QtPlugin>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QComboBox>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QIcon>
#include <QFont>
#include "extense.H"

class OnboardingExtension : public QObject, public IExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "onu.Iext/0.5" )
    Q_INTERFACES(IExtension)

public:
    ExtensionMetadata metadata() const override {
        ExtensionMetadata md;
        md.name = "Onboarding";
        md.version = "1.0";
        md.maintainer = "Onu Team";
        md.description = "First-time setup wizard";
        md.icon = QIcon::fromTheme("dialog-information");
        return md;
    }

    bool apply(QMainWindow* window, Browser_Backend* backend) override {
        Q_UNUSED(window);
        Q_UNUSED(backend);

        if (showOnboardingDialog()) {
            Settings_Backend::instance().setExtensionState("Onboarding", "Onu Team", 2);
            selfDestruct();
        }

        return true;
    }

private:
    QString selectedTheme = "System";
    int selectedSearchEngineIndex = 0;
    bool enableAdBlock = true;
    bool enableJavaScript = true;
    bool restoreSessions = true;
    QString originalTheme;
    bool originalAdBlock;
    bool originalJavaScript;
    bool originalRestoreSessions;

    void selfDestruct() {
        QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins/libonboarding.so";
#ifdef Q_OS_WIN
        pluginPath = QCoreApplication::applicationDirPath() + "/plugins/onboarding.dll";
#elif defined(Q_OS_MAC)
        pluginPath = QCoreApplication::applicationDirPath() + "/plugins/libonboarding.dylib";
#endif

        if (!QFile::remove(pluginPath)) {
            qWarning() << "Failed to remove onboarding plugin:" << pluginPath;
        }
    }

    void applyThemeRealtime(const QString& theme) {
        if (theme == "System") {
            qApp->setStyleSheet("");
        } else {
            QString qss = Settings_Backend::instance().loadThemeQSS(theme);
            qApp->setStyleSheet(qss);
        }
    }

    void storeOriginalSettings() {
        auto& settings = Settings_Backend::instance();
        originalTheme = settings.currentTheme();
        originalAdBlock = settings.publicValue("adblock/enabled").toBool();
        originalJavaScript = settings.publicValue("privacy/javascript").toBool();
        originalRestoreSessions = settings.publicValue("privacy/restoreSessions").toBool();
    }

    void restoreOriginalSettings() {
        auto& settings = Settings_Backend::instance();
        settings.setCurrentTheme(originalTheme);
        settings.setPublicValue("adblock/enabled", originalAdBlock);
        settings.setPublicValue("privacy/javascript", originalJavaScript);
        settings.setPublicValue("privacy/restoreSessions", originalRestoreSessions);
        applyThemeRealtime(originalTheme);
    }

    bool showOnboardingDialog() {
        storeOriginalSettings();

        QDialog dialog;
        dialog.setWindowTitle(tr("Welcome to Onu"));
        dialog.setWindowIcon(QIcon::fromTheme("onu"));
        dialog.setModal(true);
        dialog.resize(700, 500);

        QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

        QStackedWidget* stack = new QStackedWidget();

        QWidget* welcomePage = createWelcomePage();
        QWidget* settingsPage = createSettingsPage();
        QWidget* finishPage = createFinishPage();

        stack->addWidget(welcomePage);
        stack->addWidget(settingsPage);
        stack->addWidget(finishPage);

        mainLayout->addWidget(stack);

        QHBoxLayout* navLayout = new QHBoxLayout();

        QPushButton* skipBtn = new QPushButton(tr("Skip"));
        QPushButton* backBtn = new QPushButton(tr("Back"));
        backBtn->setEnabled(false);
        backBtn->setIcon(QIcon::fromTheme("go-previous"));

        QPushButton* nextBtn = new QPushButton(tr("Next"));
        nextBtn->setDefault(true);
        nextBtn->setIcon(QIcon::fromTheme("go-next"));

        QPushButton* finishBtn = new QPushButton(tr("Finish"));
        finishBtn->setVisible(false);
        finishBtn->setIcon(QIcon::fromTheme("dialog-ok-apply"));

        navLayout->addWidget(skipBtn);
        navLayout->addWidget(backBtn);
        navLayout->addStretch();
        navLayout->addWidget(nextBtn);
        navLayout->addWidget(finishBtn);

        mainLayout->addLayout(navLayout);

        connect(nextBtn, &QPushButton::clicked, [&]() {
            int current = stack->currentIndex();
            if (current < stack->count() - 1) {
                stack->setCurrentIndex(current + 1);
                backBtn->setEnabled(true);

                if (current + 1 == stack->count() - 1) {
                    nextBtn->setVisible(false);
                    finishBtn->setVisible(true);
                    finishBtn->setDefault(true);

                    QLabel* iconLabel = finishPage->findChild<QLabel*>("finishIcon");
                    if (iconLabel) {
                        QPropertyAnimation* bounce = new QPropertyAnimation(iconLabel, "geometry");
                        bounce->setDuration(500);
                        bounce->setKeyValueAt(0, iconLabel->geometry());
                        bounce->setKeyValueAt(0.5, iconLabel->geometry().adjusted(0, -10, 0, -10));
                        bounce->setKeyValueAt(1, iconLabel->geometry());
                        bounce->setEasingCurve(QEasingCurve::OutBounce);
                        bounce->start(QAbstractAnimation::DeleteWhenStopped);
                    }
                }
            }
        });

        connect(backBtn, &QPushButton::clicked, [&]() {
            int current = stack->currentIndex();
            if (current > 0) {
                stack->setCurrentIndex(current - 1);
                nextBtn->setVisible(true);
                nextBtn->setDefault(true);
                finishBtn->setVisible(false);

                if (current - 1 == 0) {
                    backBtn->setEnabled(false);
                }
            }
        });

        connect(finishBtn, &QPushButton::clicked, [&]() {
            applySettings();
            dialog.accept();
        });

        connect(skipBtn, &QPushButton::clicked, [&]() {
            restoreOriginalSettings();
            dialog.reject();
        });

        return dialog.exec() == QDialog::Accepted;
    }

    QWidget* createWelcomePage() {
        QWidget* page = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(20);

        QLabel* iconLabel = new QLabel();
        QIcon icon = QIcon::fromTheme("onu");
        if (icon.isNull()) icon = QIcon::fromTheme("internet-web-browser");
        iconLabel->setPixmap(icon.pixmap(96, 96));
        iconLabel->setAlignment(Qt::AlignCenter);

        QLabel* titleLabel = new QLabel(tr("Welcome to Onu"));
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(18);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLabel->setAlignment(Qt::AlignCenter);

        QLabel* descLabel = new QLabel(
            tr("A fast, lightweight web browser\n\n"
               "Let's set up Onu to match your preferences.\n"
               "This will only take a moment.")
            );
        descLabel->setWordWrap(true);
        descLabel->setAlignment(Qt::AlignCenter);

        layout->addStretch();
        layout->addWidget(iconLabel);
        layout->addWidget(titleLabel);
        layout->addWidget(descLabel);
        layout->addStretch();

        return page;
    }

    QWidget* createSettingsPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setSpacing(15);

        QLabel* titleLabel = new QLabel(tr("Quick Settings"));
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(14);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        layout->addWidget(titleLabel);

        QGroupBox* themeGroup = new QGroupBox(tr("Appearance"));
        QVBoxLayout* themeLayout = new QVBoxLayout(themeGroup);

        QButtonGroup* themeButtons = new QButtonGroup(themeGroup);
        QRadioButton* darkRadio = new QRadioButton(tr("Dark theme"));
        QRadioButton* lightRadio = new QRadioButton(tr("Light theme"));
        QRadioButton* systemRadio = new QRadioButton(tr("System default"));

        systemRadio->setChecked(true);
        themeButtons->addButton(darkRadio);
        themeButtons->addButton(lightRadio);
        themeButtons->addButton(systemRadio);

        themeLayout->addWidget(darkRadio);
        themeLayout->addWidget(lightRadio);
        themeLayout->addWidget(systemRadio);

        connect(darkRadio, &QRadioButton::toggled, [this](bool checked) {
            if (checked) {
                selectedTheme = "Onu-Dark";
                applyThemeRealtime("Onu-Dark");
            }
        });
        connect(lightRadio, &QRadioButton::toggled, [this](bool checked) {
            if (checked) {
                selectedTheme = "Onu-Light";
                applyThemeRealtime("Onu-Light");
            }
        });
        connect(systemRadio, &QRadioButton::toggled, [this](bool checked) {
            if (checked) {
                selectedTheme = "System";
                applyThemeRealtime("System");
            }
        });

        layout->addWidget(themeGroup);

        QGroupBox* searchGroup = new QGroupBox(tr("Search Engine"));
        QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);

        QComboBox* searchCombo = new QComboBox();
        QVariantList engines = Settings_Backend::instance().searchEngines();

        for (const QVariant& engineVar : engines) {
            QVariantMap engine = engineVar.toMap();
            QString iconName = engine["icon"].toString();
            QIcon icon = iconName.isEmpty() ? QIcon::fromTheme("edit-find") : QIcon::fromTheme(iconName);
            searchCombo->addItem(icon, engine["name"].toString());
        }

        connect(searchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            selectedSearchEngineIndex = index;
        });

        searchLayout->addWidget(searchCombo);
        layout->addWidget(searchGroup);

        QGroupBox* privacyGroup = new QGroupBox(tr("Privacy & Features"));
        QVBoxLayout* privacyLayout = new QVBoxLayout(privacyGroup);

        QCheckBox* adBlockCheck = new QCheckBox(tr("Enable ad blocking"));
        adBlockCheck->setChecked(true);

        QCheckBox* jsCheck = new QCheckBox(tr("Enable JavaScript"));
        jsCheck->setChecked(true);

        QCheckBox* sessionCheck = new QCheckBox(tr("Restore tabs on startup"));
        sessionCheck->setChecked(true);

        connect(adBlockCheck, &QCheckBox::toggled, [this](bool checked) { enableAdBlock = checked; });
        connect(jsCheck, &QCheckBox::toggled, [this](bool checked) { enableJavaScript = checked; });
        connect(sessionCheck, &QCheckBox::toggled, [this](bool checked) { restoreSessions = checked; });

        privacyLayout->addWidget(adBlockCheck);
        privacyLayout->addWidget(jsCheck);
        privacyLayout->addWidget(sessionCheck);

        layout->addWidget(privacyGroup);
        layout->addStretch();

        return page;
    }

    QWidget* createFinishPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(20);

        QLabel* iconLabel = new QLabel();
        iconLabel->setObjectName("finishIcon");
        iconLabel->setPixmap(QIcon::fromTheme("dialog-ok-apply").pixmap(64, 64));
        iconLabel->setAlignment(Qt::AlignCenter);

        QLabel* titleLabel = new QLabel(tr("You're all set!"));
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(16);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLabel->setAlignment(Qt::AlignCenter);

        QLabel* descLabel = new QLabel(
            tr("Onu is ready to use.\n\n"
               "You can change these settings anytime\n"
               "from the menu or press Ctrl+,"
               )
            );
        descLabel->setWordWrap(true);
        descLabel->setAlignment(Qt::AlignCenter);

        layout->addStretch();
        layout->addWidget(iconLabel);
        layout->addWidget(titleLabel);
        layout->addWidget(descLabel);
        layout->addStretch();

        return page;
    }

    void applySettings() {
        auto& settings = Settings_Backend::instance();

        int engineCount = settings.searchEngines().count();
        if (selectedSearchEngineIndex >= 0 && selectedSearchEngineIndex < engineCount) {
            settings.setCurrentSearchEngineIndex(selectedSearchEngineIndex);
        }

        settings.setCurrentTheme(selectedTheme);
        settings.setPublicValue("adblock/enabled", enableAdBlock);
        settings.setPublicValue("privacy/javascript", enableJavaScript);
        settings.setPublicValue("privacy/restoreSessions", restoreSessions);
    }
};

#include "main.moc"

