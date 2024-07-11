#pragma once

#include "virtualtfa.h"

void virtualtfa_util_set_file_metadata(const char *filepath, tfa_mode_t mode, tfa_utime_t ctime, tfa_utime_t mtime);
