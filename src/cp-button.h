#ifndef __CP_BUTTON_H__
#define __CP_BUTTON_H__

extern int *cp_old_map, *cp_last_button;

void cp_button_start (void);
void cp_button_motion (int *map);
void cp_button_down (int *map);
int cp_button_up (int *map);

#endif /* __CP_BUTTON_H__ */

