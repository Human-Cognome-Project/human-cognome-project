#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QLabel>
#include <QStatusBar>
#include <QProgressBar>
#endif

namespace HCPEngine
{
    class HCPEngineSystemComponent;

    /// Main window for the HCP Source Workstation.
    /// Crystal Reports-style data surfing tool — document navigator (left),
    /// tabbed data panels (right), status bar (bottom).
    ///
    /// Engine pointer injected at construction — no singleton access.
    class HCPWorkstationWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit HCPWorkstationWindow(HCPEngineSystemComponent* engine,
                                       QWidget* parent = nullptr);
        ~HCPWorkstationWindow() override;

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    private slots:
        void OnOpenFile();
        void OnOpenFolder();
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
        void BuildMenuBar();
        void BuildStatusBar();
        void BuildCentralWidget();
        void BuildInfoTab(QWidget* parent);
        void BuildMetadataTab(QWidget* parent);
        void BuildEntitiesTab(QWidget* parent);
        void BuildVarsTab(QWidget* parent);
        void BuildBondsTab(QWidget* parent);
        void BuildTextTab(QWidget* parent);

        void PopulateDocumentList();
        void ShowDocumentInfo(const QString& docId);
        void ShowBonds(const QString& docId, const QString& tokenId = {});
        void ShowEntities(const QString& docId, const QString& filterEntityId = {});
        void ShowVars(const QString& docId, const QString& filterEntityId = {});
        void ShowText(const QString& docId);
        void UpdateBreadcrumb(const QString& segment);

        void IngestFile(const QString& filePath);
        void IngestFolder(const QString& folderPath);
        void IngestJsonSource(const QString& jsonPath);
        void IngestRawText(const QString& textPath);
        void ProcessThroughPipeline(const QString& docName, const QByteArray& rawBytes,
                                     const QString& metadataJson = {});

        void UpdateStatusBar();

        // Engine — injected, not owned
        HCPEngineSystemComponent* m_engine = nullptr;

        // Left panel — document navigator
        QTreeWidget* m_docList = nullptr;

        // Right panel — tabs
        QTabWidget* m_tabs = nullptr;

        // Info tab widgets
        QLabel* m_infoDocId = nullptr;
        QLabel* m_infoName = nullptr;
        QLabel* m_infoSlots = nullptr;
        QLabel* m_infoUnique = nullptr;
        QLabel* m_infoStarters = nullptr;
        QLabel* m_infoBonds = nullptr;

        // Metadata tab widgets
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

        // Breadcrumb navigation
        QLabel* m_breadcrumb = nullptr;
        QPushButton* m_breadcrumbReset = nullptr;

        // Status bar widgets
        QLabel* m_statusEngine = nullptr;
        QLabel* m_statusDb = nullptr;
        QLabel* m_statusGpu = nullptr;
        QProgressBar* m_progressBar = nullptr;

        // State
        QString m_selectedDocId;
        int m_selectedDocPk = 0;
        QString m_activeFilter;

        // Tab indices
        int m_tabInfo = 0;
        int m_tabMeta = 1;
        int m_tabEntities = 2;
        int m_tabVars = 3;
        int m_tabBonds = 4;
        int m_tabText = 5;
    };

} // namespace HCPEngine
