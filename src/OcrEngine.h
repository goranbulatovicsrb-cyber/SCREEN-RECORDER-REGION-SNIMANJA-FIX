#pragma once
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

class OcrEngine : public QObject {
    Q_OBJECT
public:
    explicit OcrEngine(QObject *parent = nullptr) : QObject(parent) {}

    QString recognize(const QPixmap &pixmap) {
        if (pixmap.isNull()) return "";
#ifdef Q_OS_WIN
        return runOcr(pixmap);
#else
        return "(OCR only supported on Windows 10/11)";
#endif
    }

private:
#ifdef Q_OS_WIN
    QString runOcr(const QPixmap &pixmap) {
        QString tempPng = QDir::temp().filePath("smp_ocr_in.png");
        QString tempPs  = QDir::temp().filePath("smp_ocr.ps1");
        QString tempOut = QDir::temp().filePath("smp_ocr_out.txt");

        if (!pixmap.save(tempPng, "PNG"))
            return "(Failed to save temp image)";

        // Use MakeGenericMethod with correct IAsyncOperation`1 overload
        // This is the proven working approach for Windows 10/11 WinRT OCR
        QString winPath = tempPng;
        winPath.replace("/", "\\");

        QString ps = QString(R"(
$ErrorActionPreference = 'Stop'
try {
    Add-Type -AssemblyName System.Runtime.WindowsRuntime

    $null = [Windows.Storage.StorageFile, Windows.Storage, ContentType=WindowsRuntime]
    $null = [Windows.Media.Ocr.OcrEngine, Windows.Foundation, ContentType=WindowsRuntime]
    $null = [Windows.Graphics.Imaging.BitmapDecoder, Windows.Foundation, ContentType=WindowsRuntime]
    $null = [Windows.Storage.Streams.IRandomAccessStream, Windows.Storage.Streams, ContentType=WindowsRuntime]
    $null = [Windows.Globalization.Language, Windows.Foundation, ContentType=WindowsRuntime]

    $methods = [System.WindowsRuntimeSystemExtensions].GetMethods()
    $asTaskMethod = $methods | Where-Object {
        $_.Name -eq 'AsTask' -and $_.IsGenericMethod -and $_.GetParameters().Count -eq 1
    } | Select-Object -First 1

    function WinRTAwait($task, $type) {
        $m = $asTaskMethod.MakeGenericMethod($type)
        $t = $m.Invoke($null, @($task))
        $t.Wait()
        return $t.Result
    }

    $file    = WinRTAwait ([Windows.Storage.StorageFile]::GetFileFromPathAsync('%1')) ([Windows.Storage.StorageFile])
    $stream  = WinRTAwait ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
    $decoder = WinRTAwait ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
    $bitmap  = WinRTAwait ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])

    # Try languages in order: sr-Latn (Serbian Latin), hr (Croatian),
    # bs-Latn (Bosnian), sr-Cyrl (Serbian Cyrillic), then user profile language
    $langTags = @('sr-Latn-RS', 'sr-Latn', 'hr-HR', 'hr', 'bs-Latn-BA', 'bs', 'sr-Cyrl-RS', 'sr-Cyrl')
    $engine = $null

    foreach ($tag in $langTags) {
        try {
            $lang = [Windows.Globalization.Language]::new($tag)
            if ([Windows.Media.Ocr.OcrEngine]::IsLanguageSupported($lang)) {
                $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($lang)
                if ($null -ne $engine) { break }
            }
        } catch { }
    }

    # Fallback to user profile language (usually works for mixed content)
    if ($null -eq $engine) {
        $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
    }

    if ($null -eq $engine) {
        [System.IO.File]::WriteAllText('%2', 'OCR_NO_ENGINE')
    } else {
        $result = WinRTAwait ($engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
        [System.IO.File]::WriteAllText('%2', $result.Text, [System.Text.Encoding]::UTF8)
    }
} catch {
    [System.IO.File]::WriteAllText('%2', "OCR_FAIL: $_", [System.Text.Encoding]::UTF8)
}
)").arg(winPath, tempOut.replace("/","\\"));

        QFile psFile(tempPs);
        psFile.open(QFile::WriteOnly | QFile::Text);
        psFile.write(ps.toUtf8());
        psFile.close();

        QProcess proc;
        proc.start("powershell", {
            "-NoProfile", "-NonInteractive",
            "-ExecutionPolicy", "Bypass",
            "-File", tempPs
        });
        proc.waitForFinished(25000);

        QFile::remove(tempPng);
        QFile::remove(tempPs);

        // Read output from file (avoids encoding issues)
        QString result;
        QFile outFile(tempOut);
        if (outFile.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream ts(&outFile);
            ts.setEncoding(QStringConverter::Utf8);
            result = ts.readAll().trimmed();
            outFile.close();
        }
        QFile::remove(tempOut);

        if (result.isEmpty())
            result = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

        if (result.startsWith("OCR_FAIL:") || result.startsWith("OCR_NO_ENGINE"))
            return "(" + result + ")";

        return result.isEmpty() ? "(No text detected)" : result;
    }
#endif
};
