#include "runtime.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace {
const string STR_METHOD = "__str__"s;
const string EQ_METHOD = "__eq__"s;
const string LESS_METHOD = "__lt__"s;

template<class BinaryPredicate>
bool Comp(const runtime::ObjectHolder &lhs, const runtime::ObjectHolder &rhs, BinaryPredicate pred) {
    {
        auto l = lhs.TryAs<runtime::Bool>();
        auto r = rhs.TryAs<runtime::Bool>();
        if (l && r) {
            return pred(l->GetValue(), r->GetValue());
        }
    }

    {
        auto l = lhs.TryAs<runtime::Number>();
        auto r = rhs.TryAs<runtime::Number>();
        if (l && r) {
            return pred(l->GetValue(), r->GetValue());
        }
    }

    {
        auto l = lhs.TryAs<runtime::String>();
        auto r = rhs.TryAs<runtime::String>();
        if (l && r) {
            return pred(l->GetValue(), r->GetValue());
        }
    }
    throw std::runtime_error("Cannot compare objects"s);
}
}  // namespace

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Return a non-possessing shared_ptr (its deleter does nothing)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder &object) {
    if (auto obj = object.TryAs<Bool>()) {
        return obj->GetValue() == true;
    }
    if (auto obj = object.TryAs<Number>()) {
        return !(obj->GetValue() == 0);
    }
    if (auto obj = object.TryAs<String>()) {
        return !(obj->GetValue().empty());
    }
    return false;
}

void ClassInstance::Print(std::ostream &os, Context &context) {
    if (HasMethod(STR_METHOD, 0U)) {
        Call(STR_METHOD, {}, context)->Print(os, context);
    } else {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string &method, size_t argument_count) const {
    if (auto mtd = cls_.GetMethod(method)) {
        if (mtd->formal_params.size() == argument_count) {
            return true;
        }
    }
    return false;
}

Closure& ClassInstance::Fields() {
    return closure_;
}

const Closure& ClassInstance::Fields() const {
    return closure_;
}

ClassInstance::ClassInstance(const Class &cls) : cls_(cls) {
}

ObjectHolder ClassInstance::Call(const std::string &method,
                                 const std::vector<ObjectHolder> &actual_args,
                                 Context& context) {
    if (!HasMethod(method, actual_args.size())) {
        throw std::runtime_error("No method "s + method +" in class "s + cls_.GetName()
                                 + " with "s + std::to_string(actual_args.size()) + " arguments."s);
    }

    auto mtd = cls_.GetMethod(method);
    Closure args;
    args["self"s] = ObjectHolder::Share(*this);

    size_t index = 0;
    for (auto &param : mtd->formal_params) {
        args[param] = actual_args.at(index++);
    }

    return mtd->body->Execute(args, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
    : name_{std::move(name)}, methods_{std::move(methods)}, parent_{parent} {
}

const Method* Class::GetMethod(const std::string &name) const {
    auto find_by_name = [&name](const Method &mtd){
        return mtd.name == name;
    };
    // first look in the list of methods of the class itself
    if(auto res = std::find_if(methods_.begin(), methods_.end(), find_by_name);
            res != methods_.end()) {
        return &(*res);
    } else {
        // If not, look for the parent (if there is one)
        if (parent_ != nullptr) {
            return parent_->GetMethod(name);
        } else {
            return nullptr;
        }
    }
}

[[nodiscard]] const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
    os << "Class "s << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    try {
        return Comp(lhs, rhs, std::equal_to());
    }  catch (std::runtime_error&) {
        if (auto l = lhs.TryAs<ClassInstance>()) {
            return IsTrue(l->Call(EQ_METHOD, {rhs}, context));
        }
        if (!lhs && !rhs) {
            return true;
        }
        throw;
    }
}

bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    try {
        return Comp(lhs, rhs, std::less());
    }  catch (std::runtime_error&) {
        if (auto l = lhs.TryAs<ClassInstance>()) {
            return IsTrue(l->Call(LESS_METHOD, {rhs}, context));
        }
        throw;
    }
}

bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !(Less(lhs, rhs, context) || Equal(lhs, rhs, context));
}

bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !Greater(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime
