#include "vortex_script/compiler.hpp"

#include "internal.hpp"
#include "vortex_script/limits.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace vortex_script {
namespace {

class Interner final {
public:
    explicit Interner(std::vector<std::string>& strings) : strings_(strings) {}

    [[nodiscard]] bool intern(const std::string_view value, StringId& id, Diagnostic& diagnostic, const Span span) {
        const auto found = index_.find(std::string(value));
        if (found != index_.end()) { id = found->second; return true; }
        if (strings_.size() >= limits::kMaxInternedStrings || bytes_ + value.size() > limits::kMaxInternedStringBytes) {
            diagnostic = Diagnostic{DiagnosticCode::LimitExceeded, span, "interned string table limit exceeded"};
            return false;
        }
        id = static_cast<StringId>(strings_.size());
        strings_.emplace_back(value);
        bytes_ += value.size();
        index_.emplace(strings_.back(), id);
        return true;
    }

private:
    std::vector<std::string>& strings_;
    std::unordered_map<std::string, StringId> index_;
    std::size_t bytes_ = 0;
};

[[nodiscard]] ComparisonMask maskFor(const CompareOp op) noexcept {
    switch (op) {
        case CompareOp::Set: return ComparisonMask::Set;
        case CompareOp::Equal: return ComparisonMask::Equal;
        case CompareOp::LessEqual: return ComparisonMask::LessEqual;
    }
    return ComparisonMask::None;
}

[[nodiscard]] ValueType valueType(const Literal& literal) noexcept {
    if (std::holds_alternative<NumberLiteral>(literal)) return ValueType::Number;
    if (std::holds_alternative<StringLiteral>(literal)) return ValueType::String;
    if (std::holds_alternative<BoolLiteral>(literal)) return ValueType::Bool;
    if (std::holds_alternative<EntityRefLiteral>(literal)) return ValueType::EntityRef;
    return ValueType::Identifier;
}

[[nodiscard]] const ArgumentSpec* findArgument(const CommandSpec& spec, const std::string_view name) noexcept {
    for (const auto& argument : spec.arguments) if (argument.name == name) return &argument;
    return nullptr;
}

[[nodiscard]] bool validateSchema(const Program& program, const CommandSchema& schema, Diagnostic& diagnostic) {
    if (schema.contractVersion != kEngineContractVersion) {
        diagnostic = Diagnostic{DiagnosticCode::SchemaVersionMismatch, {}, "command schema engine-contract version mismatch"};
        return false;
    }
    for (const auto& transaction : program.transactions) {
        for (const auto& command : transaction.commands) {
            const CommandSpec* commandSpec = schema.findCommand(command.name);
            if (commandSpec == nullptr) {
                diagnostic = Diagnostic{DiagnosticCode::UnknownCommand, command.span, "unknown command '" + command.name + "'"};
                return false;
            }
            std::unordered_set<std::string_view> seen;
            for (const auto& argument : command.arguments) {
                if (!seen.insert(argument.name).second) {
                    diagnostic = Diagnostic{DiagnosticCode::DuplicateArgument, argument.span, "duplicate argument '" + argument.name + "'"};
                    return false;
                }
                const ArgumentSpec* argumentSpec = findArgument(*commandSpec, argument.name);
                if (argumentSpec == nullptr) {
                    diagnostic = Diagnostic{DiagnosticCode::UnknownArgument, argument.span, "unknown argument '" + argument.name + "'"};
                    return false;
                }
                if (argumentSpec->type != valueType(argument.value)) {
                    diagnostic = Diagnostic{DiagnosticCode::TypeMismatch, argument.span, "argument '" + argument.name + "' has wrong value type"};
                    return false;
                }
                if (!contains(argumentSpec->comparisons, maskFor(argument.op))) {
                    diagnostic = Diagnostic{DiagnosticCode::InvalidComparison, argument.span, "comparison operator is not allowed for argument '" + argument.name + "'"};
                    return false;
                }
                if (argumentSpec->constrainEntityKind && std::holds_alternative<EntityRefLiteral>(argument.value)) {
                    const auto& entity = std::get<EntityRefLiteral>(argument.value);
                    if (entity.kind != argumentSpec->entityKind) {
                        diagnostic = Diagnostic{DiagnosticCode::TypeMismatch, argument.span, "entity reference kind does not match command schema"};
                        return false;
                    }
                }
            }
            for (const auto& argumentSpec : commandSpec->arguments) {
                if (argumentSpec.required && seen.find(argumentSpec.name) == seen.end()) {
                    diagnostic = Diagnostic{DiagnosticCode::MissingArgument, command.span, "missing required argument '" + argumentSpec.name + "'"};
                    return false;
                }
            }
        }
    }
    return true;
}

[[nodiscard]] bool lowerLiteral(const Literal& literal, const Span span, Interner& interner,
                                PlannedValue& output, Diagnostic& diagnostic) {
    StringId id = 0;
    if (const auto* number = std::get_if<NumberLiteral>(&literal)) {
        if (!interner.intern(number->lexeme, id, diagnostic, span)) return false;
        output = NumberText{id};
    } else if (const auto* string = std::get_if<StringLiteral>(&literal)) {
        if (!interner.intern(string->value, id, diagnostic, span)) return false;
        output = StringValue{id};
    } else if (const auto* boolean = std::get_if<BoolLiteral>(&literal)) {
        output = boolean->value;
    } else if (const auto* identifier = std::get_if<IdentifierLiteral>(&literal)) {
        if (!interner.intern(identifier->value, id, diagnostic, span)) return false;
        output = IdentifierValue{id};
    } else {
        const auto& entity = std::get<EntityRefLiteral>(literal);
        output = EntityRefValue{entity.kind, entity.id};
    }
    return true;
}

[[nodiscard]] bool lower(const Program& program, Plan& plan, Diagnostic& diagnostic) {
    Interner interner(plan.strings);
    for (const auto& transaction : program.transactions) {
        PlannedTransaction plannedTransaction;
        if (!interner.intern(transaction.name, plannedTransaction.name, diagnostic, transaction.span)) return false;
        for (const auto& command : transaction.commands) {
            PlannedCommand plannedCommand;
            if (!interner.intern(command.name, plannedCommand.name, diagnostic, command.span)) return false;
            for (const auto& argument : command.arguments) {
                PlannedArgument plannedArgument;
                plannedArgument.op = argument.op;
                if (!interner.intern(argument.name, plannedArgument.name, diagnostic, argument.span)) return false;
                if (!lowerLiteral(argument.value, argument.span, interner, plannedArgument.value, diagnostic)) return false;
                plannedCommand.arguments.push_back(std::move(plannedArgument));
            }
            plannedTransaction.commands.push_back(std::move(plannedCommand));
        }
        plan.transactions.push_back(std::move(plannedTransaction));
    }
    return true;
}

} // namespace

CompileResult compile(const std::string_view source, const CommandSchema* schema) {
    CompileResult result;
    std::vector<Token> tokens;
    Diagnostic diagnostic;
    if (!detail::lex(source, tokens, diagnostic)) {
        result.diagnostics.push_back(std::move(diagnostic));
        return result;
    }
    Program program;
    if (!detail::parse(tokens, program, diagnostic)) {
        result.diagnostics.push_back(std::move(diagnostic));
        return result;
    }
    if (schema != nullptr && !validateSchema(program, *schema, diagnostic)) {
        result.diagnostics.push_back(std::move(diagnostic));
        return result;
    }
    if (!lower(program, result.plan, diagnostic)) {
        result.diagnostics.push_back(std::move(diagnostic));
    }
    return result;
}

} // namespace vortex_script
