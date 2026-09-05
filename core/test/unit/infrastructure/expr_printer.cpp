// expr_printer renders expr trees to a stream via injected var and functor
// printers. Named-strategy tests wrap the name tables. Id-injection cases
// cover re-parseable `_N` / `fN` spelling.

#include <gtest/gtest.h>
#include <sstream>
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/id_functor_printer.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/named_functor_printer.hpp"
#include "infrastructure/named_var_printer.hpp"
#include "infrastructure/var_names.hpp"
#include "infrastructure/functor_names.hpp"

struct ExprPrinterTest : public ::testing::Test {
    var_names vn;
    named_var_printer<var_names> print_var{vn};
    functor_names fn;
    named_functor_printer<functor_names> print_functor{fn};
    std::ostringstream os;
    expr_printer<named_var_printer<var_names>, named_functor_printer<functor_names>>
        printer{os, print_var, print_functor};

    expr var0{expr::var{0}};
    expr atom_a{expr::functor{2, {}}};
    expr atom_b{expr::functor{3, {}}};
    expr nil{expr::functor{k_nil_functor_id, {}}};

    std::string print(const expr* e) {
        os.str("");
        os.clear();
        printer.print(e);
        return os.str();
    }
};

TEST_F(ExprPrinterTest, PrintUnnamedVar) {
    EXPECT_EQ(print(&var0), "?0");
}

TEST_F(ExprPrinterTest, PrintUnnamedAtom) {
    EXPECT_EQ(print(&atom_a), "!2");
}

TEST_F(ExprPrinterTest, PrintUnnamedFunctor) {
    expr f{expr::functor{6, {&var0}}};
    EXPECT_EQ(print(&f), "!6(?0)");
}

TEST_F(ExprPrinterTest, PrintNamedVar) {
    vn.set_name(0, "X");
    EXPECT_EQ(print(&var0), "X");
}

TEST_F(ExprPrinterTest, PrintAtom) {
    fn.set_name(2, "a");
    EXPECT_EQ(print(&atom_a), "a");
}

TEST_F(ExprPrinterTest, PrintNil) {
    EXPECT_EQ(print(&nil), "[]");
}

TEST_F(ExprPrinterTest, PrintSingletonList) {
    fn.set_name(2, "a");
    expr list{expr::functor{k_cons_functor_id, {&atom_a, &nil}}};
    EXPECT_EQ(print(&list), "[a]");
}

TEST_F(ExprPrinterTest, PrintListWithTail) {
    fn.set_name(2, "a");
    fn.set_name(3, "b");
    expr list{expr::functor{k_cons_functor_id, {&atom_a, &atom_b}}};
    EXPECT_EQ(print(&list), "[a|b]");
}

TEST_F(ExprPrinterTest, PrintMultiElementList) {
    fn.set_name(2, "a");
    fn.set_name(3, "b");
    expr tail{expr::functor{k_cons_functor_id, {&atom_b, &nil}}};
    expr list{expr::functor{k_cons_functor_id, {&atom_a, &tail}}};
    EXPECT_EQ(print(&list), "[a, b]");
}

TEST_F(ExprPrinterTest, PrintFourElementList) {
    fn.set_name(2, "a");
    fn.set_name(3, "b");
    fn.set_name(4, "c");
    fn.set_name(5, "d");
    expr c{expr::functor{4, {}}};
    expr d{expr::functor{5, {}}};
    expr t3{expr::functor{k_cons_functor_id, {&d, &nil}}};
    expr t2{expr::functor{k_cons_functor_id, {&c, &t3}}};
    expr t1{expr::functor{k_cons_functor_id, {&atom_b, &t2}}};
    expr list{expr::functor{k_cons_functor_id, {&atom_a, &t1}}};
    EXPECT_EQ(print(&list), "[a, b, c, d]");
}

TEST_F(ExprPrinterTest, PrintUnaryFunctor) {
    fn.set_name(2, "a");
    fn.set_name(6, "f");
    expr f{expr::functor{6, {&atom_a}}};
    EXPECT_EQ(print(&f), "f(a)");
}

TEST_F(ExprPrinterTest, PrintBinaryFunctor) {
    fn.set_name(2, "a");
    fn.set_name(3, "b");
    fn.set_name(6, "f");
    expr f{expr::functor{6, {&atom_a, &atom_b}}};
    EXPECT_EQ(print(&f), "f(a, b)");
}

TEST_F(ExprPrinterTest, PrintTernaryFunctor) {
    fn.set_name(2, "a");
    fn.set_name(3, "b");
    fn.set_name(4, "c");
    fn.set_name(7, "h");
    expr c{expr::functor{4, {}}};
    expr h{expr::functor{7, {&atom_a, &atom_b, &c}}};
    EXPECT_EQ(print(&h), "h(a, b, c)");
}

TEST_F(ExprPrinterTest, PrintNestedFunctorArgs) {
    fn.set_name(2, "a");
    fn.set_name(8, "g");
    fn.set_name(6, "f");
    expr g{expr::functor{8, {&atom_a}}};
    expr f{expr::functor{6, {&g}}};
    EXPECT_EQ(print(&f), "f(g(a))");
}

TEST_F(ExprPrinterTest, PrintAtomViaFunctorNames) {
    fn.set_name(10, "abc");
    expr atom{expr::functor{10, {}}};
    EXPECT_EQ(print(&atom), "abc");
}

TEST_F(ExprPrinterTest, PrintListViaConsIds) {
    fn.set_name(2, "a");
    expr list{expr::functor{k_cons_functor_id, {&atom_a, &nil}}};
    EXPECT_EQ(print(&list), "[a]");
}

struct ExprPrinterIdVarTest : public ::testing::Test {
    id_var_printer print_var;
    functor_names fn;
    named_functor_printer<functor_names> print_functor{fn};
    std::ostringstream os;
    expr_printer<id_var_printer, named_functor_printer<functor_names>>
        printer{os, print_var, print_functor};
    expr var0{expr::var{0}};
};

TEST_F(ExprPrinterIdVarTest, PrintVarAsUnderscoreIndex) {
    printer.print(&var0);
    EXPECT_EQ(os.str(), "_0");
}

struct ExprPrinterIdFunctorTest : public ::testing::Test {
    var_names vn;
    named_var_printer<var_names> print_var{vn};
    id_functor_printer print_functor;
    std::ostringstream os;
    expr_printer<named_var_printer<var_names>, id_functor_printer>
        printer{os, print_var, print_functor};
    expr atom_a{expr::functor{2, {}}};
};

TEST_F(ExprPrinterIdFunctorTest, PrintAtomAsFThenId) {
    printer.print(&atom_a);
    EXPECT_EQ(os.str(), "f2");
}
