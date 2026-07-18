#include <stdint.h>

enum {
	MSG_EXCESSIVE, MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR
};

int fhost_wpa_debug_level = MSG_INFO;

