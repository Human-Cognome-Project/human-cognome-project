/// HCP Source Workstation — Standalone Entry Point
///
/// Bootstraps the HCPEngine system component outside the O3DE Editor lifecycle,
/// then launches the Qt main window.

#include "HCPWorkstationWindow.h"
#include "../HCPEngineSystemComponent.h"

#include <AzCore/Memory/SystemAllocator.h>

#include <QApplication>
#include <QStyleFactory>
#include <QCommandLineParser>

#include <cstdio>

namespace HCPEngine
{
    /// Standalone bootstrap wrapper — exposes protected AZ::Component lifecycle
    /// for use outside the O3DE entity system.
    class StandaloneEngine : public HCPEngineSystemComponent
    {
    public:
        void StartUp()
        {
            Init();
            Activate();
        }

        void ShutDown()
        {
            Deactivate();
        }

        /// Expose ProcessText for the workstation's pipeline ingestion.
        AZStd::string IngestText(
            const AZStd::string& text,
            const AZStd::string& docName,
            const AZStd::string& centuryCode)
        {
            return ProcessText(text, docName, centuryCode);
        }
    };
} // namespace HCPEngine

namespace
{
    struct WorkstationConfig
    {
        bool gpuMode = true;
        const char* dbBackend = "postgres";
        const char* dbConnection = nullptr;
        const char* vocabPath = nullptr;
    };

    WorkstationConfig ParseCommandLine(QApplication& app)
    {
        WorkstationConfig config;

        QCommandLineParser parser;
        parser.setApplicationDescription("HCP Source Workstation");
        parser.addHelpOption();
        parser.addVersionOption();

        QCommandLineOption cpuOption("cpu", "Force CPU-only mode (no GPU acceleration)");
        parser.addOption(cpuOption);

        QCommandLineOption dbOption("db",
            "Database backend: postgres (default) or sqlite", "backend", "postgres");
        parser.addOption(dbOption);

        QCommandLineOption dbConnOption("db-connection",
            "Database connection string", "connstr");
        parser.addOption(dbConnOption);

        QCommandLineOption vocabOption("vocab",
            "LMDB vocabulary path", "path");
        parser.addOption(vocabOption);

        parser.process(app);

        if (parser.isSet(cpuOption))
            config.gpuMode = false;

        if (parser.isSet(dbOption))
        {
            static QByteArray dbVal;
            dbVal = parser.value(dbOption).toUtf8();
            config.dbBackend = dbVal.constData();
        }

        if (parser.isSet(dbConnOption))
        {
            static QByteArray connVal;
            connVal = parser.value(dbConnOption).toUtf8();
            config.dbConnection = connVal.constData();
        }

        if (parser.isSet(vocabOption))
        {
            static QByteArray vocabVal;
            vocabVal = parser.value(vocabOption).toUtf8();
            config.vocabPath = vocabVal.constData();
        }

        return config;
    }
}

int main(int argc, char* argv[])
{
    // Initialize Qt
    QApplication app(argc, argv);
    app.setApplicationName("HCP Source Workstation");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("HCP");

    // Dark fusion style
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    WorkstationConfig config = ParseCommandLine(app);

    fprintf(stderr, "[HCP Workstation] Starting...\n");
    fprintf(stderr, "  GPU mode: %s\n", config.gpuMode ? "enabled" : "disabled (CPU only)");
    fprintf(stderr, "  DB backend: %s\n", config.dbBackend);
    fflush(stderr);

    // Create and activate the engine via standalone wrapper
    HCPEngine::StandaloneEngine engine;
    engine.StartUp();

    if (!engine.IsEngineReady())
    {
        fprintf(stderr, "[HCP Workstation] WARNING: Engine not fully ready — "
            "some features may be unavailable\n");
        fflush(stderr);
    }
    else
    {
        fprintf(stderr, "[HCP Workstation] Engine initialized successfully\n");
        fflush(stderr);
    }

    // Create and show main window
    HCPEngine::HCPWorkstationWindow window(&engine);
    window.show();

    int result = app.exec();

    // Cleanup
    engine.ShutDown();

    return result;
}
