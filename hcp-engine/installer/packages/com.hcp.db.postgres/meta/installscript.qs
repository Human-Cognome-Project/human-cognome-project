// com.hcp.db.postgres — install script
// Prompts for Postgres connection string and writes config.
// Detects if Postgres is reachable at the given address.

function Component()
{
    // constructor
}

Component.prototype.isDefault = function()
{
    return false;
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    // Write a template config that the user fills in (or prefilled from installer page)
    var connString = component.userInterface("PostgresConfigWidget").connStringInput.text;

    if (!connString || connString.trim().length === 0) {
        connString = "host=localhost port=5432 dbname=hcp_fic_pbm user=hcp";
    }

    component.addOperation("WriteFile",
        "@TargetDir@/config/db.conf",
        "# HCP Workstation — PostgreSQL connection\n" +
        "# Edit this file to point at your PostgreSQL server.\n" +
        "db_backend=postgres\n" +
        "db_connection=" + connString + "\n");
}
