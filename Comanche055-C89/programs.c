/*
 * programs.c -- P00 (CMC Idling), V37 dispatch, program stubs.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "programs.h"
#include "pinball.h"
#include "alarm.h"
#include "executive.h"
#include "agc_cpu.h"

/* ----------------------------------------------------------------
 * P00: CMC Idling
 * ---------------------------------------------------------------- */

void program_p00(void)
{
    pinball_show_prog(0);
    agc_current_program = 0;
    exec_endofjob();
}

/* ----------------------------------------------------------------
 * Stub for unimplemented programs
 * ---------------------------------------------------------------- */

void program_stub(int prognum)
{
    pinball_show_prog(prognum);
    alarm_set(00115);
}

/* ----------------------------------------------------------------
 * V37: Change program dispatch
 * ---------------------------------------------------------------- */

void program_change(int prognum)
{
    pinball_monitor_active = 0;

    switch (prognum) {
    case 0:
        pinball_show_prog(0);
        agc_current_program = 0;
        break;

    case 6:     /* P06  -- Power down */
    case 11:    /* P11  -- Earth orbit insertion monitor */
    case 20:    /* P20  -- Rendezvous navigation */
    case 21:    /* P21  -- Ground tracking */
    case 22:    /* P22  -- Orbital navigation */
    case 23:    /* P23  -- Cislunar midcourse navigation */
    case 24:    /* P24  -- Rate-aided optics tracking */
    case 25:    /* P25  -- Preferred tracking attitude */
    case 30:    /* P30  -- External delta-V */
    case 31:    /* P31  -- Lambert aim point */
    case 32:    /* P32  -- CSI */
    case 33:    /* P33  -- CDH */
    case 34:    /* P34  -- Transfer phase initiation */
    case 35:    /* P35  -- Transfer phase midcourse */
    case 37:    /* P37  -- Return to Earth */
    case 38:    /* P38  -- Stable orbit rendezvous */
    case 39:    /* P39  -- Stable orbit rendezvous */
    case 40:    /* P40  -- SPS thrusting */
    case 41:    /* P41  -- RCS thrusting */
    case 47:    /* P47  -- Thrust monitor */
    case 51:    /* P51  -- IMU orientation */
    case 52:    /* P52  -- IMU realign */
    case 53:    /* P53  -- Backup IMU orientation */
    case 61:    /* P61  -- Entry preparation */
    case 62:    /* P62  -- CM/SM separation and pre-entry */
    case 63:    /* P63  -- Entry initialization */
    case 64:    /* P64  -- Post-.05G */
    case 65:    /* P65  -- Up control */
    case 66:    /* P66  -- Ballistic */
    case 67:    /* P67  -- Final phase */
    case 72:    /* P72  -- LM CSI */
    case 73:    /* P73  -- LM CDH */
    case 74:    /* P74  -- LM transfer phase initiation */
    case 75:    /* P75  -- LM transfer phase midcourse */
    case 76:    /* P76  -- Target delta-V */
        program_stub(prognum);
        break;

    default:
        alarm_set(01520);
        break;
    }
}
