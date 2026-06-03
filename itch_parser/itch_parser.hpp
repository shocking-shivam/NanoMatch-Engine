#pragma once
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include "../Limit_Order_Book/Book.hpp"

#pragma pack(push, 1)
struct ItchAddOrder {
    char     msg_type;      
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint8_t  timestamp[6];
    uint64_t order_ref;     
    char     buy_sell;      
    uint32_t shares;
    char     stock[8];      
    uint32_t price;
};

struct ItchDeleteOrder {
    char     msg_type;      
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint8_t  timestamp[6];
    uint64_t order_ref;     
};
#pragma pack(pop)

class ItchParser {
public:
    static void processFile(const char* filepath, Book& engine) {
        int fd = open(filepath, O_RDONLY);
        if (fd < 0) {
            std::cerr << "Failed to open ITCH file.\n";
            return;
        }

        struct stat sb;
        fstat(fd, &sb);

        uint8_t* data = (uint8_t*)mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        madvise(data, sb.st_size, MADV_SEQUENTIAL); 

        size_t offset = 0;
        int messages_processed = 0;

        std::cout << "Parsing NASDAQ ITCH 5.0 Feed...\n";

        while (offset < sb.st_size && messages_processed < 2000000) { 
            char msg_type = data[offset];

            if (msg_type == 'A') {
                auto* msg = reinterpret_cast<ItchAddOrder*>(data + offset);
                uint64_t order_id = __builtin_bswap64(msg->order_ref);
                uint32_t shares = __builtin_bswap32(msg->shares);
                uint32_t price = __builtin_bswap32(msg->price);
                bool is_buy = (msg->buy_sell == 'B');

                if (msg->stock[0] == 'A' && msg->stock[1] == 'A' && msg->stock[2] == 'P' && msg->stock[3] == 'L') {
                    engine.addOrder(order_id, is_buy, shares, price);
                }
                offset += 36; 
                messages_processed++;
            } 
            else if (msg_type == 'D') {
                auto* msg = reinterpret_cast<ItchDeleteOrder*>(data + offset);
                uint64_t order_id = __builtin_bswap64(msg->order_ref);
                engine.cancelOrder(order_id);
                offset += 19;
                messages_processed++;
            }
            else {
                offset++; 
            }
        }

        munmap(data, sb.st_size);
        close(fd);
        std::cout << "Successfully parsed and routed " << messages_processed << " ITCH messages.\n";
    }
};
