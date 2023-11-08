#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <readline/readline.h>

#include "yatl.h"

int read_expr(const char* input, const char** end, Atom* result);

Atom cons(Atom car_val, Atom cdr_val)
{
  Atom p;

  p.type = AtomType_Pair;
  p.value.pair = malloc(sizeof(struct Pair));

  car(p) = car_val;
  cdr(p) = cdr_val;

  return p;
}

Atom make_int(int val) {
  Atom a;

  a.type = AtomType_Integer;
  a.value.integer = val;

  return a;
}

Atom make_sym(const char* s) {
  Atom a, p;

  p = sym_table;
  while (!nilp(p)) {
    a = car(p);

    if (strcmp(a.value.symbol, s) == 0)
      return a;
    p = cdr(p);
  }

  a.type = AtomType_Symbol;
  a.value.symbol = strdup(s);
  sym_table = cons(a, sym_table);

  return a;
}

// returns the start and end of the next token in the string
int lex(const char* str, const char** start, const char** end) {
  const char* ws = " \t\n";
  const char* delim = "() \t\n";
  const char* prefix = "()";

  str += strspn(str, ws);

  if (str[0] == '\0') {
    *start = *end = NULL;
    return Error_Syntax;
  }

  *start = str;

  if (strchr(prefix, str[0]) != NULL)
    *end = str + 1;
  else
    *end = str + strcspn(str, delim);

  return Error_OK;
}

int parse_simple(const char* start, const char* end, Atom* result) {
  char *buf, *p;

  // Is it an integer?
  long val = strtol(start, &p, 10);
  if (p == end) {
    result->type = AtomType_Integer;
    result->value.integer = val;
    return Error_OK;
  }

  // NIL or symbol
  buf = malloc(end - start + 1);
  p = buf;
  while (start != end)
    *p++ = toupper(*start), ++start;
  *p = '\0';

  if (strcmp(buf, "NIL") == 0)
    *result = nil;
  else
    *result = make_sym(buf);

  free(buf);

  return Error_OK;
}

int read_list(const char* start, const char** end, Atom* result) {
  Atom p = *result = nil;
  *end = start;

  for (;;) {
    const char* token;
    Atom item;
    Error err;

    err = lex(*end, &token, end);
    if (err)
      return err;

    if (token[0] == ')')
      return Error_OK;

    if (token[0] == '.' && *end - token == 1) {
      // improper list
      if (nilp(p))
	return Error_Syntax;

      err = read_expr(*end, end, &item);
      if (err)
	return err;

      cdr(p) = item;

      // read the closing ')'
      err = lex(*end, &token, end);
      if (!err && token[0] != ')')
	err = Error_Syntax;

      return err;
    }

    err = read_expr(token, end, &item);
    if (err)
      return err;

    if (nilp(p)) {
      *result = cons(item, nil);
      p = *result;
    } else {
      cdr(p) = cons(item, nil);
      p = cdr(p);
    }
  }
}

int read_expr(const char* input, const char** end, Atom* result) {
  const char* token;
  Error err;

  err = lex(input, &token, end);
  if (err)
    return err;

  if (token[0] == '(')
    return read_list(*end, end, result);
  else if (token[0] == ')')
    return Error_Syntax;
  else
    return parse_simple(token, *end, result);
}

void print_expr(Atom atom) {
  switch (atom.type) {
  case AtomType_Nil:
    printf("NIL");
    break;
  case AtomType_Integer:
    printf("%ld", atom.value.integer);
    break;
  case AtomType_Symbol:
    printf("%s", atom.value.symbol);
    break;
  case AtomType_Pair:
    // car
    putchar('(');
    print_expr(car(atom));
    // cdr
    atom = cdr(atom);
    while (!nilp(atom)) {
      if (atom.type == AtomType_Pair) {
	putchar(' ');
	print_expr(car(atom));
	atom = cdr(atom);
      } else {
	printf(" . ");
	print_expr(atom);
	break;
      }
    }
    putchar(')');
    break;
  }
}


int main() {
  char* input;

  while ((input = readline("> ")) != NULL) {
    const char* p = input;
    Error err;
    Atom expr;

    err = read_expr(p, &p, &expr);

    switch(err) {
    case Error_OK:
      print_expr(expr);
      putchar('\n');
      break;
    case Error_Syntax:
      printf("Syntax Error. Good luck.\n");
      break;
    }

    free(input);
  }

  return 0;
}
