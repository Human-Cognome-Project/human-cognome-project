#include "HCPEngineWidget.h"
#include "../HCPEngineSystemComponent.h"
#include "../HCPStorage.h"
#include "../HCPVocabulary.h"
#include "../HCPTokenizer.h"

#include <AzCore/Component/ComponentApplicationBus.h>

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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace HCPEngine
{
    // Find the engine system component via the component application bus
    HCPEngineSystemComponent* HCPEngineWidget::GetEngine()
    {
        HCPEngineSystemComponent* engine = nullptr;
        AZ::Entity* systemEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            systemEntity, &AZ::ComponentApplicationBus::Events::FindEntity,
            AZ::SystemEntityId);
        if (systemEntity)
        {
            engine = systemEntity->FindComponent<HCPEngineSystemComponent>();
        }
        return engine;
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
        m_tabs = new QTabWidget(splitter);

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

        m_tabs->addTab(metaWidget, "Metadata");

        // -- Bonds tab --
        auto* bondsWidget = new QWidget();
        auto* bondsLayout = new QVBoxLayout(bondsWidget);

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

        splitter->addWidget(m_tabs);
        splitter->setStretchFactor(0, 1);  // doc list
        splitter->setStretchFactor(1, 2);  // detail panel

        mainLayout->addWidget(splitter, 1);
        setLayout(mainLayout);
    }

    void HCPEngineWidget::PopulateDocumentList()
    {
        m_docList->clear();

        auto* engine = GetEngine();
        if (!engine) return;

        auto& wk = engine->GetWriteKernel();
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

    void HCPEngineWidget::OnRefreshDocuments()
    {
        PopulateDocumentList();
    }

    void HCPEngineWidget::OnDocumentSelected(QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (!item) return;
        m_selectedDocId = item->data(0, Qt::UserRole).toString();
        ShowDocumentInfo(m_selectedDocId);
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
            // Parse the JSON and populate table
            // Using a simple key-value extraction since we have raw JSON
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
            AZStd::string surface = engine->GetVocabulary().TokenToWord(azTokenId);
            QString surfaceQ = surface.empty() ? tokenId
                : QString("%1 (%2)").arg(tokenId,
                    QString::fromUtf8(surface.c_str(), static_cast<int>(surface.size())));
            m_bondHeader->setText(QString("Bonds for: %1").arg(surfaceQ));
        }

        for (const auto& be : bonds)
        {
            auto* item = new QTreeWidgetItem(m_bondTree);
            item->setText(0, QString::fromUtf8(be.tokenB.c_str(), static_cast<int>(be.tokenB.size())));

            // Resolve surface form
            AZStd::string surface = engine->GetVocabulary().TokenToWord(be.tokenB);
            if (!surface.empty())
            {
                item->setText(1, QString::fromUtf8(surface.c_str(), static_cast<int>(surface.size())));
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

} // namespace HCPEngine

#include <moc_HCPEngineWidget.cpp>
