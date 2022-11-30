#include "statement.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
const string EMPTY_OBJECT = "None"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure &closure, Context &context) {
    closure[var_] = rv_->Execute(closure, context);
    return closure.at(var_);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    : var_{std::move(var)}, rv_{std::move(rv)} {
}

VariableValue::VariableValue(const std::string& var_name)
    : var_name_{var_name} {
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) {
    if (auto size = dotted_ids.size(); size > 0) {
        var_name_ = std::move(dotted_ids.at(0));
        tail_.resize(size - 1);
        std::move(std::next(dotted_ids.begin()), dotted_ids.end(), tail_.begin());
    }
}

ObjectHolder VariableValue::Execute(Closure &closure, Context &context) {
    if (closure.count(var_name_)) {
        auto result = closure.at(var_name_);
        if (tail_.size() > 0) {
            if (auto obj = result.TryAs<runtime::ClassInstance>()) {
                return VariableValue(tail_).Execute(obj->Fields(), context);
            } else {
                throw std::runtime_error("Variable " + var_name_+ " is not class"s);
            }
        }
        return result;
    } else {
        throw std::runtime_error("Variable "s + var_name_ + " not found"s);
    }
}

unique_ptr<Print> Print::Variable(const std::string &name) {
    return std::make_unique<Print>(std::make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    : args_{std::move(args)} {
}

ObjectHolder Print::Execute(Closure &closure, Context &context) {
    bool first = true;
    auto &os = context.GetOutputStream();
    for (auto &arg : args_) {
        if (!first) {
            os << " "s;
        }
        auto obj = arg->Execute(closure, context);
        if (obj) {
            obj->Print(os, context);
        } else {
            os << EMPTY_OBJECT;
        }
        first = false;
    }
    os << "\n"s;
    return {};
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args)
    : object_{std::move(object)}, method_{std::move(method)}, args_{std::move(args)} {
}

ObjectHolder MethodCall::Execute(Closure &closure, Context &context) {

    if (auto class_instance =
        object_->Execute(closure, context).TryAs<runtime::ClassInstance>()) {
        std::vector<runtime::ObjectHolder> actual_args;
        for (auto &arg : args_) {
            actual_args.push_back(arg->Execute(closure, context));
        }
        return class_instance->Call(method_, actual_args, context);
    } else {
        throw std::runtime_error("Object is not class instance"s);
    }
}

ObjectHolder Stringify::Execute(Closure &closure, Context &context) {
    auto obj = argument_->Execute(closure, context);
    if (obj) {
        std::ostringstream os;
        obj->Print(os, context);
        return ObjectHolder::Own(runtime::String(os.str()));
    } else {
        return ObjectHolder::Own(runtime::String(EMPTY_OBJECT));
    }
}

#define BINARY_OPERATION(type, operation) {                                        \
    auto l = left_holder.TryAs<type>();                                            \
    auto r = right_holder.TryAs<type>();                                           \
    if (l && r) {                                                                  \
        return ObjectHolder::Own(type(l->GetValue() operation r->GetValue()));     \
    }                                                                              \
}

ObjectHolder Add::Execute(Closure &closure, Context &context)
{
    ObjectHolder left_holder = lhs_->Execute(closure, context);
    ObjectHolder right_holder = rhs_->Execute(closure, context);
    BINARY_OPERATION(runtime::Number, +);
    BINARY_OPERATION(runtime::String, +);
    if (auto left_class = left_holder.TryAs<runtime::ClassInstance>()) {
        return left_class->Call(ADD_METHOD, {right_holder}, context);
    }
    throw std::runtime_error("Can add only numbers, strings and class instances with "s + ADD_METHOD);
}

ObjectHolder Sub::Execute(Closure &closure, Context &context)
{
    ObjectHolder left_holder = lhs_->Execute(closure, context);
    ObjectHolder right_holder = rhs_->Execute(closure, context);
    BINARY_OPERATION(runtime::Number, -);
    throw std::runtime_error("Can subtract only numbers"s);
}

ObjectHolder Mult::Execute(Closure &closure, Context &context)
{
    ObjectHolder left_holder = lhs_->Execute(closure, context);
    ObjectHolder right_holder = rhs_->Execute(closure, context);
    BINARY_OPERATION(runtime::Number, *);
    throw std::runtime_error("Can multiply only numbers"s);
}

ObjectHolder Div::Execute(Closure &closure, Context &context)
{
    ObjectHolder left_holder = lhs_->Execute(closure, context);
    ObjectHolder right_holder = rhs_->Execute(closure, context);
    auto right = right_holder.TryAs<runtime::Number>();
    if (right && right->GetValue() == 0) {
        throw runtime_error("Division by zero"s);
    }
    BINARY_OPERATION(runtime::Number, /);
    throw std::runtime_error("Can divide only numbers"s);
}

#undef BINARY_OPERATION

ObjectHolder Compound::Execute(Closure &closure, Context &context) {
    for (auto &arg : args_) {
        arg->Execute(closure, context);
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure &closure, Context &context) {
    throw statement_->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    : cls_{std::move(cls)} {
}

ObjectHolder ClassDefinition::Execute(Closure &closure, [[maybe_unused]] Context &context) {
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return ObjectHolder::None();
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv)
    : object_{std::move(object)}, field_name_{std::move(field_name)}, rv_{std::move(rv)} {
}

ObjectHolder FieldAssignment::Execute(Closure &closure, Context &context) {
    auto obj = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
    if (obj) {
        return obj->Fields()[field_name_] = rv_->Execute(closure, context);
    } else {
        throw runtime_error("Object is not class!"s);
    }
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body)
    : condition_{std::move(condition)}
    , if_body_{std::move(if_body)}
    , else_body_{std::move(else_body)} {
}

ObjectHolder IfElse::Execute(Closure &closure, Context &context) {
    if (runtime::IsTrue(condition_->Execute(closure, context))) {
        return if_body_->Execute(closure, context);
    } else if (else_body_) {
        return else_body_->Execute(closure, context);
    }
     return runtime::ObjectHolder::None();
}

ObjectHolder Or::Execute(Closure &closure, Context &context) {
    if (runtime::IsTrue(lhs_->Execute(closure, context)))
        {
            return ObjectHolder::Own(runtime::Bool(true));
        }
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue
                                               (rhs_->Execute(closure, context))));
}

ObjectHolder And::Execute(Closure &closure, Context &context) {
    if (runtime::IsTrue(lhs_->Execute(closure, context)))
        {
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue
                                               (rhs_->Execute(closure, context))));
        }
        return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure &closure, Context &context) {
    bool result = !runtime::IsTrue(argument_->Execute(closure, context));
    return ObjectHolder::Own(runtime::Bool(result));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)) {
    cmp_ = std::move(cmp);
}

ObjectHolder Comparison::Execute(Closure &closure, Context &context) {
    auto result = cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context);
    return runtime::ObjectHolder::Own(runtime::Bool(result));
}

NewInstance::NewInstance(const runtime::Class& class_,
                         std::vector<std::unique_ptr<Statement>> args)
    : class_instance_{class_}, args_{std::move(args)} {
}

NewInstance::NewInstance(const runtime::Class &class_)
    : NewInstance{class_, {}} {
}

ObjectHolder NewInstance::Execute(Closure &closure, Context &context) {
    if (class_instance_.HasMethod(INIT_METHOD, args_.size())) {
        std::vector<runtime::ObjectHolder> actual_args;
        for (auto &arg : args_) {
            actual_args.push_back(arg->Execute(closure, context));
        }
        class_instance_.Call(INIT_METHOD, actual_args, context);
    }
    return runtime::ObjectHolder::Share(class_instance_);
}

MethodBody::MethodBody(std::unique_ptr<Statement> &&body)
    : body_{std::move(body)} {
}

ObjectHolder MethodBody::Execute(Closure &closure, Context &context) {
    try {
        body_->Execute(closure, context);
        return runtime::ObjectHolder::None();
    }  catch (runtime::ObjectHolder &result) {
        return result;
    }  catch (...) {
        throw;
    }
}

}  // namespace ast
