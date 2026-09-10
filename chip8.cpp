#include "chip8.h"
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

//note: dont add using namespace std,is unoptizimie asf 

static constexpr std::uint8_t font[chip8::font_count * chip8::font_byte] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

chip8::chip8() { chip8_reset(); }

void chip8::chip8_reset() {
  memory.fill(0);
  v.fill(0);
  i = 0;
  pc = prog_start;
  stack.fill(0);
  opcode = 0;
  sp = 0;
  keypad.fill(false);
  delaytimer = 0;
  soundtimer = 0;
  drawflag = true;
  halted = false;

  std::copy(std::begin(font), std::end(font), memory.begin() + font_addr);
}

bool chip8::loadrom(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) return false;

  std::streamsize size = file.tellg();
  if (size > static_cast<std::streamsize>(mem_size - prog_start)) return false; //bradar file is too big for my ass 

  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char*>(memory.data() + prog_start), size);
  return true;
}

void chip8::step() {
  if (halted) return;

  if (pc + 1 >= static_cast<std::uint16_t>(mem_size)) { halted = true; return; }

  opcode = (static_cast<std::uint16_t>(memory[pc]) << 8) | memory[pc + 1];
  pc += 2;

//decode
  std::uint8_t first = (opcode & 0xF000) >> 12; //top 4 bit
  std::uint8_t x = (opcode & 0x0F00) >> 8; //vx
  std::uint8_t y = (opcode & 0x00F0) >> 4; //vy
  std::uint8_t n = opcode & 0x000F; //low 4 bit
  std::uint8_t kk = opcode & 0x00FF; //kk mean kawkaw
  std::uint16_t nnn = opcode & 0x0FFF; //low 12 bit
  
  //exec (hell)
  switch (first) {
case 0x0: {
            std::uint8_t low = opcode & 0x00FF;
            if (low == 0xE0) {
              display.fill(0);
              drawflag = true; //cls screen btw
            } else if (low == 0xEE) {
              if (sp == 0) { halted = true; break; }
              pc = stack[--sp];
            } else {
              halted = true; //lazy
            }
            break;
    }
    case 0x1: pc = nnn; break; //jump
    case 0x2: {
      if (sp >= stack_size) { halted = true; break; }
      stack[sp++] = pc;
      pc = nnn;
      break;
    }
    case 0x3: if (v[x] == kk) pc += 2; break; //se 
    case 0x4: if (v[x] != kk) pc += 2; break; //sne 
    case 0x5: if (v[x] == v[y]) pc += 2; break; //se vx, vy
    case 0x6: v[x] = kk; break; //ld
    case 0x7: v[x] += kk; break; //add,the most important shit ever yay
    case 0x8: {
      switch (n) {
        case 0x0: v[x] = v[y]; break;
        case 0x1: v[x] |= v[y]; break;
        case 0x2: v[x] &= v[y]; break;
        case 0x3: v[x] ^= v[y]; break;
        case 0x4: {
          std::uint16_t sum = v[x] + v[y];
          v[0xF] = (sum > 0xFF);
          v[x] = static_cast<std::uint8_t>(sum);
          break;
        }
        case 0x5: {
          v[0xF] = (v[x] >= v[y]);
          v[x] -= v[y];
          break;
        }
        case 0x6: {
          v[0xF] = v[x] & 0x1;
          v[x] >>= 1;
          break;
        }
        case 0x7: {
          v[0xF] = (v[y] >= v[x]);
          v[x] = v[y] - v[x];
          break;
        }
        case 0xE: {
          v[0xF] = (v[x] & 0x80) >> 7;
          v[x] <<= 1;
          break;
        }
        default: halted = true;
      }
      break;
    }
    case 0x9: if (v[x] != v[y]) pc += 2; break;
    case 0xA: i = nnn; break;
    case 0xB: pc = v[0] + nnn; break;
    case 0xC: v[x] = static_cast<std::uint8_t>(std::rand()) & kk; break; // rnd

    case 0xD: { // DXYN,idk what this is
      std::uint8_t height = n;
      v[0xF] = 0;

      for (std::uint8_t row = 0; row < height; ++row) {
        std::uint8_t spriteByte = memory[i + row];
        for (std::uint8_t bit = 0; bit < 8; ++bit) {
          if (spriteByte & (0x80 >> bit)) {
            std::size_t px = (v[x] + bit) % screen_w;
            std::size_t py = (v[y] + row) % screen_h;
            std::size_t idx = py * screen_w + px;
            if (display[idx]) v[0xF] = 1;
            display[idx] ^= 1;
          }
        }
      }
      drawflag = true;
      break;
    }

    case 0xE: {
      if (kk == 0x9E) { // EX9E
        if (keypad[v[x]]) pc += 2;
      } else if (kk == 0xA1) { // EXA1
        if (!keypad[v[x]]) pc += 2;
      } else {
        halted = true;
      }
      break;
    }

    case 0xF: {
      switch (kk) {
        case 0x07: // FX07
          v[x] = delaytimer;
          break;
        case 0x0A: { // FX0A
          bool pressed = false;
          for (std::size_t k = 0; k < 16; ++k) {
            if (keypad[k]) { v[x] = static_cast<std::uint8_t>(k); pressed = true; break; }
          }
          if (!pressed) pc -= 2; // re-fetch same opcode
          break;
        }
        case 0x15: // FX15
          delaytimer = v[x];
          break;
        case 0x18: // FX18
          soundtimer = v[x];
          break;
        case 0x1E: // i forgot what this is 
          i += v[x];
          break;
        case 0x29: // FX29 (font sprite)
          i = font_addr + v[x] * font_byte;
          break;
        case 0x33: { // FX33 (BCD)
          std::uint8_t val = v[x];
          memory[i]     = val / 100;
          memory[i + 1] = (val / 10) % 10;
          memory[i + 2] = val % 10;
          break;
        }
        case 0x55: // FX55
          for (std::uint8_t reg = 0; reg <= x; ++reg)
            memory[i + reg] = v[reg];
          break;
        case 0x65: // FX65
          for (std::uint8_t reg = 0; reg <= x; ++reg)
            v[reg] = memory[i + reg];
          break;
        default:
          halted = true;
      }
      break;
    }

    default:
      halted = true; //sad nyon 
  }
}

void chip8::ticktimer() {
  if (delaytimer > 0) {
    --delaytimer;
  }
  if (soundtimer > 0) {
    --soundtimer; //DO NOT TYOE WITHOUT SHIFT 
  }
}
