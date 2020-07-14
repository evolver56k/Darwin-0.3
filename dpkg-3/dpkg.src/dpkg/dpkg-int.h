#include "config.h"

#if HAVE_LOCALE_H
# include <locale.h>
#endif
#if !HAVE_SETLOCALE
# define setlocale(category, locale)
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(text) gettext (text)
# define G_(text) (text)
# define N_(text) (text)
#else
# define bindtextdomain(domain, directory)
# define textdomain(domain)
# define _(text) (text)
# define G_(text) (text)
# define N_(text) (text)
#endif

#define internerr(s) do_internerr (s,__LINE__,__FILE__)

#ifndef S_ISLNK
# define S_ISLNK(mode) ((mode&0xF000) == S_IFLNK)
#endif
