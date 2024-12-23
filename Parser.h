#pragma once
#include <memory>
#include <string>

class Lexxer;

class Node {
public:
    virtual ~Node() = default;
};

class FunDefNode : public Node {
public:
    FunDefNode(std::string_view::iterator pos, std::string_view id, std::unique_ptr<Node> body);
};

class FunCallNode : public Node {
public:
    FunCallNode(std::string_view::iterator pos, std::string_view calleeId);
    auto AddArgument(std::unique_ptr<Node>) -> void;
};

class ExprSeqNode : public Node {
public:
    ExprSeqNode();
    auto Add(std::unique_ptr<Node>) -> void;
};

class LiteralIntegerNode : public Node {
public:
    LiteralIntegerNode(std::string_view::iterator pos, Integer value);
};

class IdentifierNode : public Node {
public:
    IdentifierNode(std::string_view::iterator pos, std::string_view id);
};


class Parser {
public:
    Parser(std::unique_ptr<Lexxer>);

    auto Parse() -> std::unique_ptr<Node>;

private:
    auto ParseDef() -> std::unique_ptr<Node>;
    auto ParseExprSeq() -> std::unique_ptr<Node>;
    auto ParseTopLevelExpr() -> std::unique_ptr<Node>;
    auto ParsePrecedence1Expr() -> std::unique_ptr<Node>;
    auto ParsePrimaryExpr() -> std::unique_ptr<Node>;

    std::unique_ptr<Lexxer> miLexxer;
};
