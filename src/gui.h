// Lightweight GUI lock/unlock API used across modules
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void gui_lock(void);
void gui_unlock(void);

#ifdef __cplusplus
}
#endif
