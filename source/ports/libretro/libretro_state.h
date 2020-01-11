#ifndef LIBRETRO_STATE_H
#define LIBRETRO_STATE_H

uint32_t system_save_state_mem(void* data, size_t size);
void system_load_state_mem(const void* data, size_t size);

#endif /* LIBRETRO_STATE_H */