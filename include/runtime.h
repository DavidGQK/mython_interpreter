#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace runtime {

// Mython Instruction Execution Context
class Context {
public:
    // Returns the output stream for print commands
    virtual std::ostream& GetOutputStream() = 0;

protected:
    ~Context() = default;
};

// A base class for all Mython objects
class Object {
public:
    virtual ~Object() = default;
    // outputs its representation as a string to the os
    virtual void Print(std::ostream &os, Context &context) = 0;
};

// A special wrapper class for storing an object in a Mython program
class ObjectHolder {
public:
    // Creates an empty value
    ObjectHolder() = default;

    // Returns an ObjectHolder that owns an object of type T
    // The T type is a specific successor class to Object.
    // object is copied or moved to the heap
    template <typename T>
    [[nodiscard]] static ObjectHolder Own(T &&object) {
        return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
    }

    // Creates an ObjectHolder that does not own the object (analogous to a weak reference)
    [[nodiscard]] static ObjectHolder Share(Object &object);
    // Creates an empty ObjectHolder corresponding to the value None
    [[nodiscard]] static ObjectHolder None();

    // Returns a reference to an Object inside the ObjectHolder.
    // ObjectHolder must be non-empty
    Object& operator*() const;

    Object* operator->() const;

    [[nodiscard]] Object* Get() const;

    // Returns a pointer to an object of type T or nullptr if the ObjectHolder does not store
    // object of this type
    template <typename T>
    [[nodiscard]] T* TryAs() const {
        return dynamic_cast<T*>(this->Get());
    }

    // Returns true if ObjectHolder is not empty
    explicit operator bool() const;

private:
    explicit ObjectHolder(std::shared_ptr<Object> data);
    void AssertIsValid() const;

    std::shared_ptr<Object> data_;
};

// A value object storing a value of type T
template <typename T>
class ValueObject : public Object {
public:
    ValueObject(T v)  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        : value_(v) {
    }

    void Print(std::ostream &os, [[maybe_unused]] Context &context) override {
        os << value_;
    }

    [[nodiscard]] const T& GetValue() const {
        return value_;
    }

private:
    T value_;
};

// A table of symbols linking the object name with its value
using Closure = std::unordered_map<std::string, ObjectHolder>;

// Checks if object contains a value, which is cast to True
// For non-zero numbers, True and non-empty strings returns true. In other cases it returns false.
bool IsTrue(const ObjectHolder &object);

// Interface to perform actions on Mython objects
class Executable {
public:
    virtual ~Executable() = default;
    // Performs an action on the objects inside the closure, using the context
    // Returns the resulting value either None
    virtual ObjectHolder Execute(Closure &closure, Context &context) = 0;
};

// String value
using String = ValueObject<std::string>;
// Numerical value
using Number = ValueObject<int>;

// Logical value
class Bool : public ValueObject<bool> {
public:
    using ValueObject<bool>::ValueObject;
    void Print(std::ostream& os, Context& context) override;
};

// Class method
struct Method {
    // Method name
    std::string name;
    // Names of formal method parameters
    std::vector<std::string> formal_params;
    // The body of the method
    std::unique_ptr<Executable> body;
};

// Class
class Class : public Object {
public:
    // Creates a class with the name name and a set of methods, inherited from parent class
    // If parent is nullptr, a base class is created
    explicit Class(std::string name, std::vector<Method> methods, const Class* parent);

    // Returns a pointer to the method name or nullptr if there is no method with that name
    [[nodiscard]] const Method* GetMethod(const std::string& name) const;

    // Returns the class name
    [[nodiscard]] const std::string& GetName() const;

    // Outputs the string "Class <class name>" to the os, for example "Class cat"
    void Print(std::ostream& os, Context& context) override;
private:
    std::string name_;
    std::vector<Method> methods_;
    const Class* parent_;
};

// A class instance
class ClassInstance : public Object {
public:
    explicit ClassInstance(const Class& cls);

    /*
     * If the object has a __str__ method, outputs the result returned by this method to os.
     * Otherwise it outputs the address of the object to os.
     */
    void Print(std::ostream& os, Context& context) override;

    /*
     * Calls the method method from the object, passing the actual_args parameters to it.
     * The context parameter sets the context for executing the method.
     * If neither the class nor its parents contain a method method, the method throws an exception
     * runtime_error
     */
    ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                      Context& context);

    // Returns true if the object has a method that accepts argument_count parameters
    [[nodiscard]] bool HasMethod(const std::string& method, size_t argument_count) const;

    // Returns a reference to the Closure containing the object fields
    [[nodiscard]] Closure& Fields();
    // Returns a constant reference to the Closure containing the object fields
    [[nodiscard]] const Closure& Fields() const;

    void PrintClass(std::ostream& os, Context& context) {
        const_cast<Class*>(&cls_)->Print(os, context);
    }

private:
    const Class &cls_;
    Closure closure_;
};

/*
 * Returns true if lhs and rhs contain the same numbers, strings or values of type Bool.
 * If lhs is an object with __eq__ method, the function returns the result of calling lhs.__eq__(rhs),
 * reduced to the Bool type. If lhs and rhs are None, the function returns true.
 * Otherwise the function throws a runtime_error exception.
 *
 * The context parameter sets the context for executing the __eq__ method
 */
bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);

/*
 * If lhs and rhs are numbers, strings or bool values, the function returns the result of their comparison
 * with the < operator.
 * If lhs is an object with __lt__ method, it returns the result of calling lhs.__lt__(rhs),
 * reduced to the bool type. In other cases the function throws a runtime_error exception.
 *
 * The context parameter sets the context for executing the __lt__ method
 */
bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
// Returns the value opposite Equal(lhs, rhs, context)
bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
// Returns lhs>rhs using the Equal and Less functions
bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
// Returns lhs<=rhs using the Equal and Less functions
bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
// Returns the value opposite Less(lhs, rhs, context)
bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);

// A stub context, used in tests.
// In this context all output is redirected to the output string
struct DummyContext : Context {
    std::ostream& GetOutputStream() override {
        return output;
    }

    std::ostringstream output;
};

// A simple context, in which the output is output to the output stream passed to the constructor
class SimpleContext : public runtime::Context {
public:
    explicit SimpleContext(std::ostream& output)
        : output_(output) {
    }

    std::ostream& GetOutputStream() override {
        return output_;
    }

private:
    std::ostream& output_;
};

}  // namespace runtime
