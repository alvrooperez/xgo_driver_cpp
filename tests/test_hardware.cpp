#include "xgo_driver/xgo_mini2s.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    xgo::XgoMini2s robot;
    std::cout << "Conectando a /dev/ttyAMA0 a 115200..." << std::endl;
    if (!robot.connect("/dev/ttyAMA0", 115200)) {
        std::cerr << "Error abriendo el puerto serie /dev/ttyAMA0!" << std::endl;
        return 1;
    }
    std::cout << "¡Conectado con exito a /dev/ttyAMA0!" << std::endl;

    // Leer bateria
    if (auto bat = robot.read_battery()) {
        std::cout << "Bateria: " << static_cast<int>(*bat) << "%" << std::endl;
    } else {
        std::cout << "No se pudo leer la bateria." << std::endl;
    }

    // Leer firmware
    if (auto fw = robot.read_firmware()) {
        std::cout << "Firmware: " << *fw << std::endl;
    }

    // Leer IMU durante 2 segundos
    std::cout << "Leyendo IMU..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        if (auto imu = robot.read_imu()) {
            std::cout << "IMU -> Roll: " << imu->roll 
                      << " | Pitch: " << imu->pitch 
                      << " | Yaw: " << imu->yaw << " deg" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    robot.disconnect();
    std::cout << "Test completado." << std::endl;
    return 0;
}
