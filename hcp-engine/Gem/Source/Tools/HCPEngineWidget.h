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
    //! Tabs: Info, Metadata, Bonds, Text
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

    private:
        void BuildLayout();
        void PopulateDocumentList();
        void ShowDocumentInfo(const QString& docId);
        void ShowBonds(const QString& docId, const QString& tokenId = {});
        void ShowText(const QString& docId);

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

        // Bonds tab
        QTreeWidget* m_bondTree = nullptr;
        QLabel* m_bondHeader = nullptr;

        // Text tab
        QTextEdit* m_textView = nullptr;
        QPushButton* m_retrieveBtn = nullptr;

        // State
        QString m_selectedDocId;
        int m_selectedDocPk = 0;
    };

} // namespace HCPEngine
