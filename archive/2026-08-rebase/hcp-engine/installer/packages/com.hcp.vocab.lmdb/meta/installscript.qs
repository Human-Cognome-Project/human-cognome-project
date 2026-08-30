// com.hcp.vocab.lmdb — install script
// Writes vocab path config after extracting LMDB data.

function Component()
{
    // constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    component.addOperation("CreateDirectory", "@TargetDir@/vocab");

    // Write vocab path into workstation config
    component.addOperation("WriteFile",
        "@TargetDir@/config/vocab.conf",
        "# HCP Workstation — LMDB vocabulary path\n" +
        "vocab_path=@TargetDir@/vocab/vocab.lmdb\n");
}
