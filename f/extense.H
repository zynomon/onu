/*--------------------------

                 Zynomon aelius <zynomon@proton.me> ©️2026, Apache 2.0
                                                     project : Onu 0.5

                                                             -----------------------*/

#pragma once
#include <QtPlugin>
#include <QString>
#include <QDir>
#include <QPluginLoader>
#include "i.H"

class Onu_Window;
class Browser_Backend;
class Onu_Web;
class Dialogs;

struct ExtensionMetadata {
    QString name;
    QString version;
    QString maintainer;
    QString description;
    QIcon icon;
};

class IExtension {
public:
    virtual ~IExtension() {}
    virtual ExtensionMetadata metadata() const = 0;
    virtual bool apply(QMainWindow* window, Browser_Backend* backend) = 0;
    virtual QString injectScript() const { return QString(); }
    virtual void onPageCreated(QWebEnginePage* page) { Q_UNUSED(page); }
    virtual QList<QAction*> getContextMenuActions(QWebEngineView* view) const {
        Q_UNUSED(view);
        return QList<QAction*>();
    }
};

#define IExtension_iid "onu.Iext/0.5"
Q_DECLARE_INTERFACE(IExtension, IExtension_iid)

struct ExtInfo {
    QString path;
    QString file;
    ExtensionMetadata meta;
    int state;
    QPluginLoader* loader;
    IExtension* inst;
    ExtInfo() : state(0), loader(nullptr), inst(nullptr) {}
};

class Extense {
public:
    static QList<ExtInfo>& all() {
        static QList<ExtInfo> list;
        return list;
    }

    static void discover() {
        all().clear();
        QDir dir(Settings_Backend::instance().extensionsPath());
        QStringList files = dir.entryList(QStringList() << "*.so" << "*.dll" << "*.dylib", QDir::Files);

        for (const QString& f : files) {
            QString full = dir.absoluteFilePath(f);
            auto* loader = new QPluginLoader(full);
            auto* obj = loader->instance();

            if (!obj) {
                delete loader;
                continue;
            }

            auto* ext = qobject_cast<IExtension*>(obj);
            if (!ext) {
                loader->unload();
                delete loader;
                continue;
            }

            ExtInfo i;
            i.path = full;
            i.file = f;
            i.meta = ext->metadata();
            i.loader = loader;
            i.inst = ext;
            i.state = Settings_Backend::instance().getExtensionState(i.meta.name, i.meta.maintainer);
            all().append(i);
        }
    }

    static void applyEnabled(QMainWindow* w, Browser_Backend* b) {
        for (auto& i : all()) {
            if (i.state == 1 && i.inst) {
                i.inst->apply(w, b);
            }
        }
    }

    static void setState(const QString& name, const QString& maintainer, int s) {
        Settings_Backend::instance().setExtensionState(name, maintainer, s);
        for (auto& i : all()) {
            if (i.meta.name == name && i.meta.maintainer == maintainer) {
                i.state = s;
            }
        }
    }

    static QString collectScripts() {
        QString s;
        for (const auto& i : all()) {
            if (i.state == 1 && i.inst) {
                s += i.inst->injectScript() + "\n";
            }
        }
        return s;
    }

    static void notifyPage(QWebEnginePage* p) {
        for (const auto& i : all()) {
            if (i.state == 1 && i.inst) {
                i.inst->onPageCreated(p);
            }
        }
    }
};


/*--------------------------

                 Zynomon aelius <zynomon@proton.me> ©️2026, Apache 2.0
                                                     project : Onu 0.5

                                                             -----------------------*/
