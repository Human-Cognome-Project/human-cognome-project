#include "HCPWorkstationWindow.h"
#include "../HCPEngineSystemComponent.h"
#include <HCPEngine/HCPEngineBus.h>
#include "../HCPStorage.h"
#include "../HCPVocabulary.h"
#include "../HCPTokenizer.h"
#include "../HCPCacheMissResolver.h"

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

namespace
{
    // Resolve a token ID to a human-readable surface form for UI display.
    // Shared utility — same logic as the editor panel version.
    QString ResolveSurface(const AZStd::string& tokenId, const HCPEngine::HCPVocabulary& vocab)
    {
        AZStd::string word = vocab.TokenToWord(tokenId);
        if (!word.empty())
            return QString::fromUtf8(word.c_str(), static_cast<int>(word.size()));

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

        if (tokenId.starts_with("AA.AE."))
        {
            static const AZStd::unordered_map<AZStd::string, const char*> markers = {
                {"AA.AE.AA.AA", "document_start"}, {"AA.AE.AA.AB", "document_end"},
                {"AA.AE.AA.AC", "part_break"}, {"AA.AE.AA.AD", "chapter_break"},
                {"AA.AE.AA.AE", "section_break"}, {"AA.AE.AA.AF", "subsection_break"},
                {"AA.AE.AA.AG", "subsubsection_break"}, {"AA.AE.AA.AH", "minor_break"},
                {"AA.AE.AA.AI", "paragraph_start"}, {"AA.AE.AA.AJ", "paragraph_end"},
                {"AA.AE.AA.AK", "line_break"}, {"AA.AE.AA.AL", "page_break"},
                {"AA.AE.AA.AM", "horizontal_rule"},
                {"AA.AE.AA.AN", "block_quote_start"}, {"AA.AE.AA.AP", "block_quote_end"},
            };
            auto it = markers.find(tokenId);
            if (it != markers.end())
                return QString("[%1]").arg(it->second);
            return QString("[marker:%1]").arg(
                QString::fromUtf8(tokenId.c_str(), static_cast<int>(tokenId.size())));
        }

        return {};
    }
} // anonymous namespace

namespace HCPEngine
{
    HCPWorkstationWindow::HCPWorkstationWindow(
        HCPEngineSystemComponent* engine, QWidget* parent)
        : QMainWindow(parent)
        , m_engine(engine)
    {
        setWindowTitle("HCP Source Workstation");
        setMinimumSize(1200, 800);
        setAcceptDrops(true);

        BuildMenuBar();
        BuildStatusBar();
        BuildCentralWidget();
        PopulateDocumentList();
        UpdateStatusBar();
    }

    HCPWorkstationWindow::~HCPWorkstationWindow() = default;

    // ---- Menu bar ----

    void HCPWorkstationWindow::BuildMenuBar()
    {
        auto* fileMenu = menuBar()->addMenu("&File");

        auto* openFileAction = fileMenu->addAction("&Open File...");
        openFileAction->setShortcut(QKeySequence::Open);
        connect(openFileAction, &QAction::triggered, this, &HCPWorkstationWindow::OnOpenFile);

        auto* openFolderAction = fileMenu->addAction("Open &Folder...");
        connect(openFolderAction, &QAction::triggered, this, &HCPWorkstationWindow::OnOpenFolder);

        fileMenu->addSeparator();

        auto* refreshAction = fileMenu->addAction("&Refresh Document List");
        refreshAction->setShortcut(QKeySequence::Refresh);
        connect(refreshAction, &QAction::triggered, this, &HCPWorkstationWindow::OnRefreshDocuments);

        fileMenu->addSeparator();

        auto* quitAction = fileMenu->addAction("&Quit");
        quitAction->setShortcut(QKeySequence::Quit);
        connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

        auto* viewMenu = menuBar()->addMenu("&View");
        // Tab visibility toggles will go here once data surfing is implemented
        Q_UNUSED(viewMenu);
    }

    // ---- Status bar ----

    void HCPWorkstationWindow::BuildStatusBar()
    {
        m_statusEngine = new QLabel("Engine: --");
        m_statusDb = new QLabel("DB: --");
        m_statusGpu = new QLabel("GPU: --");
        m_progressBar = new QProgressBar();
        m_progressBar->setMaximumWidth(200);
        m_progressBar->setVisible(false);

        statusBar()->addWidget(m_statusEngine);
        statusBar()->addWidget(m_statusDb);
        statusBar()->addWidget(m_statusGpu);
        statusBar()->addPermanentWidget(m_progressBar);
    }

    void HCPWorkstationWindow::UpdateStatusBar()
    {
        if (m_engine && m_engine->IsEngineReady())
        {
            m_statusEngine->setText("Engine: Ready");
            m_statusEngine->setStyleSheet("color: green;");
        }
        else
        {
            m_statusEngine->setText("Engine: Not Ready");
            m_statusEngine->setStyleSheet("color: red;");
        }

        if (m_engine)
        {
            auto& wk = m_engine->GetWriteKernel();
            if (wk.IsConnected())
            {
                m_statusDb->setText("DB: Connected");
                m_statusDb->setStyleSheet("color: green;");
            }
            else
            {
                m_statusDb->setText("DB: Disconnected");
                m_statusDb->setStyleSheet("color: orange;");
            }
        }

        // GPU mode — check if particle pipeline has CUDA
        if (m_engine && m_engine->GetParticlePipeline().IsInitialized())
        {
            m_statusGpu->setText("GPU: Active");
            m_statusGpu->setStyleSheet("color: green;");
        }
        else
        {
            m_statusGpu->setText("GPU: CPU Mode");
            m_statusGpu->setStyleSheet("color: gray;");
        }
    }

    // ---- Central widget ----

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

    // ---- Document list ----

    void HCPWorkstationWindow::PopulateDocumentList()
    {
        m_docList->clear();
        if (!m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        if (!wk.IsConnected()) wk.Connect();
        if (!wk.IsConnected()) return;

        auto docs = wk.ListDocuments();
        for (const auto& d : docs)
        {
            auto* item = new QTreeWidgetItem(m_docList);
            item->setText(0, QString::fromUtf8(d.name.c_str(), static_cast<int>(d.name.size())));
            item->setText(1, QString::number(d.starters));
            item->setText(2, QString::number(d.bonds));
            item->setData(0, Qt::UserRole,
                QString::fromUtf8(d.docId.c_str(), static_cast<int>(d.docId.size())));
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
        m_activeFilter.clear();
        m_breadcrumb->clear();
        m_breadcrumbReset->setVisible(false);
        ShowDocumentInfo(m_selectedDocId);
        ShowEntities(m_selectedDocId);
        ShowVars(m_selectedDocId);
        ShowBonds(m_selectedDocId);
    }

    // ---- Panel data display ----

    void HCPWorkstationWindow::ShowDocumentInfo(const QString& docId)
    {
        if (!m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string azDocId(docId.toUtf8().constData());

        auto detail = wk.GetDocumentDetail(azDocId);
        if (detail.pk == 0) return;

        m_selectedDocPk = detail.pk;

        m_infoDocId->setText(docId);
        m_infoName->setText(QString::fromUtf8(detail.name.c_str(), static_cast<int>(detail.name.size())));
        m_infoSlots->setText(QString::number(detail.totalSlots));
        m_infoUnique->setText(QString::number(detail.uniqueTokens));
        m_infoStarters->setText(QString::number(detail.starters));
        m_infoBonds->setText(QString::number(detail.bonds));

        // Metadata table
        m_metaTable->setRowCount(0);
        if (!detail.metadataJson.empty() && detail.metadataJson != "{}")
        {
            QByteArray jsonBytes(detail.metadataJson.c_str(),
                                 static_cast<int>(detail.metadataJson.size()));
            QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes);
            if (jdoc.isObject())
            {
                QJsonObject obj = jdoc.object();
                m_metaTable->setRowCount(obj.size());
                int row = 0;
                for (auto it = obj.begin(); it != obj.end(); ++it, ++row)
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
    }

    void HCPWorkstationWindow::ShowEntities(const QString& docId, const QString& filterEntityId)
    {
        m_entityTree->clear();
        if (!m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string azDocId(docId.toUtf8().constData());
        int docPk = wk.GetDocPk(azDocId);
        if (docPk == 0) return;

        // Fiction characters
        PGconn* ficConn = m_engine->GetResolver().GetConnection("hcp_fic_entities");
        PGconn* pbmConn = wk.GetConnection();
        if (ficConn && pbmConn)
        {
            auto ficEntities = GetFictionEntitiesForDocument(ficConn, pbmConn, docPk);
            if (!ficEntities.empty())
            {
                auto* ficGroup = new QTreeWidgetItem(m_entityTree);
                ficGroup->setText(0, QString("Fiction Characters (%1)").arg(ficEntities.size()));
                ficGroup->setExpanded(true);
                QFont boldFont = ficGroup->font(0);
                boldFont.setBold(true);
                ficGroup->setFont(0, boldFont);

                for (const auto& ent : ficEntities)
                {
                    QString entId = QString::fromUtf8(ent.entityId.c_str(),
                        static_cast<int>(ent.entityId.size()));
                    if (!filterEntityId.isEmpty() && entId != filterEntityId)
                        continue;

                    auto* item = new QTreeWidgetItem(ficGroup);
                    item->setText(0, QString::fromUtf8(ent.name.c_str(),
                        static_cast<int>(ent.name.size())));
                    item->setText(1, entId);
                    item->setText(2, QString::fromUtf8(ent.category.c_str(),
                        static_cast<int>(ent.category.size())));

                    QString propStr;
                    for (const auto& [k, v] : ent.properties)
                    {
                        if (!propStr.isEmpty()) propStr += ", ";
                        propStr += QString::fromUtf8(k.c_str(), static_cast<int>(k.size()))
                            + "=" + QString::fromUtf8(v.c_str(), static_cast<int>(v.size()));
                    }
                    item->setText(3, propStr);
                }
            }
        }

        // Non-fiction author
        PGconn* nfConn = m_engine->GetResolver().GetConnection("hcp_nf_entities");
        if (nfConn)
        {
            auto detail = wk.GetDocumentDetail(azDocId);
            AZStd::string authorSearch;

            if (!detail.metadataJson.empty() && detail.metadataJson != "{}")
            {
                QByteArray jsonBytes(detail.metadataJson.c_str(),
                                     static_cast<int>(detail.metadataJson.size()));
                QJsonDocument jdoc = QJsonDocument::fromJson(jsonBytes);
                if (jdoc.isObject())
                {
                    QJsonObject obj = jdoc.object();
                    if (obj.contains("authors") && obj["authors"].isArray())
                    {
                        QJsonArray authors = obj["authors"].toArray();
                        if (!authors.isEmpty())
                        {
                            QString name = authors[0].toObject()["name"].toString();
                            if (name.contains(','))
                                name = name.split(',').first().trimmed();
                            authorSearch = AZStd::string(name.toUtf8().constData());
                        }
                    }
                    else if (obj.contains("author") && obj["author"].isString())
                    {
                        QString name = obj["author"].toString();
                        if (name.contains(','))
                            name = name.split(',').first().trimmed();
                        authorSearch = AZStd::string(name.toUtf8().constData());
                    }
                }
            }

            if (!authorSearch.empty())
            {
                auto author = GetNfAuthorEntity(nfConn, authorSearch);
                if (!author.entityId.empty())
                {
                    auto* nfGroup = new QTreeWidgetItem(m_entityTree);
                    nfGroup->setText(0, "Author / People");
                    nfGroup->setExpanded(true);
                    QFont boldFont = nfGroup->font(0);
                    boldFont.setBold(true);
                    nfGroup->setFont(0, boldFont);

                    auto* item = new QTreeWidgetItem(nfGroup);
                    QString displayName = QString::fromUtf8(
                        author.name.c_str(), static_cast<int>(author.name.size()));
                    displayName.replace('_', ' ');
                    item->setText(0, displayName);
                    item->setText(1, QString::fromUtf8(
                        author.entityId.c_str(), static_cast<int>(author.entityId.size())));
                    item->setText(2, QString::fromUtf8(
                        author.category.c_str(), static_cast<int>(author.category.size())));

                    QString propStr;
                    for (const auto& [k, v] : author.properties)
                    {
                        if (!propStr.isEmpty()) propStr += ", ";
                        propStr += QString::fromUtf8(k.c_str(), static_cast<int>(k.size()))
                            + "=" + QString::fromUtf8(v.c_str(), static_cast<int>(v.size()));
                    }
                    item->setText(3, propStr);
                }
            }
        }
    }

    void HCPWorkstationWindow::ShowBonds(const QString& docId, const QString& tokenId)
    {
        if (!m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string azDocId(docId.toUtf8().constData());
        AZStd::string azTokenId(tokenId.toUtf8().constData());

        int docPk = wk.GetDocPk(azDocId);
        if (docPk == 0) return;

        auto bonds = wk.GetBondsForToken(docPk, azTokenId);
        m_bondTree->clear();

        if (tokenId.isEmpty())
        {
            m_bondHeader->setText(QString("Top starters (%1 shown)").arg(bonds.size()));
        }
        else
        {
            QString surfaceStr = ResolveSurface(azTokenId, m_engine->GetVocabulary());
            QString surfaceQ = surfaceStr.isEmpty() ? tokenId
                : QString("%1 (%2)").arg(tokenId, surfaceStr);
            m_bondHeader->setText(QString("Bonds for: %1").arg(surfaceQ));
        }

        for (const auto& be : bonds)
        {
            auto* item = new QTreeWidgetItem(m_bondTree);
            item->setText(0, QString::fromUtf8(be.tokenB.c_str(),
                static_cast<int>(be.tokenB.size())));

            QString surface = ResolveSurface(be.tokenB, m_engine->GetVocabulary());
            if (!surface.isEmpty())
                item->setText(1, surface);

            item->setText(2, QString::number(be.count));
            item->setTextAlignment(2, Qt::AlignRight);
            item->setData(0, Qt::UserRole,
                QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));
        }

        m_bondTree->sortByColumn(2, Qt::DescendingOrder);
    }

    void HCPWorkstationWindow::ShowVars(const QString& docId, const QString& filterEntityId)
    {
        m_varTree->clear();
        if (!m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string azDocId(docId.toUtf8().constData());
        int docPk = wk.GetDocPk(azDocId);
        if (docPk == 0) return;

        auto vars = wk.GetDocVarsExtended(docPk);

        for (const auto& v : vars)
        {
            if (!filterEntityId.isEmpty())
            {
                QString sugId = QString::fromUtf8(v.suggestedId.c_str(),
                    static_cast<int>(v.suggestedId.size()));
                if (sugId != filterEntityId)
                    continue;
            }

            auto* item = new QTreeWidgetItem(m_varTree);
            item->setText(0, QString::fromUtf8(v.surface.c_str(),
                static_cast<int>(v.surface.size())));
            item->setText(1, QString::fromUtf8(v.varId.c_str(),
                static_cast<int>(v.varId.size())));
            item->setText(2, QString::fromUtf8(v.category.c_str(),
                static_cast<int>(v.category.size())));
            item->setText(3, v.groupId ? QString::number(v.groupId) : QString("-"));
            item->setText(4, v.suggestedId.empty() ? QString("-")
                : QString::fromUtf8(v.suggestedId.c_str(),
                    static_cast<int>(v.suggestedId.size())));

            item->setData(0, Qt::UserRole,
                QString::fromUtf8(v.suggestedId.c_str(),
                    static_cast<int>(v.suggestedId.size())));

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
        }
    }

    void HCPWorkstationWindow::ShowText(const QString& docId)
    {
        if (!m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string azDocId(docId.toUtf8().constData());

        auto tokenIds = wk.LoadPositions(azDocId);
        if (tokenIds.empty())
        {
            m_textView->setPlainText("(no positions stored)");
            return;
        }

        AZStd::string text = TokenIdsToText(tokenIds, m_engine->GetVocabulary());
        m_textView->setPlainText(
            QString::fromUtf8(text.c_str(), static_cast<int>(text.size())));
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
        if (m_selectedDocPk == 0 || !m_engine) return;

        QString key = m_metaKeyInput->text().trimmed();
        QString value = m_metaValueInput->text().trimmed();
        if (key.isEmpty()) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string setJson = "{\"" +
            AZStd::string(key.toUtf8().constData()) + "\":\"" +
            AZStd::string(value.toUtf8().constData()) + "\"}";

        AZStd::vector<AZStd::string> removeKeys;
        wk.UpdateMetadata(m_selectedDocPk, setJson, removeKeys);

        m_metaKeyInput->clear();
        m_metaValueInput->clear();
        ShowDocumentInfo(m_selectedDocId);
    }

    void HCPWorkstationWindow::OnSearchBonds()
    {
        if (m_selectedDocId.isEmpty()) return;
        QString searchText = m_bondSearch->text().trimmed();
        if (searchText.isEmpty()) return;
        if (!m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string azDocId(m_selectedDocId.toUtf8().constData());
        int docPk = wk.GetDocPk(azDocId);
        if (docPk == 0) return;

        auto allStarters = wk.GetAllStarters(docPk);
        m_bondTree->clear();
        int matchCount = 0;

        for (const auto& be : allStarters)
        {
            AZStd::string azToken(be.tokenB);
            QString surface = ResolveSurface(azToken, m_engine->GetVocabulary());
            if (surface.isEmpty())
                surface = QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size()));

            if (surface.contains(searchText, Qt::CaseInsensitive))
            {
                auto* item = new QTreeWidgetItem(m_bondTree);
                item->setText(0, QString::fromUtf8(be.tokenB.c_str(),
                    static_cast<int>(be.tokenB.size())));
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
    }

    void HCPWorkstationWindow::OnClearBondSearch()
    {
        m_bondSearch->clear();
        if (!m_selectedDocId.isEmpty())
            ShowBonds(m_selectedDocId);
    }

    void HCPWorkstationWindow::OnImportMetadata()
    {
        if (m_selectedDocPk == 0 || m_selectedDocId.isEmpty() || !m_engine) return;

        auto& wk = m_engine->GetWriteKernel();
        AZStd::string azDocId(m_selectedDocId.toUtf8().constData());

        auto detail = wk.GetDocumentDetail(azDocId);
        QString docName = QString::fromUtf8(detail.name.c_str(),
            static_cast<int>(detail.name.size()));

        auto prov = wk.GetProvenance(m_selectedDocPk);
        QString catalogId;
        if (prov.found && !prov.catalogId.empty())
            catalogId = QString::fromUtf8(prov.catalogId.c_str(),
                static_cast<int>(prov.catalogId.size()));

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
            m_bondHeader->setText(QString("No catalog match found for \"%1\"").arg(docName));
            return;
        }

        QJsonObject meta;
        if (matchedEntry.contains("title"))
            meta["title"] = matchedEntry["title"];
        if (matchedEntry.contains("authors"))
            meta["authors"] = matchedEntry["authors"];
        if (matchedEntry.contains("subjects"))
            meta["subjects"] = matchedEntry["subjects"];
        if (matchedEntry.contains("bookshelves"))
            meta["bookshelves"] = matchedEntry["bookshelves"];
        if (matchedEntry.contains("languages"))
            meta["languages"] = matchedEntry["languages"];
        if (matchedEntry.contains("copyright"))
            meta["copyright"] = matchedEntry["copyright"];
        if (matchedEntry.contains("id"))
            meta["gutenberg_id"] = matchedEntry["id"];

        QJsonDocument metaDoc(meta);
        QByteArray metaBytes = metaDoc.toJson(QJsonDocument::Compact);
        AZStd::string metaJson(metaBytes.constData(), metaBytes.size());

        wk.StoreDocumentMetadata(m_selectedDocPk, metaJson);
        ShowDocumentInfo(m_selectedDocId);
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

    // ---- File ingestion ----

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
        if (event->mimeData()->hasUrls())
            event->acceptProposedAction();
    }

    void HCPWorkstationWindow::dropEvent(QDropEvent* event)
    {
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

        // First pass: pair JSONs with source files
        QMap<QString, QString> jsonSources; // baseName -> json path
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
                // Paired: JSON has metadata, text file is the source
                IngestJsonSource(jsonSources[fi.baseName()]);
                jsonSources.remove(fi.baseName());
            }
            else
            {
                orphanTexts.append(fi.absoluteFilePath());
            }
        }

        // Remaining JSONs without text pair
        for (const QString& jsonPath : jsonSources.values())
            IngestJsonSource(jsonPath);

        // Orphan text files
        for (const QString& textPath : orphanTexts)
            IngestRawText(textPath);

        PopulateDocumentList();
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

        // Try to find referenced source file
        QString sourcePath;
        if (obj.contains("source_file"))
        {
            sourcePath = obj["source_file"].toString();
            // Resolve relative to JSON location
            QFileInfo jsonFi(jsonPath);
            QFileInfo sourceFi(jsonFi.dir(), sourcePath);
            if (sourceFi.exists())
                sourcePath = sourceFi.absoluteFilePath();
        }

        // If no explicit source_file, look for paired .txt/.md
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

        // Extract metadata JSON (everything except source_file field)
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
        if (!m_engine || !m_engine->IsEngineReady()) return;

        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0); // indeterminate

        AZStd::string text(rawBytes.constData(), rawBytes.size());
        AZStd::string name(docName.toUtf8().constData());

        // Process through the HCP pipeline via EBus (ProcessText is protected)
        AZStd::string docId;
        HCPEngineRequestBus::BroadcastResult(docId, &HCPEngineRequests::ProcessText, text, name, AZStd::string("AS"));

        if (!docId.empty() && !metadataJson.isEmpty())
        {
            auto& wk = m_engine->GetWriteKernel();
            int docPk = wk.GetDocPk(docId);
            if (docPk > 0)
            {
                AZStd::string metaJson(metadataJson.toUtf8().constData());
                wk.StoreDocumentMetadata(docPk, metaJson);
            }
        }

        m_progressBar->setVisible(false);

        if (!docId.empty())
        {
            PopulateDocumentList();
            statusBar()->showMessage(
                QString("Ingested: %1 -> %2").arg(docName,
                    QString::fromUtf8(docId.c_str(), static_cast<int>(docId.size()))),
                5000);
        }
    }

} // namespace HCPEngine

#include <Workstation/moc_HCPWorkstationWindow.cpp>
