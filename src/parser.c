#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "symbol.h"
#include "buffer.h"
#include "parser.h"
#include "ast.h"
#include "utils.h"
#include "stack.h"
#include "lexer.h"

#define DEBUG true

extern symbol_t **pglobal_table;
extern ast_t **past;

void stack_print_status(mystack_t *stack, char name)
{
  printf("\n----------------\n");
  printf("name = \"%c\"\n", name);
  printf("stack_count = %d\n", stack_count(*stack));
  printf("stack_top = \"%p\"\n", stack_top(*stack));
  if (stack_top(*stack))
    printf("*(char*)stack_top = '%c'\n", *(char *)stack_top(*stack));
  printf("----------------\n\n");
}

ast_t *charConvertor(char *c)
{
  if (*c == '+')
  {
    return ast_new_binary(AST_BIN_PLUS, NULL, NULL);
  }
  if (*c == '-')
  {
    return ast_new_binary(AST_BIN_MINUS, NULL, NULL);
  }
  if (*c == '*')
  {
    return ast_new_binary(AST_BIN_MULT, NULL, NULL);
  }
  if (*c == '/')
  {
    return ast_new_binary(AST_BIN_DIV, NULL, NULL);
  }
  if (isnbr(*c))
  {
    long x;
    sscanf(c, "%ld", &x);
    return ast_new_integer(x);
  }
  return NULL;
}

//compare(a,b) if a > b terurn true else return false
bool compare(char *a, char *b)
{
  if (ast_binary_priority((ast_t*)charConvertor(a)) > ast_binary_priority((ast_t*)charConvertor(b)))
    return true;
  return false;
}

mystack_t deal_with_equations()
{
  char string[] = "1+2*3-4+5"; // temporary setup
  mystack_t stack = NULL;
  mystack_t exit = NULL;

  // positionner le curseur i sur le début de la chaîne
  int i = 0;

  // empiler le caractère nul dans pile
  stack_push(&stack, (char *)'\0');

  // répéter indéfiniment:
  for (;;)
  {
    // si i pointe sur le caractère nul et le sommet de la pile contient également le caractère nul
    char topStack = '\0';
    if (stack_top(stack))
      topStack = *(char *)stack_top(stack);
    if (string[i] == '\0' && topStack == '\0')
    {
      // retourner sortie
      printf("returning result\n");
      return exit;
    }
    // sinon
    else
    {
      // soient a le symbole en sommet de pile et b le symbole pointé par i
      char *a = stack_top(stack);
      char *b = &string[i];
      bool comparaison = false;
      if (stack_top(stack))
        comparaison = compare(a, b);

      // si a a une priorité plus basse que b
      if (!comparaison)
      {
        // empiler b dans pile
        stack_push(&stack, b); //printf("pushing '%c' on to stack\n", *b);

        // avancer i sur le symbole suivant
        i++;
      }
      // sinon
      else
      {
        // répéter
        do
        {
          // dépiler pile et empiler la valeur dépilée dans sortie
          char *temp = stack_pop(&stack);
          ast_t *test = charConvertor(temp);
          stack_push(&exit, test);
          printf("pushing '%c' on to exit\n", *temp);

          // jusqu’à ce que le symbole en sommet de pile aie une priorité plus basse que le symbole le plus récemment dépilé
          if (stack_top(stack))
          {
            char c = *(char *)stack_top(stack);
            if ((c != '\0') && c != ' ')
            {
              comparaison = compare(stack_top(stack), temp);
            }
            else
              comparaison = false;
          }
          else
            comparaison = false;
        } while (comparaison); //tourne tant que a > b ;
      }
    }
  }
}

int parse_var_type(buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "entier") == 0)
  {
    return AST_INTEGER;
  }
  printf("Expected a valid type. exiting.\n");
  exit(1);
}

/**
 * 
 * (entier a, entier b, entier c) => une liste d'ast_t contenue dans un ast_list_t
 */
ast_list_t *parse_parameters(buffer_t *buffer)
{
  ast_list_t *list = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbrace(buffer, "Expected a '(' after function name. exiting.\n");

  for (;;)
  {
    int type = parse_var_type(buffer);

    buf_skipblank(buffer);
    char *var_name = lexer_getalphanum(buffer);
    buf_skipblank(buffer);

    ast_list_add(&list, ast_new_variable(var_name, type));

    char next = buf_getchar(buffer);
    if (next == ')')
      break;
    if (next != ',')
    {
      printf("Expected either a ',' or a ')'. exiting.\n");
      exit(1);
    }
  }
  return list;
}

int parse_return_type(buffer_t *buffer)
{
  buf_skipblank(buffer);
  lexer_assert_twopoints(buffer, "Expected ':' after function parameters");
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "entier") == 0)
  {
    return AST_INTEGER;
  }
  else if (strcmp(lexem, "rien") == 0)
  {
    return AST_VOID;
  }
  printf("Expected a valid type for a parameter. exiting.\n");
  exit(1);
}

bool parse_is_type(char *lexem)
{
  if (strcmp(lexem, "entier") != 0)
  { // si le mot-clé n'est pas "entier", on retourne faux
    return false;
  }
  return true;
}

ast_t *parse_expression(buffer_t *buffer)
{
  mystack_t stack = NULL;
  mystack_t exit = NULL;

  // positionner le curseur i sur le début de la chaîne
  buf_skipblank(buffer);

  // empiler le caractère nul dans pile
  stack_push(&stack, NULL);

  // répéter indéfiniment:
  for (;;)
  {
    // si i pointe sur le caractère nul et le sommet de la pile contient également le caractère nul
    char topStack = '\0';
    if (stack_top(stack))
      topStack = *(char *)stack_top(stack);
    if ((buf_getchar_rollback(buffer) == '\0' && topStack == '\0') || (buf_getchar_rollback(buffer) == ';'))
    {
      // retourner sortie
      printf("returning result\n");
      return pile_vers_arbre(&exit);
    }
    // sinon
    else
    {
      // soient a le symbole en sommet de pile et b le symbole pointé par i
      char *a = (char *)stack_top(stack);
      char *b = (char *)buf_getchar_rollback(buffer);
      bool comparaison = false;
      if (stack_top(stack))
        comparaison = compare(a, b); //check if stack_top != (nil)

      // si a a une priorité plus basse que b
      if (!comparaison)
      {
        // empiler b dans pile
        stack_push(&stack, b); //printf("pushing '%c' on to stack\n", *b);

        // avancer i sur le symbole suivant
        buf_forward(buffer, 1);
      }
      // sinon
      else
      {
        // répéter
        do
        {
          // dépiler pile et empiler la valeur dépilée dans sortie
          char *temp = stack_pop(&stack);
          ast_t *test = charConvertor(temp);
          stack_push(&exit, test);
          printf("pushing '%c' on to exit\n", *temp);

          // jusqu’à ce que le symbole en sommet de pile aie une priorité plus basse que le symbole le plus récemment dépilé
          if (stack_top(stack))
          {
            char c = *(char *)stack_top(stack);
            if ((c != '\0') && c != ' ')
            {
              comparaison = compare(stack_top(stack), temp);
            }
            else
              comparaison = false;
          }
          else
            comparaison = false;
        } while (comparaison); //tourne tant que a > b ;
      }
    }
  }
}

/**
 * entier a;
 * entier a = 2;
 */
ast_t *parse_declaration(buffer_t *buffer)
{
  int type = parse_var_type(buffer);
  buf_skipblank(buffer);
  char *var_name = lexer_getalphanum(buffer);
  if (var_name == NULL)
  {
    printf("Expected a variable name. exiting.\n");
    exit(1);
  }

  ast_t *var = ast_new_variable(var_name, type);
  buf_skipblank(buffer);
  char next = buf_getchar(buffer);
  if (next == ';')
  {

    return ast_new_declaration(var, NULL);
  }
  else if (next == '=')
  {
    ast_t *expression = parse_expression(buffer);
    return ast_new_declaration(var, expression);
  }
  printf("Expected either a ';' or a '='. exiting.\n");
  buf_print(buffer);
  exit(1);
}

ast_t *parse_statement(buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum_rollback(buffer);
  if (parse_is_type(lexem))
  {
    // ceci est une déclaration de variable
    return parse_declaration(buffer);
  }
  // TODO:
  return NULL;
}

ast_list_t *parse_function_body(buffer_t *buffer)
{
  ast_list_t *stmts = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbracket(buffer, "Function body should start with a '{'");
  char next;
  do
  {
    ast_t *statement = parse_statement(buffer);
    ast_list_add(&stmts, statement);
    buf_skipblank(buffer);
    next = buf_getchar_rollback(buffer);
  } while (next != '}');

  return stmts;
}

/**
 * exercice: cf slides: https://docs.google.com/presentation/d/1AgCeW0vBiNX23ALqHuSaxAneKvsteKdgaqWnyjlHTTA/edit#slide=id.g86e19090a1_0_527
 */
ast_t *parse_function(buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "fonction") != 0)
  {
    printf("Expected a 'fonction' keyword on global scope.exiting.\n");
    buf_print(buffer);
    exit(1);
  }
  buf_skipblank(buffer);
  char *name = lexer_getalphanum(buffer);

  ast_list_t *params = parse_parameters(buffer);
  int return_type = parse_return_type(buffer);
  ast_list_t *stmts = parse_function_body(buffer);

  return ast_new_function(name, return_type, params, stmts);
}

ast_t *pile_vers_arbre(mystack_t *pile)
{
  if (stack_isempty(*pile))
    return NULL;

  ast_t *curr = stack_pop(pile);
  if (curr->type == AST_BINARY)
  {
    if (!curr->binary.right)
      curr->binary.right = pile_vers_arbre(pile);
    if (!curr->binary.left)
      curr->binary.left = pile_vers_arbre(pile);
  }
  return curr;
}

/**
 * This function generates ASTs for each global-scope function
 */
ast_list_t *parse(buffer_t *buffer)
{
  // 1 + 2 * 3 - 4 + 5
  // + 5 - 4 + * 3 2 1
  mystack_t stack = deal_with_equations(buffer);

  ast_t *sortie = pile_vers_arbre(&stack);
  ast_print(sortie);

  ast_t *function = parse_function(buffer);
  ast_print(function);

  if (DEBUG)
    printf("** end of file. **\n");
  return NULL;
}
