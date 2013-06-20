/*
g++ 4.5.3: g++ -Wall -Wextra -g -std=c++0x protolisp.cpp
           g++ -Wall -Wextra -fwhole-program -Os -std=c++0x protolisp.cpp

This is about as simple a lisp interpreter as I can imagine.

It supports the following data types: symbols, lists, closures, and primitives.
Data is immutable.

A symbol is represented by a string not containing ')', '(', white space,
or '\''. Symbol names are case sensitive.

A list is either the empty list or a head and a tail. The head is any data, the
tail is a list.

A closure captures the environment in the usual way. Variadic functions are not
supported. Evaluation is call by value.

A primitive is one of the following:
  cons     Construct a list by prepending data to a list.
  head     Get the head of a list.
  tail     Get the tail of a list.
  eq?      Compare data for equality. Equal if same object.
  if       Evaluate arg, branch accordingly.
  define   Create a global variable.
  lambda   Construct a function.
  quote    Return the argument without evaluating it.

The reader supports single line semi-colon comments and the quote macro.

Garbage collection is achieved through reference counting. (Data is immutable
so cycles are not possible.)
*/

#include <cctype> // isspace
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>

namespace ProtoLisp {
  namespace Repl {
    int repl(std::istream&, std::ostream&);
} }

bool trace = false;

int main(int argc, char* argv[]) {
  if (argc > 1 && std::string("--trace-eval") == argv[1]) { trace = true; }

  return ProtoLisp::Repl::repl(std::cin, std::cout); }

namespace ProtoLisp {

  void assert(bool b, const std::string& msg) {
    if (!b) {
      std::cerr << "error: " << msg << std::endl;
      throw std::exception(); } }

  namespace Expr {
    // Lisp objects are symbols, lists, and closures. Object is the base type.
    class Object {
    public:
      virtual operator const std::string() const = 0;

      virtual ~Object() {}
    };
  
    typedef std::shared_ptr<const Object> ObjectPtr;
  
    class Symbol : public Object {
    public:
      explicit Symbol(const std::string& s) : glyph(s) {}

      virtual operator const std::string() const { return glyph; }

    private:
      const std::string glyph;
    };

    typedef std::shared_ptr<const Symbol> SymbolPtr;
  
    namespace {
      // Symbols are singletons.
      class Intern {
      public:
        Intern() : table() {}

        SymbolPtr insert(const std::string& s) {
          return table.insert(
              std::make_pair(s,
              std::make_shared<SymbolPtr::element_type>(s))).first->second; }

      private:
        std::map<const std::string, const SymbolPtr> table;
      } intern;
    }
  
    SymbolPtr make_symbol(const std::string& s) { return intern.insert(s); }

    template <typename T> T as(const ObjectPtr& o) {
      return std::dynamic_pointer_cast<typename T::element_type>(o); }

    template <typename T> bool is(const ObjectPtr& o) {
      return as<T>(o).get(); }
  
    const auto& CONS = make_symbol("cons");
    const auto& HEAD = make_symbol("head");
    const auto& TAIL = make_symbol("tail");
    const auto& EQP = make_symbol("eq?");
    const auto& SYMBOLP = make_symbol("symbol?");
    const auto& PROCEDUREP = make_symbol("procedure?");
    const auto& LISTP = make_symbol("list?");
    const auto& IF = make_symbol("if");
    const auto& DEFINE = make_symbol("define");
    const auto& LAMBDA = make_symbol("lambda");
    const auto& QUOTE = make_symbol("quote");
    const auto& QUOTEMACRO = make_symbol("'");
    const auto& TRUE = make_symbol("#t");
    const auto& FALSE = make_symbol("#f");
    const auto& LPARENS = make_symbol("(");
    const auto& RPARENS = make_symbol(")");
    const auto& APPLY = make_symbol("apply");
  
    class List;
    typedef std::shared_ptr<const List> ListPtr;
  
    class List : public Object {
    public:
      List(const ObjectPtr& h, const ListPtr& t) : head(h), tail(t) {}

      virtual operator const std::string() const {
        return std::string("(") + inner_string("") + ")"; }

      static const ListPtr Empty;

      const ObjectPtr head;

      const ListPtr tail;

    private:
      std::string inner_string(const std::string& s) const {
        if (this == Empty.get()) { return ""; }
        return s + static_cast<std::string>(*head) + tail->inner_string(" "); }
    };
  
    ListPtr make_list(const ObjectPtr& h, const ListPtr& t) {
      return std::make_shared<ListPtr::element_type>(h, t); }
  
    const ListPtr List::Empty = make_list(ObjectPtr(), ListPtr());

    unsigned len(const ListPtr& list) {
      return list == List::Empty ? 0 : 1 + len(list->tail); }
  
    // The environment is a lexically scoped symbol table.
    class Environment;
    typedef std::shared_ptr<Environment> EnvironmentPtr;
  
    class Environment {
    public:
      Environment(ListPtr vars, ListPtr vals, const EnvironmentPtr& p)
      : env(), parent(p) {
        assert(len(vars) == len(vals), "wrong number of arguments");
        while (vars != List::Empty) {
          const auto& var = as<SymbolPtr>(vars->head);
          assert(var.get(), "expected symbol for argument name");
          insert(var, vals->head);
          vars = vars->tail; vals = vals->tail; } }
  
      void insert(const SymbolPtr& symbol, const ObjectPtr& data) { env[symbol] = data; }
  
      std::pair<bool, ObjectPtr> find(const SymbolPtr& symbol) const {
        const auto& val = env.find(symbol);
        if (val == env.end()) {
          if (parent == EnvironmentPtr()) {
            return std::make_pair(false, ObjectPtr()); }
          else { return parent->find(symbol); } }
        return std::make_pair(true, val->second); }

    private:
      std::map<const SymbolPtr, ObjectPtr> env;

      const EnvironmentPtr parent;
    };
  
    class Closure : public Object {
    public:
      Closure(const ListPtr& a, const ObjectPtr& b, const EnvironmentPtr& e)
        : vars(a), body(b), env(e) {}
  
      virtual operator const std::string() const {
        return std::string("#<closure: (lambda ")
            + static_cast<std::string>(*vars) + " "
            + static_cast<std::string>(*body) + ")>"; }

      const ListPtr vars;

      const ObjectPtr body;

      const EnvironmentPtr env;
    };
  
    typedef std::shared_ptr<const Closure> ClosurePtr;

    class Primitive : public Object {
    public:
      typedef std::function<ObjectPtr (const ListPtr&, const EnvironmentPtr&)>
          primitive_type;

      Primitive(const SymbolPtr& n, const primitive_type& f) : name(n), exec(f) {}
  
      virtual operator const std::string() const {
        return std::string("#<primitive: ") + static_cast<std::string>(*name) + ">"; }

      const SymbolPtr name;

      const primitive_type exec;
    };
  
    typedef std::shared_ptr<const Primitive> PrimitivePtr;

    PrimitivePtr make_primitive(const SymbolPtr& n, const Primitive::primitive_type& f) {
      return std::make_shared<PrimitivePtr::element_type>(n, f); }
  }


  // The follow factory methods convert a token stream into lisp objects.
  namespace Reader {
    // Token and operator>> convert character stream into a token stream.
    class Token {
    public:
      Token() : val(), line(0) {}

      operator const std::string() const { return val; }

      friend std::istream& operator>>(std::istream& is, Token& token);

    private:
      std::string val;

      int line;
    };

    typedef std::istream_iterator<Token> TokenIter;

    Expr::ObjectPtr read(TokenIter& iter, TokenIter& end);

    namespace {
      Expr::ListPtr read_quote(TokenIter& iter, TokenIter& end) {
        return Expr::make_list(
            Expr::QUOTE,
            Expr::make_list(read(++iter, end), Expr::List::Empty)); }

      Expr::ListPtr read_list(TokenIter& iter, TokenIter& end) {
        if (Expr::make_symbol(*iter) == Expr::RPARENS) {
          return Expr::List::Empty; }
        const auto& h = read(iter, end);  // explicit read order
        const auto& t = read_list(++iter, end);
        return Expr::make_list(h, t); }
    }

    std::istream& operator>>(std::istream& is, Token& token) {
      token.val = "";
      while (std::isspace(is.peek())) {
        const auto& c = is.get(); if (c == '\n') { ++token.line; } }
      if (is.peek() == ';') {
        while (is.peek() != '\n') { is.get(); ++token.line; }
        return is >> token; }
      while (is) {
        const auto& c = is.get();
        if (!is || std::isspace(c)) { if (c == '\n') ++token.line; }
        else if (c == ';') {
          is.putback(c); }
        else if (c == '(' || c == ')') {
          if (token.val.empty()) { token.val = c; } else { is.putback(c); } }
        else if (c == '\'') {
          if (token.val.empty()) { token.val = "'"; } else { is.putback(c); } }
        else {
          token.val += c;
          continue; }
        break; }
      return is; }

    Expr::ObjectPtr read(TokenIter& iter, TokenIter& end) {
      const auto& symbol = Expr::make_symbol(*iter);
      assert(symbol != Expr::RPARENS, "unexpected ')'");

      if (symbol == Expr::LPARENS) { return read_list(++iter, end); }
      else if (symbol == Expr::QUOTEMACRO) { return read_quote(iter, end); }
      else { return symbol; } }
  }


  namespace Eval {
    Expr::ObjectPtr eval(const Expr::ObjectPtr& exp, const Expr::EnvironmentPtr& env);

    namespace {
      Expr::ObjectPtr lookup(const Expr::SymbolPtr& sym, const Expr::EnvironmentPtr& env) {
        const auto& val = env->find(sym);
        assert(val.first, "undefined symbol");
        return val.second; }

      Expr::ObjectPtr Boolean(bool b) { return b ? Expr::TRUE : Expr::FALSE; }

      template <typename T> Expr::ObjectPtr isp(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 1, "wrong number of arguments");
        return Boolean(Expr::is<T>(eval(list->head, env))); }

      Expr::ObjectPtr cons(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 2, "wrong number of arguments");
        const auto& arg0 = eval(list->head, env);
        const auto& arg1 = Expr::as<Expr::ListPtr>(eval(list->tail->head, env));
        assert(arg1.get(), "expected list for arg1");
        return Expr::make_list(arg0, arg1); }

      Expr::ObjectPtr head(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 1, "wrong number of arguments");
        const auto& arg0 = Expr::as<Expr::ListPtr>(eval(list->head, env));
        assert(arg0.get(), "expected list for arg0");
        return arg0->head; }

      Expr::ObjectPtr tail(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 1, "wrong number of arguments");
        const auto& arg0 = Expr::as<Expr::ListPtr>(eval(list->head, env));
        assert(arg0.get(), "expected list for arg0");
        return arg0->tail; }

      Expr::ObjectPtr eqp(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 2, "wrong number of arguments");
        const auto& arg0 = eval(list->head, env);
        const auto& arg1 = eval(list->tail->head, env);
        return Boolean(arg0 == arg1); }

      // #f is false, everything else is true.
      Expr::ObjectPtr if_(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 3, "wrong number of arguments");
        const auto& arg0 = eval(list->head, env);
        const auto& arg1 = list->tail->head;
        const auto& arg2 = list->tail->tail->head;
        return eval(Expr::FALSE == arg0 ? arg2 : arg1, env); }

      Expr::ObjectPtr quote(const Expr::ListPtr& list, const Expr::EnvironmentPtr&) {
        assert(len(list) == 1, "wrong number of arguments");
        return list->head; }

      Expr::ObjectPtr define(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 2, "wrong number of arguments");
        const auto& arg0 = Expr::as<Expr::SymbolPtr>(list->head);
        assert(arg0.get(), "expected symbol for arg0");
        const auto& arg1 = eval(list->tail->head, env);
        env->insert(arg0, arg1);
        return Expr::TRUE; } // Should return Void.

      Expr::ObjectPtr apply(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 2, "wrong number of arguments");
        const auto& arg0 = eval(list->head, env);
        assert(Expr::is<Expr::ClosurePtr>(arg0) || Expr::as<Expr::PrimitivePtr>(arg0), "expected procedure for arg0");
        const auto& arg1 = Expr::as<Expr::ListPtr>(eval(list->tail->head, env));
        assert(arg1.get(), "expected list for arg1");
        return eval(make_list(arg0, arg1), env); }

      Expr::ObjectPtr lambda(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        assert(len(list) == 2, "wrong number of arguments");
        const auto& arg0 = Expr::as<Expr::ListPtr>(list->head);
        assert(arg0.get(), "expected list for arg0");
        const auto& arg1 = list->tail->head;
        return std::make_shared<Expr::Closure>(arg0, arg1, env); }

      Expr::ListPtr eval_all(const Expr::ListPtr& list, const Expr::EnvironmentPtr& env) {
        return list == Expr::List::Empty ? list :
            Expr::make_list(eval(list->head, env), eval_all(list->tail, env)); }

       bool isConstant(const Expr::ObjectPtr& exp) {
         return exp == Expr::List::Empty ? true :
             exp == Expr::FALSE ? true :
             exp == Expr::TRUE ? true : false; }
    }

    void init(const Expr::EnvironmentPtr& env) {
      env->insert(Expr::CONS, Expr::make_primitive(Expr::CONS, cons));
      env->insert(Expr::HEAD, Expr::make_primitive(Expr::HEAD, head));
      env->insert(Expr::TAIL, Expr::make_primitive(Expr::TAIL, tail));
      env->insert(Expr::EQP, Expr::make_primitive(Expr::EQP, eqp));
      env->insert(Expr::PROCEDUREP, Expr::make_primitive(Expr::PROCEDUREP, isp<Expr::ClosurePtr>));
      env->insert(Expr::SYMBOLP, Expr::make_primitive(Expr::SYMBOLP, isp<Expr::SymbolPtr>));
      env->insert(Expr::LISTP, Expr::make_primitive(Expr::LISTP, isp<Expr::ListPtr>));
      env->insert(Expr::IF, Expr::make_primitive(Expr::IF, if_));
      env->insert(Expr::QUOTE, Expr::make_primitive(Expr::QUOTE, quote));
      env->insert(Expr::DEFINE, Expr::make_primitive(Expr::DEFINE, define));
      env->insert(Expr::APPLY, Expr::make_primitive(Expr::APPLY, apply));
      env->insert(Expr::LAMBDA, Expr::make_primitive(Expr::LAMBDA, lambda)); }

    Expr::ObjectPtr eval(const Expr::ObjectPtr& exp, const Expr::EnvironmentPtr& env) {
      if (trace) { std::cerr << static_cast<std::string>(*exp) << std::endl; }
      if (isConstant(exp)) { return exp; }
      const auto& sym = Expr::as<Expr::SymbolPtr>(exp);
      if (sym) { return lookup(sym, env); }
      const auto& list = Expr::as<Expr::ListPtr>(exp);
      if (list) {
        const auto& fn = eval(list->head, env);
        const auto& args = list->tail;
        const auto& builtin = Expr::as<Expr::PrimitivePtr>(fn);
        if (builtin) { return builtin->exec(args, env); }
        const auto& closure = Expr::as<Expr::ClosurePtr>(fn);
        if (closure) {
          return eval(closure->body, std::make_shared<Expr::Environment>(
              closure->vars,
              eval_all(args, env),
              closure->env)); } }
      assert(false, "unknown expression");
      return Expr::FALSE; }
  }

  namespace Repl {
    namespace {
      void prompt(std::ostream& os) { os << "ok "; }

      std::ostream& operator<<(std::ostream& os, Expr::ObjectPtr pv) {
        os << static_cast<std::string>(*pv);
        return os; }
    }

    int repl(std::istream& is, std::ostream& os) {
      auto global_env = std::make_shared<Expr::Environment>(
          Expr::List::Empty, Expr::List::Empty, Expr::EnvironmentPtr());
      Eval::init(global_env);
  
      prompt(os);

      Reader::TokenIter iter(is);  // read
      Reader::TokenIter end;

      while (iter != end) {
        try {
          Expr::ObjectPtr exp = Reader::read(iter, end);
          Expr::ObjectPtr result = Eval::eval(exp, global_env);
          os << result << std::endl;
        } catch (...) {}
        prompt(os);
        ++iter; }
      return 0; }
} }
