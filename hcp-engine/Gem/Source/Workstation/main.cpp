/// HCP Source Workstation — Dual-mode entry point.
///
/// Initializes both:
///   1. HCPWorkstationEngine (embedded DB kernels + LMDB vocab) for offline browsing
///   2. HCPSocketClient (daemon connection) for physics operations
///
/// The workstation works without a daemon for data browsing and editing.
/// Physics operations (ingest, tokenize, phys_resolve) require the daemon.

#include "HCPWorkstationEngine.h"
#include "HCPWorkstationWindow.h"
#include "HCPSocketClient.h"

#include <QApplication>
#include <QStyleFactory>
#include <QCommandLineParser>
#include <QProcess>

#include <cstdio>

namespace
{
    struct WorkstationConfig
    {
        // Daemon connection
        QString host = "127.0.0.1";
        quint16 port = 9720;
        bool spawnDaemon = false;
        QString daemonPath;

        // Direct DB access
        QString dbConnection;  // libpq connection string (empty = default)
        QString vocabPath;     // LMDB vocab directory (empty = default)

        // Entity DBs (optional)
        QString ficEntConnection;
        QString nfEntConnection;

        // Viewer mode — hides all write controls, read-only presentation
        bool viewerMode = false;
    };

    WorkstationConfig ParseCommandLine(QApplication& app)
    {
        WorkstationConfig config;

        QCommandLineParser parser;
        parser.setApplicationDescription("HCP Source Workstation");
        parser.addHelpOption();
        parser.addVersionOption();

        // Daemon connection options
        QCommandLineOption hostOption("host",
            "Engine daemon host (default: 127.0.0.1)", "address", "127.0.0.1");
        parser.addOption(hostOption);

        QCommandLineOption portOption("port",
            "Engine daemon port (default: 9720)", "port", "9720");
        parser.addOption(portOption);

        QCommandLineOption viewerOption("viewer",
            "Viewer mode — read-only, hides write controls and ingest features");
        parser.addOption(viewerOption);

        QCommandLineOption spawnOption("spawn-daemon",
            "Spawn the engine daemon on startup and kill on exit");
        parser.addOption(spawnOption);

        QCommandLineOption daemonPathOption("daemon-path",
            "Path to the engine daemon binary (HeadlessServerLauncher)", "path");
        parser.addOption(daemonPathOption);

        // Direct DB options
        QCommandLineOption dbOption("db-connection",
            "PostgreSQL connection string for hcp_fic_pbm (default: local dev)", "conninfo");
        parser.addOption(dbOption);

        QCommandLineOption vocabOption("vocab",
            "LMDB vocabulary directory path", "path");
        parser.addOption(vocabOption);

        QCommandLineOption ficEntOption("fic-entities-db",
            "PostgreSQL connection string for hcp_fic_entities", "conninfo");
        parser.addOption(ficEntOption);

        QCommandLineOption nfEntOption("nf-entities-db",
            "PostgreSQL connection string for hcp_nf_entities", "conninfo");
        parser.addOption(nfEntOption);

        parser.process(app);

        config.viewerMode = parser.isSet(viewerOption);
        config.host = parser.value(hostOption);
        config.port = static_cast<quint16>(parser.value(portOption).toUInt());
        config.spawnDaemon = parser.isSet(spawnOption);
        config.daemonPath = parser.value(daemonPathOption);
        config.dbConnection = parser.value(dbOption);
        config.vocabPath = parser.value(vocabOption);
        config.ficEntConnection = parser.value(ficEntOption);
        config.nfEntConnection = parser.value(nfEntOption);

        return config;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HCP Source Workstation");
    app.setApplicationVersion("0.1.0-alpha");
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

    fprintf(stderr, "[HCP Workstation] Starting (v%s)...\n",
        app.applicationVersion().toUtf8().constData());

    // ---- Initialize embedded engine (DB kernels + LMDB vocab) ----
    HCPEngine::HCPWorkstationEngine engine;

    // Keep QByteArrays alive so constData() pointers remain valid
    QByteArray dbConnBuf = config.dbConnection.toUtf8();
    QByteArray vocabBuf = config.vocabPath.toUtf8();
    QByteArray ficEntBuf = config.ficEntConnection.toUtf8();
    QByteArray nfEntBuf = config.nfEntConnection.toUtf8();

    engine.Initialize(
        config.dbConnection.isEmpty() ? nullptr : dbConnBuf.constData(),
        config.vocabPath.isEmpty() ? nullptr : vocabBuf.constData(),
        config.ficEntConnection.isEmpty() ? nullptr : ficEntBuf.constData(),
        config.nfEntConnection.isEmpty() ? nullptr : nfEntBuf.constData());

    // ---- Initialize socket client (daemon connection, optional) ----
    HCPEngine::HCPSocketClient client(config.host, config.port);

    fprintf(stderr, "  DB: %s | Vocab: %s | Daemon: %s:%d\n",
        engine.IsDbConnected() ? "connected" : "unavailable",
        engine.IsVocabLoaded() ? "loaded" : "unavailable",
        config.host.toUtf8().constData(), config.port);
    fflush(stderr);

    // Optional daemon management
    QProcess* daemonProc = nullptr;
    if (config.spawnDaemon && !config.daemonPath.isEmpty())
    {
        daemonProc = new QProcess(&app);
        daemonProc->setProcessChannelMode(QProcess::ForwardedChannels);
        daemonProc->start(config.daemonPath, QStringList{});
        if (!daemonProc->waitForStarted(5000))
        {
            fprintf(stderr, "[HCP Workstation] WARNING: Failed to spawn daemon: %s\n",
                config.daemonPath.toUtf8().constData());
            fflush(stderr);
        }
        else
        {
            fprintf(stderr, "[HCP Workstation] Spawned daemon (PID %lld)\n",
                daemonProc->processId());
            fflush(stderr);
        }
    }

    // ---- Create and show main window ----
    HCPEngine::HCPWorkstationWindow window(&engine, &client, nullptr, config.viewerMode);
    window.show();

    // Begin async daemon connection (non-blocking)
    client.ConnectToEngine();

    int result = app.exec();

    // Shutdown
    engine.Shutdown();

    if (daemonProc && daemonProc->state() != QProcess::NotRunning)
    {
        fprintf(stderr, "[HCP Workstation] Stopping daemon...\n");
        fflush(stderr);
        daemonProc->terminate();
        if (!daemonProc->waitForFinished(3000))
            daemonProc->kill();
    }

    return result;
}
