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

Atom make_builtin(Builtin fn) {
  Atom a;
  a.type = AtomType_Builtin;
  a.value.builtin = fn;
  return a;
}

int builtin_car(Atom args, Atom* result) {
  if (nilp(args) || !nilp(cdr(args)))
    return Error_Args;

  if (nilp(car(args)))
    *result = nil;
  else if (car(args).type != AtomType_Pair)
    return Error_Type;
  else
    *result = car(car(args));

  return Error_OK;
}

int builtin_cdr(Atom args, Atom* result) {
  if (nilp(args) || !nilp(cdr(args)))
    return Error_Args;

  if (nilp(car(args)))
    *result = nil;
  else if (car(args).type != AtomType_Pair)
    return Error_Type;
  else
    *result = cdr(car(args));

  return Error_OK;
}

int builtin_cons(Atom args, Atom* result) {
  if (nilp(args) || nilp(cdr(args)) || !nilp(cdr(cdr(args))))
    return Error_Args;

  *result = cons(car(args), car(cdr(args)));

  return Error_OK;
}

Atom copy_list(Atom list) {
  Atom a, p;

  if (nilp(list))
    return nil;

  a = cons(car(list), nil);
  list = cdr(list);

  while(!nilp(list)) {
    cdr(p) = cons(car(list), nil);
  }

  return a;
}

int listp(Atom expr) {
  while (!nilp(expr)) {
    if (expr.type != AtomType_Pair)
      return 0;
    expr = cdr(expr);
  }

  return 1;
}

int apply(Atom fn, Atom args, Atom* result) {
  if (fn.type == AtomType_Builtin)
    return (*fn.value.builtin)(args, result);

  return Error_Type;
}

Atom env_create(Atom parent) {
  return cons(parent, nil);
}

int env_get(Atom env, Atom symbol, Atom* result) {
  Atom parent = car(env);
  Atom bs = cdr(env);

  while(!nilp(bs)) {
    Atom b = car(bs);
    if (car(b).value.symbol == symbol.value.symbol) {
      *result = cdr(b);
      return Error_OK;
    }
    bs = cdr(bs);
  }

  if (nilp(parent))
    return Error_Unbound;

  return env_get(parent, symbol, result);
}

int env_set(Atom env, Atom symbol, Atom value) {
  Atom bs = cdr(env);
  Atom b = nil;

  while (!nilp(bs)) {
    Atom b = car(bs);

    if (car(b).value.symbol == symbol.value.symbol) {
      cdr(b) = value;
      return Error_OK;
    }
    bs = cdr(bs);
  }
  b = cons(symbol, value);
  // bs = cons(b, bs); // why not this???
  cdr(env) = cons(b, cdr(env));

  return Error_OK;
}

int eval_expr(Atom expr, Atom env, Atom* result) {
  Atom op, args, p;
  Error err;

  if (expr.type == AtomType_Symbol) {
    return env_get(env, expr, result);
  } else if (expr.type != AtomType_Pair) {
    *result = expr;
    return Error_OK;
  }

  if (!listp(expr))
    return Error_Syntax;

  op = car(expr);
  args = cdr(expr);

  if (op.type == AtomType_Symbol) {
    if (strcmp(op.value.symbol, "QUOTE") == 0) {
      if (nilp(args) || !nilp(cdr(args)))
	return Error_Args;

      *result = car(args);
      return Error_OK;
    } else if (strcmp(op.value.symbol, "DEFINE") == 0) {
      Atom sym, val;

      if (nilp(args) || nilp(cdr(args)) || !nilp(cdr(cdr(args))))
	return Error_Args;

      sym = car(args);
      if (sym.type != AtomType_Symbol)
	return Error_Type;

      err = eval_expr(car(cdr(args)), env, &val);
      if (err)
	return err;

      *result = sym;
      return env_set(env, sym, val);
    }
  }

  // evaluate operator
  err = eval_expr(op, env, &op);
  if (err)
    return err;

  // evaluate arguments
  args = copy_list(args);
  p = args;
  while (!nilp(p)) {
    err = eval_expr(car(p), env, &car(p));
    if (err)
      return err;

    p = cdr(p);
  }

  return apply(op, args, result);

  return Error_Syntax;
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
  case AtomType_Builtin:
    printf("#<BUILTIN:%p>", atom.value.builtin);
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
  Atom env;
  char* input;

  env = env_create(nil);

  env_set(env, make_sym("CAR"), make_builtin(builtin_car));
  env_set(env, make_sym("CDR"), make_builtin(builtin_cdr));
  env_set(env, make_sym("CONS"), make_builtin(builtin_cons));

  while ((input = readline("> ")) != NULL) {
    const char* p = input;
    Error err;
    Atom expr, result;

    err = read_expr(p, &p, &expr);

    if (!err)
      err = eval_expr(expr, env, &result);

    switch(err) {
    case Error_OK:
      print_expr(result);
      putchar('\n');
      break;
    case Error_Syntax:
      printf("Syntax Error. Good luck.\n");
      break;
    case Error_Unbound:
      printf("Error: Value Unbound. Good luck.\n");
      break;
    case Error_Args:
      printf("Error: Incorrect number of arguments. Good luck.\n");
      break;
    case Error_Type:
      printf("Error: Incorrect type. Should've used Haskell.\n");
      break;
    }

    free(input);
  }

  return 0;
}
