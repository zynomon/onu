#include <QObject>
#include <QtPlugin>
#include <QMainWindow>
#include <QMessageBox>
#include <QIcon>
#include <QAction>
#include <QToolBar>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QFileDialog>
#include <QDateTime>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QSvgGenerator>
#include <QSlider>
#include <QScrollArea>
#include <QGroupBox>
#include <QColorDialog>
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDir>
#include <QComboBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <qrencode.h>
#include <cmath>
#include <algorithm>

#include "extense.H"

class QR_Creator : public QObject, public IExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IExtension_iid)
    Q_INTERFACES(IExtension)

private:
    QMainWindow* m_window = nullptr;

    enum class DotStyle {
        Square,
        Rounded,
        Circle,
        Diamond
    };

    enum class EyeStyle {
        Square,
        Rounded,
        Circle
    };

    enum class LogoFrameStyle {
        None,
        Circle,
        Square,
        RoundedSquare
    };

    struct QRSettings {
        int version = 0;
        int errorCorrectionLevel = 1;
        bool caseSensitive = true;
        int size = 300;
        int margin = 20;
        QColor foregroundColor = Qt::black;
        QColor backgroundColor = Qt::white;
        int dotStyleIndex = 0;
        int eyeStyleIndex = 0;
        int cornerRadius = 15;
        bool useGradient = false;
        QColor gradientColor2 = QColor("#2196f3");
        int gradientAngle = 45;
        bool embedLogo = false;
        QString logoPath;
        int logoSize = 60;
        int logoFrameStyle = 1;
        int logoPadding = 5;
        bool logoWhiteBackground = true;
        bool addFrame = false;
        QString frameText;
        QColor frameColor = QColor("#00bcd4");
        int frameThickness = 2;
    };

    QRSettings m_settings;

public:
    ExtensionMetadata metadata() const override {
        ExtensionMetadata md;
        md.name = "QR Code Generator <Advanced>";
        md.version = "2.0";
        md.maintainer = "Zynomon Aelius";
        md.description = "QR code generator with Advanced customizations, used Qrencode (disable the basic qr code one to resolve conflicts)";
        md.icon = QIcon::fromTheme("qtqr", QIcon::fromTheme("qrscanner-symbolic"));
        return md;
    }

    bool apply(QMainWindow* window, Browser_Backend* backend) override {
        Q_UNUSED(backend);
        m_window = window;

        QToolBar* navToolbar = window->findChild<QToolBar*>("NavigationToolbar");
        if (navToolbar) {
            QAction* action = new QAction(
                QIcon::fromTheme("qrscanner-symbolic", QIcon::fromTheme("insert-image")),
                "QR Code",
                this
                );
            action->setToolTip("Generate QR code for current page (Ctrl+Shift+Q)");
            action->setShortcut(QKeySequence("Ctrl+Shift+Q"));

            connect(action, &QAction::triggered, this, [this]() {
                generateQRFromCurrentPage();
            });

            navToolbar->addAction(action);
        }

        return true;
    }

    QList<QAction*> getContextMenuActions(QWebEngineView* view) const override {
        QList<QAction*> actions;
        if (!view) return actions;

        QUrl url = view->url();
        if (url.isEmpty() || url.scheme() == "onu") {
            return actions;
        }

        QAction* qrAction = new QAction(
            QIcon::fromTheme("qrscanner-symbolic", QIcon::fromTheme("insert-image")),
            "Generate QR Code",
            view
            );

        QString urlStr = url.toString();
        QString title = view->title();

        connect(qrAction, &QAction::triggered, this, [this, urlStr, title]() {
            const_cast<QR_Creator*>(this)->generateQR(urlStr, title);
        });

        actions.append(qrAction);
        return actions;
    }

private:
    QWebEngineView* getCurrentWebView() {
        if (!m_window) return nullptr;

        QWidget* central = m_window->centralWidget();
        if (!central) return nullptr;

        QList<QWebEngineView*> views = central->findChildren<QWebEngineView*>();

        auto it = std::find_if(views.begin(), views.end(), [](QWebEngineView* view) {
            return view && view->isVisible();
        });

        return it != views.end() ? *it : nullptr;
    }

    void generateQRFromCurrentPage() {
        QWebEngineView* view = getCurrentWebView();
        if (!view) {
            QMessageBox::warning(m_window, "QR Code", "No active page");
            return;
        }

        QUrl url = view->url();
        if (url.isEmpty() || url.scheme() == "onu") {
            QMessageBox::warning(m_window, "QR Code", "Cannot generate QR for internal pages");
            return;
        }

        generateQR(url.toString(), view->title());
    }

    QRecLevel sliderToECLevel(int value) {
        switch(value) {
        case 0: return QR_ECLEVEL_L;
        case 1: return QR_ECLEVEL_M;
        case 2: return QR_ECLEVEL_Q;
        case 3: return QR_ECLEVEL_H;
        default: return QR_ECLEVEL_M;
        }
    }

    QString getECName(int value) {
        switch(value) {
        case 0: return "Low";
        case 1: return "Medium";
        case 2: return "Quartile";
        case 3: return "High";
        default: return "Medium";
        }
    }

    QString getDotStyleName(int value) {
        switch(value) {
        case 0: return "Square";
        case 1: return "Rounded";
        case 2: return "Circle";
        case 3: return "Diamond";
        default: return "Square";
        }
    }

    QString getEyeStyleName(int value) {
        switch(value) {
        case 0: return "Square";
        case 1: return "Rounded";
        case 2: return "Circle";
        default: return "Square";
        }
    }

    QString getLogoFrameName(int value) {
        switch(value) {
        case 0: return "None";
        case 1: return "Circle";
        case 2: return "Square";
        case 3: return "Rounded";
        default: return "Circle";
        }
    }

    void generateQR(const QString& url, const QString& title) {
        QRcode* qr = QRcode_encodeString(
            url.toUtf8().constData(),
            m_settings.version,
            sliderToECLevel(m_settings.errorCorrectionLevel),
            QR_MODE_8,
            m_settings.caseSensitive ? 1 : 0
            );

        if (!qr) {
            QMessageBox::critical(m_window, "Error", "Failed to generate QR code");
            return;
        }

        QDialog dialog(m_window);
        dialog.setWindowTitle("QR Code Generator");
        dialog.setWindowIcon(QIcon::fromTheme("qrscanner-symbolic"));
        dialog.resize(900, 700);

        QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
        mainLayout->setSpacing(10);

        QLabel* titleLabel = new QLabel("<b>" + title + "</b>");
        titleLabel->setWordWrap(true);
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);

        QLabel* urlLabel = new QLabel(url);
        urlLabel->setWordWrap(true);
        urlLabel->setAlignment(Qt::AlignCenter);
        QFont urlFont = urlLabel->font();
        urlFont.setPointSize(9);
        urlLabel->setFont(urlFont);
        mainLayout->addWidget(urlLabel);

        QHBoxLayout* contentLayout = new QHBoxLayout();
        contentLayout->setSpacing(15);

        QVBoxLayout* previewLayout = new QVBoxLayout();

        QLabel* imageLabel = new QLabel();
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setMinimumSize(350, 350);
        imageLabel->setMaximumSize(350, 350);
        imageLabel->setStyleSheet("border: 1px solid palette(mid); background: white; padding: 5px;");

        QLabel* infoLabel = new QLabel();
        infoLabel->setAlignment(Qt::AlignCenter);
        QFont infoFont = infoLabel->font();
        infoFont.setPointSize(9);
        infoLabel->setFont(infoFont);

        previewLayout->addWidget(imageLabel);
        previewLayout->addWidget(infoLabel);
        contentLayout->addLayout(previewLayout);

        QScrollArea* scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setMaximumWidth(450);
        scrollArea->setMinimumWidth(400);

        QWidget* settingsWidget = new QWidget();
        QVBoxLayout* settingsLayout = new QVBoxLayout(settingsWidget);
        settingsLayout->setSpacing(15);
        settingsLayout->setContentsMargins(10, 10, 10, 10);

        QList<QSlider*> allSliders;

        QGroupBox* sizeGroup = new QGroupBox("Size");
        QFormLayout* sizeForm = new QFormLayout(sizeGroup);

        QSpinBox* sizeSpin = new QSpinBox();
        sizeSpin->setRange(150, 800);
        sizeSpin->setSingleStep(10);
        sizeSpin->setValue(m_settings.size);
        sizeSpin->setSuffix(" px");
        sizeForm->addRow("QR Code:", sizeSpin);

        QSpinBox* marginSpin = new QSpinBox();
        marginSpin->setRange(0, 50);
        marginSpin->setValue(m_settings.margin);
        marginSpin->setSuffix(" px");
        sizeForm->addRow("Margin:", marginSpin);

        settingsLayout->addWidget(sizeGroup);

        QGroupBox* ecGroup = new QGroupBox("Error Correction");
        QVBoxLayout* ecLayout = new QVBoxLayout(ecGroup);

        QSlider* ecSlider = new QSlider(Qt::Horizontal);
        ecSlider->setRange(0, 3);
        ecSlider->setValue(m_settings.errorCorrectionLevel);
        ecSlider->setTickPosition(QSlider::TicksBelow);
        ecSlider->setTickInterval(1);

        QLabel* ecValueLabel = new QLabel(getECName(m_settings.errorCorrectionLevel));
        ecValueLabel->setAlignment(Qt::AlignCenter);

        ecLayout->addWidget(ecValueLabel);
        ecLayout->addWidget(ecSlider);
        allSliders.append(ecSlider);

        settingsLayout->addWidget(ecGroup);

        QGroupBox* dotGroup = new QGroupBox("Dot Style");
        QVBoxLayout* dotLayout = new QVBoxLayout(dotGroup);

        QSlider* dotSlider = new QSlider(Qt::Horizontal);
        dotSlider->setRange(0, 3);
        dotSlider->setValue(m_settings.dotStyleIndex);
        dotSlider->setTickPosition(QSlider::TicksBelow);
        dotSlider->setTickInterval(1);

        QLabel* dotValueLabel = new QLabel(getDotStyleName(m_settings.dotStyleIndex));
        dotValueLabel->setAlignment(Qt::AlignCenter);

        dotLayout->addWidget(dotValueLabel);
        dotLayout->addWidget(dotSlider);
        allSliders.append(dotSlider);

        QLabel* radiusLabel = new QLabel("Corner Radius:");
        QSlider* radiusSlider = new QSlider(Qt::Horizontal);
        radiusSlider->setRange(0, 50);
        radiusSlider->setValue(m_settings.cornerRadius);
        radiusSlider->setTickPosition(QSlider::TicksBelow);
        radiusSlider->setTickInterval(10);

        QLabel* radiusValueLabel = new QLabel(QString::number(m_settings.cornerRadius) + "%");
        radiusValueLabel->setAlignment(Qt::AlignCenter);

        dotLayout->addWidget(radiusLabel);
        dotLayout->addWidget(radiusValueLabel);
        dotLayout->addWidget(radiusSlider);
        allSliders.append(radiusSlider);

        settingsLayout->addWidget(dotGroup);

        QGroupBox* eyeGroup = new QGroupBox("Eye Style");
        QVBoxLayout* eyeLayout = new QVBoxLayout(eyeGroup);

        QSlider* eyeSlider = new QSlider(Qt::Horizontal);
        eyeSlider->setRange(0, 2);
        eyeSlider->setValue(m_settings.eyeStyleIndex);
        eyeSlider->setTickPosition(QSlider::TicksBelow);
        eyeSlider->setTickInterval(1);

        QLabel* eyeValueLabel = new QLabel(getEyeStyleName(m_settings.eyeStyleIndex));
        eyeValueLabel->setAlignment(Qt::AlignCenter);

        eyeLayout->addWidget(eyeValueLabel);
        eyeLayout->addWidget(eyeSlider);
        allSliders.append(eyeSlider);

        settingsLayout->addWidget(eyeGroup);

        QGroupBox* colorGroup = new QGroupBox("Colors");
        QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);

        QPushButton* fgColorBtn = new QPushButton("Foreground");
        fgColorBtn->setAutoFillBackground(true);
        QString fgStyle = QString("background-color: %1; color: %2; padding: 5px;")
                              .arg(m_settings.foregroundColor.name())
                              .arg(m_settings.foregroundColor.lightness() < 128 ? "white" : "black");
        fgColorBtn->setStyleSheet(fgStyle);

        QPushButton* bgColorBtn = new QPushButton("Background");
        bgColorBtn->setAutoFillBackground(true);
        QString bgStyle = QString("background-color: %1; color: %2; padding: 5px;")
                              .arg(m_settings.backgroundColor.name())
                              .arg(m_settings.backgroundColor.lightness() < 128 ? "white" : "black");
        bgColorBtn->setStyleSheet(bgStyle);

        QCheckBox* gradientCheck = new QCheckBox("Use Gradient");
        gradientCheck->setChecked(m_settings.useGradient);

        QPushButton* gradient2Btn = new QPushButton("Gradient End");
        gradient2Btn->setEnabled(m_settings.useGradient);
        QString gradStyle = QString("background-color: %1; color: %2; padding: 5px;")
                                .arg(m_settings.gradientColor2.name())
                                .arg(m_settings.gradientColor2.lightness() < 128 ? "white" : "black");
        gradient2Btn->setStyleSheet(gradStyle);

        QLabel* angleLabel = new QLabel("Angle:");
        QSlider* angleSlider = new QSlider(Qt::Horizontal);
        angleSlider->setRange(0, 360);
        angleSlider->setValue(m_settings.gradientAngle);
        angleSlider->setEnabled(m_settings.useGradient);

        QLabel* angleValueLabel = new QLabel(QString::number(m_settings.gradientAngle) + "°");
        angleValueLabel->setAlignment(Qt::AlignCenter);
        angleValueLabel->setEnabled(m_settings.useGradient);

        colorLayout->addWidget(fgColorBtn);
        colorLayout->addWidget(bgColorBtn);
        colorLayout->addWidget(gradientCheck);
        colorLayout->addWidget(gradient2Btn);
        colorLayout->addWidget(angleLabel);
        colorLayout->addWidget(angleValueLabel);
        colorLayout->addWidget(angleSlider);
        allSliders.append(angleSlider);

        settingsLayout->addWidget(colorGroup);

        QGroupBox* logoGroup = new QGroupBox("Logo");
        QVBoxLayout* logoLayout = new QVBoxLayout(logoGroup);

        QCheckBox* logoCheck = new QCheckBox("Embed Logo");
        logoCheck->setChecked(m_settings.embedLogo);

        QPushButton* logoBtn = new QPushButton("Select Image");
        logoBtn->setEnabled(m_settings.embedLogo);

        QLabel* logoPreview = new QLabel();
        if (!m_settings.logoPath.isEmpty()) {
            QPixmap pix(m_settings.logoPath);
            if (!pix.isNull()) {
                logoPreview->setPixmap(pix.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        } else {
            logoPreview->setText("No logo");
        }
        logoPreview->setAlignment(Qt::AlignCenter);
        logoPreview->setMinimumHeight(40);

        QLabel* logoSizeLabel = new QLabel("Size:");
        QSlider* logoSizer = new QSlider(Qt::Horizontal);
        logoSizer->setRange(20, 150);
        logoSizer->setValue(m_settings.logoSize);
        logoSizer->setEnabled(m_settings.embedLogo);

        QLabel* logoSizeValue = new QLabel(QString::number(m_settings.logoSize) + " px");
        logoSizeValue->setAlignment(Qt::AlignCenter);
        logoSizeValue->setEnabled(m_settings.embedLogo);

        QLabel* frameStyleLabel = new QLabel("Frame:");
        QSlider* frameStyleSlider = new QSlider(Qt::Horizontal);
        frameStyleSlider->setRange(0, 3);
        frameStyleSlider->setValue(m_settings.logoFrameStyle);
        frameStyleSlider->setTickPosition(QSlider::TicksBelow);
        frameStyleSlider->setTickInterval(1);
        frameStyleSlider->setEnabled(m_settings.embedLogo);

        QLabel* frameStyleValue = new QLabel(getLogoFrameName(m_settings.logoFrameStyle));
        frameStyleValue->setAlignment(Qt::AlignCenter);
        frameStyleValue->setEnabled(m_settings.embedLogo);

        QLabel* paddingLabel = new QLabel("Padding:");
        QSlider* paddingSizer = new QSlider(Qt::Horizontal);
        paddingSizer->setRange(0, 20);
        paddingSizer->setValue(m_settings.logoPadding);
        paddingSizer->setEnabled(m_settings.embedLogo);

        QLabel* paddingValue = new QLabel(QString::number(m_settings.logoPadding) + " px");
        paddingValue->setAlignment(Qt::AlignCenter);
        paddingValue->setEnabled(m_settings.embedLogo);

        QCheckBox* whiteBgCheck = new QCheckBox("White Background");
        whiteBgCheck->setChecked(m_settings.logoWhiteBackground);
        whiteBgCheck->setEnabled(m_settings.embedLogo);

        logoLayout->addWidget(logoCheck);
        logoLayout->addWidget(logoBtn);
        logoLayout->addWidget(logoPreview);
        logoLayout->addWidget(logoSizeLabel);
        logoLayout->addWidget(logoSizeValue);
        logoLayout->addWidget(logoSizer);
        logoLayout->addWidget(frameStyleLabel);
        logoLayout->addWidget(frameStyleValue);
        logoLayout->addWidget(frameStyleSlider);
        logoLayout->addWidget(paddingLabel);
        logoLayout->addWidget(paddingValue);
        logoLayout->addWidget(paddingSizer);
        logoLayout->addWidget(whiteBgCheck);
        allSliders.append(logoSizer);
        allSliders.append(frameStyleSlider);
        allSliders.append(paddingSizer);

        settingsLayout->addWidget(logoGroup);

        QGroupBox* frameGroup = new QGroupBox("Text Frame");
        QVBoxLayout* frameLayout = new QVBoxLayout(frameGroup);

        QCheckBox* frameCheck = new QCheckBox("Add Frame");
        frameCheck->setChecked(m_settings.addFrame);

        QLineEdit* frameTextEdit = new QLineEdit();
        frameTextEdit->setPlaceholderText("Enter frame text");
        frameTextEdit->setText(m_settings.frameText);
        frameTextEdit->setEnabled(m_settings.addFrame);

        QPushButton* frameColorBtn = new QPushButton("Frame Color");
        frameColorBtn->setEnabled(m_settings.addFrame);
        QString frameStyle = QString("background-color: %1; color: %2; padding: 5px;")
                                 .arg(m_settings.frameColor.name())
                                 .arg(m_settings.frameColor.lightness() < 128 ? "white" : "black");
        frameColorBtn->setStyleSheet(frameStyle);

        QLabel* thicknessLabel = new QLabel("Thickness:");
        QSlider* thicknessSlider = new QSlider(Qt::Horizontal);
        thicknessSlider->setRange(1, 10);
        thicknessSlider->setValue(m_settings.frameThickness);
        thicknessSlider->setEnabled(m_settings.addFrame);

        QLabel* thicknessValue = new QLabel(QString::number(m_settings.frameThickness) + " px");
        thicknessValue->setAlignment(Qt::AlignCenter);
        thicknessValue->setEnabled(m_settings.addFrame);

        frameLayout->addWidget(frameCheck);
        frameLayout->addWidget(frameTextEdit);
        frameLayout->addWidget(frameColorBtn);
        frameLayout->addWidget(thicknessLabel);
        frameLayout->addWidget(thicknessValue);
        frameLayout->addWidget(thicknessSlider);
        allSliders.append(thicknessSlider);

        settingsLayout->addWidget(frameGroup);
        settingsLayout->addStretch();

        scrollArea->setWidget(settingsWidget);
        contentLayout->addWidget(scrollArea);
        mainLayout->addLayout(contentLayout);

        auto updatePreview = [&]() {
            m_settings.size = sizeSpin->value();
            m_settings.margin = marginSpin->value();
            m_settings.errorCorrectionLevel = ecSlider->value();
            m_settings.dotStyleIndex = dotSlider->value();
            m_settings.eyeStyleIndex = eyeSlider->value();
            m_settings.cornerRadius = radiusSlider->value();
            m_settings.gradientAngle = angleSlider->value();
            m_settings.logoSize = logoSizer->value();
            m_settings.logoFrameStyle = frameStyleSlider->value();
            m_settings.logoPadding = paddingSizer->value();
            m_settings.frameThickness = thicknessSlider->value();

            ecValueLabel->setText(getECName(ecSlider->value()));
            dotValueLabel->setText(getDotStyleName(dotSlider->value()));
            eyeValueLabel->setText(getEyeStyleName(eyeSlider->value()));
            radiusValueLabel->setText(QString::number(radiusSlider->value()) + "%");
            angleValueLabel->setText(QString::number(angleSlider->value()) + "°");
            logoSizeValue->setText(QString::number(logoSizer->value()) + " px");
            frameStyleValue->setText(getLogoFrameName(frameStyleSlider->value()));
            paddingValue->setText(QString::number(paddingSizer->value()) + " px");
            thicknessValue->setText(QString::number(thicknessSlider->value()) + " px");

            QRcode* newQr = QRcode_encodeString(
                url.toUtf8().constData(),
                m_settings.version,
                sliderToECLevel(m_settings.errorCorrectionLevel),
                QR_MODE_8,
                m_settings.caseSensitive ? 1 : 0
                );

            if (newQr) {
                QImage preview = createAdvancedQRImage(newQr);
                if (!preview.isNull()) {
                    QPixmap pixmap = QPixmap::fromImage(preview);
                    imageLabel->setPixmap(pixmap.scaled(340, 340, Qt::KeepAspectRatio, Qt::SmoothTransformation));

                    infoLabel->setText(QString("Version %1 • %2×%3 modules • %4 EC")
                                           .arg(newQr->version)
                                           .arg(newQr->width)
                                           .arg(newQr->width)
                                           .arg(getECName(m_settings.errorCorrectionLevel)));
                }
                QRcode_free(newQr);
            }
        };

        connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), updatePreview);
        connect(marginSpin, QOverload<int>::of(&QSpinBox::valueChanged), updatePreview);

        for (QSlider* slider : allSliders) {
            connect(slider, &QSlider::valueChanged, updatePreview);
        }

        connect(fgColorBtn, &QPushButton::clicked, [this, fgColorBtn, updatePreview]() {
            QColor color = QColorDialog::getColor(m_settings.foregroundColor, nullptr, "Select Foreground Color");
            if (color.isValid()) {
                m_settings.foregroundColor = color;
                fgColorBtn->setStyleSheet(QString("background-color: %1; color: %2; padding: 5px;")
                                              .arg(color.name())
                                              .arg(color.lightness() < 128 ? "white" : "black"));
                updatePreview();
            }
        });

        connect(bgColorBtn, &QPushButton::clicked, [this, bgColorBtn, updatePreview]() {
            QColor color = QColorDialog::getColor(m_settings.backgroundColor, nullptr, "Select Background Color");
            if (color.isValid()) {
                m_settings.backgroundColor = color;
                bgColorBtn->setStyleSheet(QString("background-color: %1; color: %2; padding: 5px;")
                                              .arg(color.name())
                                              .arg(color.lightness() < 128 ? "white" : "black"));
                updatePreview();
            }
        });

        connect(gradientCheck, &QCheckBox::toggled, [this, gradient2Btn, angleSlider, angleValueLabel, updatePreview](bool checked) {
            m_settings.useGradient = checked;
            gradient2Btn->setEnabled(checked);
            angleSlider->setEnabled(checked);
            angleValueLabel->setEnabled(checked);
            updatePreview();
        });

        connect(gradient2Btn, &QPushButton::clicked, [this, gradient2Btn, updatePreview]() {
            QColor color = QColorDialog::getColor(m_settings.gradientColor2, nullptr, "Select Gradient End Color");
            if (color.isValid()) {
                m_settings.gradientColor2 = color;
                gradient2Btn->setStyleSheet(QString("background-color: %1; color: %2; padding: 5px;")
                                                .arg(color.name())
                                                .arg(color.lightness() < 128 ? "white" : "black"));
                updatePreview();
            }
        });

        connect(logoCheck, &QCheckBox::toggled, [this, logoBtn, logoSizer, logoSizeValue, frameStyleSlider, frameStyleValue, paddingSizer, paddingValue, whiteBgCheck, updatePreview](bool checked) {
            m_settings.embedLogo = checked;
            logoBtn->setEnabled(checked);
            logoSizer->setEnabled(checked);
            logoSizeValue->setEnabled(checked);
            frameStyleSlider->setEnabled(checked);
            frameStyleValue->setEnabled(checked);
            paddingSizer->setEnabled(checked);
            paddingValue->setEnabled(checked);
            whiteBgCheck->setEnabled(checked);
            updatePreview();
        });

        connect(logoBtn, &QPushButton::clicked, [this, logoPreview, updatePreview]() {
            QString filter = "Images (*.png *.jpg *.jpeg *.svg *.bmp *.ico)";
            QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Logo", QDir::homePath(), filter);

            if (!fileName.isEmpty()) {
                m_settings.logoPath = fileName;
                QPixmap pix(fileName);
                if (!pix.isNull()) {
                    logoPreview->setPixmap(pix.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

                    if (pix.width() != pix.height()) {
                        QImage img(fileName);
                        int size = qMin(img.width(), img.height());
                        img = img.copy((img.width() - size) / 2, (img.height() - size) / 2, size, size);
                        m_settings.logoPath = saveTempImage(img);
                    }
                }
                updatePreview();
            }
        });

        connect(whiteBgCheck, &QCheckBox::toggled, [this, updatePreview](bool checked) {
            m_settings.logoWhiteBackground = checked;
            updatePreview();
        });

        connect(frameCheck, &QCheckBox::toggled, [this, frameTextEdit, frameColorBtn, thicknessSlider, thicknessValue, updatePreview](bool checked) {
            m_settings.addFrame = checked;
            frameTextEdit->setEnabled(checked);
            frameColorBtn->setEnabled(checked);
            thicknessSlider->setEnabled(checked);
            thicknessValue->setEnabled(checked);
            updatePreview();
        });

        connect(frameTextEdit, &QLineEdit::textChanged, [this, updatePreview](const QString& text) {
            m_settings.frameText = text;
            updatePreview();
        });

        connect(frameColorBtn, &QPushButton::clicked, [this, frameColorBtn, updatePreview]() {
            QColor color = QColorDialog::getColor(m_settings.frameColor, nullptr, "Select Frame Color");
            if (color.isValid()) {
                m_settings.frameColor = color;
                frameColorBtn->setStyleSheet(QString("background-color: %1; color: %2; padding: 5px;")
                                                 .arg(color.name())
                                                 .arg(color.lightness() < 128 ? "white" : "black"));
                updatePreview();
            }
        });

        updatePreview();

        QDialogButtonBox* buttonBox = new QDialogButtonBox();
        QPushButton* copyBtn = buttonBox->addButton("Copy URL", QDialogButtonBox::ActionRole);
        copyBtn->setIcon(QIcon::fromTheme("edit-copy"));

        QPushButton* pngBtn = buttonBox->addButton("Save PNG", QDialogButtonBox::ActionRole);
        pngBtn->setIcon(QIcon::fromTheme("image-x-generic"));

        QPushButton* svgBtn = buttonBox->addButton("Save SVG", QDialogButtonBox::ActionRole);
        svgBtn->setIcon(QIcon::fromTheme("image-svg+xml"));

        QPushButton* closeBtn = buttonBox->addButton("Close", QDialogButtonBox::RejectRole);
        closeBtn->setIcon(QIcon::fromTheme("window-close"));

        connect(copyBtn, &QPushButton::clicked, [url]() {
            QApplication::clipboard()->setText(url);
        });

        connect(pngBtn, &QPushButton::clicked, [this, url, title]() {
            savePNG(url, title);
        });

        connect(svgBtn, &QPushButton::clicked, [this, url, title]() {
            saveSVG(url, title);
        });

        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

        mainLayout->addWidget(buttonBox);

        QRcode_free(qr);
        dialog.exec();
    }

    QString saveTempImage(const QImage& image) {
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        if (tempFile.open()) {
            QString fileName = tempFile.fileName() + ".png";
            image.save(fileName, "PNG");
            return fileName;
        }
        return QString();
    }

    QImage createAdvancedQRImage(QRcode* qr) {
        if (!qr) return QImage();

        int cellSize = (m_settings.size - 2 * m_settings.margin) / qr->width;
        int imageSize = qr->width * cellSize + 2 * m_settings.margin;

        int totalSize = imageSize;
        int frameHeight = 0;
        if (m_settings.addFrame && !m_settings.frameText.isEmpty()) {
            frameHeight = 40 + m_settings.frameThickness * 2;
            totalSize += frameHeight;
        }

        QImage image(imageSize, totalSize, QImage::Format_ARGB32);
        image.fill(m_settings.backgroundColor);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        QBrush brush;
        if (m_settings.useGradient) {
            double angleRad = m_settings.gradientAngle * M_PI / 180.0;
            QPointF center(imageSize/2.0, imageSize/2.0);
            QPointF end(center.x() + imageSize * cos(angleRad), center.y() + imageSize * sin(angleRad));
            QLinearGradient gradient(center, end);
            gradient.setColorAt(0, m_settings.foregroundColor);
            gradient.setColorAt(1, m_settings.gradientColor2);
            brush = QBrush(gradient);
        } else {
            brush = QBrush(m_settings.foregroundColor);
        }

        for (int y = 0; y < qr->width; y++) {
            for (int x = 0; x < qr->width; x++) {
                if (qr->data[y * qr->width + x] & 1) {
                    bool isEye = isFinderPattern(x, y, qr->width);
                    int px = m_settings.margin + x * cellSize;
                    int py = m_settings.margin + y * cellSize;

                    painter.setBrush(brush);

                    if (isEye) {
                        drawEyeModule(painter, px, py, cellSize,
                                      static_cast<EyeStyle>(m_settings.eyeStyleIndex));
                    } else {
                        drawModule(painter, px, py, cellSize,
                                   static_cast<DotStyle>(m_settings.dotStyleIndex),
                                   m_settings.cornerRadius);
                    }
                }
            }
        }

        if (m_settings.embedLogo && !m_settings.logoPath.isEmpty()) {
            QImage logo(m_settings.logoPath);
            if (!logo.isNull()) {
                if (logo.width() != logo.height()) {
                    int size = qMin(logo.width(), logo.height());
                    logo = logo.copy((logo.width() - size) / 2, (logo.height() - size) / 2, size, size);
                }

                logo = logo.scaled(m_settings.logoSize, m_settings.logoSize,
                                   Qt::KeepAspectRatio, Qt::SmoothTransformation);

                int logoX = (imageSize - logo.width()) / 2;
                int logoY = (imageSize - logo.height()) / 2;

                if (m_settings.logoFrameStyle > 0) {
                    int frameSize = logo.width() + m_settings.logoPadding * 2;
                    int frameX = logoX - m_settings.logoPadding;
                    int frameY = logoY - m_settings.logoPadding;

                    painter.setBrush(m_settings.logoWhiteBackground ? Qt::white : m_settings.backgroundColor);
                    painter.setPen(Qt::NoPen);

                    switch (static_cast<LogoFrameStyle>(m_settings.logoFrameStyle)) {
                    case LogoFrameStyle::Circle:
                        painter.drawEllipse(frameX, frameY, frameSize, frameSize);
                        break;
                    case LogoFrameStyle::Square:
                        painter.drawRect(frameX, frameY, frameSize, frameSize);
                        break;
                    case LogoFrameStyle::RoundedSquare:
                        painter.drawRoundedRect(frameX, frameY, frameSize, frameSize, 10, 10);
                        break;
                    default:
                        break;
                    }
                }

                painter.drawImage(QRect(logoX, logoY, logo.width(), logo.height()), logo);
            }
        }

        if (m_settings.addFrame && !m_settings.frameText.isEmpty()) {
            QRect frameRect(0, imageSize, imageSize, frameHeight);
            painter.fillRect(frameRect, m_settings.frameColor);

            painter.setPen(QPen(m_settings.foregroundColor, m_settings.frameThickness));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(frameRect.adjusted(1, 1, -1, -1));

            painter.setPen(m_settings.foregroundColor.lightness() < 128 ? Qt::white : Qt::black);
            QFont font = painter.font();
            font.setPointSize(14);
            font.setBold(true);
            painter.setFont(font);
            painter.drawText(frameRect, Qt::AlignCenter, m_settings.frameText);
        }

        return image;
    }

    void savePNG(const QString& url, const QString& title) {
        QString defaultName = QString("qr_%1_%2.png")
        .arg(sanitizeFilename(title))
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

        QString filename = QFileDialog::getSaveFileName(
            m_window,
            "Save QR Code as PNG",
            defaultName,
            "PNG Images (*.png);;All Files (*)"
            );

        if (filename.isEmpty()) return;

        QRcode* qr = QRcode_encodeString(
            url.toUtf8().constData(),
            m_settings.version,
            sliderToECLevel(m_settings.errorCorrectionLevel),
            QR_MODE_8,
            m_settings.caseSensitive ? 1 : 0
            );

        if (!qr) {
            QMessageBox::critical(m_window, "Error", "Failed to generate QR code");
            return;
        }

        QImage image = createAdvancedQRImage(qr);
        QRcode_free(qr);

        if (image.isNull()) {
            QMessageBox::critical(m_window, "Error", "Failed to create image");
            return;
        }

        if (image.save(filename, "PNG")) {
            QMessageBox::information(m_window, "Success", "QR code saved successfully");
        }
    }

    void saveSVG(const QString& url, const QString& title) {
        QString defaultName = QString("qr_%1_%2.svg")
        .arg(sanitizeFilename(title))
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

        QString filename = QFileDialog::getSaveFileName(
            m_window,
            "Save QR Code as SVG",
            defaultName,
            "SVG Images (*.svg);;All Files (*)"
            );

        if (filename.isEmpty()) return;

        QRcode* qr = QRcode_encodeString(
            url.toUtf8().constData(),
            m_settings.version,
            sliderToECLevel(m_settings.errorCorrectionLevel),
            QR_MODE_8,
            m_settings.caseSensitive ? 1 : 0
            );

        if (!qr) {
            QMessageBox::critical(m_window, "Error", "Failed to generate QR code");
            return;
        }

        bool success = createAdvancedQRSVG(qr, filename);
        QRcode_free(qr);

        if (success) {
            QMessageBox::information(m_window, "Success", "QR code saved as SVG");
        }
    }

    bool createAdvancedQRSVG(QRcode* qr, const QString& filename) {
        if (!qr) return false;

        int cellSize = (m_settings.size - 2 * m_settings.margin) / qr->width;
        int imageSize = qr->width * cellSize + 2 * m_settings.margin;

        QSvgGenerator generator;
        generator.setFileName(filename);
        generator.setSize(QSize(imageSize, imageSize));
        generator.setViewBox(QRect(0, 0, imageSize, imageSize));
        generator.setTitle("QR Code");

        QPainter painter;
        if (!painter.begin(&generator)) return false;

        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(0, 0, imageSize, imageSize, m_settings.backgroundColor);
        painter.setPen(Qt::NoPen);

        QBrush brush;
        if (m_settings.useGradient) {
            double angleRad = m_settings.gradientAngle * M_PI / 180.0;
            QPointF center(imageSize/2.0, imageSize/2.0);
            QPointF end(center.x() + imageSize * cos(angleRad), center.y() + imageSize * sin(angleRad));
            QLinearGradient grad(center, end);
            grad.setColorAt(0, m_settings.foregroundColor);
            grad.setColorAt(1, m_settings.gradientColor2);
            brush = QBrush(grad);
        } else {
            brush = QBrush(m_settings.foregroundColor);
        }

        for (int y = 0; y < qr->width; y++) {
            for (int x = 0; x < qr->width; x++) {
                if (qr->data[y * qr->width + x] & 1) {
                    bool isEye = isFinderPattern(x, y, qr->width);
                    int px = m_settings.margin + x * cellSize;
                    int py = m_settings.margin + y * cellSize;

                    painter.setBrush(brush);

                    if (isEye) {
                        drawEyeModule(painter, px, py, cellSize,
                                      static_cast<EyeStyle>(m_settings.eyeStyleIndex));
                    } else {
                        drawModule(painter, px, py, cellSize,
                                   static_cast<DotStyle>(m_settings.dotStyleIndex),
                                   m_settings.cornerRadius);
                    }
                }
            }
        }

        painter.end();
        return true;
    }

    void drawModule(QPainter& painter, int x, int y, int size, DotStyle style, int cornerRadiusPercent) {
        switch (style) {
        case DotStyle::Square:
            painter.drawRect(x, y, size, size);
            break;
        case DotStyle::Rounded: {
            int radius = size * cornerRadiusPercent / 100;
            painter.drawRoundedRect(x, y, size, size, radius, radius);
            break;
        }
        case DotStyle::Circle:
            painter.drawEllipse(x, y, size, size);
            break;
        case DotStyle::Diamond: {
            QPainterPath path;
            path.moveTo(x + size/2, y);
            path.lineTo(x + size, y + size/2);
            path.lineTo(x + size/2, y + size);
            path.lineTo(x, y + size/2);
            path.closeSubpath();
            painter.drawPath(path);
            break;
        }
        }
    }

    void drawEyeModule(QPainter& painter, int x, int y, int size, EyeStyle style) {
        switch (style) {
        case EyeStyle::Square:
            painter.drawRect(x, y, size, size);
            break;
        case EyeStyle::Rounded:
            painter.drawRoundedRect(x, y, size, size, size * 0.4, size * 0.4);
            break;
        case EyeStyle::Circle:
            painter.drawEllipse(x, y, size, size);
            break;
        }
    }

    bool isFinderPattern(int x, int y, int width) {
        return (x < 7 && y < 7) ||
               (x >= width - 7 && y < 7) ||
               (x < 7 && y >= width - 7);
    }

    QString sanitizeFilename(const QString& filename) {
        QString safe = filename;
        safe = safe.left(30);
        safe = safe.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "_");
        safe = safe.replace(QRegularExpression("_+"), "_");
        if (safe.isEmpty()) safe = "qr";
        return safe;
    }
};

#include "main.moc"

