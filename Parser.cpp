#include "Parser.h"

#include "Lexxer.h"

#include <stdexcept>

std::unique_ptr<Node> Parser::Parse() {
    while (true) {
        auto def = ParseDef();
    }
}

auto Parser::ParseDef() -> std::unique_ptr<Node> {
    const auto& iFunDef = miLexxer->PopToken();
    if (!(iFunDef->Type() == TokenType::Identifier && iFunDef->Text() == "fun"))
        throw std::logic_error{"Unexpected token"};

    const auto& iFunId = miLexxer->PopToken();
    if (iFunId->Type() != TokenType::Identifier)
        throw std::logic_error{"Unexpected token"};

    const auto& iEqual = miLexxer->PopToken();
    if (!(iEqual->Type() == TokenType::Symbol && iEqual->Text() == "=="))
        throw std::logic_error{"Unexpected token"};    

    auto&& iBody = ParseTopLevelExpr();
    return std::make_unique<FunDefNode>(iFunDef->Text().begin(), iFunId->Text(), move(iBody));
}

auto Parser::ParseExprSeq() -> std::unique_ptr<Node>{
    const auto& iOpenParen = miLexxer->PopToken();
    if (!(iOpenParen->Type() == TokenType::Symbol && iOpenParen->Text() == "("))
        throw std::logic_error{"Unexpected token"};
    
    auto&& iSeq = std::make_unique<ExprSeqNode>();
    while (true)
    {
        iSeq->Add(ParseTopLevelExpr());

        const auto& iDelimiter = miLexxer->PopToken();
        if (iDelimiter->Type() != TokenType::Symbol)
            throw std::logic_error{"Unexpected token"};
        if (iDelimiter->Text() == ";")
            continue;
        if (iDelimiter->Text() == ")")
            break;
        throw std::logic_error{"Unexpected token"};
    }
    return iSeq;
}

auto Parser::ParseTopLevelExpr() -> std::unique_ptr<Node> {

    auto&& iLeft = ParsePrecedence1Expr();

    while (true)
    {
        const auto& iBinaryOp = miLexxer->PeekToken();
        if (iBinaryOp->Type() == TokenType::Identifier && iBinaryOp->Text() == "+")
        {
            miLexxer->PopToken();
            auto&& iRight = ParsePrecedence1Expr();
            auto&& iFunCall = std::make_unique<FunCallNode>(iBinaryOp->Text().begin(), iBinaryOp->Text());
            iFunCall->AddArgument(std::move(iLeft));
            iFunCall->AddArgument(std::move(iRight));

            iLeft = std::move(iFunCall);
        }
    }
    return iLeft;
}

auto Parser::ParsePrecedence1Expr() -> std::unique_ptr<Node> {
  
    auto&& iLeft = ParsePrimaryExpr();

    const auto& iBinaryOp = miLexxer->PeekToken();
    if (iBinaryOp->Type() == TokenType::Identifier && iBinaryOp->Text() == "*")
    {
        miLexxer->PopToken();
        auto&& iRight = ParsePrimaryExpr();
        auto&& iFunCall = std::make_unique<FunCallNode>(iBinaryOp->Text().begin(), iBinaryOp->Text());
        iFunCall->AddArgument(std::move(iLeft));
        iFunCall->AddArgument(std::move(iRight));
        return iFunCall;
    }
    else
    {
        return iLeft;
    }
}

auto Parser::ParsePrimaryExpr() -> std::unique_ptr<Node>{
    const auto& iPeek = miLexxer->PeekToken();
    if (iPeek->Type() == TokenType::Symbol && iPeek->Text() == "(")
        return ParseExprSeq();
    if (iPeek->Type() == TokenType::IntegerLiteral) {
        miLexxer->PopToken();
        return std::make_unique<LiteralIntegerNode>(iPeek->Text().begin(), iPeek->IntegerValue().value());
    }
    if (iPeek->Type() == TokenType::Identifier) {
        miLexxer->PopToken();
        return std::make_unique<IdentifierNode>(iPeek->Text().begin(), iPeek->Text());
    }
    throw std::logic_error{"Unexpected token"};
}
