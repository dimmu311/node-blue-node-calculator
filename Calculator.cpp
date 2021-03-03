#include "Calculator.h"
#include <regex>

Flows::INode *MyFactory::createNode(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected) {
    return new Calculator::Calculator(path, type, frontendConnected);
}

Flows::NodeFactory *getFactory() {
    return (Flows::NodeFactory *)(new MyFactory);
}

namespace Calculator {

Calculator::Calculator(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected) : Flows::INode(path, type, frontendConnected) {
}
Calculator::~Calculator() = default;

bool Calculator::init(const Flows::PNodeInfo &info) {
    try {
        auto settingsIterator = info->info->structValue->find("name");
        if (settingsIterator != info->info->structValue->end()) _out->setNodeId(settingsIterator->second->stringValue);

        settingsIterator = info->info->structValue->find("formula");
        if (settingsIterator == info->info->structValue->end()) return false;
        _formula = settingsIterator->second->stringValue;

        _usedInputs.reserve(4);

        std::string formula = _formula;
        std::smatch match;
        while(regex_search(formula, match, std::regex(R"([$]([0-9]))"))){
            uint32_t input = stoi(match[1].str());
            if(std::find(_usedInputs.begin(), _usedInputs.end(), input) == _usedInputs.end()) _usedInputs.push_back(input);
            formula = match.suffix();
        }
        return true;
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

void Calculator::input(const Flows::PNodeInfo &info, uint32_t index, const Flows::PVariable &message) {
    try {
        //check if this index is used in the given formula
        if (std::find(_usedInputs.begin(), _usedInputs.end(), index) == _usedInputs.end()) {
            _out->printDebug("Input with index " + std::to_string(index) + " is not used in the given formula");
            return;
        }
        //check if the VariableType is a number
        if (message->structValue->at("payload")->type != Flows::VariableType::tFloat &&
            message->structValue->at("payload")->type != Flows::VariableType::tInteger &&
            message->structValue->at("payload")->type != Flows::VariableType::tInteger64) {

            _out->printDebug("VariableType must be of type int or float");
            return;
        }
        //save the new value
        std::lock_guard<std::mutex> valuesGuard(_valuesMutex);
        {
            std::string value;
            if (message->structValue->at("payload")->type == Flows::VariableType::tFloat)
                value = std::to_string(message->structValue->at("payload")->floatValue);
            else if (message->structValue->at("payload")->type == Flows::VariableType::tInteger)
                value = std::to_string(message->structValue->at("payload")->integerValue);
            else if (message->structValue->at("payload")->type == Flows::VariableType::tInteger64)
                value = std::to_string(message->structValue->at("payload")->integerValue64);
            else
                return;

            if (_values.find(index) == _values.end()) _values.emplace(std::pair(index, value));
            else _values.at(index) = value;
        }

        //check if all needed inputs have a value
        for (auto i: _usedInputs) {
            if (_values.find(i) == _values.end()) {
                _out->printDebug("Not all values of the given formula are present");
                return;
            }
        }

        //replace the $input with the given value
        std::string formula = _formula;
        for (auto &value:_values) {
            std::string regString = ("[$][");
            regString.append(std::to_string(value.first));
            regString.append("]");

            formula = std::regex_replace(formula, std::regex(regString), value.second);
        }

        //follow the rules () before */ before +-
        std::smatch match;
        while (regex_search(formula, match, std::regex(R"(\*|\/|\+|\-|\(|\))"))) {
            _out->printDebug("Formula -> " + formula);

            if(!calculateBracket(formula))break;
            if(!calculateMultiplication(formula))break;
            if(!calculateDivision(formula))break;
            if(!calculateAddition(formula))break;
            if(!calculateSubtraction(formula))break;
        }

        _out->printDebug("Result -> " + formula);
        double result = std::stod(formula);
        Flows::PVariable newMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        newMessage->structValue->emplace("payload", std::make_shared<Flows::Variable>(result));
        output(0, newMessage);
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}
bool Calculator::calculateBracket(std::string &formula){
    try {
        double result;
        std::smatch match;

        while(regex_search(formula, match, std::regex(R"(\((((\d+\.\d+)|\d+)[\*|\/|\+|\-]((\d+\.\d+)|\d+))\))"))) {
            std::string operation = match.str();
            std::string tempOperation(operation.begin() + 1, operation.end()-1); //remove () at the beginning and the end, so that the calc would work

            if(!calculate(tempOperation,result)) return false;
            tempOperation.insert(0,1,'(');
            tempOperation.insert (0, 1, '\\');
            tempOperation.push_back('\\');
            tempOperation.push_back(')');
            formula = std::regex_replace(formula, std::regex(tempOperation), std::to_string(result));

            _out->printDebug("CalculateBracket -> " + tempOperation.append(" results in ").append(formula));
        }
        return true;
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        return false;
    }
}

bool Calculator::calculateMultiplication(std::string &formula){
    try {
        double result;
        std::smatch match;

        while(regex_search(formula, match, std::regex(R"((((\d+\.\d+)|\d+)\*((\d+\.\d+)|\d+)))"))) {
            std::string operation = match.str();
            if(!calculate(operation,result)) return false;
            formula = std::regex_replace(formula, std::regex(operation), std::to_string(result));
            _out->printDebug("CalculateMultiplication -> " + operation.append(" results in ").append(formula));
            if(!calculateBracket(formula)) return false;
        }
        return true;
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        return false;
    }
}
bool Calculator::calculateDivision(std::string &formula){
    try {
        double result;
        std::smatch match;
        while(regex_search(formula, match, std::regex(R"((((\d+\.\d+)|\d+)\/((\d+\.\d+)|\d+)))"))) {
            std::string operation = match.str();
            if(!calculate(operation,result)) return false;
            formula = std::regex_replace(formula, std::regex(operation), std::to_string(result));
            _out->printDebug("CalculateDivision -> " + operation.append(" results in ").append(formula));
            if(!calculateBracket(formula)) return false;
        }
        return true;
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        return false;
    }
}
bool Calculator::calculateAddition(std::string &formula){
    try {
        double result;
        std::smatch match;
        while(regex_search(formula, match, std::regex(R"((((\d+\.\d+)|\d+)\+((\d+\.\d+)|\d+)))"))) {
            std::string operation = match.str();
            if(!calculate(operation,result)) return false;
            formula = std::regex_replace(formula, std::regex(operation), std::to_string(result));
            _out->printDebug("CalculateAddition -> " + operation.append(" results in ").append(formula));
            if(!calculateBracket(formula)) return false;
        }
        return true;
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        return false;
    }
}
bool Calculator::calculateSubtraction(std::string &formula){
    try {
        double result;
        std::smatch match;
        while(regex_search(formula, match, std::regex (R"((((\d+\.\d+)|\d+)\-((\d+\.\d+)|\d+)))"))) {
            std::string operation = match.str();
            if(!calculate(operation,result)) return true;
            formula = std::regex_replace(formula, std::regex(operation), std::to_string(result));
            _out->printDebug("CalculateSubtraction -> " + operation.append(" results in ").append(formula));
            if(!calculateBracket(formula)) return false;
        }
        return false;
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        return false;
    }
}

bool Calculator::calculate(std::string &operation, double &value) {
    try {
        std::stringstream oSS(operation);
        double a, b;
        char op;
        oSS >> a >> op >> b;
        switch (op) {
            case '*':
                value = a * b;
                operation = std::regex_replace(operation, std::regex(R"(\*)"), "\\*");
                return true;
            case '/':
                value = a / b;
                operation = std::regex_replace(operation, std::regex(R"(\/)"), "\\/");
                return true;
            case '+':
                value = a + b;
                operation = std::regex_replace(operation, std::regex(R"(\+)"), "\\+");
                return true;
            case '-':
                value = a - b;
                operation = std::regex_replace(operation, std::regex(R"(\-)"), "\\-");
                return true;
            default:
                return false;
        }
    }
    catch (const std::exception &ex) {
        _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        return false;
    }
}

}