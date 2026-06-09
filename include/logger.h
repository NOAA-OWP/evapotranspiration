#ifndef PET_LOGGER_H
#define PET_LOGGER_H

#include "ewts/module_constants.h"
#include "ewts/logger.h"
#include "ewts/log_levels.h"

#define Log(level, ...) EwtsLogModule(EWTS_ID_PET, (level), __VA_ARGS__)
#define LOG(level, ...) EwtsLogModule(EWTS_ID_PET, (level), __VA_ARGS__)
#define GetLogLevel() EwtsGetLogLevelModule(EWTS_ID_PET)
#define IsLoggingEnabled() EwtsIsLoggingEnabledModule(EWTS_ID_PET)

#endif /* PET_LOGGER_H */
