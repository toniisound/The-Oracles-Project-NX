/* The Switch's CPU at 1785 MHz while the launcher runs, the clock the console itself takes while it loads: the core,
 * the neighbours' workers and the drawing share three cores at 1020 MHz otherwise.  Put back as it was at the end.
 * Through clkrst (firmware 8.0.0 and later); without it, the clock stays as the system has it.  A file of its own:
 * <switch.h> names Thread and Mutex as mGBA's headers do. */
#include <switch.h>

void oracles_switch_step(const char *what);   /* main.c */

static ClkrstSession cpu_clock;
static u32 cpu_clock_before;
static int session_open, clock_set;

void oracles_switch_overclock(void)
{
    oracles_switch_step("overclock: CPU to 1785 MHz");
    if (!hosversionAtLeast(8, 0, 0) || R_FAILED(clkrstInitialize())) { oracles_switch_step("overclock: no clkrst, clock unchanged"); return; }
    if (R_FAILED(clkrstOpenSession(&cpu_clock, PcvModuleId_CpuBus, 3))) {
        clkrstExit();
        oracles_switch_step("overclock: no CPU session, clock unchanged");
        return;
    }
    session_open = 1;
    if (R_FAILED(clkrstGetClockRate(&cpu_clock, &cpu_clock_before))) cpu_clock_before = 1020000000;
    clock_set = R_SUCCEEDED(clkrstSetClockRate(&cpu_clock, 1785000000));
    oracles_switch_step(clock_set ? "overclock: done" : "overclock: refused, clock unchanged");
}

void oracles_switch_clock_back(void)
{
    if (!session_open) return;
    if (clock_set) clkrstSetClockRate(&cpu_clock, cpu_clock_before);
    clkrstCloseSession(&cpu_clock);
    clkrstExit();
    session_open = clock_set = 0;
}
