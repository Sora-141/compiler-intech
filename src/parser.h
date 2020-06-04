#ifndef PARSER_H
#define PARSER_H
#include "buffer.h"
#include "ast.h"
#include "symbol.h"
#include "stack.h"

ast_list_t *parse (buffer_t *buffer);

mystack_t deal_with_equations();

#endif /* PARSER_H */
