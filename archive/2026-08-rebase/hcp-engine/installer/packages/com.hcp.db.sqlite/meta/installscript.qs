// com.hcp.db.sqlite — install script
// Writes default SQLite config. No detection needed — bundled.

function Component()
{
    // constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    component.addOperation("CreateDirectory", "@TargetDir@/data");

    component.addOperation("WriteFile",
        "@TargetDir@/config/db.conf",
        "# HCP Workstation — SQLite configuration\n" +
        "db_backend=sqlite\n" +
        "db_path=@TargetDir@/data/hcp.db\n");
}
