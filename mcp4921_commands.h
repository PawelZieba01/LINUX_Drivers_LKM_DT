#pragma once

#define MCP4921_RESET _IO('k', 0)
#define MCP4921_ENABLE _IOW('k', 1, int)
#define MCP4921_GAIN _IOW('k', 2, int)
#define MCP4921_VREF_BUFF _IOW('k', 3, int)
