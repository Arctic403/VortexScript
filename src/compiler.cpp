#include "vortex_script/compiler.hpp"

#include "internal.hpp"
#include "vortex_script/limits.hpp"

#include <cctype>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace vortex_script {
namespace {

struct TransparentHash final {
    using is_transparent = void;
    [[nodiscard]] std::size_t operator()(const std::string_view value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
    [[nodiscard]] std::size_t operator()(const std::string& value) const noexcept {
        return (*this)(std::string_view{value});
    }
};

struct TransparentEqual final {
    using is_transparent = void;
    [[nodiscard]] bool operator()(const std::string_view a, const std::string_view b) const noexcept {
        return a == b;
    }
};

class Interner final {
public:
    explicit Interner(std::vector<std::string>& strings) : strings_(strings) {}

    [[nodiscard]] bool intern(const std::string_view value, StringId& id, Diagnostic& diagnostic, const Span span) {
        const auto found = index_.find(value);
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
    std::unordered_map<std::string, StringId, TransparentHash, TransparentEqual> index_;
    std::size_t bytes_ = 0;
};

struct IndexedCommand final {
    const CommandSpec* spec = nullptr;
    std::unordered_map<std::string_view, const ArgumentSpec*> arguments;
};

struct SchemaIndex final {
    std::unordered_map<std::string_view, IndexedCommand> commands;
};

[[nodiscard]] bool isIdentifier(const std::string_view value) noexcept {
    if (value.empty() || value.size() > limits::kMaxIdentifierBytes) return false;
    const auto start = static_cast<unsigned char>(value.front());
    if (!(std::isalpha(start) != 0 || start == '_')) return false;
    for (const char raw : value.substr(1)) {
        const auto c = static_cast<unsigned char>(raw);
        if (!(std::isalnum(c) != 0 || c == '_')) return false;
    }
    return true;
}

[[nodiscard]] bool isNumberLexeme(const std::string_view value) noexcept {
    if (value.empty() || value.size() > limits::kMaxNumberBytes) return false;
    std::size_t i = 0;
    if (value[i] == '-') {
        ++i;
        if (i == value.size()) return false;
    }
    const auto digit = [](const char c) noexcept { return c >= '0' && c <= '9'; };
    if (!digit(value[i])) return false;
    while (i < value.size() && digit(value[i])) ++i;
    if (i < value.size() && value[i] == '.') {
        ++i;
        if (i == value.size() || !digit(value[i])) return false;
        while (i < value.size() && digit(value[i])) ++i;
    }
    if (i < value.size() && (value[i] == 'e' || value[i] == 'E')) {
        ++i;
        if (i < value.size() && (value[i] == '+' || value[i] == '-')) ++i;
        if (i == value.size() || !digit(value[i])) return false;
        while (i < value.size() && digit(value[i])) ++i;
    }
    return i == value.size();
}

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

[[nodiscard]] bool buildSchemaIndex(const CommandSchema& schema, SchemaIndex& index, Diagnostic& diagnostic) {
    if (schema.contractVersion != kEngineContractVersion) {
        diagnostic = Diagnostic{DiagnosticCode::SchemaVersionMismatch, {}, "command schema engine-contract version mismatch"};
        return false;
    }
    if (schema.commands.size() > limits::kMaxCommands) {
        diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "command schema exceeds command limit"};
        return false;
    }
    index.commands.reserve(schema.commands.size());
    for (const auto& command : schema.commands) {
        if (!isIdentifier(command.name)) {
            diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "command schema contains an invalid command name"};
            return false;
        }
        if (command.arguments.size() > limits::kMaxArgumentsPerCommand) {
            diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "command schema exceeds argument-per-command limit"};
            return false;
        }
        IndexedCommand indexed;
        indexed.spec = &command;
        indexed.arguments.reserve(command.arguments.size());
        for (const auto& argument : command.arguments) {
            if (!isIdentifier(argument.name)) {
                diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "command schema contains an invalid argument name"};
                return false;
            }
            if (argument.comparisons == ComparisonMask::None) {
                diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "command schema argument allows no comparison form"};
                return false;
            }
            if (argument.constrainEntityKind && argument.type != ValueType::EntityRef) {
                diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "entity-kind constraint requires EntityRef argument type"};
                return false;
            }
            if (!indexed.arguments.emplace(argument.name, &argument).second) {
                diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "command schema contains a duplicate argument name"};
                return false;
            }
        }
        if (!index.commands.emplace(command.name, std::move(indexed)).second) {
            diagnostic = Diagnostic{DiagnosticCode::InvalidSchema, {}, "command schema contains a duplicate command name"};
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool validateProgramAgainstSchema(const Program& program, const SchemaIndex& index, Diagnostic& diagnostic) {
    for (const auto& transaction : program.transactions) {
        for (const auto& command : transaction.commands) {
            const auto commandFound = index.commands.find(command.name);
            if (commandFound == index.commands.end()) {
                diagnostic = Diagnostic{DiagnosticCode::UnknownCommand, command.span, "unknown command '" + command.name + "'"};
                return false;
            }
            const IndexedCommand& indexed = commandFound->second;
            std::unordered_set<std::string_view> seen;
            seen.reserve(command.arguments.size());
            for (const auto& argument : command.arguments) {
                if (!seen.insert(argument.name).second) {
                    diagnostic = Diagnostic{DiagnosticCode::DuplicateArgument, argument.span, "duplicate argument '" + argument.name + "'"};
                    return false;
                }
                const auto argumentFound = indexed.arguments.find(argument.name);
                if (argumentFound == indexed.arguments.end()) {
                    diagnostic = Diagnostic{DiagnosticCode::UnknownArgument, argument.span, "unknown argument '" + argument.name + "'"};
                    return false;
                }
                const ArgumentSpec& argumentSpec = *argumentFound->second;
                if (argumentSpec.type != valueType(argument.value)) {
                    diagnostic = Diagnostic{DiagnosticCode::TypeMismatch, argument.span, "argument '" + argument.name + "' has wrong value type"};
                    return false;
                }
                if (!contains(argumentSpec.comparisons, maskFor(argument.op))) {
                    diagnostic = Diagnostic{DiagnosticCode::InvalidComparison, argument.span, "comparison operator is not allowed for argument '" + argument.name + "'"};
                    return false;
                }
                if (argumentSpec.constrainEntityKind && std::holds_alternative<EntityRefLiteral>(argument.value)) {
                    const auto& entity = std::get<EntityRefLiteral>(argument.value);
                    if (entity.kind != argumentSpec.entityKind) {
                        diagnostic = Diagnostic{DiagnosticCode::TypeMismatch, argument.span, "entity reference kind does not match command schema"};
                        return false;
                    }
                }
            }
            for (const auto& argumentSpec : indexed.spec->arguments) {
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

[[nodiscard]] bool validStringId(const Plan& plan, const StringId id) noexcept {
    return static_cast<std::size_t>(id) < plan.strings.size();
}

} // namespace

std::vector<Diagnostic> validateCommandSchema(const CommandSchema& schema) {
    SchemaIndex index;
    Diagnostic diagnostic;
    if (!buildSchemaIndex(schema, index, diagnostic)) return {std::move(diagnostic)};
    return {};
}

std::vector<Diagnostic> validatePlan(const Plan& plan) {
    if (plan.engineContractVersion != kEngineContractVersion) {
        return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan engine-contract version mismatch"}};
    }
    if (plan.strings.size() > limits::kMaxInternedStrings || plan.transactions.size() > limits::kMaxTransactions) {
        return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan exceeds structural limits"}};
    }
    std::size_t stringBytes = 0;
    for (const auto& string : plan.strings) {
        stringBytes += string.size();
        if (string.size() > limits::kMaxStringBytes || stringBytes > limits::kMaxInternedStringBytes) {
            return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan string table exceeds limits"}};
        }
    }

    std::size_t commandCount = 0;
    std::size_t argumentCount = 0;
    for (const auto& transaction : plan.transactions) {
        if (!validStringId(plan, transaction.name) || !isIdentifier(plan.strings[transaction.name])) {
            return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan contains invalid transaction name"}};
        }
        commandCount += transaction.commands.size();
        if (commandCount > limits::kMaxCommands) {
            return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan exceeds command limit"}};
        }
        for (const auto& command : transaction.commands) {
            if (!validStringId(plan, command.name) || !isIdentifier(plan.strings[command.name])) {
                return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan contains invalid command name"}};
            }
            if (command.arguments.size() > limits::kMaxArgumentsPerCommand) {
                return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan exceeds argument-per-command limit"}};
            }
            argumentCount += command.arguments.size();
            if (argumentCount > limits::kMaxArgumentsTotal) {
                return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan exceeds total argument limit"}};
            }
            for (const auto& argument : command.arguments) {
                if (!validStringId(plan, argument.name) || !isIdentifier(plan.strings[argument.name])) {
                    return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan contains invalid argument name"}};
                }
                bool valueValid = true;
                if (const auto* number = std::get_if<NumberText>(&argument.value)) {
                    valueValid = validStringId(plan, number->text) && isNumberLexeme(plan.strings[number->text]);
                } else if (const auto* string = std::get_if<StringValue>(&argument.value)) {
                    valueValid = validStringId(plan, string->text) && plan.strings[string->text].size() <= limits::kMaxStringBytes;
                } else if (const auto* identifier = std::get_if<IdentifierValue>(&argument.value)) {
                    valueValid = validStringId(plan, identifier->text) && isIdentifier(plan.strings[identifier->text]);
                } else if (const auto* entity = std::get_if<EntityRefValue>(&argument.value)) {
                    valueValid = entity->id != kInvalidPersistentId;
                }
                if (!valueValid) {
                    return {Diagnostic{DiagnosticCode::InvalidPlan, {}, "plan contains an invalid value reference"}};
                }
            }
        }
    }
    return {};
}

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
    if (schema != nullptr) {
        SchemaIndex index;
        if (!buildSchemaIndex(*schema, index, diagnostic)) {
            result.diagnostics.push_back(std::move(diagnostic));
            return result;
        }
        if (!validateProgramAgainstSchema(program, index, diagnostic)) {
            result.diagnostics.push_back(std::move(diagnostic));
            return result;
        }
    }
    if (!lower(program, result.plan, diagnostic)) {
        result.diagnostics.push_back(std::move(diagnostic));
        return result;
    }
    result.diagnostics = validatePlan(result.plan);
    return result;
}

} // namespace vortex_script
