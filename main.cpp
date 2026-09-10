#include "chip8.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <ctime>

using namespace std::chrono_literals;

struct Term {
    termios old{};
    Term() {
        tcgetattr(STDIN_FILENO, &old);
        termios raw = old;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    }
    ~Term() { tcsetattr(STDIN_FILENO, TCSANOW, &old); }
};

static const int KEYMAP[16] = {
    '1','2','3','4',
    'q','w','e','r',
    'a','s','d','f',
    'z','x','c','v'
};

int main (int argc, char *argv[]) {
  if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom.ch8>\n";
        return 1;
    }

    chip8 cpu;
    if (!cpu.loadrom(argv[1])) {
        std::cerr << "Failed to load ROM,dont ask me why,ask ralsei\n";
        return 1;
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    Term term; //kawkaw ref ig 

    constexpr auto CYCLE_NS = 1s / 700;
    constexpr auto TIMER_NS = 1s / 60;
    constexpr auto FRAME_NS = 1s / 60;

    auto next_cycle = std::chrono::steady_clock::now();
    auto next_timer = next_cycle;
    auto next_frame = next_cycle;

    while (!cpu.halted) {
        auto now = std::chrono::steady_clock::now();

        if (now >= next_cycle) {
            cpu.step();
            next_cycle += CYCLE_NS;
        }
        if (now >= next_timer) {
            cpu.ticktimer();
            next_timer += TIMER_NS;
        }
        if (now >= next_frame) {
            if (cpu.drawflag) {
                std::cout << "\033[H";
                for (std::size_t row = 0; row < chip8::screen_h; ++row) {
                    for (std::size_t col = 0; col < chip8::screen_w; ++col) {
                        std::size_t idx = row * chip8::screen_w + col;
                        std::cout << (cpu.display[idx] ? "██" : "  ");
                    }
                    std::cout << '\n';
                }
                std::cout.flush();
                cpu.drawflag = false;
            }
            next_frame += FRAME_NS;
        }

        // keyboard poll
        char ch;
        while (read(STDIN_FILENO, &ch, 1) == 1) {
            for (int k = 0; k < 16; ++k) {
                if (ch == KEYMAP[k] || ch == KEYMAP[k] - 32) {
                    cpu.keypad[k] = true;
                }
            }
            if (ch == 27) { cpu.halted = true; break; } //esc 
        }
        for (int k = 0; k < 16; ++k) cpu.keypad[k] = false;
        
        std::this_thread::sleep_for(100us);
    }

    if (cpu.halted) {
        std::cerr << "\nCPU halted. Last opcode: 0x" << std::hex << cpu.opcode << std::dec << "\n";
    }
  return 0;
}

