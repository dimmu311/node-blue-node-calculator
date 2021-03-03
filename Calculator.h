#ifndef MATH_H_
#define MATH_H_

#include <homegear-node/NodeFactory.h>
#include <homegear-node/INode.h>

class MyFactory : Flows::NodeFactory {
public:
    Flows::INode *createNode(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected) override;
};

extern "C" Flows::NodeFactory *getFactory();

namespace Calculator {

class Calculator : public Flows::INode {
public:
    Calculator(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected);
    ~Calculator() override;
    bool init(const Flows::PNodeInfo &info) override;
    void input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message) override;
private:
    std::string _formula;
    std::vector<uint32_t> _usedInputs;
    std::map<uint32_t,std::string> _values;

    std::mutex _valuesMutex;

    bool calculateBracket(std::string &formula);
    bool calculateMultiplication(std::string &formula);
    bool calculateDivision(std::string &formula);
    bool calculateAddition(std::string &formula);
    bool calculateSubtraction(std::string &formula);
    bool calculate(std::string &operation, double &value);
};

}
#endif