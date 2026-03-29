#pragma once
#include <QSettings>
#include <QString>
#include <QStandardPaths>

class SettingsManager {
public:
    static SettingsManager& instance() {
        static SettingsManager sm;
        return sm;
    }

    // Region
    bool regionAutoCapture()    { return s.value("regionAutoCapture", false).toBool(); }
    bool showMagnifier()        { return s.value("showMagnifier", true).toBool(); }
    bool showDimensions()       { return s.value("showDimensions", true).toBool(); }
    bool rememberRegion()       { return s.value("rememberRegion", false).toBool(); }

    // After capture
    bool autoCopy()             { return s.value("autoCopy", true).toBool(); }
    bool autoSave()             { return s.value("autoSave", false).toBool(); }
    bool autoAnnotate()         { return s.value("autoAnnotate", false).toBool(); }
    bool autoPreview()          { return s.value("autoPreview", false).toBool(); }
    bool miniAfter()            { return s.value("miniAfter", false).toBool(); }
    bool flashEffect()          { return s.value("flashEffect", true).toBool(); }
    bool captureSound()         { return s.value("captureSound", true).toBool(); }
    bool notify()               { return s.value("notify", true).toBool(); }
    bool pinToDesktop()         { return s.value("pinToDesktop", false).toBool(); }

    // Save
    QString savePath()          { return s.value("savePath",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
        + "/ScreenMasterPro").toString(); }
    QString filenameTemplate()  { return s.value("filenameTemplate","Screenshot_%Y%m%d_%H%M%S").toString(); }
    QString format()            { return s.value("format","PNG").toString(); }
    int     quality()           { return s.value("quality",95).toInt(); }
    bool    addShadow()         { return s.value("addShadow",false).toBool(); }
    bool    addBorder()         { return s.value("addBorder",false).toBool(); }
    bool    watermark()         { return s.value("watermark",false).toBool(); }
    QString watermarkText()     { return s.value("watermarkText","© ScreenMaster Pro").toString(); }

    // Tools
    int     defaultStroke()     { return s.value("defaultStroke",3).toInt(); }
    QString defaultTool()       { return s.value("defaultTool","Arrow").toString(); }

    // Recording
    QString recFps()            { return s.value("recFps","30 FPS").toString(); }
    QString recFormat()         { return s.value("recFormat","Image Sequence (PNG)").toString(); }
    QString recPath()           { return s.value("recPath",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
        + "/ScreenMasterPro").toString(); }
    bool    recCursor()         { return s.value("recCursor",true).toBool(); }
    bool    recCountdown()      { return s.value("recCountdown",true).toBool(); }

    // Appearance
    QString theme()             { return s.value("theme","Dark (default)").toString(); }
    QString language()          { return s.value("language","English").toString(); }
    bool    startTray()         { return s.value("startTray",false).toBool(); }
    bool    alwaysOnTop()       { return s.value("alwaysOnTop",false).toBool(); }

    // Advanced
    int     historyLimit()      { return s.value("historyLimit",100).toInt(); }
    bool    gpuAccel()          { return s.value("gpuAccel",true).toBool(); }

    void set(const QString &key, const QVariant &val) { s.setValue(key, val); }

private:
    SettingsManager() {}
    QSettings s;
};

#define SM SettingsManager::instance()
