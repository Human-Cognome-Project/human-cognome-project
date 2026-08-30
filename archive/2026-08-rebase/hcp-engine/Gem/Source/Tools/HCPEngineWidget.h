#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QTextEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QSplitter;
#endif

namespace HCPEngine
{
    class HCPEngineSystemComponent;

    //! Main editor widget for the HCP Asset Manager.
    //! Layout: document list (left) + tabbed detail panel (right).
    //! Tabs: Info, Metadata, Entities, Vars, Bonds, Text
    //! Cross-link navigation: click entities/vars/bonds to drill down.
    class HCPEngineWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit HCPEngineWidget(QWidget* parent = nullptr);

    private slots:
        void OnRefreshDocuments();
        void OnDocumentSelected(QTreeWidgetItem* item, int column);
        void OnBondTokenClicked(QTreeWidgetItem* item, int column);
        void OnRetrieveText();
        void OnSaveMetadata();
        void OnSearchBonds();
        void OnClearBondSearch();
        void OnImportMetadata();
        void OnVarClicked(QTreeWidgetItem* item, int column);
        void OnEntityClicked(QTreeWidgetItem* item, int column);
        void OnBreadcrumbReset();

    private:
        void BuildLayout();
        void PopulateDocumentList();
        void ShowDocumentInfo(const QString& docId);
        void ShowBonds(const QString& docId, const QString& tokenId = {});
        void ShowEntities(const QString& docId, const QString& filterEntityId = {});
        void ShowVars(const QString& docId, const QString& filterEntityId = {});
        void ShowText(const QString& docId);
        void NavigateTo(int tabIndex, const QString& filter = {});
        void UpdateBreadcrumb(const QString& segment);

        HCPEngineSystemComponent* GetEngine();

        // Left panel — document list
        QTreeWidget* m_docList = nullptr;
        QPushButton* m_refreshBtn = nullptr;

        // Right panel — tabs
        QTabWidget* m_tabs = nullptr;

        // Info tab
        QLabel* m_infoDocId = nullptr;
        QLabel* m_infoName = nullptr;
        QLabel* m_infoSlots = nullptr;
        QLabel* m_infoUnique = nullptr;
        QLabel* m_infoStarters = nullptr;
        QLabel* m_infoBonds = nullptr;

        // Metadata tab
        QTableWidget* m_metaTable = nullptr;
        QLineEdit* m_metaKeyInput = nullptr;
        QLineEdit* m_metaValueInput = nullptr;
        QPushButton* m_metaSaveBtn = nullptr;
        QPushButton* m_metaImportBtn = nullptr;

        // Entities tab
        QTreeWidget* m_entityTree = nullptr;

        // Vars tab
        QTreeWidget* m_varTree = nullptr;

        // Bonds tab
        QTreeWidget* m_bondTree = nullptr;
        QLabel* m_bondHeader = nullptr;
        QLineEdit* m_bondSearch = nullptr;
        QPushButton* m_bondSearchClear = nullptr;

        // Text tab
        QTextEdit* m_textView = nullptr;
        QPushButton* m_retrieveBtn = nullptr;

        // Navigation breadcrumb
        QLabel* m_breadcrumb = nullptr;
        QPushButton* m_breadcrumbReset = nullptr;

        // State
        QString m_selectedDocId;
        int m_selectedDocPk = 0;
        QString m_activeFilter;  // entity ID filter for cross-linking

        // Tab indices (set in BuildLayout)
        int m_tabInfo = 0;
        int m_tabMeta = 1;
        int m_tabEntities = 2;
        int m_tabVars = 3;
        int m_tabBonds = 4;
        int m_tabText = 5;
    };

} // namespace HCPEngine
