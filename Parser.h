#pragma once
#include <memory>
#include <string>

class Lexxer;
class Node;
class Token;

class Parser {
public:
    Parser(std::unique_ptr<Lexxer>);

    auto Parse() -> std::unique_ptr<Node>;

private:
    auto ParseElement() -> std::unique_ptr<Node>;
    auto ParseFunDef(const Token& introToken) -> std::unique_ptr<Node>;
    auto ParseExprSeq() -> std::unique_ptr<Node>;
    auto ParseTopLevelExpr() -> std::unique_ptr<Node>;
    auto ParsePrecedence1Expr() -> std::unique_ptr<Node>;
    auto ParsePrimaryExpr() -> std::unique_ptr<Node>;

    std::unique_ptr<Lexxer> miLexxer;
};
