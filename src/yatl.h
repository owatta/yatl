#define car(p) ((p).value.pair->atom[0])
#define cdr(p) ((p).value.pair->atom[1])
#define nilp(atom) ((atom).type == AtomType_Nil)

struct Atom {
  enum {
    AtomType_Nil,
    AtomType_Pair,
    AtomType_Symbol,
    AtomType_Integer
  } type;

  union {
    struct Pair* pair;
    const char* symbol;
    long integer;
  } value;
};

struct Pair {
  struct Atom atom[2]; // this is weird..?
};

typedef struct Atom Atom;

static const Atom nil = { AtomType_Nil };

// list of all created symbols
static Atom sym_table = { AtomType_Nil };
