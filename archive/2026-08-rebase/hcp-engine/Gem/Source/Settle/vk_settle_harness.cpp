// vk_settle_harness.cpp — runs HCPSettleCompute.azsl's SPIR-V on a real GPU and
// asserts the result matches the CPU reference (SettleKernel.h). This is the
// resolution-equivalence test ON HARDWARE for the settle kernel, and a reusable
// rig for every future AZSL kernel.
//
// Zero external deps: O3DE's self-contained GLAD single-header provides the
// Vulkan API + an internal loader (dlopens libvulkan.so.1); no Vulkan SDK.
//
// Pipeline that produced the SPIR-V (build_spirv.sh): AZSL -> azslc HLSL ->
// dxc -spirv. SRG bindings (azslc --unique-idx, set 0):
//   binding 0  RWStructuredBuffer m_positions      (u0)
//   binding 1  RWStructuredBuffer m_prevPositions  (u1)
//   binding 2  StructuredBuffer   m_restY          (t2)
//   binding 3  RWStructuredBuffer m_settled        (u3)
//   binding 4  ConstantBuffer     params           (b4)

#define GLAD_VULKAN_IMPLEMENTATION
#include <glad/vulkan.h>

#include "SettleKernel.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

using hcp::settle::Float4;
namespace sk = hcp::settle;

// GLAD here is pure multi-context mode: function pointers live in a context
// struct (no global vkXxx aliases). One global context + name aliases keep the
// call sites idiomatic. Member name = vk name minus the "vk" prefix.
static GladVulkanContext g_vk;
#define vkCreateInstance                       g_vk.CreateInstance
#define vkEnumeratePhysicalDevices             g_vk.EnumeratePhysicalDevices
#define vkGetPhysicalDeviceProperties          g_vk.GetPhysicalDeviceProperties
#define vkGetPhysicalDeviceMemoryProperties    g_vk.GetPhysicalDeviceMemoryProperties
#define vkGetPhysicalDeviceQueueFamilyProperties g_vk.GetPhysicalDeviceQueueFamilyProperties
#define vkCreateDevice                         g_vk.CreateDevice
#define vkGetDeviceQueue                       g_vk.GetDeviceQueue
#define vkCreateBuffer                         g_vk.CreateBuffer
#define vkGetBufferMemoryRequirements          g_vk.GetBufferMemoryRequirements
#define vkAllocateMemory                       g_vk.AllocateMemory
#define vkBindBufferMemory                     g_vk.BindBufferMemory
#define vkMapMemory                            g_vk.MapMemory
#define vkUnmapMemory                          g_vk.UnmapMemory
#define vkCreateDescriptorSetLayout            g_vk.CreateDescriptorSetLayout
#define vkCreatePipelineLayout                 g_vk.CreatePipelineLayout
#define vkCreateShaderModule                   g_vk.CreateShaderModule
#define vkCreateComputePipelines               g_vk.CreateComputePipelines
#define vkCreateDescriptorPool                 g_vk.CreateDescriptorPool
#define vkAllocateDescriptorSets               g_vk.AllocateDescriptorSets
#define vkUpdateDescriptorSets                 g_vk.UpdateDescriptorSets
#define vkCreateCommandPool                    g_vk.CreateCommandPool
#define vkAllocateCommandBuffers               g_vk.AllocateCommandBuffers
#define vkBeginCommandBuffer                   g_vk.BeginCommandBuffer
#define vkCmdBindPipeline                      g_vk.CmdBindPipeline
#define vkCmdBindDescriptorSets                g_vk.CmdBindDescriptorSets
#define vkCmdDispatch                          g_vk.CmdDispatch
#define vkCmdPipelineBarrier                   g_vk.CmdPipelineBarrier
#define vkEndCommandBuffer                     g_vk.EndCommandBuffer
#define vkQueueSubmit                          g_vk.QueueSubmit
#define vkQueueWaitIdle                        g_vk.QueueWaitIdle
#define vkDestroyCommandPool                   g_vk.DestroyCommandPool

#define VK_CHECK(expr) do { VkResult _r = (expr); if (_r != VK_SUCCESS) { \
    std::fprintf(stderr, "VK error %d at %s:%d (%s)\n", (int)_r, __FILE__, __LINE__, #expr); \
    std::exit(2); } } while (0)

// Matches the cbuffer (b4) member order in HCPSettleSrg.azsli (std140 scalars).
struct Params {
    uint32_t particleCount;
    float    dt, gravity, damping, friction, velSettleThreshold, noFloor;
};

struct Ctx {
    VkInstance       instance = VK_NULL_HANDLE;
    VkPhysicalDevice phys     = VK_NULL_HANDLE;
    VkDevice         device   = VK_NULL_HANDLE;
    VkQueue          queue    = VK_NULL_HANDLE;
    uint32_t         queueFamily = 0;
    VkPhysicalDeviceMemoryProperties memProps{};
};

struct Buf { VkBuffer buf = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; VkDeviceSize size = 0; };

static uint32_t FindMemoryType(const Ctx& c, uint32_t typeBits, VkMemoryPropertyFlags want)
{
    for (uint32_t i = 0; i < c.memProps.memoryTypeCount; ++i)
        if ((typeBits & (1u << i)) && (c.memProps.memoryTypes[i].propertyFlags & want) == want)
            return i;
    std::fprintf(stderr, "no memory type with flags 0x%x\n", want); std::exit(2);
}

static Buf CreateHostBuffer(const Ctx& c, VkDeviceSize size, VkBufferUsageFlags usage)
{
    Buf b; b.size = size;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = size; bi.usage = usage; bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(c.device, &bi, nullptr, &b.buf));
    VkMemoryRequirements req; vkGetBufferMemoryRequirements(c.device, b.buf, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = FindMemoryType(c, req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK(vkAllocateMemory(c.device, &ai, nullptr, &b.mem));
    VK_CHECK(vkBindBufferMemory(c.device, b.buf, b.mem, 0));
    return b;
}

static void Upload(const Ctx& c, const Buf& b, const void* src, size_t bytes)
{
    void* p = nullptr; VK_CHECK(vkMapMemory(c.device, b.mem, 0, bytes, 0, &p));
    std::memcpy(p, src, bytes); vkUnmapMemory(c.device, b.mem);
}
static void Download(const Ctx& c, const Buf& b, void* dst, size_t bytes)
{
    void* p = nullptr; VK_CHECK(vkMapMemory(c.device, b.mem, 0, bytes, 0, &p));
    std::memcpy(dst, p, bytes); vkUnmapMemory(c.device, b.mem);
}

static void InitVulkan(Ctx& c)
{
    if (!gladLoaderLoadVulkanContext(&g_vk, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE)) {
        std::fprintf(stderr, "SKIP: could not load libvulkan (no Vulkan loader)\n"); std::exit(0);
    }
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "hcp_settle_harness"; app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ii{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; ii.pApplicationInfo = &app;
    VK_CHECK(vkCreateInstance(&ii, nullptr, &c.instance));
    gladLoaderLoadVulkanContext(&g_vk, c.instance, VK_NULL_HANDLE, VK_NULL_HANDLE);

    uint32_t n = 0; VK_CHECK(vkEnumeratePhysicalDevices(c.instance, &n, nullptr));
    if (n == 0) { std::fprintf(stderr, "SKIP: no Vulkan physical devices\n"); std::exit(0); }
    std::vector<VkPhysicalDevice> devs(n);
    VK_CHECK(vkEnumeratePhysicalDevices(c.instance, &n, devs.data()));

    // Prefer a discrete GPU; never pick a CPU (llvmpipe) if a GPU exists.
    VkPhysicalDevice chosen = VK_NULL_HANDLE; int chosenRank = -1;
    for (auto d : devs) {
        VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(d, &p);
        int rank = (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)   ? 3 :
                   (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) ? 2 :
                   (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)            ? 0 : 1;
        if (rank > chosenRank) { chosenRank = rank; chosen = d; }
    }
    // Optional override: HCP_VK_DEVICE=<name substring> pins a specific GPU (e.g. "750").
    if (const char* want = std::getenv("HCP_VK_DEVICE")) {
        for (auto d : devs) {
            VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(d, &p);
            if (std::strstr(p.deviceName, want)) { chosen = d; break; }
        }
    }
    c.phys = chosen;
    VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(c.phys, &p);
    std::printf("GPU: %s\n", p.deviceName);
    vkGetPhysicalDeviceMemoryProperties(c.phys, &c.memProps);

    uint32_t qn = 0; vkGetPhysicalDeviceQueueFamilyProperties(c.phys, &qn, nullptr);
    std::vector<VkQueueFamilyProperties> qf(qn);
    vkGetPhysicalDeviceQueueFamilyProperties(c.phys, &qn, qf.data());
    bool found = false;
    for (uint32_t i = 0; i < qn; ++i)
        if (qf[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { c.queueFamily = i; found = true; break; }
    if (!found) { std::fprintf(stderr, "no compute queue family\n"); std::exit(2); }

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qi.queueFamilyIndex = c.queueFamily; qi.queueCount = 1; qi.pQueuePriorities = &prio;
    VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    di.queueCreateInfoCount = 1; di.pQueueCreateInfos = &qi;
    VK_CHECK(vkCreateDevice(c.phys, &di, nullptr, &c.device));
    gladLoaderLoadVulkanContext(&g_vk, c.instance, c.phys, c.device);
    vkGetDeviceQueue(c.device, c.queueFamily, 0, &c.queue);
}

static std::vector<uint32_t> LoadSpirv(const char* path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { std::fprintf(stderr, "SKIP: %s not found (run build_spirv.sh)\n", path); std::exit(0); }
    size_t bytes = (size_t)f.tellg(); f.seekg(0);
    std::vector<uint32_t> code(bytes / 4);
    f.read(reinterpret_cast<char*>(code.data()), bytes);
    return code;
}

// Run `steps` settle steps on the GPU. cur/prev are in/out; settledOut filled.
static void RunSettleGPU(Ctx& c, VkPipeline pipe, VkPipelineLayout layout,
                         VkDescriptorSet set, Buf& posB, Buf& prevB, Buf& restB,
                         Buf& setB, Buf& parB, std::vector<Float4>& cur,
                         std::vector<Float4>& prev, const std::vector<float>& restY,
                         const Params& params, std::vector<uint32_t>& settledOut, int steps)
{
    const uint32_t N = params.particleCount;
    Upload(c, posB,  cur.data(),   N * sizeof(Float4));
    Upload(c, prevB, prev.data(),  N * sizeof(Float4));
    Upload(c, restB, restY.data(), N * sizeof(float));
    Upload(c, parB,  &params,      sizeof(Params));

    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.queueFamilyIndex = c.queueFamily;
    VkCommandPool pool; VK_CHECK(vkCreateCommandPool(c.device, &pci, nullptr, &pool));
    VkCommandBufferAllocateInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbi.commandPool = pool; cbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cbi.commandBufferCount = 1;
    VkCommandBuffer cmd; VK_CHECK(vkAllocateCommandBuffers(c.device, &cbi, &cmd));

    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &bi));
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, nullptr);
    const uint32_t groups = (N + 63) / 64;
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    for (int s = 0; s < steps; ++s) {
        vkCmdDispatch(cmd, groups, 1, 1);
        if (s + 1 < steps)
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    }
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1; si.pCommandBuffers = &cmd;
    VK_CHECK(vkQueueSubmit(c.queue, 1, &si, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(c.queue));

    Download(c, posB, cur.data(),  N * sizeof(Float4));
    Download(c, prevB, prev.data(), N * sizeof(Float4));
    settledOut.resize(N);
    Download(c, setB, settledOut.data(), N * sizeof(uint32_t));

    vkDestroyCommandPool(c.device, pool, nullptr);
}

// ---------------------------------------------------------------------------
static int g_pass = 0, g_fail = 0;
static void Check(const char* label, bool ok) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", label);
    if (ok) ++g_pass; else ++g_fail;
}
static bool BitEq(const Float4& a, const Float4& b) { return std::memcmp(&a, &b, sizeof(Float4)) == 0; }

int main(int argc, char** argv)
{
    const char* spvPath = argc > 1 ? argv[1] : "settle.spv";
    std::printf("=== HCP settle GPU==CPU equivalence ===\n");

    Ctx c; InitVulkan(c);
    auto code = LoadSpirv(spvPath);

    // ---- pipeline + descriptor plumbing (shared across runs) ----
    VkDescriptorSetLayoutBinding binds[5]{};
    for (int i = 0; i < 5; ++i) {
        binds[i].binding = i; binds[i].descriptorCount = 1;
        binds[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        binds[i].descriptorType = (i == 4) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                           : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    VkDescriptorSetLayoutCreateInfo dli{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dli.bindingCount = 5; dli.pBindings = binds;
    VkDescriptorSetLayout dsl; VK_CHECK(vkCreateDescriptorSetLayout(c.device, &dli, nullptr, &dsl));

    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1; pli.pSetLayouts = &dsl;
    VkPipelineLayout layout; VK_CHECK(vkCreatePipelineLayout(c.device, &pli, nullptr, &layout));

    VkShaderModuleCreateInfo smi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    smi.codeSize = code.size() * 4; smi.pCode = code.data();
    VkShaderModule module; VK_CHECK(vkCreateShaderModule(c.device, &smi, nullptr, &module));

    VkComputePipelineCreateInfo cpi{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    cpi.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cpi.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cpi.stage.module = module; cpi.stage.pName = "SettleStep";
    cpi.layout = layout;
    VkPipeline pipe; VK_CHECK(vkCreateComputePipelines(c.device, VK_NULL_HANDLE, 1, &cpi, nullptr, &pipe));

    VkDescriptorPoolSize sizes[2] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};
    VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpi.maxSets = 1; dpi.poolSizeCount = 2; dpi.pPoolSizes = sizes;
    VkDescriptorPool dpool; VK_CHECK(vkCreateDescriptorPool(c.device, &dpi, nullptr, &dpool));
    VkDescriptorSetAllocateInfo dsa{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dsa.descriptorPool = dpool; dsa.descriptorSetCount = 1; dsa.pSetLayouts = &dsl;
    VkDescriptorSet set; VK_CHECK(vkAllocateDescriptorSets(c.device, &dsa, &set));

    const uint32_t MAXN = 4096;
    Buf posB  = CreateHostBuffer(c, MAXN * sizeof(Float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    Buf prevB = CreateHostBuffer(c, MAXN * sizeof(Float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    Buf restB = CreateHostBuffer(c, MAXN * sizeof(float),  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    Buf setB  = CreateHostBuffer(c, MAXN * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    Buf parB  = CreateHostBuffer(c, sizeof(Params), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto writeDesc = [&](uint32_t binding, const Buf& b, VkDescriptorType t) {
        VkDescriptorBufferInfo info{b.buf, 0, VK_WHOLE_SIZE};
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        w.dstSet = set; w.dstBinding = binding; w.descriptorCount = 1;
        w.descriptorType = t; w.pBufferInfo = &info;
        vkUpdateDescriptorSets(c.device, 1, &w, 0, nullptr);
    };
    writeDesc(0, posB,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writeDesc(1, prevB, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writeDesc(2, restB, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writeDesc(3, setB,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writeDesc(4, parB,  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    auto makeParams = [](uint32_t n) {
        Params p; p.particleCount = n; p.dt = sk::DT; p.gravity = sk::GRAVITY;
        p.damping = sk::DAMPING; p.friction = sk::FRICTION;
        p.velSettleThreshold = sk::VEL_SETTLE_THRESHOLD; p.noFloor = sk::NO_FLOOR;
        return p;
    };

    // ---- Test 1: one step, no contact — GPU positions ~= CPU, settled flags exact ----
    {
        std::vector<Float4> cur = {{0,5,0,1},{2,5,0,1},{4,5,0,0}};  // 2 run + 1 bed
        std::vector<float> restY = {sk::NO_FLOOR, sk::NO_FLOOR, sk::NO_FLOOR};
        std::vector<Float4> gpuCur = cur, gpuPrev = cur, cpuCur = cur, cpuPrev = cur;
        std::vector<uint32_t> gpuSettled;
        Params pr = makeParams((uint32_t)cur.size());
        RunSettleGPU(c, pipe, layout, set, posB, prevB, restB, setB, parB,
                     gpuCur, gpuPrev, restY, pr, gpuSettled, 1);
        sk::SettleStepAll(cpuCur, cpuPrev, restY);

        float maxErr = 0; for (size_t i = 0; i < cur.size(); ++i) {
            maxErr = std::fmax(maxErr, std::fabs(gpuCur[i].x - cpuCur[i].x));
            maxErr = std::fmax(maxErr, std::fabs(gpuCur[i].y - cpuCur[i].y));
            maxErr = std::fmax(maxErr, std::fabs(gpuCur[i].z - cpuCur[i].z));
        }
        Check("1 step: GPU positions match CPU within 1e-4", maxErr < 1e-4f);
        std::printf("       (max position error = %.3e)\n", maxErr);

        bool gateMatch = true;
        for (size_t i = 0; i < cur.size(); ++i) {
            bool cpuS = sk::L1Velocity(cpuCur[i], cpuPrev[i]) < sk::VEL_SETTLE_THRESHOLD;
            gateMatch = gateMatch && ((gpuSettled[i] != 0) == cpuS);
        }
        Check("1 step: GPU settled flags match CPU gate exactly", gateMatch);
        Check("1 step: bed particle (w<=0) untouched on GPU", BitEq(gpuCur[2], cur[2]));
    }

    // ---- Test 2: 60 steps with contact — gate matches CPU, run settles ----
    {
        const uint32_t N = 8;
        std::vector<Float4> cur(N); std::vector<float> restY(N);
        for (uint32_t i = 0; i < N; ++i) { cur[i] = {(float)i, 0.5f, 0.0f, 1.0f}; restY[i] = 0.0f; }
        std::vector<Float4> gpuCur = cur, gpuPrev = cur, cpuCur = cur, cpuPrev = cur;
        std::vector<uint32_t> gpuSettled;
        Params pr = makeParams(N);
        RunSettleGPU(c, pipe, layout, set, posB, prevB, restB, setB, parB,
                     gpuCur, gpuPrev, restY, pr, gpuSettled, sk::SETTLE_STEPS);
        for (int s = 0; s < sk::SETTLE_STEPS; ++s) sk::SettleStepAll(cpuCur, cpuPrev, restY);

        bool allSettledGPU = true, gateMatch = true;
        for (uint32_t i = 0; i < N; ++i) {
            allSettledGPU = allSettledGPU && (gpuSettled[i] != 0);
            bool cpuS = sk::L1Velocity(cpuCur[i], cpuPrev[i]) < sk::VEL_SETTLE_THRESHOLD;
            gateMatch = gateMatch && ((gpuSettled[i] != 0) == cpuS);
        }
        Check("60 steps: GPU run fully settles (gate fires)", allSettledGPU);
        Check("60 steps: GPU gate matches CPU per particle", gateMatch);

        float maxErr = 0; for (uint32_t i = 0; i < N; ++i)
            maxErr = std::fmax(maxErr, std::fabs(gpuCur[i].y - cpuCur[i].y));
        Check("60 steps: GPU settled positions match CPU within 1e-2", maxErr < 1e-2f);
        std::printf("       (max y error after 60 steps = %.3e)\n", maxErr);
    }

    // ---- Test 3: GPU determinism (two runs identical) ----
    {
        const uint32_t N = 64;
        std::vector<Float4> cur(N); std::vector<float> restY(N);
        for (uint32_t i = 0; i < N; ++i) {
            float w = (i % 2) ? 1.0f : 0.0f;
            cur[i] = {(float)i, 4.0f + 0.1f * i, (float)(i % 3), w};
            restY[i] = w > 0 ? 0.0f : sk::NO_FLOOR;
        }
        std::vector<Float4> a = cur, ap = cur, b = cur, bp = cur;
        std::vector<uint32_t> sa, sb; Params pr = makeParams(N);
        RunSettleGPU(c, pipe, layout, set, posB, prevB, restB, setB, parB, a, ap, restY, pr, sa, 30);
        RunSettleGPU(c, pipe, layout, set, posB, prevB, restB, setB, parB, b, bp, restY, pr, sb, 30);
        bool same = (sa == sb);
        for (uint32_t i = 0; same && i < N; ++i) same = BitEq(a[i], b[i]) && BitEq(ap[i], bp[i]);
        Check("GPU is deterministic (two identical runs agree bit-for-bit)", same);
    }

    std::printf("=== %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail;
}
