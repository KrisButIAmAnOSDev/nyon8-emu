#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

struct chip8 {
  static constexpr std::size_t mem_size = 4096; // idk but i think size t is nyoner than int
  static constexpr std::size_t screen_w = 64;
  static constexpr std::size_t screen_h = 32;
  static constexpr std::size_t stack_size = 16;
  static constexpr std::size_t font_count = 16;
  static constexpr std::size_t font_byte = 5;

  static constexpr std::uint16_t prog_start = 0x200; // unit16t for ardess ig idk
  static constexpr std::uint16_t font_addr = 0x000;
  
  chip8();
  void chip8_reset();
  bool loadrom(const std::string& path);
  void step(); //note: hold shift pls,() not 90
  void ticktimer();

  bool drawflag = true;

  std::array<std::uint8_t, screen_w * screen_h> display{};

  std::array<std::uint8_t, 16> v{};
  std::array<std::uint8_t, mem_size> memory{};

  std::uint16_t i = 0;
  std::uint16_t pc = prog_start;
  std::uint16_t opcode = 0;

  std::array<std::uint16_t, stack_size> stack{};
  std::uint8_t sp = 0;

  std::array<bool, 16> keypad{};

  std::uint8_t delaytimer = 0;
  std::uint8_t soundtimer = 0;

  bool halted = false; //dont hold shift pls
};
