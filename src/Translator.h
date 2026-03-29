#pragma once
#include <QObject>
#include <QMap>
#include <QString>
#include <QApplication>

class Translator : public QObject {
    Q_OBJECT
public:
    static Translator& instance() {
        static Translator t;
        return t;
    }

    void setLanguage(const QString &lang) {
        m_lang = lang;
        emit languageChanged();
    }

    QString get(const QString &key) const {
        if (m_lang == "English") return key;
        auto it = m_translations.find(m_lang);
        if (it == m_translations.end()) return key;
        return it->value(key, key);
    }

signals:
    void languageChanged();

private:
    Translator() {
        // ── Bosnian/BCS ──────────────────────────────────────────
        QMap<QString,QString> bs;
        bs["Screenshot"]            = "Snimak ekrana";
        bs["Recording"]             = "Snimanje";
        bs["History"]               = "Historija";
        bs["Settings"]              = "Postavke";
        bs["Capture Mode"]          = "Način snimanja";
        bs["Full Screen"]           = "Cijeli ekran";
        bs["Region"]                = "Regija";
        bs["Window"]                = "Prozor";
        bs["Scrolling"]             = "Skrolovanje";
        bs["Timed"]                 = "Tajmer";
        bs["Color Picker"]          = "Birač boja";
        bs["Preview"]               = "Pregled";
        bs["Options"]               = "Opcije";
        bs["Delay (sec):"]          = "Odgoda (sek):";
        bs["Format:"]               = "Format:";
        bs["Quality:"]              = "Kvalitet:";
        bs["Monitor:"]              = "Monitor:";
        bs["Auto copy to clipboard"]= "Automatski kopiraj";
        bs["Auto save to folder"]   = "Automatski sačuvaj";
        bs["Include cursor"]        = "Uključi pokazivač";
        bs["Capture sound"]         = "Zvuk snimanja";
        bs["No capture yet\n\nClick a capture mode to start"] =
            "Još nema snimka\n\nKlikni način snimanja za početak";
        bs["Recording Controls"]    = "Kontrole snimanja";
        bs["Start Recording"]       = "Pokreni snimanje";
        bs["Pause"]                 = "Pauza";
        bs["Resume"]                = "Nastavi";
        bs["Stop"]                  = "Zaustavi";
        bs["Recording Options"]     = "Opcije snimanja";
        bs["Frame Rate:"]           = "Broj frejmova:";
        bs["Quality:"]              = "Kvalitet:";
        bs["Record system audio"]   = "Snimi zvuk sistema";
        bs["Show cursor"]           = "Prikaži pokazivač";
        bs["Enable live annotation"]= "Žive napomene";
        bs["Capture Area"]          = "Područje snimanja";
        bs["Select Region"]         = "Odaberi regiju";
        bs["Full Screen"]           = "Cijeli ekran";
        bs["Specific Window"]       = "Određeni prozor";
        bs["Recording Status"]      = "Status snimanja";
        bs["Ready to Record"]       = "Spreman za snimanje";
        bs["Choose your capture area and\npress Start Recording"] =
            "Odaberi područje i\nklikni Pokreni snimanje";
        bs["Recent Recordings"]     = "Nedavna snimanja";
        bs["No recordings yet"]     = "Još nema snimanja";
        bs["Capture History"]       = "Historija snimaka";
        bs["Clear All"]             = "Obriši sve";
        bs["Open Folder"]           = "Otvori folder";
        bs["Select a capture to preview"] = "Odaberi snimak za pregled";
        bs["No captures yet.\nTake a screenshot to start!"] =
            "Nema snimaka.\nNapravi snimak ekrana za početak!";
        bs["Copy"]                  = "Kopiraj";
        bs["Save As"]               = "Sačuvaj kao";
        bs["Delete"]                = "Obriši";
        bs["Copy to clipboard"]     = "Kopiraj u clipboard";
        bs["Save as file"]          = "Sačuvaj kao fajl";
        bs["Open save folder"]      = "Otvori folder";
        bs["Delete this capture"]   = "Obriši ovaj snimak";
        bs["Remove"]                = "Ukloni";
        bs["from history?"]         = "iz historije?";
        bs["(File on disk will NOT be deleted)"] = "(Fajl na disku neće biti obrisan)";
        bs["Delete all captures from history?\n(Files on disk will NOT be deleted)"] =
            "Obrisati sve snimke iz historije?\n(Fajlovi na disku neće biti obrisani)";
        bs["Save As"]               = "Sačuvaj kao";
        bs["Captured"]              = "Snimljeno";
        bs["Settings saved ✔"]      = "Postavke sačuvane ✔";
        bs["Settings"]              = "Postavke";
        bs["Region Capture Behaviour"]="Ponašanje pri snimanju regije";
        bs["After Capture"]         = "Nakon snimanja";
        bs["Save & Export"]         = "Sačuvaj i izvezi";
        bs["Tools & Annotation"]    = "Alati i napomene";
        bs["Keyboard Shortcuts"]    = "Prečice na tastaturi";
        bs["Appearance"]            = "Izgled";
        bs["Advanced"]              = "Napredno";
        bs["Theme:"]                = "Tema:";
        bs["Language:"]             = "Jezik:";
        bs["Done"]                  = "Gotovo";
        bs["Settings are saved automatically"] = "Postavke se čuvaju automatski";
        bs["Ready"]                 = "Spreman";
        bs["Copied to clipboard"]   = "Kopirano u clipboard";
        bs["ScreenMaster"]          = "ScreenMaster";
        bs["PRO  v2.0"]             = "PRO  v2.0";
        bs["Tip: Use Ctrl+Shift+S for instant region capture anywhere"] =
            "Savjet: Ctrl+Shift+S za brzo snimanje regije";
        m_translations["Bosanski / BCS"] = bs;

        // ── German ───────────────────────────────────────────────
        QMap<QString,QString> de;
        de["Screenshot"]            = "Screenshot";
        de["Recording"]             = "Aufnahme";
        de["History"]               = "Verlauf";
        de["Settings"]              = "Einstellungen";
        de["Capture Mode"]          = "Aufnahmemodus";
        de["Full Screen"]           = "Vollbild";
        de["Region"]                = "Bereich";
        de["Window"]                = "Fenster";
        de["Scrolling"]             = "Scrollend";
        de["Timed"]                 = "Timer";
        de["Color Picker"]          = "Farbwähler";
        de["Preview"]               = "Vorschau";
        de["Options"]               = "Optionen";
        de["Delay (sec):"]          = "Verzögerung (s):";
        de["Format:"]               = "Format:";
        de["Quality:"]              = "Qualität:";
        de["Monitor:"]              = "Monitor:";
        de["Auto copy to clipboard"]= "Auto Kopieren";
        de["Auto save to folder"]   = "Auto Speichern";
        de["Include cursor"]        = "Cursor einbeziehen";
        de["Recording Controls"]    = "Aufnahmesteuerung";
        de["Start Recording"]       = "Aufnahme starten";
        de["Pause"]                 = "Pause";
        de["Stop"]                  = "Stopp";
        de["Theme:"]                = "Thema:";
        de["Language:"]             = "Sprache:";
        de["Done"]                  = "Fertig";
        de["Ready"]                 = "Bereit";
        de["Settings are saved automatically"] = "Einstellungen werden automatisch gespeichert";
        m_translations["Deutsch"] = de;

        // ── French ───────────────────────────────────────────────
        QMap<QString,QString> fr;
        fr["Screenshot"]            = "Capture d'écran";
        fr["Recording"]             = "Enregistrement";
        fr["History"]               = "Historique";
        fr["Settings"]              = "Paramètres";
        fr["Capture Mode"]          = "Mode de capture";
        fr["Full Screen"]           = "Plein écran";
        fr["Region"]                = "Région";
        fr["Window"]                = "Fenêtre";
        fr["Scrolling"]             = "Défilement";
        fr["Timed"]                 = "Minuterie";
        fr["Color Picker"]          = "Pipette";
        fr["Preview"]               = "Aperçu";
        fr["Options"]               = "Options";
        fr["Delay (sec):"]          = "Délai (s):";
        fr["Format:"]               = "Format:";
        fr["Quality:"]              = "Qualité:";
        fr["Monitor:"]              = "Moniteur:";
        fr["Theme:"]                = "Thème:";
        fr["Language:"]             = "Langue:";
        fr["Done"]                  = "Terminé";
        fr["Ready"]                 = "Prêt";
        fr["Settings are saved automatically"] = "Paramètres sauvegardés automatiquement";
        m_translations["Français"] = fr;

        // ── Spanish ──────────────────────────────────────────────
        QMap<QString,QString> es;
        es["Screenshot"]            = "Captura de pantalla";
        es["Recording"]             = "Grabación";
        es["History"]               = "Historial";
        es["Settings"]              = "Configuración";
        es["Capture Mode"]          = "Modo de captura";
        es["Full Screen"]           = "Pantalla completa";
        es["Region"]                = "Región";
        es["Window"]                = "Ventana";
        es["Scrolling"]             = "Desplazamiento";
        es["Timed"]                 = "Temporizador";
        es["Color Picker"]          = "Selector de color";
        es["Preview"]               = "Vista previa";
        es["Options"]               = "Opciones";
        es["Delay (sec):"]          = "Retraso (s):";
        es["Format:"]               = "Formato:";
        es["Quality:"]              = "Calidad:";
        es["Monitor:"]              = "Monitor:";
        es["Theme:"]                = "Tema:";
        es["Language:"]             = "Idioma:";
        es["Done"]                  = "Listo";
        es["Ready"]                 = "Listo";
        es["Settings are saved automatically"] = "La configuración se guarda automáticamente";
        m_translations["Español"] = es;
    }

    QString m_lang = "English";
    QMap<QString, QMap<QString,QString>> m_translations;
};

// Convenience macro
#define TR(x) Translator::instance().get(x)
