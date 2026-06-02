// com.hcp.engine — install script
// Detects CUDA/GPU and warns if absent. Installs systemd service on Linux.

function Component()
{
    // constructor
}

Component.prototype.isDefault = function()
{
    return false;
}

// Called when component is selected — run capability detection and warn if needed
Component.prototype.loaded = function()
{
    checkCudaAvailability();
}

function checkCudaAvailability()
{
    // Only informational — engine runs CPU-only, GPU just improves performance
    var hasCuda = false;

    if (systemInfo.productType !== "windows") {
        // Try nvidia-smi
        var result = installer.execute("nvidia-smi", ["--query-gpu=name", "--format=csv,noheader"]);
        if (result[1] === 0 && result[0].trim().length > 0) {
            hasCuda = true;
            console.log("[HCP Engine] GPU detected: " + result[0].trim());
        }
    } else {
        // Windows: check for nvml.dll presence as a proxy
        var nvml = installer.fileExists("C:/Windows/System32/nvml.dll");
        if (nvml) hasCuda = true;
    }

    if (!hasCuda) {
        QMessageBox.information(
            "com.hcp.engine.nocuda",
            "No GPU Detected",
            "No CUDA-capable GPU was detected on this system.\n\n" +
            "The HCP Engine will run in CPU-only mode. " +
            "Resolution of large documents may be slower than on a GPU-equipped system.\n\n" +
            "You can still install and use the engine — this is informational only.",
            QMessageBox.Ok
        );
    }
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    // Linux: install systemd service
    if (systemInfo.productType !== "windows" && systemInfo.productType !== "macos") {
        component.addOperation("CreateDirectory", "@TargetDir@/daemon");

        // Write systemd unit file
        var serviceContent = [
            "[Unit]",
            "Description=HCP Engine Daemon",
            "After=network.target",
            "",
            "[Service]",
            "Type=simple",
            "ExecStart=@TargetDir@/daemon/HeadlessServerLauncher",
            "Restart=on-failure",
            "RestartSec=5",
            "",
            "[Install]",
            "WantedBy=multi-user.target"
        ].join("\n");

        component.addOperation("WriteFile",
            "@HomeDir@/.config/systemd/user/hcp-engine.service",
            serviceContent);

        // Note: actual systemd enable/start left to user or post-install script
        // to avoid requiring elevated permissions during install
    }
}
