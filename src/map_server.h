#ifndef MAP_SERVER_H
#define MAP_SERVER_H

#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <iostream>

constexpr static std::size_t    MAP_COUNT        { 16 };
constexpr static std::size_t    MAP_SIZE         { MAP_COUNT * 4096UL  };


struct map_server
{
    bool map()
    {
        if((fd = open("/dev/mem", O_RDWR | O_SYNC))== -1)
        {
            std::cerr << std::strerror(errno) << std::endl;
            return false;
        }

        map_base = mmap(0, MAP_SIZE,
                        PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd,
                        0xff200000);

        if(map_base == MAP_FAILED)
        {
            close(fd);
            std::cerr << std::strerror(errno) << std::endl;
            return false;
        }
        return true;
    }

    void unmap()
    {
        if((munmap(map_base, MAP_SIZE))== -1)
        {
            std::cerr << std::strerror(errno) << std::endl;
            return;
        }
        close(fd);
    }

    std::uint32_t read(std::uint32_t address)               const
    {
        std::uint32_t* addr = static_cast<std::uint32_t*>(map_base) + address;
        return *addr;
    }

    void write(std::uint32_t address, std::uint32_t val)    const
    {
        std::uint32_t* addr = static_cast<std::uint32_t*>(map_base) + address;
        *addr = val;
    }

private:
    int   fd;
    void* map_base;
};

#endif // !MAP_SERVER_H
