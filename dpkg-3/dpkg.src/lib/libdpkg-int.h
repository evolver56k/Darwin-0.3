#if HAVE_LOCALE_H
# include <locale.h>
#endif
#if !HAVE_SETLOCALE
# define setlocale(category, locale)
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(text) (dgettext ("libdpkg", text))
# define G_(text) (dgettext ("libdpkg", text))
# define N_(text) (text)
#else
# define bindtextdomain(domain, directory)
# define textdomain(domain)
# define _(text) (text)
# define G_(text) (text)
# define N_(text) (text)
#endif

#if !defined (__GNUC__) || __GNUC__ < 2
#define __attribute__(x)
#endif

#define internerr(s) do_internerr (s,__LINE__,__FILE__)
