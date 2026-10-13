# xgo_driver_cpp

Modern C++20 ROS 2 driver and communication library for the **XGO Mini 2S** quadruped robot dog.

## 🚀 Key Features

- **Modern C++20**: Strict typing, `std::span`, `std::optional`, `std::chrono`, and RAII resource management.
- **Robust Serial Communication**: Linux POSIX UART (`termios`) 8N1 raw mode with hardware timeout and rate limiting.
- **State Machine Packet Parser (FSM)**: Noise-tolerant streaming decoder with checksum verification.
- **ROS 2 Native & Standalone**: Fully compatible with ROS 2 Jazzy (`ament_cmake` / `colcon`) and standalone modern CMake.
- **High Performance**: Up to **340x faster** data unpacking and **20x faster** overall execution than the original Python library.

---

## 📊 Benchmark: C++20 vs Original Python (1,000,000 Operations)

| Operation | Python (`xgolib_dog.py`) | C++20 (`xgo_driver`) | Speedup |
| :--- | :--- | :--- | :--- |
| **Physical Mapping + TX Packet Creation** | 440.74 ms (0.441 µs/op) | 30.06 ms (0.030 µs/op) | **14.6x faster** |
| **Float Unpacking (`Byte2float`)** | 285.68 ms (0.286 µs/op) | 0.84 ms (0.0008 µs/op) | **340x faster** |
| **UART RX Stream Parsing (900,000 bytes)** | 54.29 ms (0.060 µs/byte) | 7.17 ms (0.008 µs/byte) | **7.6x faster** |
| **TOTAL RUNTIME** | **780.71 ms** | **38.08 ms** | **20.5x faster** |

---

## 📁 Repository Structure

```text
xgo_driver_cpp/
├── CMakeLists.txt          # ament_cmake build configuration
├── package.xml             # ROS 2 package manifest
├── README.md               # Documentation & Benchmarks
├── .gitignore
├── include/
│   └── xgo_driver/
│       ├── xgo_mini2s.hpp  # Main driver & Protocol interface
│       └── serial.hpp      # Linux POSIX serial port wrapper (RAII)
├── src/
│   ├── xgo_mini2s.cpp      # Driver implementation
│   ├── serial.cpp          # Serial port implementation
│   └── xgo_driver_node.cpp # ROS 2 node executable (in progress)
├── tests/
│   └── test_driver.cpp     # Unit test suite (11 assertions)
└── benchmark/
    ├── benchmark.cpp       # C++20 benchmark
    └── benchmark.py        # Python benchmark
```

---

## 🛠️ Build & Test

### With ROS 2 (`colcon`):
```bash
# In your ROS 2 workspace (e.g. ~/ros2_ws)
colcon build --packages-select xgo_driver_cpp
source install/setup.bash
```

### Standalone (CMake):
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run test suite
./test_driver
```
