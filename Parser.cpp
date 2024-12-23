#include "Parser.h"

#include "Ast.h"
#include "Exception.h"
#include "Lexxer.h"

std::unique_ptr<Node> Parser::Parse() {
    auto iElementsNode = std::make_unique<ElementsNode>();
    while (miLexxer->PeekToken().Type() != TokenType::EndOfFile)
        iElementsNode->Add(ParseElement());
    return iElementsNode;
}

auto Parser::ParseElement() -> std::unique_ptr<Node> {
    const auto introToken = miLexxer->PopToken();
    if (introToken.Type() == TokenType::Identifier && introToken.Text() == "fun")
        return ParseFunDef(introToken);
    throw Exception{"Unexpected token", introToken};
}

auto Parser::ParseFunDef(const Token& introToken) -> std::unique_ptr<Node> {
    // 'fun' is already popped and in introToken.

    const auto& funId = miLexxer->PopToken();
    if (funId.Type() != TokenType::Identifier)
        throw Exception{"Unexpected token. Expected function identifier.", funId};

    const auto& equalSymbol = miLexxer->PopToken();
    if (!(equalSymbol.Type() == TokenType::Symbol && equalSymbol.Text() == "="))
        throw Exception{"Unexpected token. Expected '='.", equalSymbol};    

    auto&& iBody = ParseTopLevelExpr();
    return std::make_unique<FunDefNode>(introToken.Text().begin(), funId.Text(), move(iBody));
}

auto Parser::ParseExprSeq() -> std::unique_ptr<Node>{
    const auto& openParen = miLexxer->PopToken();
    if (!(openParen.Type() == TokenType::Symbol && openParen.Text() == "("))
        throw Exception{"Unexpected token. Expected '('.", openParen};
    
    auto&& iSeq = std::make_unique<ExprSeqNode>();
    while (true)
    {
        iSeq->Add(ParseTopLevelExpr());

        const auto& iDelimiter = miLexxer->PopToken();
        if (iDelimiter.Type() == TokenType::Symbol) {
            if (iDelimiter.Text() == ";")
                continue;
            if (iDelimiter.Text() == ")")
                break;
        }
        if (iDelimiter.Type() == TokenType::EndOfFile)
            break;
        throw Exception{"Unexpected token. Expected ';' or ')'.", iDelimiter};
    }
    return iSeq;
}

auto Parser::ParseTopLevelExpr() -> std::unique_ptr<Node> {
    auto&& iLeft = ParsePrecedence1Expr();
    while (true) {
        const auto& binaryOp = miLexxer->PeekToken();
        if (binaryOp.Type() == TokenType::Identifier && binaryOp.Text() == "+") {
            miLexxer->PopToken();
            auto&& iRight = ParsePrecedence1Expr();
            auto&& iFunCall = std::make_unique<FunCallNode>(binaryOp.Text().begin(), binaryOp.Text());
            iFunCall->AddArgument(std::move(iLeft));
            iFunCall->AddArgument(std::move(iRight));
            iLeft = std::move(iFunCall);
        }
        else
            break;
    }
    return iLeft;
}

auto Parser::ParsePrecedence1Expr() -> std::unique_ptr<Node> {
    auto&& iLeft = ParsePrimaryExpr();
    while (true) {
        const auto& binaryOp = miLexxer->PeekToken();
        if (binaryOp.Type() == TokenType::Identifier && binaryOp.Text() == "*") {
            miLexxer->PopToken();
            auto&& iRight = ParsePrimaryExpr();
            auto&& iFunCall = std::make_unique<FunCallNode>(binaryOp.Text().begin(), binaryOp.Text());
            iFunCall->AddArgument(std::move(iLeft));
            iFunCall->AddArgument(std::move(iRight));
            iLeft = std::move(iFunCall);
        }
        else
            break;
    }
    return iLeft;
}

auto Parser::ParsePrimaryExpr() -> std::unique_ptr<Node>{
    const auto& peekToken = miLexxer->PeekToken();
    if (peekToken.Type() == TokenType::Symbol && peekToken.Text() == "(")
        return ParseExprSeq();
    if (peekToken.Type() == TokenType::IntegerLiteral) {
        miLexxer->PopToken();
        return std::make_unique<LiteralIntegerNode>(peekToken.Text().begin(), peekToken.IntegerValue().value());
    }
    if (peekToken.Type() == TokenType::Identifier) {
        miLexxer->PopToken();
        return std::make_unique<IdentifierNode>(peekToken.Text().begin(), peekToken.Text());
    }
    throw Exception{"Unexpected token. Expected a primary expression.", peekToken};
}