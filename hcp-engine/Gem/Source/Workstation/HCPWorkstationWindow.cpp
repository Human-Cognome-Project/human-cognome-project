/// HCP Source Workstation — Dual-mode main window implementation.
///
/// Data browsing (list, info, metadata, bonds, vars, entities, text) uses
/// embedded kernels via HCPWorkstationEngine for direct DB + LMDB access.
/// Physics operations (ingest, tokenize, phys_resolve) use the socket client
/// to communicate with the engine daemon.

#include "HCPWorkstationWindow.h"
#include "HCPWorkstationEngine.h"
#include "HCPSocketClient.h"
#include "../HCPTokenizer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QFont>
#include <QBrush>
#include <QColor>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QProgressBar>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>

#include <AzCore/std/containers/unordered_map.h>

namespace HCPEngine
{
    // ---- Surface resolution helper (same logic as editor widget) ----

    static const AZStd::unordered_map<AZStd::string, const char*> s_structuralMarkers = {
        {"AA.AE.AA.AA", "document_start"}, {"AA.AE.AA.AB", "document_end"},
        {"AA.AE.AA.AC", "part_break"}, {"AA.AE.AA.AD", "chapter_break"},
        {"AA.AE.AA.AE", "section_break"}, {"AA.AE.AA.AF", "subsection_break"},
        {"AA.AE.AA.AG", "subsubsection_break"}, {"AA.AE.AA.AH", "minor_break"},
        {"AA.AE.AB.AA", "paragraph_start"}, {"AA.AE.AB.AB", "paragraph_end"},
        {"AA.AE.AB.AC", "sentence_start"}, {"AA.AE.AB.AD", "sentence_end"},
        {"AA.AE.AB.AE", "line_start"}, {"AA.AE.AB.AF", "line_end"},
        {"AA.AE.AF.AA.AA", "stream_start"}, {"AA.AE.AF.AA.AB", "stream_end"},
        {"AA.AE.AF.AA.AC", "var_request"},
    };

    QString HCPWorkstationWindow::ResolveSurface(const AZStd::string& tokenId)
    {
        if (!m_engine || !m_engine->IsVocabLoaded())
            return {};

        const auto& vocab = m_engine->GetVocabulary();

        // Word tokens (most common)
        AZStd::string word = vocab.TokenToWord(tokenId);
        if (!word.empty())
            return QString::fromUtf8(word.c_str(), static_cast<int>(word.size()));

        // Single-character tokens
        char c = vocab.TokenToChar(tokenId);
        if (c != '\0')
        {
            switch (c)
            {
            case '\n': return QString("\\n [LF]");
            case '\r': return QString("\\r [CR]");
            case '\t': return QString("\\t [TAB]");
            case ' ':  return QString("[SP]");
            default:   return QString(QChar(c));
            }
        }

        // Structural markers
        auto it = s_structuralMarkers.find(tokenId);
        if (it != s_structuralMarkers.end())
            return QString("[%1]").arg(it->second);

        return {};
    }

    // ---- Constructor / destructor ----

    HCPWorkstationWindow::HCPWorkstationWindow(
        HCPWorkstationEngine* engine, HCPSocketClient* client, QWidget* parent, bool viewerMode)
        : QMainWindow(parent)
        , m_engine(engine)
        , m_client(client)
        , m_viewerMode(viewerMode)
    {
        setWindowTitle(m_viewerMode ? "HCP Viewer" : "HCP Source Workstation");
        setMinimumSize(1200, 800);
        setAcceptDrops(true);

        BuildMenuBar();
        BuildStatusBar();
        BuildCentralWidget();

        // Wire up daemon connection signals (if client provided)
        if (m_client)
        {
            connect(m_client, &HCPSocketClient::connected,
                this, &HCPWorkstationWindow::OnEngineConnected);
            connect(m_client, &HCPSocketClient::disconnected,
                this, &HCPWorkstationWindow::OnEngineDisconnected);
            connect(m_client, &HCPSocketClient::connectionFailed,
                this, &HCPWorkstationWindow::OnEngineConnectionFailed);
        }

        // If we have a working engine, enable controls immediately
        if (m_engine && m_engine->IsDbConnected())
        {
            SetControlsEnabled(true);
            PopulateDocumentList();
        }
        else
        {
            SetControlsEnabled(false);
        }

        UpdateStatusBar();
    }

    HCPWorkstationWindow::~HCPWorkstationWindow() = default;

    // ---- Connection state (daemon socket) ----

    void HCPWorkstationWindow::OnEngineConnected()
    {
        // Daemon connected — update status, refresh if we didn't have DB
        UpdateStatusBar();
        if (!m_engine || !m_engine->IsDbConnected())
        {
            SetControlsEnabled(true);
            PopulateDocumentList();
        }
    }

    void HCPWorkstationWindow::OnEngineDisconnected()
    {
        UpdateStatusBar();
        // Controls stay enabled if we have direct DB access
        if (!m_engine || !m_engine->IsDbConnected())
        {
            SetControlsEnabled(false);
        }
    }

    void HCPWorkstationWindow::OnEngineConnectionFailed(const QString& reason)
    {
        m_statusEngine->setText(QString("Engine: %1").arg(reason));
        m_statusEngine->setStyleSheet("color: red;");
    }

    void HCPWorkstationWindow::SetControlsEnabled(bool enabled)
    {
        m_docList->setEnabled(enabled);
        m_tabs->setEnabled(enabled);
        menuBar()->setEnabled(enabled);
    }

    // ---- Menu bar ----

    void HCPWorkstationWindow::BuildMenuBar()
    {
        auto* fileMenu = menuBar()->addMenu("&File");

        if (!m_viewerMode)
        {
            auto* openFileAction = fileMenu->addAction("&Open File...");
            openFileAction->setShortcut(QKeySequence::Open);
            connect(openFileAction, &QAction::triggered, this, &HCPWorkstationWindow::OnOpenFile);

            auto* openFolderAction = fileMenu->addAction("Open &Folder...");
            connect(openFolderAction, &QAction::triggered, this, &HCPWorkstationWindow::OnOpenFolder);

            fileMenu->addSeparator();
        }

        auto* refreshAction = fileMenu->addAction("&Refresh Document List");
        refreshAction->setShortcut(QKeySequence::Refresh);
        connect(refreshAction, &QAction::triggered, this, &HCPWorkstationWindow::OnRefreshDocuments);

        fileMenu->addSeparator();

        auto* quitAction = fileMenu->addAction("&Quit");
        quitAction->setShortcut(QKeySequence::Quit);
        connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

        auto* viewMenu = menuBar()->addMenu("&View");
        Q_UNUSED(viewMenu);
    }

    // ---- Status bar ----

    void HCPWorkstationWindow::BuildStatusBar()
    {
        m_statusDb = new QLabel("DB: --");
        m_statusEngine = new QLabel("Engine: --");
        m_statusCounts = new QLabel("");
        m_progressBar = new QProgressBar();
        m_progressBar->setMaximumWidth(200);
        m_progressBar->setVisible(false);

        statusBar()->addWidget(m_statusDb);
        statusBar()->addWidget(m_statusEngine);
        statusBar()->addWidget(m_statusCounts);
        statusBar()->addPermanentWidget(m_progressBar);
    }

    void HCPWorkstationWindow::UpdateStatusBar()
    {
        // DB status (embedded kernels)
        if (m_engine && m_engine->IsDbConnected())
        {
            m_statusDb->setText("DB: Connected");
            m_statusDb->setStyleSheet("color: green;");
        }
        else
        {
            m_statusDb->setText("DB: Disconnected");
            m_statusDb->setStyleSheet("color: red;");
        }

        // Engine daemon status
        if (m_client && m_client->IsConnected())
        {
            m_statusEngine->setText("Engine: Connected");
            m_statusEngine->setStyleSheet("color: green;");

            // Fetch vocab counts from daemon
            m_client->Health([this](const QJsonObject& resp) {
                if (resp["status"].toString() != "ok") return;

                qint64 words = static_cast<qint64>(resp["words"].toDouble());
                qint64 labels = static_cast<qint64>(resp["labels"].toDouble());
                qint64 chars = static_cast<qint64>(resp["chars"].toDouble());
                m_statusCounts->setText(
                    QString("Words: %1 | Labels: %2 | Chars: %3")
                        .arg(words).arg(labels).arg(chars));
            });
        }
        else
        {
            m_statusEngine->setText("Engine: Offline");
            m_statusEngine->setStyleSheet("color: gray;");

            // Show local vocab counts if available
            if (m_engine && m_engine->IsVocabLoaded())
            {
                const auto& vocab = m_engine->GetVocabulary();
                m_statusCounts->setText(
                    QString("Words: %1 | Labels: %2 | Chars: %3 (local)")
                        .arg(vocab.WordCount()).arg(vocab.LabelCount()).arg(vocab.CharCount()));
            }
        }
    }

    // ---- Central widget (unchanged layout) ----

    void HCPWorkstationWindow::BuildCentralWidget()
    {
        auto* central = new QWidget(this);
        auto* mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(4, 4, 4, 4);

        auto* splitter = new QSplitter(Qt::Horizontal, central);

        // ---- Left: Document navigator ----
        auto* leftWidget = new QWidget(splitter);
        auto* leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        auto* navLabel = new QLabel("Documents", leftWidget);
        QFont navFont = navLabel->font();
        navFont.setBold(true);
        navLabel->setFont(navFont);
        leftLayout->addWidget(navLabel);

        m_docList = new QTreeWidget(leftWidget);
        m_docList->setHeaderLabels({"Document", "Starters", "Bonds"});
        m_docList->setColumnWidth(0, 200);
        m_docList->setRootIsDecorated(false);
        m_docList->setAlternatingRowColors(true);
        connect(m_docList, &QTreeWidget::itemClicked,
            this, &HCPWorkstationWindow::OnDocumentSelected);
        leftLayout->addWidget(m_docList);
        splitter->addWidget(leftWidget);

        // ---- Right: Tabbed detail panel ----
        auto* rightWidget = new QWidget(splitter);
        auto* rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setContentsMargins(0, 0, 0, 0);

        // Breadcrumb
        auto* breadcrumbRow = new QHBoxLayout();
        m_breadcrumb = new QLabel("", rightWidget);
        m_breadcrumb->setTextInteractionFlags(Qt::TextSelectableByMouse);
        QFont bcFont = m_breadcrumb->font();
        bcFont.setItalic(true);
        m_breadcrumb->setFont(bcFont);
        m_breadcrumbReset = new QPushButton("Reset", rightWidget);
        m_breadcrumbReset->setFixedWidth(50);
        m_breadcrumbReset->setVisible(false);
        connect(m_breadcrumbReset, &QPushButton::clicked,
            this, &HCPWorkstationWindow::OnBreadcrumbReset);
        breadcrumbRow->addWidget(m_breadcrumb, 1);
        breadcrumbRow->addWidget(m_breadcrumbReset);
        rightLayout->addLayout(breadcrumbRow);

        m_tabs = new QTabWidget(rightWidget);
        rightLayout->addWidget(m_tabs, 1);

        // Build all 6 tabs
        auto* infoWidget = new QWidget();
        BuildInfoTab(infoWidget);
        m_tabs->addTab(infoWidget, "Info");

        auto* metaWidget = new QWidget();
        BuildMetadataTab(metaWidget);
        m_tabs->addTab(metaWidget, "Metadata");

        auto* entityWidget = new QWidget();
        BuildEntitiesTab(entityWidget);
        m_tabs->addTab(entityWidget, "Entities");

        auto* varsWidget = new QWidget();
        BuildVarsTab(varsWidget);
        m_tabs->addTab(varsWidget, "Vars");

        auto* bondsWidget = new QWidget();
        BuildBondsTab(bondsWidget);
        m_tabs->addTab(bondsWidget, "Bonds");

        auto* textWidget = new QWidget();
        BuildTextTab(textWidget);
        m_tabs->addTab(textWidget, "Text");

        splitter->addWidget(rightWidget);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 2);

        mainLayout->addWidget(splitter, 1);
        setCentralWidget(central);
    }

    void HCPWorkstationWindow::BuildInfoTab(QWidget* parent)
    {
        auto* layout = new QVBoxLayout(parent);
        layout->setAlignment(Qt::AlignTop);

        auto addInfoRow = [&](const QString& label) -> QLabel* {
            auto* row = new QHBoxLayout();
            auto* lbl = new QLabel(label + ":", parent);
            lbl->setFixedWidth(100);
            QFont boldFont = lbl->font();
            boldFont.setBold(true);
            lbl->setFont(boldFont);
            auto* val = new QLabel("-", parent);
            val->setTextInteractionFlags(Qt::TextSelectableByMouse);
            row->addWidget(lbl);
            row->addWidget(val, 1);
            layout->addLayout(row);
            return val;
        };

        m_infoDocId = addInfoRow("Doc ID");
        m_infoName = addInfoRow("Name");
        m_infoSlots = addInfoRow("Total Slots");
        m_infoUnique = addInfoRow("Unique");
        m_infoStarters = addInfoRow("Starters");
        m_infoBonds = addInfoRow("Bonds");
        layout->addStretch();
    }

    void HCPWorkstationWindow::BuildMetadataTab(QWidget* parent)
    {
        auto* layout = new QVBoxLayout(parent);

        m_metaTable = new QTableWidget(parent);
        m_metaTable->setColumnCount(2);
        m_metaTable->setHorizontalHeaderLabels({"Key", "Value"});
        m_metaTable->horizontalHeader()->setStretchLastSection(true);
        m_metaTable->setAlternatingRowColors(true);
        layout->addWidget(m_metaTable, 1);

        if (!m_viewerMode)
        {
            auto* editRow = new QHBoxLayout();
            m_metaKeyInput = new QLineEdit(parent);
            m_metaKeyInput->setPlaceholderText("Key");
            m_metaValueInput = new QLineEdit(parent);
            m_metaValueInput->setPlaceholderText("Value");
            m_metaSaveBtn = new QPushButton("Set", parent);
            connect(m_metaSaveBtn, &QPushButton::clicked,
                this, &HCPWorkstationWindow::OnSaveMetadata);
            editRow->addWidget(m_metaKeyInput);
            editRow->addWidget(m_metaValueInput);
            editRow->addWidget(m_metaSaveBtn);
            layout->addLayout(editRow);

            m_metaImportBtn = new QPushButton("Import Catalog Metadata", parent);
            connect(m_metaImportBtn, &QPushButton::clicked,
                this, &HCPWorkstationWindow::OnImportMetadata);
            layout->addWidget(m_metaImportBtn);
        }
    }

    void HCPWorkstationWindow::BuildEntitiesTab(QWidget* parent)
    {
        auto* layout = new QVBoxLayout(parent);

        m_entityTree = new QTreeWidget(parent);
        m_entityTree->setHeaderLabels({"Name", "Entity ID", "Category", "Properties"});
        m_entityTree->setColumnWidth(0, 180);
        m_entityTree->setColumnWidth(1, 140);
        m_entityTree->setColumnWidth(2, 80);
        m_entityTree->setAlternatingRowColors(true);
        m_entityTree->setRootIsDecorated(true);
        connect(m_entityTree, &QTreeWidget::itemDoubleClicked,
            this, &HCPWorkstationWindow::OnEntityClicked);
        layout->addWidget(m_entityTree, 1);
    }

    void HCPWorkstationWindow::BuildVarsTab(QWidget* parent)
    {
        auto* layout = new QVBoxLayout(parent);

        m_varTree = new QTreeWidget(parent);
        m_varTree->setHeaderLabels({"Surface", "Var ID", "Category", "Group", "Suggested Entity"});
        m_varTree->setColumnWidth(0, 200);
        m_varTree->setColumnWidth(1, 70);
        m_varTree->setColumnWidth(2, 90);
        m_varTree->setColumnWidth(3, 50);
        m_varTree->setAlternatingRowColors(true);
        m_varTree->setRootIsDecorated(false);
        m_varTree->setSortingEnabled(true);
        connect(m_varTree, &QTreeWidget::itemDoubleClicked,
            this, &HCPWorkstationWindow::OnVarClicked);
        layout->addWidget(m_varTree, 1);
    }

    void HCPWorkstationWindow::BuildBondsTab(QWidget* parent)
    {
        auto* layout = new QVBoxLayout(parent);

        auto* searchRow = new QHBoxLayout();
        m_bondSearch = new QLineEdit(parent);
        m_bondSearch->setPlaceholderText("Search starters by surface form...");
        connect(m_bondSearch, &QLineEdit::returnPressed,
            this, &HCPWorkstationWindow::OnSearchBonds);
        m_bondSearchClear = new QPushButton("Clear", parent);
        connect(m_bondSearchClear, &QPushButton::clicked,
            this, &HCPWorkstationWindow::OnClearBondSearch);
        searchRow->addWidget(m_bondSearch, 1);
        searchRow->addWidget(m_bondSearchClear);
        layout->addLayout(searchRow);

        m_bondHeader = new QLabel("Select a document to view bonds", parent);
        layout->addWidget(m_bondHeader);

        m_bondTree = new QTreeWidget(parent);
        m_bondTree->setHeaderLabels({"Token", "Surface", "Count"});
        m_bondTree->setColumnWidth(0, 160);
        m_bondTree->setColumnWidth(1, 140);
        m_bondTree->setRootIsDecorated(false);
        m_bondTree->setAlternatingRowColors(true);
        m_bondTree->setSortingEnabled(true);
        connect(m_bondTree, &QTreeWidget::itemDoubleClicked,
            this, &HCPWorkstationWindow::OnBondTokenClicked);
        layout->addWidget(m_bondTree, 1);
    }

    void HCPWorkstationWindow::BuildTextTab(QWidget* parent)
    {
        auto* layout = new QVBoxLayout(parent);

        m_retrieveBtn = new QPushButton("Load Text", parent);
        connect(m_retrieveBtn, &QPushButton::clicked,
            this, &HCPWorkstationWindow::OnRetrieveText);
        layout->addWidget(m_retrieveBtn);

        m_textView = new QTextEdit(parent);
        m_textView->setReadOnly(true);
        m_textView->setFont(QFont("Monospace", 9));
        layout->addWidget(m_textView, 1);
    }

    // ========================================================================
    // Data display methods — use embedded kernels when DB is connected
    // ========================================================================

    void HCPWorkstationWindow::PopulateDocumentList()
    {
        m_docList->clear();

        // Prefer direct DB access
        if (m_engine && m_engine->IsDbConnected())
        {
            auto docs = m_engine->GetDocumentQuery().ListDocuments();
            for (const auto& d : docs)
            {
                auto* item = new QTreeWidgetItem(m_docList);
                item->setText(0, QString::fromUtf8(d.name.c_str(), static_cast<int>(d.name.size())));
                item->setText(1, QString::number(d.starters));
                item->setText(2, QString::number(d.bonds));
                item->setData(0, Qt::UserRole,
                    QString::fromUtf8(d.docId.c_str(), static_cast<int>(d.docId.size())));
            }
            return;
        }

        // Fallback to socket
        if (m_client && m_client->IsConnected())
        {
            m_client->ListDocuments([this](const QJsonObject& resp) {
                if (resp["status"].toString() != "ok") return;
                m_docList->clear();
                QJsonArray docs = resp["documents"].toArray();
                for (int i = 0; i < docs.size(); ++i)
                {
                    QJsonObject d = docs[i].toObject();
                    auto* item = new QTreeWidgetItem(m_docList);
                    item->setText(0, d["name"].toString());
                    item->setText(1, QString::number(d["starters"].toInt()));
                    item->setText(2, QString::number(d["bonds"].toInt()));
                    item->setData(0, Qt::UserRole, d["doc_id"].toString());
                }
            });
        }
    }

    void HCPWorkstationWindow::OnRefreshDocuments()
    {
        PopulateDocumentList();
        UpdateStatusBar();
    }

    void HCPWorkstationWindow::OnDocumentSelected(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item) return;
        m_selectedDocId = item->data(0, Qt::UserRole).toString();
        m_selectedDocPk = 0;
        m_activeFilter.clear();
        m_breadcrumb->clear();
        m_breadcrumbReset->setVisible(false);

        ShowDocumentInfo(m_selectedDocId);
        ShowEntities(m_selectedDocId);
        ShowVars(m_selectedDocId);
        ShowBonds(m_selectedDocId);
    }

    void HCPWorkstationWindow::ShowDocumentInfo(const QString& docId)
    {
        AZStd::string azDocId(docId.toUtf8().constData(), docId.toUtf8().size());

        // Direct DB path
        if (m_engine && m_engine->IsDbConnected())
        {
            auto detail = m_engine->GetDocumentQuery().GetDocumentDetail(azDocId);
            m_selectedDocPk = detail.pk;

            m_infoDocId->setText(QString::fromUtf8(detail.docId.c_str(), static_cast<int>(detail.docId.size())));
            m_infoName->setText(QString::fromUtf8(detail.name.c_str(), static_cast<int>(detail.name.size())));
            m_infoSlots->setText(QString::number(detail.totalSlots));
            m_infoUnique->setText(QString::number(detail.uniqueTokens));
            m_infoStarters->setText(QString::number(detail.starters));
            m_infoBonds->setText(QString::number(detail.bonds));

            // Metadata table
            m_metaTable->setRowCount(0);
            if (!detail.metadataJson.empty())
            {
                QByteArray jsonBytes(detail.metadataJson.c_str(),
                    static_cast<int>(detail.metadataJson.size()));
                QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes);
                if (jdoc.isObject())
                {
                    QJsonObject meta = jdoc.object();
                    m_metaTable->setRowCount(meta.size());
                    int row = 0;
                    for (auto it = meta.begin(); it != meta.end(); ++it, ++row)
                    {
                        m_metaTable->setItem(row, 0, new QTableWidgetItem(it.key()));
                        QString valStr;
                        if (it.value().isString())
                            valStr = it.value().toString();
                        else
                            valStr = QString::fromUtf8(
                                QJsonDocument(QJsonArray({it.value()})).toJson(QJsonDocument::Compact));
                        m_metaTable->setItem(row, 1, new QTableWidgetItem(valStr));
                    }
                }
            }
            return;
        }

        // Socket fallback
        if (m_client && m_client->IsConnected())
        {
            m_client->GetDocumentInfo(docId, [this](const QJsonObject& resp) {
                if (resp["status"].toString() != "ok") return;

                m_infoDocId->setText(resp["doc_id"].toString());
                m_infoName->setText(resp["name"].toString());
                m_infoSlots->setText(QString::number(resp["total_slots"].toInt()));
                m_infoUnique->setText(QString::number(resp["unique"].toInt()));
                m_infoStarters->setText(QString::number(resp["starters"].toInt()));
                m_infoBonds->setText(QString::number(resp["bonds"].toInt()));

                m_metaTable->setRowCount(0);
                if (resp.contains("metadata") && resp["metadata"].isObject())
                {
                    QJsonObject meta = resp["metadata"].toObject();
                    m_metaTable->setRowCount(meta.size());
                    int row = 0;
                    for (auto it = meta.begin(); it != meta.end(); ++it, ++row)
                    {
                        m_metaTable->setItem(row, 0, new QTableWidgetItem(it.key()));
                        QString valStr;
                        if (it.value().isString())
                            valStr = it.value().toString();
                        else
                            valStr = QString::fromUtf8(
                                QJsonDocument(QJsonArray({it.value()})).toJson(QJsonDocument::Compact));
                        m_metaTable->setItem(row, 1, new QTableWidgetItem(valStr));
                    }
                }
            });
        }
    }

    void HCPWorkstationWindow::ShowEntities(
        const QString& docId, const QString& filterEntityId)
    {
        m_entityTree->clear();

        if (!m_engine || !m_engine->IsDbConnected() || m_selectedDocPk == 0)
            return;

        // Fiction entities — direct DB cross-reference
        auto ficEntities = m_engine->GetFictionEntities(m_selectedDocPk);
        if (!ficEntities.empty())
        {
            auto* ficRoot = new QTreeWidgetItem(m_entityTree);
            ficRoot->setText(0, "Fiction Characters");
            ficRoot->setExpanded(true);

            for (const auto& ent : ficEntities)
            {
                QString entId = QString::fromUtf8(ent.entityId.c_str(), static_cast<int>(ent.entityId.size()));

                // Apply filter if active
                if (!filterEntityId.isEmpty() && entId != filterEntityId)
                    continue;

                auto* item = new QTreeWidgetItem(ficRoot);
                item->setText(0, QString::fromUtf8(ent.name.c_str(), static_cast<int>(ent.name.size())));
                item->setText(1, entId);
                item->setText(2, QString::fromUtf8(ent.category.c_str(), static_cast<int>(ent.category.size())));

                // Properties as comma-separated string
                QStringList props;
                for (const auto& [k, v] : ent.properties)
                {
                    props.append(QString("%1=%2")
                        .arg(QString::fromUtf8(k.c_str(), static_cast<int>(k.size())))
                        .arg(QString::fromUtf8(v.c_str(), static_cast<int>(v.size()))));
                }
                item->setText(3, props.join(", "));
            }
        }

        // Author entity — lookup from document metadata
        if (m_engine->HasEntityConnections() && !m_engine->GetVocabulary().IsLoaded())
        {
            // Skip author lookup if we can't resolve metadata
        }
        else if (m_engine->HasEntityConnections())
        {
            // Try to get author from metadata for NF entity lookup
            auto detail = m_engine->GetDocumentQuery().GetDocumentDetail(
                AZStd::string(docId.toUtf8().constData(), docId.toUtf8().size()));
            if (!detail.metadataJson.empty())
            {
                QJsonDocument jdoc = QJsonDocument::fromJson(
                    QByteArray(detail.metadataJson.c_str(), static_cast<int>(detail.metadataJson.size())));
                if (jdoc.isObject())
                {
                    QJsonObject meta = jdoc.object();
                    QString authorName;
                    if (meta.contains("authors"))
                    {
                        QJsonValue authVal = meta["authors"];
                        if (authVal.isString())
                            authorName = authVal.toString();
                        else if (authVal.isArray() && authVal.toArray().size() > 0)
                            authorName = authVal.toArray()[0].toString();
                    }

                    if (!authorName.isEmpty())
                    {
                        AZStd::string azAuthor(authorName.toUtf8().constData(), authorName.toUtf8().size());
                        auto nfEnt = m_engine->GetNfAuthor(azAuthor);
                        if (!nfEnt.entityId.empty())
                        {
                            auto* authRoot = new QTreeWidgetItem(m_entityTree);
                            authRoot->setText(0, "Authors / People");
                            authRoot->setExpanded(true);

                            auto* item = new QTreeWidgetItem(authRoot);
                            item->setText(0, QString::fromUtf8(nfEnt.name.c_str(), static_cast<int>(nfEnt.name.size())));
                            item->setText(1, QString::fromUtf8(nfEnt.entityId.c_str(), static_cast<int>(nfEnt.entityId.size())));
                            item->setText(2, QString::fromUtf8(nfEnt.category.c_str(), static_cast<int>(nfEnt.category.size())));
                        }
                    }
                }
            }
        }
    }

    void HCPWorkstationWindow::ShowBonds(const QString& docId, const QString& tokenId)
    {
        m_bondTree->clear();

        AZStd::string azDocId(docId.toUtf8().constData(), docId.toUtf8().size());
        AZStd::string azTokenId(tokenId.toUtf8().constData(), tokenId.toUtf8().size());

        // Direct DB path
        if (m_engine && m_engine->IsDbConnected() && m_selectedDocPk > 0)
        {
            auto bonds = m_engine->GetBondQuery().GetBondsForToken(m_selectedDocPk, azTokenId);

            if (tokenId.isEmpty())
            {
                m_bondHeader->setText(QString("Top starters (%1 shown)").arg(bonds.size()));
            }
            else
            {
                QString surface = ResolveSurface(azTokenId);
                QString header = surface.isEmpty() ? tokenId
                    : QString("%1 (%2)").arg(tokenId, surface);
                m_bondHeader->setText(QString("Bonds for: %1").arg(header));
            }

            for (const auto& be : bonds)
            {
                auto* item = new QTreeWidgetItem(m_bondTree);
                item->setText(0, QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));

                QString surface = ResolveSurface(be.tokenB);
                if (!surface.isEmpty())
                    item->setText(1, surface);

                item->setText(2, QString::number(be.count));
                item->setTextAlignment(2, Qt::AlignRight);
                item->setData(0, Qt::UserRole,
                    QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));
            }

            m_bondTree->sortByColumn(2, Qt::DescendingOrder);
            return;
        }

        // Socket fallback
        if (m_client && m_client->IsConnected())
        {
            m_client->GetBonds(docId, tokenId, [this, tokenId](const QJsonObject& resp) {
                if (resp["status"].toString() != "ok") return;

                m_bondTree->clear();
                QJsonArray bonds = resp["bonds"].toArray();

                if (tokenId.isEmpty())
                {
                    m_bondHeader->setText(QString("Top starters (%1 shown)").arg(bonds.size()));
                }
                else
                {
                    QString surface = resp["surface"].toString();
                    QString header = surface.isEmpty() ? tokenId
                        : QString("%1 (%2)").arg(tokenId, surface);
                    m_bondHeader->setText(QString("Bonds for: %1").arg(header));
                }

                for (int i = 0; i < bonds.size(); ++i)
                {
                    QJsonObject be = bonds[i].toObject();
                    auto* item = new QTreeWidgetItem(m_bondTree);
                    item->setText(0, be["token"].toString());
                    item->setText(1, be["surface"].toString());
                    item->setText(2, QString::number(be["count"].toInt()));
                    item->setTextAlignment(2, Qt::AlignRight);
                    item->setData(0, Qt::UserRole, be["token"].toString());
                }

                m_bondTree->sortByColumn(2, Qt::DescendingOrder);
            });
        }
    }

    void HCPWorkstationWindow::ShowVars(
        const QString& docId, [[maybe_unused]] const QString& filterEntityId)
    {
        m_varTree->clear();

        // Direct DB path
        if (m_engine && m_engine->IsDbConnected() && m_selectedDocPk > 0)
        {
            auto vars = m_engine->GetDocVarQuery().GetDocVarsExtended(m_selectedDocPk);
            for (const auto& v : vars)
            {
                auto* item = new QTreeWidgetItem(m_varTree);
                item->setText(0, QString::fromUtf8(v.surface.c_str(), static_cast<int>(v.surface.size())));
                item->setText(1, QString::fromUtf8(v.varId.c_str(), static_cast<int>(v.varId.size())));
                item->setText(2, QString::fromUtf8(v.category.c_str(), static_cast<int>(v.category.size())));
                item->setText(3, v.groupId ? QString::number(v.groupId) : "-");
                item->setText(4, v.suggestedId.empty() ? "-" :
                    QString::fromUtf8(v.suggestedId.c_str(), static_cast<int>(v.suggestedId.size())));

                // Category-based styling (matches editor widget)
                if (v.category == "proper")
                {
                    QFont f = item->font(0);
                    f.setBold(true);
                    item->setFont(0, f);
                }
                else if (v.category == "sic")
                {
                    QFont f = item->font(0);
                    f.setItalic(true);
                    item->setFont(0, f);
                }
                else if (v.category == "uri_metadata")
                {
                    item->setForeground(0, QBrush(QColor(128, 128, 128)));
                }

                item->setData(0, Qt::UserRole,
                    QString::fromUtf8(v.suggestedId.c_str(), static_cast<int>(v.suggestedId.size())));
            }
            return;
        }

        // Socket fallback
        if (m_client && m_client->IsConnected())
        {
            m_client->GetDocumentInfo(docId, [this](const QJsonObject& resp) {
                if (resp["status"].toString() != "ok") return;

                m_varTree->clear();
                QJsonArray vars = resp["vars"].toArray();
                for (int i = 0; i < vars.size(); ++i)
                {
                    QJsonObject v = vars[i].toObject();
                    auto* item = new QTreeWidgetItem(m_varTree);
                    item->setText(0, v["surface"].toString());
                    item->setText(1, v["var_id"].toString());
                    item->setText(2, v.value("category").toString("-"));
                    item->setText(3, v.contains("group_id") ? QString::number(v["group_id"].toInt()) : "-");
                    item->setText(4, v.value("suggested_entity").toString("-"));
                    item->setData(0, Qt::UserRole, v.value("suggested_entity").toString());
                }
            });
        }
    }

    void HCPWorkstationWindow::ShowText(const QString& docId)
    {
        m_textView->setPlainText("Loading...");

        AZStd::string azDocId(docId.toUtf8().constData(), docId.toUtf8().size());

        // Direct DB + vocab path — reconstruct text from positional tokens
        if (m_engine && m_engine->IsDbConnected() && m_engine->IsVocabLoaded())
        {
            auto tokenIds = m_engine->GetPbmReader().LoadPositions(azDocId);
            if (tokenIds.empty())
            {
                m_textView->setPlainText("(no text stored)");
                return;
            }

            AZStd::string text = TokenIdsToText(tokenIds, m_engine->GetVocabulary());
            if (text.empty())
                m_textView->setPlainText("(reconstruction failed)");
            else
                m_textView->setPlainText(
                    QString::fromUtf8(text.c_str(), static_cast<int>(text.size())));
            return;
        }

        // Socket fallback
        if (m_client && m_client->IsConnected())
        {
            m_client->RetrieveText(docId, [this](const QJsonObject& resp) {
                if (resp["status"].toString() != "ok")
                {
                    m_textView->setPlainText(
                        QString("Error: %1").arg(resp["message"].toString("Unknown error")));
                    return;
                }

                QString text = resp["text"].toString();
                if (text.isEmpty())
                    m_textView->setPlainText("(no text stored)");
                else
                    m_textView->setPlainText(text);
            });
        }
    }

    // ---- Slot handlers ----

    void HCPWorkstationWindow::OnBondTokenClicked(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item || m_selectedDocId.isEmpty()) return;
        QString tokenId = item->data(0, Qt::UserRole).toString();
        ShowBonds(m_selectedDocId, tokenId);
    }

    void HCPWorkstationWindow::OnRetrieveText()
    {
        if (m_selectedDocId.isEmpty()) return;
        ShowText(m_selectedDocId);
    }

    void HCPWorkstationWindow::OnSaveMetadata()
    {
        if (m_selectedDocId.isEmpty()) return;

        QString key = m_metaKeyInput->text().trimmed();
        QString value = m_metaValueInput->text().trimmed();
        if (key.isEmpty()) return;

        // Direct DB path
        if (m_engine && m_engine->IsDbConnected() && m_selectedDocPk > 0)
        {
            AZStd::string azKey(key.toUtf8().constData(), key.toUtf8().size());
            AZStd::string azVal(value.toUtf8().constData(), value.toUtf8().size());
            bool ok = m_engine->GetDocumentQuery().StoreMetadata(m_selectedDocPk, azKey, azVal);
            if (ok)
            {
                m_metaKeyInput->clear();
                m_metaValueInput->clear();
                ShowDocumentInfo(m_selectedDocId);
            }
            else
            {
                statusBar()->showMessage("Metadata save failed", 5000);
            }
            return;
        }

        // Socket fallback
        if (m_client && m_client->IsConnected())
        {
            QJsonObject setFields;
            setFields[key] = value;

            m_client->UpdateMeta(m_selectedDocId, setFields, {}, [this](const QJsonObject& resp) {
                if (resp["status"].toString() != "ok")
                {
                    statusBar()->showMessage(
                        QString("Metadata error: %1").arg(resp["message"].toString()), 5000);
                    return;
                }
                m_metaKeyInput->clear();
                m_metaValueInput->clear();
                ShowDocumentInfo(m_selectedDocId);
            });
        }
    }

    void HCPWorkstationWindow::OnSearchBonds()
    {
        if (m_selectedDocId.isEmpty()) return;
        QString searchText = m_bondSearch->text().trimmed();
        if (searchText.isEmpty()) return;

        // Direct DB path — local bond search with surface resolution
        if (m_engine && m_engine->IsDbConnected() && m_engine->IsVocabLoaded() && m_selectedDocPk > 0)
        {
            auto allStarters = m_engine->GetBondQuery().GetAllStarters(m_selectedDocPk);

            m_bondTree->clear();
            int matchCount = 0;

            for (const auto& be : allStarters)
            {
                QString surface = ResolveSurface(be.tokenB);
                if (surface.isEmpty())
                    surface = QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size()));

                if (surface.contains(searchText, Qt::CaseInsensitive))
                {
                    auto* item = new QTreeWidgetItem(m_bondTree);
                    item->setText(0, QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));
                    item->setText(1, surface);
                    item->setText(2, QString::number(be.count));
                    item->setTextAlignment(2, Qt::AlignRight);
                    item->setData(0, Qt::UserRole,
                        QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));
                    ++matchCount;
                }
            }

            m_bondHeader->setText(QString("Search: \"%1\" (%2 matches from %3 starters)")
                .arg(searchText).arg(matchCount).arg(allStarters.size()));
            m_bondTree->sortByColumn(2, Qt::DescendingOrder);
            return;
        }

        // No local search capability without DB + vocab
        statusBar()->showMessage(
            "Bond search requires local DB connection or engine daemon", 5000);
    }

    void HCPWorkstationWindow::OnClearBondSearch()
    {
        m_bondSearch->clear();
        if (!m_selectedDocId.isEmpty())
            ShowBonds(m_selectedDocId);
    }

    void HCPWorkstationWindow::OnImportMetadata()
    {
        if (m_selectedDocId.isEmpty()) return;

        // Get current document name for catalog matching
        QString docName = m_infoName->text();
        QString catalogId;

        // Try provenance for catalog ID (direct DB)
        if (m_engine && m_engine->IsDbConnected() && m_selectedDocPk > 0)
        {
            auto prov = m_engine->GetDocumentQuery().GetProvenance(m_selectedDocPk);
            if (prov.found)
                catalogId = QString::fromUtf8(prov.catalogId.c_str(), static_cast<int>(prov.catalogId.size()));
        }

        // Search local Gutenberg catalog files
        static const char* gutenbergFiles[] = {
            "/opt/project/repo/data/gutenberg/metadata.json",
            "/opt/project/repo/data/gutenberg/metadata_batch2.json"
        };

        QJsonObject matchedEntry;
        bool found = false;

        for (const char* path : gutenbergFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) continue;
            QJsonDocument jdoc = QJsonDocument::fromJson(file.readAll());
            file.close();
            if (!jdoc.isArray()) continue;

            QJsonArray arr = jdoc.array();
            for (int idx = 0; idx < arr.size(); ++idx)
            {
                QJsonObject obj = arr[idx].toObject();

                if (!catalogId.isEmpty())
                {
                    if (QString::number(obj["id"].toInt()) == catalogId)
                    {
                        matchedEntry = obj;
                        found = true;
                        break;
                    }
                }
                else if (obj["title"].toString().compare(docName, Qt::CaseInsensitive) == 0)
                {
                    matchedEntry = obj;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        if (!found)
        {
            statusBar()->showMessage(
                QString("No catalog match found for \"%1\"").arg(docName), 5000);
            return;
        }

        // Build metadata fields
        QJsonObject setFields;
        if (matchedEntry.contains("title"))
            setFields["title"] = matchedEntry["title"];
        if (matchedEntry.contains("authors"))
            setFields["authors"] = matchedEntry["authors"];
        if (matchedEntry.contains("subjects"))
            setFields["subjects"] = matchedEntry["subjects"];
        if (matchedEntry.contains("bookshelves"))
            setFields["bookshelves"] = matchedEntry["bookshelves"];
        if (matchedEntry.contains("languages"))
            setFields["languages"] = matchedEntry["languages"];
        if (matchedEntry.contains("copyright"))
            setFields["copyright"] = matchedEntry["copyright"];
        if (matchedEntry.contains("id"))
            setFields["gutenberg_id"] = matchedEntry["id"];

        // Write via direct DB
        if (m_engine && m_engine->IsDbConnected() && m_selectedDocPk > 0)
        {
            QString metaJson = QString::fromUtf8(
                QJsonDocument(setFields).toJson(QJsonDocument::Compact));
            AZStd::string azJson(metaJson.toUtf8().constData(), metaJson.toUtf8().size());
            bool ok = m_engine->GetDocumentQuery().StoreDocumentMetadata(m_selectedDocPk, azJson);
            if (ok)
                ShowDocumentInfo(m_selectedDocId);
            else
                statusBar()->showMessage("Import failed: DB write error", 5000);
            return;
        }

        // Socket fallback
        if (m_client && m_client->IsConnected())
        {
            m_client->UpdateMeta(m_selectedDocId, setFields, {}, [this](const QJsonObject& resp) {
                if (resp["status"].toString() == "ok")
                    ShowDocumentInfo(m_selectedDocId);
                else
                    statusBar()->showMessage(
                        QString("Import error: %1").arg(resp["message"].toString()), 5000);
            });
        }
    }

    void HCPWorkstationWindow::OnVarClicked(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item || m_selectedDocId.isEmpty()) return;
        QString suggestedId = item->data(0, Qt::UserRole).toString();
        if (suggestedId.isEmpty()) return;

        UpdateBreadcrumb(QString("Var: %1 > Entity").arg(item->text(0)));
        ShowEntities(m_selectedDocId, suggestedId);
        m_tabs->setCurrentIndex(m_tabEntities);
    }

    void HCPWorkstationWindow::OnEntityClicked(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item || m_selectedDocId.isEmpty()) return;
        if (item->childCount() > 0) return;

        QString entityId = item->text(1);
        if (entityId.isEmpty()) return;

        UpdateBreadcrumb(QString("Entity: %1 > Vars").arg(item->text(0)));
        ShowVars(m_selectedDocId, entityId);
        m_tabs->setCurrentIndex(m_tabVars);
    }

    void HCPWorkstationWindow::OnBreadcrumbReset()
    {
        m_activeFilter.clear();
        m_breadcrumb->clear();
        m_breadcrumbReset->setVisible(false);
        if (!m_selectedDocId.isEmpty())
        {
            ShowEntities(m_selectedDocId);
            ShowVars(m_selectedDocId);
        }
    }

    void HCPWorkstationWindow::UpdateBreadcrumb(const QString& segment)
    {
        m_breadcrumb->setText(segment);
        m_breadcrumbReset->setVisible(true);
    }

    // ---- File ingestion (requires daemon for physics pipeline) ----

    void HCPWorkstationWindow::OnOpenFile()
    {
        QString filePath = QFileDialog::getOpenFileName(this,
            "Open Source File", QString(),
            "All Supported (*.json *.txt *.md);;JSON (*.json);;Text (*.txt *.md)");
        if (filePath.isEmpty()) return;
        IngestFile(filePath);
    }

    void HCPWorkstationWindow::OnOpenFolder()
    {
        QString folderPath = QFileDialog::getExistingDirectory(this,
            "Open Source Folder");
        if (folderPath.isEmpty()) return;
        IngestFolder(folderPath);
    }

    void HCPWorkstationWindow::dragEnterEvent(QDragEnterEvent* event)
    {
        if (!m_viewerMode && event->mimeData()->hasUrls())
            event->acceptProposedAction();
    }

    void HCPWorkstationWindow::dropEvent(QDropEvent* event)
    {
        if (m_viewerMode) return;

        for (const QUrl& url : event->mimeData()->urls())
        {
            QString path = url.toLocalFile();
            QFileInfo fi(path);
            if (fi.isDir())
                IngestFolder(path);
            else
                IngestFile(path);
        }
    }

    void HCPWorkstationWindow::IngestFile(const QString& filePath)
    {
        QFileInfo fi(filePath);
        QString ext = fi.suffix().toLower();

        if (ext == "json")
            IngestJsonSource(filePath);
        else
            IngestRawText(filePath);
    }

    void HCPWorkstationWindow::IngestFolder(const QString& folderPath)
    {
        QDir dir(folderPath);
        QStringList filters = {"*.json", "*.txt", "*.md"};
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

        QMap<QString, QString> jsonSources;
        QStringList orphanTexts;

        for (const QFileInfo& fi : files)
        {
            if (fi.suffix().toLower() == "json")
                jsonSources[fi.baseName()] = fi.absoluteFilePath();
        }

        for (const QFileInfo& fi : files)
        {
            if (fi.suffix().toLower() == "json") continue;

            if (jsonSources.contains(fi.baseName()))
            {
                IngestJsonSource(jsonSources[fi.baseName()]);
                jsonSources.remove(fi.baseName());
            }
            else
            {
                orphanTexts.append(fi.absoluteFilePath());
            }
        }

        for (const QString& jsonPath : jsonSources.values())
            IngestJsonSource(jsonPath);

        for (const QString& textPath : orphanTexts)
            IngestRawText(textPath);
    }

    void HCPWorkstationWindow::IngestJsonSource(const QString& jsonPath)
    {
        QFile jsonFile(jsonPath);
        if (!jsonFile.open(QIODevice::ReadOnly)) return;
        QByteArray jsonData = jsonFile.readAll();
        jsonFile.close();

        QJsonDocument jdoc = QJsonDocument::fromJson(jsonData);
        if (!jdoc.isObject()) return;

        QJsonObject obj = jdoc.object();

        QString sourcePath;
        if (obj.contains("source_file"))
        {
            sourcePath = obj["source_file"].toString();
            QFileInfo jsonFi(jsonPath);
            QFileInfo sourceFi(jsonFi.dir(), sourcePath);
            if (sourceFi.exists())
                sourcePath = sourceFi.absoluteFilePath();
        }

        if (sourcePath.isEmpty())
        {
            QFileInfo fi(jsonPath);
            for (const QString& ext : {".txt", ".md"})
            {
                QString candidate = fi.dir().absoluteFilePath(fi.baseName() + ext);
                if (QFileInfo::exists(candidate))
                {
                    sourcePath = candidate;
                    break;
                }
            }
        }

        if (sourcePath.isEmpty() || !QFileInfo::exists(sourcePath))
        {
            statusBar()->showMessage(
                QString("No source file found for %1").arg(jsonPath), 5000);
            return;
        }

        QFile sourceFile(sourcePath);
        if (!sourceFile.open(QIODevice::ReadOnly)) return;
        QByteArray rawBytes = sourceFile.readAll();
        sourceFile.close();

        QString docName = obj.value("name").toString();
        if (docName.isEmpty())
            docName = QFileInfo(sourcePath).baseName();

        QJsonObject meta = obj;
        meta.remove("source_file");
        QString metaJson = QString::fromUtf8(
            QJsonDocument(meta).toJson(QJsonDocument::Compact));

        ProcessThroughPipeline(docName, rawBytes, metaJson);
    }

    void HCPWorkstationWindow::IngestRawText(const QString& textPath)
    {
        QFile file(textPath);
        if (!file.open(QIODevice::ReadOnly)) return;
        QByteArray rawBytes = file.readAll();
        file.close();

        QString docName = QFileInfo(textPath).baseName();
        ProcessThroughPipeline(docName, rawBytes);
    }

    void HCPWorkstationWindow::ProcessThroughPipeline(
        const QString& docName, const QByteArray& rawBytes, const QString& metadataJson)
    {
        // Ingestion requires daemon (physics pipeline)
        if (!m_client || !m_client->IsConnected())
        {
            statusBar()->showMessage(
                "Ingestion requires engine daemon connection (physics pipeline)", 5000);
            return;
        }

        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0);

        QString text = QString::fromUtf8(rawBytes);

        m_client->Ingest(text, docName, metadataJson,
            [this, docName](const QJsonObject& resp) {
                m_progressBar->setVisible(false);

                if (resp["status"].toString() != "ok")
                {
                    statusBar()->showMessage(
                        QString("Ingest error: %1").arg(resp["message"].toString()), 5000);
                    return;
                }

                PopulateDocumentList();
                statusBar()->showMessage(
                    QString("Ingested: %1 -> %2").arg(docName, resp["doc_id"].toString()),
                    5000);
            });
    }

} // namespace HCPEngine
