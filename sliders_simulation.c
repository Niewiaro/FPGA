// sliders_fsm.c — prosta FSM sterowana "sliderami" wpisywanymi jako "10100"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// --- Typy danych ---
// Oś i faza świateł
typedef enum
{
    AXIS_NS,
    AXIS_EW
} Axis;
typedef enum
{
    PH_RED,
    PH_RED_YELLOW,
    PH_GREEN,
    PH_YELLOW
} Phase;

// Stan kontrolera
typedef struct
{
    bool powerOn;        // SW0
    bool emergencyBlink; // SW1
    Axis currentAxis;    // aktywna oś
    Phase currentPhase;  // faza aktywnej osi
} ControllerState;

typedef struct
{
    bool sw4; // zielone
    bool sw3; // żółte
    bool sw2; // czerwone
    bool sw1; // awaryjny
    bool sw0; // zasilanie
} Sliders;

/* --- Pomocnicze --- */

static const char *phase_symbol(Phase p)
{
    switch (p)
    {
    case PH_RED:
        return "R";
    case PH_RED_YELLOW:
        return "R+Y";
    case PH_GREEN:
        return "G";
    case PH_YELLOW:
        return "Y";
    default:
        return "?";
    }
}

static bool is_legal_next(Phase from, Phase to)
{
    return (from == PH_RED && to == PH_RED_YELLOW) ||
           (from == PH_RED_YELLOW && to == PH_GREEN) ||
           (from == PH_GREEN && to == PH_YELLOW) ||
           (from == PH_YELLOW && to == PH_RED);
}

static void print_lights(const ControllerState *s)
{
    if (!s->powerOn)
    {
        printf("NS: OFF | EW: OFF  [power=OFF]\n");
        return;
    }
    if (s->emergencyBlink)
    {
        printf("NS: Y~  | EW: Y~   [awaryjny]\n");
        return;
    }
    /* aktywna oś = currentPhase, druga oś = R */
    const char *ns = (s->currentAxis == AXIS_NS) ? phase_symbol(s->currentPhase) : "R";
    const char *ew = (s->currentAxis == AXIS_EW) ? phase_symbol(s->currentPhase) : "R";
    printf("NS: %s   | EW: %s\n", ns, ew);
}

/* Oczekujemy dokładnie 5 znaków '0'/'1' w kolejności SW4 SW3 SW2 SW1 SW0 */
static bool parse_sliders(const char *inputRaw, Sliders *out)
{
    char s[8] = {0};
    int j = 0;
    for (int i = 0; inputRaw[i] != '\0' && j < 5; ++i)
    {
        if (!isspace((unsigned char)inputRaw[i]))
        {
            s[j++] = inputRaw[i];
        }
    }
    if (j != 5)
        return false;
    for (int i = 0; i < 5; ++i)
    {
        if (s[i] != '0' && s[i] != '1')
            return false;
    }
    out->sw4 = (s[0] == '1');
    out->sw3 = (s[1] == '1');
    out->sw2 = (s[2] == '1');
    out->sw1 = (s[3] == '1');
    out->sw0 = (s[4] == '1');
    return true;
}

/* Z SW2..SW4 dekodujemy żądaną fazę aktywnej osi */
static bool decode_desired_phase(const Sliders *sw, Phase *desiredOut)
{
    bool r = sw->sw2, y = sw->sw3, g = sw->sw4;
    if (r && y && !g)
    {
        *desiredOut = PH_RED_YELLOW;
        return true;
    }
    if (g && !r && !y)
    {
        *desiredOut = PH_GREEN;
        return true;
    }
    if (y && !r && !g)
    {
        *desiredOut = PH_YELLOW;
        return true;
    }
    if (r && !y && !g)
    {
        *desiredOut = PH_RED;
        return true;
    }
    return false; /* inne kombinacje ignorujemy */
}

int main(void)
{
    printf("Start\n");

    ControllerState st;
    st.powerOn = false;
    st.emergencyBlink = false;
    st.currentAxis = AXIS_NS;
    st.currentPhase = PH_RED;

    printf("Wejscie: 5 znakow 0/1 (SW4 SW3 SW2 SW1 SW0), np. 10100.\n");
    printf("SW0=power, SW1=awaria, SW2=R, SW3=Y, SW4=G; R+Y = SW2+SW3.\n");
    printf("Komendy: wpisuj np. 00101 (R + power ON), 'out' aby wydrukowac, 'quit' aby wyjsc.\n");

    char line[128];
    while (1)
    {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
            break;

        /* trim newline */
        size_t n = strlen(line);
        if (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = '\0';

        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0)
            break;
        if (strcmp(line, "out") == 0)
        {
            print_lights(&st);
            continue;
        }

        Sliders sw = {0};
        if (!parse_sliders(line, &sw))
        {
            printf("Podaj dokladnie 5 znakow 0/1 (SW4 SW3 SW2 SW1 SW0), np. 10100\n");
            continue;
        }

        /* Zasilanie (SW0) */
        if (!sw.sw0)
        {
            /* power OFF = reset do stanu początkowego */
            st.powerOn = false;
            st.emergencyBlink = false;
            st.currentAxis = AXIS_NS;
            st.currentPhase = PH_RED;
            printf("[POWER OFF] Reset do NS=R, EW=R.\n");
            print_lights(&st);
            continue;
        }
        if (!st.powerOn && sw.sw0)
        {
            st.powerOn = true;
            st.currentAxis = AXIS_NS;
            st.currentPhase = PH_RED;
            printf("[POWER ON] Start: NS=R, EW=R.\n");
        }

        /* Tryb awaryjny */
        st.emergencyBlink = sw.sw1;

        /* Żądana faza z SW2..SW4 */
        Phase desired;
        if (!decode_desired_phase(&sw, &desired))
        {
            printf("[INFO] Ustaw R (00101), Y (01001), G (10001) lub R+Y (01101).\n");
            print_lights(&st);
            continue;
        }

        /* Walidacja przejścia */
        if (!is_legal_next(st.currentPhase, desired))
        {
            printf("[BLAD] Nie mozna przejsc z %s do %s (oś %s).\n",
                   phase_symbol(st.currentPhase),
                   phase_symbol(desired),
                   (st.currentAxis == AXIS_NS ? "NS" : "EW"));
            print_lights(&st);
            continue;
        }

        /* Wykonanie przejścia; na Y->R zmieniamy oś */
        if (st.currentPhase == PH_YELLOW && desired == PH_RED)
        {
            st.currentPhase = PH_RED;
            st.currentAxis = (st.currentAxis == AXIS_NS) ? AXIS_EW : AXIS_NS;
        }
        else
        {
            st.currentPhase = desired;
        }

        printf("[OK] Przejscie: %s %s\n",
               (st.currentAxis == AXIS_NS ? "NS" : "EW"),
               phase_symbol(st.currentPhase));

        print_lights(&st);
    }

    printf("Koniec.\n");
    return 0;
}
