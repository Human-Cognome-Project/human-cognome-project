#include "portgnome.h"
#include <iostream>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * _________________________________
 * |  PORTGNOME: Capacity Guardian   |
 * |   "I build for the limited."    |
 * |_________________________________|
 * | |
 */

namespace HCP {

    void PortGnome::WakeUp(const std::string& shardPath) {
        struct sysinfo info;
        sysinfo(&info);

        // Convert capacity to Megabytes for the logs
        long free_mb = (info.freeram * info.mem_unit) / 1024 / 1024;

        std::cout << "[PORTGNOME] Sensed " << free_mb << "MB available.\n";

        // Logic: If we have > 2GB free, we eat RAM. If not, we use Disk-Mapping.
        if (free_mb > 2048) {
            EngageAggressiveMode(shardPath);
        } else {
            EngageSurvivalMode(shardPath);
        }
    }

    void PortGnome::EngageSurvivalMode(const std::string& path) {
        std::cout << "(>_<) -> Low Capacity! Using mmap() for zero-RAM operation.\n";

        int fd = open(path.c_str(), O_RDONLY);
        if (fd == -1) return;

        // Get file size
        size_t size = lseek(fd, 0, SEEK_END);
        
        // This maps the 1.4M tokens directly from storage to the CPU address space
        void* map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        
        if (map != MAP_FAILED) {
            m_shardPointer = static_cast<OptimizedToken*>(map);
            m_tokenCount = size / sizeof(OptimizedToken);
            std::cout << "[PORTGNOME] Successfully mapped " << m_tokenCount << " tokens.\n";
        }
        close(fd);
    }
// I do not expect this to work. I wanted to make it easy for older hardware that do not have the power to do so.
// Availability and portability is essential for large scale projects. This file and the header file follows that principle.

    void PortGnome::EngageAggressiveMode(const std::string& path) {
        std::cout << "(^_^) -> High Capacity! Loading shard into Active RAM.\n";
        // Implementation for high-speed RAM loading goes here
    }
}
