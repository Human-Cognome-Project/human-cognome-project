#include "HCPEngineWidget.h"
#include "HCPEngineEditorSystemComponent.h"
#include "../HCPEngineSystemComponent.h"
#include "../HCPStorage.h"
#include "../HCPVocabulary.h"
#include "../HCPTokenizer.h"
#include "../HCPCacheMissResolver.h"

#include <AzCore/Component/ComponentApplicationBus.h>

namespace
{
    // Resolve a token ID to a human-readable surface form for UI display.
    // Tries: word lookup, char lookup (with control char notation), label lookup.
    QString ResolveSurface(const AZStd::string& tokenId, const HCPEngine::HCPVocabulary& vocab)
    {
        // Word tokens (most common)
        AZStd::string word = vocab.TokenToWord(tokenId);
        if (!word.empty())
            return QString::fromUtf8(word.c_str(), static_cast<int>(word.size()));

        // Single-character tokens — show control chars in standard notation
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

        // Structural marker tokens (AA.AE.* namespace)
        // These live in hcp_core and may not be in the LMDB word cache.
        // Use known markers; fall back to showing the token ID.
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace HCPEngine
{
    // Find the engine system component via the component application bus
    HCPEngineSystemComponent* HCPEngineWidget::GetEngine()
    {
        return HCPEngineSystemComponent::Get();
    }

    HCPEngineWidget::HCPEngineWidget(QWidget* parent)
        : QWidget(parent)
    {
        BuildLayout();
        PopulateDocumentList();
    }

    void HCPEngineWidget::BuildLayout()
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(4, 4, 4, 4);

        // Header
        auto* header = new QLabel("HCP Asset Manager", this);
        QFont headerFont = header->font();
        headerFont.setPointSize(12);
        headerFont.setBold(true);
        header->setFont(headerFont);
        mainLayout->addWidget(header);

        // Splitter: doc list (left) | detail panel (right)
        auto* splitter = new QSplitter(Qt::Horizontal, this);

        // ---- Left: Document list ----
        auto* leftWidget = new QWidget(splitter);
        auto* leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        m_refreshBtn = new QPushButton("Refresh", leftWidget);
        connect(m_refreshBtn, &QPushButton::clicked, this, &HCPEngineWidget::OnRefreshDocuments);
        leftLayout->addWidget(m_refreshBtn);

        m_docList = new QTreeWidget(leftWidget);
        m_docList->setHeaderLabels({"Document", "Starters", "Bonds"});
        m_docList->setColumnWidth(0, 200);
        m_docList->setRootIsDecorated(false);
        m_docList->setAlternatingRowColors(true);
        connect(m_docList, &QTreeWidget::itemClicked, this, &HCPEngineWidget::OnDocumentSelected);
        leftLayout->addWidget(m_docList);

        splitter->addWidget(leftWidget);

        // ---- Right: Tabbed detail panel ----
        auto* rightWidget = new QWidget(splitter);
        auto* rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setContentsMargins(0, 0, 0, 0);

        // Breadcrumb navigation bar
        auto* breadcrumbRow = new QHBoxLayout();
        m_breadcrumb = new QLabel("", rightWidget);
        m_breadcrumb->setTextInteractionFlags(Qt::TextSelectableByMouse);
        QFont bcFont = m_breadcrumb->font();
        bcFont.setItalic(true);
        m_breadcrumb->setFont(bcFont);
        m_breadcrumbReset = new QPushButton("Reset", rightWidget);
        m_breadcrumbReset->setFixedWidth(50);
        m_breadcrumbReset->setVisible(false);
        connect(m_breadcrumbReset, &QPushButton::clicked, this, &HCPEngineWidget::OnBreadcrumbReset);
        breadcrumbRow->addWidget(m_breadcrumb, 1);
        breadcrumbRow->addWidget(m_breadcrumbReset);
        rightLayout->addLayout(breadcrumbRow);

        m_tabs = new QTabWidget(rightWidget);
        rightLayout->addWidget(m_tabs, 1);

        // -- Info tab --
        auto* infoWidget = new QWidget();
        auto* infoLayout = new QVBoxLayout(infoWidget);
        infoLayout->setAlignment(Qt::AlignTop);

        auto addInfoRow = [&](const QString& label) -> QLabel* {
            auto* row = new QHBoxLayout();
            auto* lbl = new QLabel(label + ":", infoWidget);
            lbl->setFixedWidth(100);
            QFont boldFont = lbl->font();
            boldFont.setBold(true);
            lbl->setFont(boldFont);
            auto* val = new QLabel("-", infoWidget);
            val->setTextInteractionFlags(Qt::TextSelectableByMouse);
            row->addWidget(lbl);
            row->addWidget(val, 1);
            infoLayout->addLayout(row);
            return val;
        };

        m_infoDocId = addInfoRow("Doc ID");
        m_infoName = addInfoRow("Name");
        m_infoSlots = addInfoRow("Total Slots");
        m_infoUnique = addInfoRow("Unique");
        m_infoStarters = addInfoRow("Starters");
        m_infoBonds = addInfoRow("Bonds");
        infoLayout->addStretch();

        m_tabs->addTab(infoWidget, "Info");

        // -- Metadata tab --
        auto* metaWidget = new QWidget();
        auto* metaLayout = new QVBoxLayout(metaWidget);

        m_metaTable = new QTableWidget(metaWidget);
        m_metaTable->setColumnCount(2);
        m_metaTable->setHorizontalHeaderLabels({"Key", "Value"});
        m_metaTable->horizontalHeader()->setStretchLastSection(true);
        m_metaTable->setAlternatingRowColors(true);
        metaLayout->addWidget(m_metaTable, 1);

        auto* metaEditRow = new QHBoxLayout();
        m_metaKeyInput = new QLineEdit(metaWidget);
        m_metaKeyInput->setPlaceholderText("Key");
        m_metaValueInput = new QLineEdit(metaWidget);
        m_metaValueInput->setPlaceholderText("Value");
        m_metaSaveBtn = new QPushButton("Set", metaWidget);
        connect(m_metaSaveBtn, &QPushButton::clicked, this, &HCPEngineWidget::OnSaveMetadata);
        metaEditRow->addWidget(m_metaKeyInput);
        metaEditRow->addWidget(m_metaValueInput);
        metaEditRow->addWidget(m_metaSaveBtn);
        metaLayout->addLayout(metaEditRow);

        m_metaImportBtn = new QPushButton("Import Catalog Metadata", metaWidget);
        connect(m_metaImportBtn, &QPushButton::clicked, this, &HCPEngineWidget::OnImportMetadata);
        metaLayout->addWidget(m_metaImportBtn);

        m_tabs->addTab(metaWidget, "Metadata");

        // -- Entities tab --
        auto* entityWidget = new QWidget();
        auto* entityLayout = new QVBoxLayout(entityWidget);

        m_entityTree = new QTreeWidget(entityWidget);
        m_entityTree->setHeaderLabels({"Name", "Entity ID", "Category", "Properties"});
        m_entityTree->setColumnWidth(0, 180);
        m_entityTree->setColumnWidth(1, 140);
        m_entityTree->setColumnWidth(2, 80);
        m_entityTree->setAlternatingRowColors(true);
        m_entityTree->setRootIsDecorated(true);
        connect(m_entityTree, &QTreeWidget::itemDoubleClicked, this, &HCPEngineWidget::OnEntityClicked);
        entityLayout->addWidget(m_entityTree, 1);

        m_tabs->addTab(entityWidget, "Entities");

        // -- Vars tab --
        auto* varsWidget = new QWidget();
        auto* varsLayout = new QVBoxLayout(varsWidget);

        m_varTree = new QTreeWidget(varsWidget);
        m_varTree->setHeaderLabels({"Surface", "Var ID", "Category", "Group", "Suggested Entity"});
        m_varTree->setColumnWidth(0, 200);
        m_varTree->setColumnWidth(1, 70);
        m_varTree->setColumnWidth(2, 90);
        m_varTree->setColumnWidth(3, 50);
        m_varTree->setAlternatingRowColors(true);
        m_varTree->setRootIsDecorated(false);
        m_varTree->setSortingEnabled(true);
        connect(m_varTree, &QTreeWidget::itemDoubleClicked, this, &HCPEngineWidget::OnVarClicked);
        varsLayout->addWidget(m_varTree, 1);

        m_tabs->addTab(varsWidget, "Vars");

        // -- Bonds tab --
        auto* bondsWidget = new QWidget();
        auto* bondsLayout = new QVBoxLayout(bondsWidget);

        // Search row
        auto* bondSearchRow = new QHBoxLayout();
        m_bondSearch = new QLineEdit(bondsWidget);
        m_bondSearch->setPlaceholderText("Search starters by surface form...");
        connect(m_bondSearch, &QLineEdit::returnPressed, this, &HCPEngineWidget::OnSearchBonds);
        m_bondSearchClear = new QPushButton("Clear", bondsWidget);
        connect(m_bondSearchClear, &QPushButton::clicked, this, &HCPEngineWidget::OnClearBondSearch);
        bondSearchRow->addWidget(m_bondSearch, 1);
        bondSearchRow->addWidget(m_bondSearchClear);
        bondsLayout->addLayout(bondSearchRow);

        m_bondHeader = new QLabel("Select a document to view bonds", bondsWidget);
        bondsLayout->addWidget(m_bondHeader);

        m_bondTree = new QTreeWidget(bondsWidget);
        m_bondTree->setHeaderLabels({"Token", "Surface", "Count"});
        m_bondTree->setColumnWidth(0, 160);
        m_bondTree->setColumnWidth(1, 140);
        m_bondTree->setRootIsDecorated(false);
        m_bondTree->setAlternatingRowColors(true);
        m_bondTree->setSortingEnabled(true);
        connect(m_bondTree, &QTreeWidget::itemDoubleClicked, this, &HCPEngineWidget::OnBondTokenClicked);
        bondsLayout->addWidget(m_bondTree, 1);

        m_tabs->addTab(bondsWidget, "Bonds");

        // -- Text tab --
        auto* textWidget = new QWidget();
        auto* textLayout = new QVBoxLayout(textWidget);

        m_retrieveBtn = new QPushButton("Load Text", textWidget);
        connect(m_retrieveBtn, &QPushButton::clicked, this, &HCPEngineWidget::OnRetrieveText);
        textLayout->addWidget(m_retrieveBtn);

        m_textView = new QTextEdit(textWidget);
        m_textView->setReadOnly(true);
        m_textView->setFont(QFont("Monospace", 9));
        textLayout->addWidget(m_textView, 1);

        m_tabs->addTab(textWidget, "Text");

        splitter->addWidget(rightWidget);
        splitter->setStretchFactor(0, 1);  // doc list
        splitter->setStretchFactor(1, 2);  // detail panel

        mainLayout->addWidget(splitter, 1);
        setLayout(mainLayout);
    }

    void HCPEngineWidget::PopulateDocumentList()
    {
        m_docList->clear();

        auto* engine = GetEngine();
        if (!engine)
        {
            FILE* df = fopen("/tmp/hcp_editor_diag.txt","a");
            if(df){fprintf(df,"Widget: GetEngine() returned null\n");fclose(df);}
            return;
        }

        auto& wk = engine->GetWriteKernel();
        if (!wk.IsConnected()) wk.Connect();
        if (!wk.IsConnected())
        {
            FILE* df = fopen("/tmp/hcp_editor_diag.txt","a");
            if(df){fprintf(df,"Widget: WriteKernel failed to connect\n");fclose(df);}
            return;
        }

        auto docs = wk.ListDocuments();
        {
            FILE* df = fopen("/tmp/hcp_editor_diag.txt","a");
            if(df){fprintf(df,"Widget: ListDocuments returned %zu docs\n", docs.size());fclose(df);}
        }
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

    void HCPEngineWidget::OnRefreshDocuments()
    {
        PopulateDocumentList();
    }

    void HCPEngineWidget::OnDocumentSelected(QTreeWidgetItem* item, [[maybe_unused]] int column)
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

    void HCPEngineWidget::ShowDocumentInfo(const QString& docId)
    {
        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
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

        // Populate metadata table from JSON
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

    void HCPEngineWidget::ShowEntities(const QString& docId, const QString& filterEntityId)
    {
        m_entityTree->clear();

        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
        AZStd::string azDocId(docId.toUtf8().constData());
        int docPk = wk.GetDocPk(azDocId);
        if (docPk == 0) return;

        // Fiction characters — cross-reference starters with fiction entity names
        PGconn* ficConn = engine->GetResolver().GetConnection("hcp_fic_entities");
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
                    // Filter: if we have a target entity ID, skip non-matches
                    QString entId = QString::fromUtf8(ent.entityId.c_str(), static_cast<int>(ent.entityId.size()));
                    if (!filterEntityId.isEmpty() && entId != filterEntityId)
                        continue;

                    auto* item = new QTreeWidgetItem(ficGroup);
                    item->setText(0, QString::fromUtf8(ent.name.c_str(), static_cast<int>(ent.name.size())));
                    item->setText(1, entId);
                    item->setText(2, QString::fromUtf8(ent.category.c_str(), static_cast<int>(ent.category.size())));

                    // Properties as comma-separated key=value
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

        // Non-fiction author — match via Gutenberg metadata or provenance
        PGconn* nfConn = engine->GetResolver().GetConnection("hcp_nf_entities");
        if (nfConn)
        {
            // Try to get author name from document metadata first
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
                    // Check for "authors" array (Gutenberg format)
                    if (obj.contains("authors") && obj["authors"].isArray())
                    {
                        QJsonArray authors = obj["authors"].toArray();
                        if (!authors.isEmpty())
                        {
                            QJsonObject firstAuthor = authors[0].toObject();
                            QString name = firstAuthor["name"].toString();
                            // Gutenberg format: "Surname, Firstname" — extract surname
                            if (name.contains(','))
                                name = name.split(',').first().trimmed();
                            authorSearch = AZStd::string(name.toUtf8().constData());
                        }
                    }
                    // Check for plain "author" string
                    else if (obj.contains("author") && obj["author"].isString())
                    {
                        QString name = obj["author"].toString();
                        if (name.contains(','))
                            name = name.split(',').first().trimmed();
                        authorSearch = AZStd::string(name.toUtf8().constData());
                    }
                }
            }

            // If no metadata yet, try to find author from Gutenberg JSON by title match
            if (authorSearch.empty())
            {
                QString docName = QString::fromUtf8(detail.name.c_str(), static_cast<int>(detail.name.size()));
                static const char* gutenbergFiles[] = {
                    "/opt/project/repo/data/gutenberg/metadata.json",
                    "/opt/project/repo/data/gutenberg/metadata_batch2.json"
                };
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
                        if (obj["title"].toString().compare(docName, Qt::CaseInsensitive) == 0)
                        {
                            QJsonArray authors = obj["authors"].toArray();
                            if (!authors.isEmpty())
                            {
                                QString name = authors[0].toObject()["name"].toString();
                                if (name.contains(','))
                                    name = name.split(',').first().trimmed();
                                authorSearch = AZStd::string(name.toUtf8().constData());
                            }
                            break;
                        }
                    }
                    if (!authorSearch.empty()) break;
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
                    // Display name with underscores replaced by spaces
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

    void HCPEngineWidget::ShowBonds(const QString& docId, const QString& tokenId)
    {
        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
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
            // Resolve surface for header
            QString surfaceStr = ResolveSurface(azTokenId, engine->GetVocabulary());
            QString surfaceQ = surfaceStr.isEmpty() ? tokenId
                : QString("%1 (%2)").arg(tokenId, surfaceStr);
            m_bondHeader->setText(QString("Bonds for: %1").arg(surfaceQ));
        }

        for (const auto& be : bonds)
        {
            auto* item = new QTreeWidgetItem(m_bondTree);
            item->setText(0, QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));

            // Resolve surface form — words, chars (with control notation), and markers
            QString surface = ResolveSurface(be.tokenB, engine->GetVocabulary());
            if (!surface.isEmpty())
            {
                item->setText(1, surface);
            }
            item->setText(2, QString::number(be.count));
            item->setTextAlignment(2, Qt::AlignRight);

            // Store token ID for drill-down
            item->setData(0, Qt::UserRole,
                QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));
        }

        m_bondTree->sortByColumn(2, Qt::DescendingOrder);
    }

    void HCPEngineWidget::OnBondTokenClicked(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item || m_selectedDocId.isEmpty()) return;
        QString tokenId = item->data(0, Qt::UserRole).toString();
        ShowBonds(m_selectedDocId, tokenId);
    }

    void HCPEngineWidget::OnSearchBonds()
    {
        if (m_selectedDocId.isEmpty()) return;
        QString searchText = m_bondSearch->text().trimmed();
        if (searchText.isEmpty()) return;

        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
        AZStd::string azDocId(m_selectedDocId.toUtf8().constData());
        int docPk = wk.GetDocPk(azDocId);
        if (docPk == 0) return;

        // Fetch ALL starters (no LIMIT 50)
        auto allStarters = wk.GetAllStarters(docPk);

        // Resolve each to surface form and filter by substring match
        m_bondTree->clear();
        int matchCount = 0;

        for (const auto& be : allStarters)
        {
            AZStd::string azToken(be.tokenB);
            QString surface = ResolveSurface(azToken, engine->GetVocabulary());
            if (surface.isEmpty())
            {
                surface = QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size()));
            }

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
    }

    void HCPEngineWidget::OnClearBondSearch()
    {
        m_bondSearch->clear();
        if (!m_selectedDocId.isEmpty())
        {
            ShowBonds(m_selectedDocId);
        }
    }

    void HCPEngineWidget::ShowVars(const QString& docId, const QString& filterEntityId)
    {
        m_varTree->clear();

        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
        AZStd::string azDocId(docId.toUtf8().constData());
        int docPk = wk.GetDocPk(azDocId);
        if (docPk == 0) return;

        auto vars = wk.GetDocVarsExtended(docPk);

        for (const auto& v : vars)
        {
            // If filtering by entity, only show vars whose group points to that entity
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

            // Store suggested entity ID for cross-link navigation
            item->setData(0, Qt::UserRole,
                QString::fromUtf8(v.suggestedId.c_str(),
                    static_cast<int>(v.suggestedId.size())));

            // Category-based styling
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
            // lingo = normal (default)
        }
    }

    void HCPEngineWidget::OnVarClicked(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item || m_selectedDocId.isEmpty()) return;

        QString suggestedId = item->data(0, Qt::UserRole).toString();
        if (suggestedId.isEmpty()) return;

        // Navigate to Entities tab filtered to this entity
        UpdateBreadcrumb(QString("Var: %1 > Entity").arg(item->text(0)));
        ShowEntities(m_selectedDocId, suggestedId);
        m_tabs->setCurrentIndex(m_tabEntities);
    }

    void HCPEngineWidget::OnEntityClicked(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item || m_selectedDocId.isEmpty()) return;

        // Skip group headers (items with children)
        if (item->childCount() > 0) return;

        QString entityId = item->text(1); // Entity ID column
        if (entityId.isEmpty()) return;

        // Navigate to Vars tab filtered to vars linked to this entity
        UpdateBreadcrumb(QString("Entity: %1 > Vars").arg(item->text(0)));
        ShowVars(m_selectedDocId, entityId);
        m_tabs->setCurrentIndex(m_tabVars);
    }

    void HCPEngineWidget::OnBreadcrumbReset()
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

    void HCPEngineWidget::NavigateTo(int tabIndex, const QString& filter)
    {
        m_activeFilter = filter;
        m_tabs->setCurrentIndex(tabIndex);
    }

    void HCPEngineWidget::UpdateBreadcrumb(const QString& segment)
    {
        m_breadcrumb->setText(segment);
        m_breadcrumbReset->setVisible(true);
    }

    void HCPEngineWidget::OnRetrieveText()
    {
        if (m_selectedDocId.isEmpty()) return;

        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
        AZStd::string azDocId(m_selectedDocId.toUtf8().constData());

        auto tokenIds = wk.LoadPositions(azDocId);
        if (tokenIds.empty())
        {
            m_textView->setPlainText("(no positions stored)");
            return;
        }

        AZStd::string text = TokenIdsToText(tokenIds, engine->GetVocabulary());
        m_textView->setPlainText(
            QString::fromUtf8(text.c_str(), static_cast<int>(text.size())));
    }

    void HCPEngineWidget::OnSaveMetadata()
    {
        if (m_selectedDocPk == 0) return;

        QString key = m_metaKeyInput->text().trimmed();
        QString value = m_metaValueInput->text().trimmed();
        if (key.isEmpty()) return;

        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();

        // Build JSON object for the set operation
        AZStd::string setJson = "{\"" +
            AZStd::string(key.toUtf8().constData()) + "\":\"" +
            AZStd::string(value.toUtf8().constData()) + "\"}";

        AZStd::vector<AZStd::string> removeKeys;
        wk.UpdateMetadata(m_selectedDocPk, setJson, removeKeys);

        // Refresh the info panel
        m_metaKeyInput->clear();
        m_metaValueInput->clear();
        ShowDocumentInfo(m_selectedDocId);
    }

    void HCPEngineWidget::OnImportMetadata()
    {
        if (m_selectedDocPk == 0 || m_selectedDocId.isEmpty()) return;

        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
        AZStd::string azDocId(m_selectedDocId.toUtf8().constData());

        // Get document name for title matching
        auto detail = wk.GetDocumentDetail(azDocId);
        QString docName = QString::fromUtf8(detail.name.c_str(), static_cast<int>(detail.name.size()));

        // Try provenance catalog_id first
        auto prov = wk.GetProvenance(m_selectedDocPk);
        QString catalogId;
        if (prov.found && !prov.catalogId.empty())
        {
            catalogId = QString::fromUtf8(prov.catalogId.c_str(), static_cast<int>(prov.catalogId.size()));
        }

        // Search Gutenberg JSON files
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

                // Match by catalog_id if available
                if (!catalogId.isEmpty())
                {
                    if (QString::number(obj["id"].toInt()) == catalogId)
                    {
                        matchedEntry = obj;
                        found = true;
                        break;
                    }
                }
                // Fall back to title match
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

        // Build metadata JSON from matched entry
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

        // Refresh the display
        ShowDocumentInfo(m_selectedDocId);
    }

} // namespace HCPEngine

#include <moc_HCPEngineWidget.cpp>
