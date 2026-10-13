#include "xgo_driver/xgo_mini2s.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <array>

// Función auxiliar para comparar doubles con tolerancia
bool approx_equal(double a, double b, double epsilon = 1e-2) {
    return std::abs(a - b) <= epsilon;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      TEST SUITE: XGO Conversiones      " << std::endl;
    std::cout << "========================================" << std::endl;

    // --- TEST 1: conver2u8 ---
    std::cout << "\n[1] Probando xgo::conver2u8..." << std::endl;
    
    // Rango simétrico: velocidad de -25.0 a 25.0
    double min_vel = -25.0;
    double max_vel =  25.0;

    uint8_t byte_min = xgo::conver2u8(-25.0, min_vel, max_vel);
    uint8_t byte_mid = xgo::conver2u8(  0.0, min_vel, max_vel);
    uint8_t byte_max = xgo::conver2u8( 25.0, min_vel, max_vel);

    std::cout << "  -25.0 -> " << static_cast<int>(byte_min) << " (Esperado: 0)" << std::endl;
    std::cout << "    0.0 -> " << static_cast<int>(byte_mid) << " (Esperado: 128)" << std::endl;
    std::cout << "   25.0 -> " << static_cast<int>(byte_max) << " (Esperado: 255)" << std::endl;

    assert(byte_min == 0);
    assert(byte_mid >= 127 && byte_mid <= 128);
    assert(byte_max == 255);

    // Test de saturación / clamp
    uint8_t byte_over  = xgo::conver2u8( 100.0, min_vel, max_vel);
    uint8_t byte_under = xgo::conver2u8(-100.0, min_vel, max_vel);
    std::cout << "  100.0 (Saturacion max) -> " << static_cast<int>(byte_over) << " (Esperado: 255)" << std::endl;
    std::cout << " -100.0 (Saturacion min) -> " << static_cast<int>(byte_under) << " (Esperado: 0)" << std::endl;
    assert(byte_over == 255);
    assert(byte_under == 0);

    // --- TEST 2: conver2double ---
    std::cout << "\n[2] Probando xgo::conver2double..." << std::endl;
    double val_0   = xgo::conver2double(0,   min_vel, max_vel);
    double val_128 = xgo::conver2double(128, min_vel, max_vel);
    double val_255 = xgo::conver2double(255, min_vel, max_vel);

    std::cout << "  Byte 0   -> " << val_0   << " (Esperado: -25.0)" << std::endl;
    std::cout << "  Byte 128 -> " << val_128 << " (Esperado: ~0.0)" << std::endl;
    std::cout << "  Byte 255 -> " << val_255 << " (Esperado: 25.0)" << std::endl;

    assert(approx_equal(val_0, -25.0));
    assert(approx_equal(val_128, 0.0, 0.5));
    assert(approx_equal(val_255, 25.0));

    // --- TEST 3: Ida y vuelta (Round-trip) ---
    std::cout << "\n[3] Probando Ida y Vuelta (Round-trip)..." << std::endl;
    double original = 12.5;
    uint8_t byte = xgo::conver2u8(original, min_vel, max_vel);
    double recovered = xgo::conver2double(byte, min_vel, max_vel);
    std::cout << "  Original: " << original 
              << " -> Byte: " << static_cast<int>(byte) 
              << " -> Recuperado: " << recovered << std::endl;

    assert(approx_equal(original, recovered, 0.5));

    // --- TEST 4: byte2short ---
    std::cout << "\n[4] Probando xgo::byte2short..." << std::endl;
    // Prueba valor positivo: 0x012C = 300
    int16_t s_pos = xgo::byte2short(0x01, 0x2C);
    std::cout << "  (0x01, 0x2C) -> " << s_pos << " (Esperado: 300)" << std::endl;
    assert(s_pos == 300);

    // Prueba valor negativo: 0xFFFF = -1
    int16_t s_neg = xgo::byte2short(0xFF, 0xFF);
    std::cout << "  (0xFF, 0xFF) -> " << s_neg << " (Esperado: -1)" << std::endl;
    assert(s_neg == -1);

    // Prueba valor negativo grande: 0xFC18 = -1000
    int16_t s_neg2 = xgo::byte2short(0xFC, 0x18);
    std::cout << "  (0xFC, 0x18) -> " << s_neg2 << " (Esperado: -1000)" << std::endl;
    assert(s_neg2 == -1000);

    // --- TEST 5: Byte2float ---
    std::cout << "\n[5] Probando xgo::Byte2float..." << std::endl;
    // En little-endian, el float 1.0f en IEEE 754 es: 0x3F800000 -> bytes {0x00, 0x00, 0x80, 0x3F}
    std::array<uint8_t, 4> float_bytes_1{0x00, 0x00, 0x80, 0x3F};
    float f1 = xgo::Byte2float(float_bytes_1);
    std::cout << "  Bytes de 1.0f -> " << f1 << " (Esperado: 1.0)" << std::endl;
    assert(approx_equal(f1, 1.0f));

    // Test con -12.5f: en IEEE 754 es 0xC1480000 -> {0x00, 0x00, 0x48, 0xC1}
    std::array<uint8_t, 4> float_bytes_neg{0x00, 0x00, 0x48, 0xC1};
    float f_neg = xgo::Byte2float(float_bytes_neg);
    std::cout << "  Bytes de -12.5f -> " << f_neg << " (Esperado: -12.5)" << std::endl;
    assert(approx_equal(f_neg, -12.5f));

    // --- TEST 6: calculate_checksum ---
    std::cout << "\n[6] Probando xgo::calculate_checksum con tramas reales..." << std::endl;
    // Caso 1: Comando de velocidad VX (0x30) a 128 (0x80) -> [LEN=0x09, MODE=0x00, ADDR=0x30, VAL=0x80]
    std::array<uint8_t, 4> tx_vx{0x09, 0x00, 0x30, 0x80};
    uint8_t cs_vx = xgo::calculate_checksum(tx_vx);
    std::cout << "  Trama VX (0x09, 0x00, 0x30, 0x80) -> Checksum: 0x" 
              << std::hex << static_cast<int>(cs_vx) << std::dec << " (Esperado: 0x46 / 70)" << std::endl;
    assert(cs_vx == 0x46);

    // Caso 2: Comando de lectura de bateria (ADDR=0x01, READ_LEN=0x01) -> [LEN=0x09, MODE=0x02, ADDR=0x01, LEN=0x01]
    std::array<uint8_t, 4> tx_read_bat{0x09, 0x02, 0x01, 0x01};
    uint8_t cs_bat = xgo::calculate_checksum(tx_read_bat);
    std::cout << "  Trama Read Bateria -> Checksum: 0x" 
              << std::hex << static_cast<int>(cs_bat) << std::dec << " (Esperado: 0xf2 / 242)" << std::endl;
    assert(cs_bat == 0xF2);

    // Caso 3: Comando multibyte (Translacion X, Y, Z = [10, 20, 30]) -> LEN=11 (0x0B), MODE=0x00, ADDR=0x33, DATA=[10, 20, 30]
    std::array<uint8_t, 6> tx_multi{0x0B, 0x00, 0x33, 10, 20, 30};
    uint8_t cs_multi = xgo::calculate_checksum(tx_multi);
    std::cout << "  Trama Multibyte -> Checksum: 0x" 
              << std::hex << static_cast<int>(cs_multi) << std::dec << " (Esperado: 0x85 / 133)" << std::endl;
    assert(cs_multi == 0x85);

    // --- TEST 7: create_write_packet ---
    std::cout << "\n[7] Probando xgo::create_write_packet..." << std::endl;
    std::array<uint8_t, 1> vel_payload{0x80};
    auto pkt_write = xgo::create_write_packet(Register::VX, vel_payload);
    std::vector<uint8_t> expected_write{0x55, 0x00, 0x09, 0x00, 0x30, 0x80, 0x46, 0x00, 0xAA};
    assert(pkt_write == expected_write);
    std::cout << "  Paquete Write VX generado correctamente (9 bytes) -> OK" << std::endl;

    // --- TEST 8: create_read_packet ---
    std::cout << "\n[8] Probando xgo::create_read_packet..." << std::endl;
    // Bateria (1 byte)
    auto pkt_read_bat = xgo::create_read_packet(Register::BATTERY, 1);
    std::vector<uint8_t> expected_read_bat{0x55, 0x00, 0x09, 0x02, 0x01, 0x01, 0xF2, 0x00, 0xAA};
    assert(pkt_read_bat == expected_read_bat);
    std::cout << "  Paquete Read Bateria generado correctamente (9 bytes) -> OK" << std::endl;

    // Lectura de 15 motores a la vez (read_len = 15)
    auto pkt_read_motors = xgo::create_read_packet(Register::MOTOR_ANGLE, 15);
    std::vector<uint8_t> expected_read_motors{0x55, 0x00, 0x09, 0x02, 0x50, 0x0F, 0x95, 0x00, 0xAA};
    assert(pkt_read_motors == expected_read_motors);
    std::cout << "  Paquete Read 15 Motores generado correctamente (9 bytes fijos) -> OK" << std::endl;

    // --- TEST 9: PacketParser (Respuesta de Bateria valida) ---
    std::cout << "\n[9] Probando xgo::PacketParser con respuesta de bateria..." << std::endl;
    xgo::PacketParser parser;
    // Trama simulada del perro: Bateria = 95% (0x5F)
    // Checksum = 255 - (9 + 2 + 1 + 95) = 255 - 107 = 148 (0x94)
    std::vector<uint8_t> rx_battery_stream{0x55, 0x00, 0x09, 0x02, 0x01, 0x5F, 0x94, 0x00, 0xAA};

    std::optional<xgo::RxPacket> parsed_packet;
    for (size_t i = 0; i < rx_battery_stream.size(); ++i) {
        auto res = parser.parse_byte(rx_battery_stream[i]);
        if (i < rx_battery_stream.size() - 1) {
            // Mientras no llegue el ultimo byte, debe devolver nullopt
            assert(!res.has_value());
        } else {
            // En el ultimo byte (0xAA), debe completar el paquete
            assert(res.has_value());
            parsed_packet = res;
        }
    }

    assert(parsed_packet.has_value());
    assert(parsed_packet->reg == Register::BATTERY);
    assert(parsed_packet->payload.size() == 1);
    assert(parsed_packet->payload[0] == 95);
    std::cout << "  Paquete de Bateria parseado con exito: " << static_cast<int>(parsed_packet->payload[0]) << "% -> OK" << std::endl;

    // --- TEST 10: PacketParser tolerante a ruido en el cable ---
    std::cout << "\n[10] Probando xgo::PacketParser tolerante a ruido..." << std::endl;
    // Metemos bytes basura antes de la cabecera real
    std::vector<uint8_t> noisy_stream{0xFF, 0x00, 0x12, 0x55, 0x55, 0x00, 0x09, 0x02, 0x01, 0x5F, 0x94, 0x00, 0xAA};
    std::optional<xgo::RxPacket> noisy_packet;
    for (uint8_t b : noisy_stream) {
        if (auto res = parser.parse_byte(b)) {
            noisy_packet = res;
        }
    }
    assert(noisy_packet.has_value());
    assert(noisy_packet->reg == Register::BATTERY);
    std::cout << "  Parser ignoro el ruido previo y leyo el paquete correctamente -> OK" << std::endl;

    // --- TEST 11: XgoMini2s (Ciclo de vida y seguridad) ---
    std::cout << "\n[11] Probando xgo::XgoMini2s (Comportamiento defensivo)..." << std::endl;
    xgo::XgoMini2s dog;
    // 1. Debe arrancar desconectado
    assert(!dog.is_connected());
    std::cout << "  Estado inicial desconectado -> OK" << std::endl;

    // 2. Comandos de movimiento rechazados limpiamente si no hay conexion
    assert(!dog.set_velocity(0.15, 0.0, 0.0));
    assert(!dog.stop());
    assert(!dog.reset());
    std::cout << "  Comandos de movimiento rechazados sin conexion -> OK" << std::endl;

    // 3. Consultas de telemetria devuelven nullopt sin colgar el hilo
    assert(!dog.read_battery().has_value());
    assert(!dog.read_firmware().has_value());
    assert(!dog.read_imu().has_value());
    std::cout << "  Consultas de telemetria devuelven nullopt sin conexion -> OK" << std::endl;

    // 4. Intentar conectar a puerto inexistente falla de forma segura
    assert(!dog.connect("/dev/puerto_inexistente_xgo_12345"));
    assert(!dog.is_connected());
    std::cout << "  Fallo controlado ante puerto inexistente -> OK" << std::endl;

    // 5. Desconexion defensiva
    dog.disconnect();
    assert(!dog.is_connected());
    std::cout << "  Desconexion segura sin excepciones -> OK" << std::endl;

    std::cout << "\n>>> TODOS LOS TESTS PASARON CON EXITO <<<" << std::endl;
    return 0;
}
