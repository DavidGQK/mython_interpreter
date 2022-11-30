#pragma once

#include "runtime.h"

#include <functional>

namespace ast {

using Statement = runtime::Executable;

// An expression that returns a value of type T,
// is used as the basis for creating constants
template <typename T>
class ValueStatement : public Statement {
public:
    explicit ValueStatement(T v)
        : value_(std::move(v)) {
    }

    runtime::ObjectHolder Execute(runtime::Closure& /*closure*/,
                                  runtime::Context& /*context*/) override {
        return runtime::ObjectHolder::Share(value_);
    }

private:
    T value_;
};

using NumericConst = ValueStatement<runtime::Number>;
using StringConst = ValueStatement<runtime::String>;
using BoolConst = ValueStatement<runtime::Bool>;

/*
Calculates the value of a variable or a chain of calls of object fields id1.id2.id3.
For example, the expression circle.center.x is a chain of calls of object fields in the instruction:
x = circle.center.x
*/
class VariableValue : public Statement {
public:
    explicit VariableValue(const std::string& var_name);
    explicit VariableValue(std::vector<std::string> dotted_ids);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::string var_name_;
    std::vector<std::string> tail_;
};

// Assigns the value of the expression rv to the variable whose name is given in the var parameter
class Assignment : public Statement {
public:
    Assignment(std::string var, std::unique_ptr<Statement> rv);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::string var_;
    std::unique_ptr<Statement> rv_;
};

// Assigns the value of the expression rv to the object.field_name field
class FieldAssignment : public Statement {
public:
    FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    VariableValue object_;
    std::string field_name_;
    std::unique_ptr<Statement> rv_;
};

// Value None
class None : public Statement {
public:
    runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure& closure,
                                  [[maybe_unused]] runtime::Context& context) override {
        return {};
    }
};

// Print command
class Print : public Statement {
public:
    // Initializes the print command to print the value of the argument expression
    explicit Print(std::unique_ptr<Statement> argument);
    // Initializes the print command to print a list of args values
    explicit Print(std::vector<std::unique_ptr<Statement>> args);

    // Initializes the print command to print the value of the name variable
    static std::unique_ptr<Print> Variable(const std::string& name);

    // During the execution of the print command, the output must be in the stream returned from
    // context.GetOutputStream()
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::vector<std::unique_ptr<Statement>> args_;
};

// Calls method object.method with parameter list args
class MethodCall : public Statement {
public:
    MethodCall(std::unique_ptr<Statement> object, std::string method,
               std::vector<std::unique_ptr<Statement>> args);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::unique_ptr<Statement> object_;
    std::string method_;
    std::vector<std::unique_ptr<Statement>> args_;
};

/*
Creates a new instance of class_ by passing a set of parameters args to its constructor.
If there is no __init__ method with specified number of arguments,
then the class instance is created without a constructor call (the object fields will not be initialized):

class Person:
  def set_name(name):
    self.name = name

p = Person()
# The name field will only have a value after calling the set_name method
p.set_name("Ivan")
*/
class NewInstance : public Statement {
public:
    explicit NewInstance(const runtime::Class& class_);
    NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args);
    // Returns an object containing a value of type ClassInstance
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    runtime::ClassInstance class_instance_;
    std::vector<std::unique_ptr<Statement>> args_;
};

// Base class for unary operations
class UnaryOperation : public Statement {
public:
    explicit UnaryOperation(std::unique_ptr<Statement> argument)
        : argument_{std::move(argument)} {
    }
protected:
    std::unique_ptr<Statement> argument_;
};

// The str operation, which returns the string value of its argument
class Stringify : public UnaryOperation {
public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Parent class Binary operation with lhs and rhs arguments
class BinaryOperation : public Statement {
public:
    BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs)
        : lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {
    }
protected:
    std::unique_ptr<Statement> lhs_, rhs_;
};

// Returns the result of the + operation on the lhs and rhs arguments
class Add : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    // Addition is supported:
    // number + number
    // string + string
    // object1 + object2, if object1 has custom class with _add__(rhs) method
    // otherwise, runtime_error is thrown during calculation
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Returns the result of subtracting the lhs and rhs arguments
class Sub : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    // Subtraction is supported:
    // number - number
    // If lhs and rhs are not numbers, a runtime_error exception is thrown
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Returns the result of multiplying the lhs and rhs arguments
class Mult : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    // Multiplication is supported:
    // number * number
    // If lhs and rhs are not numbers, a runtime_error exception is thrown
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Returns the result of division of lhs and rhs
class Div : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    // Division is supported:
    // number / number
    // If lhs and rhs are not numbers, a runtime_error exception is thrown
    // If rhs is 0, a runtime_error exception is thrown
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Returns the result of calculating the logical operation or over lhs and rhs
class Or : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;
    // The value of the argument rhs is calculated only if the value of lhs
    // after being cast to Bool is False
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Returns the result of calculating the logical operation and over lhs and rhs
class And : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;
    // The value of the argument rhs is calculated only if the value of lhs
    // after converting to Bool is True
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Returns the result of calculating the logical operation not over the single argument of the operation
class Not : public UnaryOperation {
public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

// Component instruction (for example: method body, contents of if branch, or else)
class Compound : public Statement {
public:
    // Constructs Compound from several instructions of type unique_ptr<Statement>
    template <typename... Args>
    explicit Compound(Args&&... args) {
        AddStatement(std::forward<Args>(args)...);
    }
    // Adds the next instruction to the end of a compound instruction
    void AddStatement(std::unique_ptr<Statement> stmt) {
        args_.push_back(std::move(stmt));
    }

    // Executes the added instructions sequentially. Returns None
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::vector<std::unique_ptr<Statement>> args_;

    template <typename... Args>
    void AddStatement(std::unique_ptr<Statement> stmt, Args&&... args) {
        args_.push_back(std::move(stmt));
        AddStatement(std::forward<Args>(args)...);
    }
    void AddStatement() {
    }
};

// The body of the method. As a rule, it contains a compound instruction
class MethodBody : public Statement {
public:
    explicit MethodBody(std::unique_ptr<Statement>&& body);

    // Calculates the instruction passed as body.
    // If the return instruction has been executed inside the body, it returns result return
    // otherwise, it returns None
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::unique_ptr<Statement> body_;
};

// Executes the return instruction with a statement
class Return : public Statement {
public:
    explicit Return(std::unique_ptr<Statement> statement)
        : statement_{std::move(statement)} {
    }

    // Stops execution of the current method. After the return instruction has been executed, the method,
    // within which it was executed, must return the result of calculating the statement expression.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::unique_ptr<Statement> statement_;
};

// Declares class
class ClassDefinition : public Statement {
public:
    // It is guaranteed that ObjectHolder contains an object of type runtime::Class
    explicit ClassDefinition(runtime::ObjectHolder cls);

    // Creates a new object inside the class that matches the name of the class and the value passed in the
    // constructor
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    runtime::ObjectHolder cls_;
};

// Instruction if <condition> <if_body> else <else_body>
class IfElse : public Statement {
public:
    // The else_body parameter can be nullptr
    IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
           std::unique_ptr<Statement> else_body);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::unique_ptr<Statement> condition_, if_body_, else_body_;
};

// Comparison operation
class Comparison : public BinaryOperation {
public:
    // Comparator defines a function that compares the values of the arguments
    using Comparator = std::function<bool(const runtime::ObjectHolder&,
                                          const runtime::ObjectHolder&, runtime::Context&)>;

    Comparison(Comparator cmp, std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs);

    // Calculates the value of lhs and rhs expressions and returns the result of the comparator,
    // reduced to runtime::Bool type
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    Comparator cmp_;
};

}  // namespace ast
