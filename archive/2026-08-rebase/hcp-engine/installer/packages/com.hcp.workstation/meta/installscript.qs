// com.hcp.workstation — install script
// Handles desktop shortcut creation and basic setup.

function Component()
{
    // constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    // Write default viewer config (placeholder — update before release)
    component.addOperation("CreateDirectory", "@TargetDir@/config");
    component.addOperation("WriteFile",
        "@TargetDir@/config/viewer.conf",
        "# HCP Viewer — default connection\n" +
        "# Replace with production server address before distributing\n" +
        "db_connection=host=hcp.example.com port=5432 dbname=hcp_fic_pbm user=hcp_viewer password=PLACEHOLDER\n" +
        "vocab_path=@TargetDir@/vocab/vocab.lmdb\n");

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut",
            "@TargetDir@/bin/HCPWorkstation.exe",
            "@StartMenuDir@/HCP Viewer.lnk",
            "arguments=--viewer",
            "workingDirectory=@TargetDir@/bin");
    } else if (systemInfo.productType === "macos") {
        // macOS: handled by package data
    } else {
        // Linux: create .desktop file with --viewer flag
        component.addOperation("CreateDesktopEntry",
            "HCPViewer.desktop",
            "Version=1.0\nType=Application\nName=HCP Viewer\nExec=@TargetDir@/bin/HCPWorkstation --viewer\nIcon=@TargetDir@/share/hcp_icon.png\nTerminal=false\nCategories=Science;Education;");
    }
}
