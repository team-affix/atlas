// expr_printer renders expr trees to a stream, consulting i_var_names for variables
// and i_atom_names for functor symbols. Unit tests mock both and assert string output.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include "locator_fixture.hpp"
#include "infrastructure/atom_names.hpp"
#include "infrastructure/expr_printer.hpp"
#include "interfaces/i_atom_names.hpp"
#include "interfaces/i_var_names.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockVarNames : public i_var_names {
    MOCK_METHOD(bool, is_named, (uint32_t), (const, override));
    MOCK_METHOD(const std::string&, name, (uint32_t), (const, override));
    MOCK_METHOD(void, set_name, (uint32_t, const std::string&), (override));
};

struct MockAtomNames : public i_atom_names {
    MOCK_METHOD(bool, is_named, (uint32_t), (const, override));
    MOCK_METHOD(const std::string&, name, (uint32_t), (const, override));
    MOCK_METHOD(void, set_name, (uint32_t, const std::string&), (override));
};

struct ExprPrinterTest : public ::testing::Test {
    locator loc;
    MockVarNames var_names;
    MockAtomNames atom_names;
    std::ostringstream os;
    expr_printer printer;

    ExprPrinterTest() : printer(init_printer()) {}

    expr_printer init_printer() {
        loc.bind_as<i_var_names>(var_names);
        loc.bind_as<i_atom_names>(atom_names);
        return expr_printer{os, loc};
    }

    void expect_atom_named(uint32_t id, const std::string& s) {
        EXPECT_CALL(atom_names, is_named(id)).WillRepeatedly(Return(true));
        EXPECT_CALL(atom_names, name(id)).WillRepeatedly(ReturnRef(s));
    }

    std::string var_name_x{"X"};
    std::string atom_a_name{"a"};
    std::string atom_b_name{"b"};

    expr var0{expr::var{0}};
    expr atom_a{expr::functor{2, {}}};
    expr atom_b{expr::functor{3, {}}};
    expr nil{expr::functor{k_nil_atom_id, {}}};

    std::string print(const expr* e) {
        os.str("");
        os.clear();
        printer.print(e);
        return os.str();
    }
};

TEST_F(ExprPrinterTest, PrintUnnamedVar) {
    EXPECT_CALL(var_names, is_named(0)).WillOnce(Return(false));

    EXPECT_EQ(print(&var0), "?0");
}

TEST_F(ExprPrinterTest, PrintNamedVar) {
    EXPECT_CALL(var_names, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(var_names, name(0)).WillOnce(ReturnRef(var_name_x));

    EXPECT_EQ(print(&var0), "X");
}

TEST_F(ExprPrinterTest, PrintAtom) {
    expect_atom_named(2, atom_a_name);
    EXPECT_EQ(print(&atom_a), "a");
}

TEST_F(ExprPrinterTest, PrintNil) {
    EXPECT_EQ(print(&nil), "[]");
}

TEST_F(ExprPrinterTest, PrintSingletonList) {
    expect_atom_named(2, atom_a_name);
    expr list{expr::functor{k_cons_atom_id, {&atom_a, &nil}}};
    EXPECT_EQ(print(&list), "[a]");
}

TEST_F(ExprPrinterTest, PrintListWithTail) {
    expect_atom_named(2, atom_a_name);
    expect_atom_named(3, atom_b_name);
    expr list{expr::functor{k_cons_atom_id, {&atom_a, &atom_b}}};
    EXPECT_EQ(print(&list), "[a|b]");
}

TEST_F(ExprPrinterTest, PrintMultiElementList) {
    expect_atom_named(2, atom_a_name);
    expect_atom_named(3, atom_b_name);
    expr tail{expr::functor{k_cons_atom_id, {&atom_b, &nil}}};
    expr list{expr::functor{k_cons_atom_id, {&atom_a, &tail}}};
    EXPECT_EQ(print(&list), "[a, b]");
}

TEST_F(ExprPrinterTest, PrintFourElementList) {
    expect_atom_named(2, atom_a_name);
    expect_atom_named(3, atom_b_name);
    std::string c_name{"c"};
    std::string d_name{"d"};
    expr c{expr::functor{4, {}}};
    expr d{expr::functor{5, {}}};
    expect_atom_named(4, c_name);
    expect_atom_named(5, d_name);
    expr t3{expr::functor{k_cons_atom_id, {&d, &nil}}};
    expr t2{expr::functor{k_cons_atom_id, {&c, &t3}}};
    expr t1{expr::functor{k_cons_atom_id, {&atom_b, &t2}}};
    expr list{expr::functor{k_cons_atom_id, {&atom_a, &t1}}};
    EXPECT_EQ(print(&list), "[a, b, c, d]");
}

TEST_F(ExprPrinterTest, PrintUnaryFunctor) {
    expect_atom_named(2, atom_a_name);
    std::string f_name{"f"};
    expect_atom_named(6, f_name);
    expr f{expr::functor{6, {&atom_a}}};
    EXPECT_EQ(print(&f), "f(a)");
}

TEST_F(ExprPrinterTest, PrintBinaryFunctor) {
    expect_atom_named(2, atom_a_name);
    expect_atom_named(3, atom_b_name);
    std::string f_name{"f"};
    expect_atom_named(6, f_name);
    expr f{expr::functor{6, {&atom_a, &atom_b}}};
    EXPECT_EQ(print(&f), "f(a, b)");
}

TEST_F(ExprPrinterTest, PrintTernaryFunctor) {
    expect_atom_named(2, atom_a_name);
    expect_atom_named(3, atom_b_name);
    std::string c_name{"c"};
    expr c{expr::functor{4, {}}};
    expect_atom_named(4, c_name);
    std::string h_name{"h"};
    expect_atom_named(7, h_name);
    expr h{expr::functor{7, {&atom_a, &atom_b, &c}}};
    EXPECT_EQ(print(&h), "h(a, b, c)");
}

TEST_F(ExprPrinterTest, PrintNestedFunctorArgs) {
    expect_atom_named(2, atom_a_name);
    std::string g_name{"g"};
    std::string f_name{"f"};
    expect_atom_named(8, g_name);
    expect_atom_named(6, f_name);
    expr g{expr::functor{8, {&atom_a}}};
    expr f{expr::functor{6, {&g}}};
    EXPECT_EQ(print(&f), "f(g(a))");
}

TEST_F(ExprPrinterTest, PrintAtomViaAtomNames) {
    std::string abc_name{"abc"};
    expr atom{expr::functor{10, {}}};
    expect_atom_named(10, abc_name);
    EXPECT_EQ(print(&atom), "abc");
}

TEST_F(ExprPrinterTest, PrintListViaConsIds) {
    expect_atom_named(2, atom_a_name);
    expr list{expr::functor{k_cons_atom_id, {&atom_a, &nil}}};
    EXPECT_EQ(print(&list), "[a]");
}
