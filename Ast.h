#pragma once
#include "AstFwd.h"
#include "ExchangeTypes.h"

#include <memory>
#include <string>

class Node {
public:
    virtual ~Node() = default;
};

class ElementsNode : public Node {
public:
    ElementsNode();
    auto Add(std::unique_ptr<Node>) -> void;
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
