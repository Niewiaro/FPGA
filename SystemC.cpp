#include <systemc.h>
#include <iostream>
#include <iomanip>

// --- Definicje takie same jak dla FPGA ---
// https://github.com/Niewiaro/FPGA/blob/main/sygnalizacja_interupt_warrning.c
enum Phase
{
  PH_RED,
  PH_RED_YELLOW,
  PH_GREEN,
  PH_YELLOW,
  PH_RED_GREEN,
  PH_NONO
};

// PHASE to string (do wypisywania)
const char *phaseToString(Phase p)
{
  switch (p)
  {
  case PH_RED:
    return "RED";
  case PH_RED_YELLOW:
    return "RED_YELLOW";
  case PH_GREEN:
    return "GREEN";
  case PH_YELLOW:
    return "YELLOW";
  case PH_RED_GREEN:
    return "RED_GREEN";
  default:
    return "UNKNOWN/NONO";
  }
}

// Globalne sygnały
sc_signal<bool> s_pwr, s_emg, s_r, s_y, s_g;

void set_sliders(bool r, bool y, bool g)
{
  s_r = r;
  s_y = y;
  s_g = g;
}

void print_status(sc_signal<Phase> &out_phase, sc_signal<bool> &out_emg, std::string comment)
{
  std::cout << std::setw(5) << sc_time_stamp() << " | "
            << s_pwr << " | "
            << out_emg << " | "
            << s_r << "," << s_y << "," << s_g << " | "
            << std::setw(12) << phaseToString(out_phase.read()) << " | "
            << comment << std::endl;
}

// --- Moduł Kontrolera Sygnalizacji ---
SC_MODULE(TrafficLightController)
{
  // PORTY WEJŚCIOWE (SLIDERS)
  sc_in<bool> clk;          // Zegar systemowy
  sc_in<bool> sw_power;     // SW0 (Power)
  sc_in<bool> sw_emergency; // SW1 (Emergency)
  sc_in<bool> sw_red;       // SW2 (RED)
  sc_in<bool> sw_yellow;    // SW3 (YELLOW)
  sc_in<bool> sw_green;     // SW4 (GREEN)

  // PORTY WYJŚCIOWE (LEDS oraz HEX (tu string))
  sc_out<Phase> current_phase_out;
  sc_out<bool> led_power_out;
  sc_out<bool> led_emergency_out;

  Phase currentPhase;

  Phase decode_desired_phase(int r, int y, int g)
  {
    if (r && y && !g)
      return PH_RED_YELLOW;
    if (g && !r && !y)
      return PH_GREEN;
    if (y && !r && !g)
      return PH_YELLOW;
    if (r && !y && !g)
      return PH_RED;
    if (r && !y && g)
      return PH_RED_GREEN;
    return PH_NONO;
  }

  int is_legal_next(Phase from, Phase to)
  {
    return (from == PH_RED && to == PH_RED_YELLOW) ||
           (from == PH_RED_YELLOW && to == PH_GREEN) ||
           (from == PH_GREEN && to == PH_YELLOW) ||
           (from == PH_YELLOW && to == PH_RED) ||
           (from == PH_RED && to == PH_RED_GREEN) ||
           (from == PH_RED_GREEN && to == PH_RED);
  }

  void main_logic(void)
  {
    bool pwr = sw_power.read();
    bool emg = sw_emergency.read();
    bool r = sw_red.read();
    bool y = sw_yellow.read();
    bool g = sw_green.read();

    if (pwr)
    {
      led_power_out.write(true);
    }
    else
    {
      led_power_out.write(false);
      currentPhase = PH_RED;
      current_phase_out.write(currentPhase);
      return;
    }

    if (emg)
    {
      led_emergency_out.write(true);
      currentPhase = PH_RED;
      current_phase_out.write(currentPhase);
      return;
    }
    else
    {
      led_emergency_out.write(false);
    }

    Phase desired = decode_desired_phase(r, y, g);

    if (desired != PH_NONO && is_legal_next(currentPhase, desired))
    {
      currentPhase = desired;
    }

    current_phase_out.write(currentPhase);
  }

  SC_CTOR(TrafficLightController)
  {
    SC_METHOD(main_logic);
    sensitive << clk.pos();

    currentPhase = PH_GREEN;
  }
};

int sc_main(int argc, char *argv[])
{
  sc_clock clk("clk", 1, SC_SEC);
  sc_signal<bool> out_pwr, out_emg;
  sc_signal<Phase> out_phase;

  TrafficLightController dut("TrafficController"); // DUT - Device Under Test
  dut.clk(clk);
  dut.sw_power(s_pwr);
  dut.sw_emergency(s_emg);
  dut.sw_red(s_r);
  dut.sw_yellow(s_y);
  dut.sw_green(s_g);
  dut.led_power_out(out_pwr);
  dut.led_emergency_out(out_emg);
  dut.current_phase_out(out_phase);

  // --- NAGŁÓWEK TABELI ---
  std::cout << "TIME  | P | E | R,Y,G | CURRENT PH   | KOMENTARZ" << std::endl;
  std::cout << "------+---+---+-------+--------------+--------------------------" << std::endl;

  // 0. Start - brak zasilania
  s_pwr = 0;
  s_emg = 0;
  set_sliders(0, 0, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Start systemu (OFF)");

  // 1. Włączenie zasilania (Startujemy od RED)
  s_pwr = 1;
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Power ON -> RED");

  // --- PEŁNY CYKL STANDARDOWY ---

  // Krok A: RED -> RED_YELLOW
  set_sliders(1, 1, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Zmiana na RED_YELLOW");

  // Krok B: RED_YELLOW -> GREEN
  set_sliders(0, 0, 1);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Zmiana na GREEN");

  // Krok C: GREEN -> YELLOW
  set_sliders(0, 1, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Zmiana na YELLOW");

  // Krok D: YELLOW -> RED
  set_sliders(1, 0, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Powrót do RED");

  // Krok E: RED -> RED_GREEN
  set_sliders(1, 0, 1); // strzałka warunkowa
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Warunkowy RED_GREEN");

  // Krok F: RED_GREEN -> RED
  set_sliders(1, 0, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Powrót do RED");

  // --- PRZYKŁADY SPECJALNE / BŁĘDY ---

  // 1. Ten sam stan (RED -> RED)
  set_sliders(1, 0, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Brak zmian (nadal RED)");

  // 2. Nielegalna kombinacja suwaków (np. wszystkie naraz)
  set_sliders(1, 1, 1);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Nielegalna kombinacja suwaków (nadal RED)");

  // 3. Nielegalne przejście (RED -> GREEN z pominięciem RY)
  set_sliders(0, 0, 1);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Nielegalne RED->GREEN (nadal RED)");

  // --- STAN AWARYJNY ---

  // 1. Włączenie awarii (Emergency)
  s_emg = 1;
  set_sliders(0, 0, 1);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "AWARIA! Reset do RED");

  // 2. Próba zmiany świateł podczas awarii
  set_sliders(0, 1, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Awaria trwa (zablokowane)");

  // 3. Wyłączenie awarii (Powrót do normalności)
  s_emg = 0;
  set_sliders(1, 0, 0);
  sc_start(1, SC_SEC);
  print_status(out_phase, out_emg, "Koniec awarii (RED po awarii)");

  return 0;
}
