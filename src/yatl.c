#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yatl.h"

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
  a.value.symbol = s;
  sym_table = cons(a, sym_table);

  return a;
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
  print_expr(make_int(42));
  putchar('\n');

  print_expr(cons(
		  make_sym("X"),
		  make_sym("Y")
		  )
	     );

  putchar('\n');

  print_expr(
	     cons(make_int(1),
		  cons(make_int(2),
		       cons(make_int(3),
			    nil))));
  return 0;
}
