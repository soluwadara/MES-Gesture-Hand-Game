#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

enum state {

    START,
    ePICKLEVEL,
    eLEVEL1,
    eLEVEL2,
    eLEVEL3,
    END
};

void next_state(enum state nextState);
void select_level_workq_submit();
void select_level_workq();
void state_machine_func();
extern void wake_state_machine_thd();

#endif
