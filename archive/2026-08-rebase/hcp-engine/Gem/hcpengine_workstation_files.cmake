
set(FILES
    # Workstation UI + socket client
    Source/Workstation/main.cpp
    Source/Workstation/HCPWorkstationEngine.cpp
    Source/Workstation/HCPWorkstationEngine.h
    Source/Workstation/HCPSocketClient.cpp
    Source/Workstation/HCPSocketClient.h
    Source/Workstation/HCPWorkstationWindow.cpp
    Source/Workstation/HCPWorkstationWindow.h

    # Embedded DB kernels (direct data access without daemon)
    Source/HCPDbConnection.cpp
    Source/HCPDbConnection.h
    Source/HCPPbmWriter.cpp
    Source/HCPPbmWriter.h
    Source/HCPPbmReader.cpp
    Source/HCPPbmReader.h
    Source/HCPDocumentQuery.cpp
    Source/HCPDocumentQuery.h
    Source/HCPDocVarQuery.cpp
    Source/HCPDocVarQuery.h
    Source/HCPBondQuery.cpp
    Source/HCPBondQuery.h

    # Vocabulary (LMDB reader) + text reconstruction + cache miss resolver
    Source/HCPVocabulary.cpp
    Source/HCPVocabulary.h
    Source/HCPTokenizer.cpp
    Source/HCPTokenizer.h
    Source/HCPCacheMissResolver.cpp
    Source/HCPCacheMissResolver.h

    # Entity cross-reference (free functions, uses raw PGconn*)
    Source/HCPStorage.cpp
    Source/HCPStorage.h

    # Shared utilities (base-50 encode/decode, token helpers)
    Source/HCPDbUtils.h
)
