#include "xgo_driver/xgo_mini2s.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <sys/resource.h>

using namespace std::chrono;

int main() {
    constexpr int N = 1'000'000;
    std::cout << "=== BENCHMARK C++20 (" << N << " iteraciones, -O3 Release) ===" << std::endl;

    // 1. Test Conversiones y Creación de Paquetes
    auto t0 = high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        double vel = -0.25 + ((i % 50) * 0.01);
        uint8_t b_vx = xgo::conver2u8(vel, xgo::Limits::VX_MIN, xgo::Limits::VX_MAX);
        auto pkt = xgo::create_write_packet(Register::VX, b_vx);
    }
    auto t1 = high_resolution_clock::now();
    double t_conv_ms = duration<double, std::milli>(t1 - t0).count();
    std::cout << "  [1] Conversión + Creación de Paquetes: " << t_conv_ms 
              << " ms (" << (t_conv_ms * 1000.0 / N) << " µs/op)" << std::endl;

    // 2. Test Desempaquetado de Float
    std::array<uint8_t, 4> raw_float{0x00, 0x00, 0x48, 0xC1};
    t0 = high_resolution_clock::now();
    float total_sink = 0.0f;
    for (int i = 0; i < N; ++i) {
        total_sink += xgo::Byte2float(raw_float);
    }
    t1 = high_resolution_clock::now();
    double t_float_ms = duration<double, std::milli>(t1 - t0).count();
    std::cout << "  [2] Byte2Float: " << t_float_ms 
              << " ms (" << (t_float_ms * 1000.0 / N) << " µs/op)" << std::endl;

    // 3. Test Parsing de Flujo de Bytes
    std::vector<uint8_t> sample_packet{0x55, 0x00, 0x09, 0x02, 0x01, 0x5F, 0x94, 0x00, 0xAA};
    std::vector<uint8_t> stream;
    stream.reserve(sample_packet.size() * (N / 10));
    for (int i = 0; i < (N / 10); ++i) {
        stream.insert(stream.end(), sample_packet.begin(), sample_packet.end());
    }

    t0 = high_resolution_clock::now();
    xgo::PacketParser parser;
    int packets_found = 0;
    for (uint8_t b : stream) {
        if (auto p = parser.parse_byte(b)) {
            packets_found++;
        }
    }
    t1 = high_resolution_clock::now();
    double t_parse_ms = duration<double, std::milli>(t1 - t0).count();
    std::cout << "  [3] Parsing RX (" << stream.size() << " bytes): " << t_parse_ms 
              << " ms (" << (t_parse_ms * 1000.0 / stream.size()) << " µs/byte)" << std::endl;

    double total_ms = t_conv_ms + t_float_ms + t_parse_ms;
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    std::cout << "  --> TIEMPO TOTAL C++: " << total_ms << " ms" << std::endl;
    std::cout << "  --> Memoria RAM pico (RSS): " << (usage.ru_maxrss / 1024.0) << " MB" << std::endl;

    return 0;
}
