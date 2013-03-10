
#include <string.h>

static const char *__progname = "<unset_progname>";

void setprogname(const char *);
void
setprogname(const char *progname)
{

        __progname = strrchr(progname, '/');
        if (__progname == NULL)
                __progname = progname;
        else
                __progname++;
}

const char *getprogname(void);
const char *
getprogname(void)
{

        return (__progname);
}
