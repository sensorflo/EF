#pragma once
#include <memory>

class Lexxer;

class Node
{

};

class Parser
{
public:
    Parser(std::unique_ptr<Lexxer>);

    std::unique_ptr<Node> Parse();

private:
    std::unique_ptr<Lexxer> miLexxer;
};
