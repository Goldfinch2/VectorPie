// Serial port, one per handle (dvg_utils had a single process-global port).
// POSIX implementation in vp_serial_posix.cpp, Windows in vp_serial_win.cpp;
// each file compiles to nothing on the other platform. Both files also
// implement vp_ticks_ms / vp_sleep_ms (vp_time.h).
#pragma once

#include <cstdint>

struct vp_serial;

// Open at 2 Mbaud, raw, no flow control. Returns VP_OK or a VP_ERR_* code
// (detail in vp_last_error()).
int  vp_serial_open(const char *port, vp_serial **out);
void vp_serial_close(vp_serial *s);

// Write all `count` bytes, waiting at most `timeout_ms` for the port to accept
// each chunk. Returns VP_OK, VP_ERR_TIMEOUT or VP_ERR_IO.
int  vp_serial_write(vp_serial *s, const void *data, int count, int timeout_ms);

// Read exactly `count` bytes, or fail once `timeout_ms` has passed in total.
// Returns VP_OK, VP_ERR_TIMEOUT or VP_ERR_IO.
int  vp_serial_read(vp_serial *s, void *data, int count, int timeout_ms);

// Throw away whatever the board sent that nobody read yet.
void vp_serial_flush_input(vp_serial *s);

// Wait until everything written has left the host.
void vp_serial_drain(vp_serial *s);
