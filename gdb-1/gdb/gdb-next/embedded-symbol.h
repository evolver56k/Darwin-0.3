
#ifndef __EMBEDDED_SYMBOL_H_
#define __EMBEDDED_SYMBOL_H_

struct embedded_symbol
{
    char*			name;
    enum language	language;
};
typedef struct embedded_symbol embedded_symbol;

embedded_symbol* search_for_embedded_symbol PARAMS((CORE_ADDR pc));

#endif
