#include "internal.hpp"

#include "vortex_script/limits.hpp"

#include <charconv>
#include <cstdint>
#include <string>

namespace vortex_script::detail {
namespace {

class Parser final {
public:
    Parser(const std::vector<Token>& tokens, Diagnostic& diagnostic) : tokens_(tokens), diagnostic_(diagnostic) {}

    [[nodiscard]] bool run(Program& program) {
        while (!at(TokenKind::Eof)) {
            if (program.transactions.size() >= limits::kMaxTransactions) return limit("transaction limit exceeded");
            TransactionDecl transaction;
            if (!parseTransaction(transaction)) return false;
            totalCommands_ += transaction.commands.size();
            if (totalCommands_ > limits::kMaxCommands) return limit("command limit exceeded");
            program.transactions.push_back(std::move(transaction));
        }
        return true;
    }

private:
    [[nodiscard]] bool parseTransaction(TransactionDecl& out) {
        const std::size_t start = index_;
        if (!expectKeyword("transaction")) return false;
        if (!expectIdentifier(out.name)) return false;
        if (!expect(TokenKind::LBrace, "expected '{' after transaction name")) return false;
        while (!at(TokenKind::RBrace)) {
            if (at(TokenKind::Eof)) return error(DiagnosticCode::ExpectedToken, current().span, "expected '}' to close transaction");
            CommandDecl command;
            if (!parseCommand(command)) return false;
            out.commands.push_back(std::move(command));
            if (out.commands.size() > limits::kMaxCommands) return limit("command limit exceeded");
        }
        const Span end = current().span;
        ++index_;
        out.span = Span{tokens_[start].span.start, end.end};
        return true;
    }

    [[nodiscard]] bool parseCommand(CommandDecl& out) {
        const Span start = current().span;
        if (!expectKeyword("command")) return false;
        if (!expectIdentifier(out.name)) return false;
        if (!expect(TokenKind::LBrace, "expected '{' after command name")) return false;
        while (!at(TokenKind::RBrace)) {
            if (at(TokenKind::Eof)) return error(DiagnosticCode::ExpectedToken, current().span, "expected '}' to close command");
            if (out.arguments.size() >= limits::kMaxArgumentsPerCommand) return limit("argument-per-command limit exceeded");
            Argument argument;
            if (!parseArgument(argument)) return false;
            ++totalArguments_;
            if (totalArguments_ > limits::kMaxArgumentsTotal) return limit("total argument limit exceeded");
            out.arguments.push_back(std::move(argument));
        }
        const Span end = current().span;
        ++index_;
        out.span = Span{start.start, end.end};
        return true;
    }

    [[nodiscard]] bool parseArgument(Argument& out) {
        const Span start = current().span;
        if (!expectIdentifier(out.name)) return false;
        if (at(TokenKind::Equal)) { out.op = CompareOp::Equal; ++index_; }
        else if (at(TokenKind::LessEqual)) { out.op = CompareOp::LessEqual; ++index_; }
        else out.op = CompareOp::Set;

        if (!parseLiteral(out.value)) return false;
        if (at(TokenKind::Semicolon)) ++index_;
        out.span = Span{start.start, previous().span.end};
        return true;
    }

    [[nodiscard]] bool parseLiteral(Literal& out) {
        const Token token = current();
        if (token.kind == TokenKind::Number) {
            out = NumberLiteral{token.text};
            ++index_;
            return true;
        }
        if (token.kind == TokenKind::String) {
            out = StringLiteral{token.text};
            ++index_;
            return true;
        }
        if (token.kind == TokenKind::Identifier) {
            if (token.text == "true" || token.text == "false") {
                out = BoolLiteral{token.text == "true"};
                ++index_;
                return true;
            }
            if (index_ + 2 < tokens_.size() && tokens_[index_ + 1].kind == TokenKind::Colon &&
                tokens_[index_ + 2].kind == TokenKind::Number) {
                EntityKind kind{};
                if (!parseEntityKind(token.text, kind)) {
                    return error(DiagnosticCode::InvalidEntityRef, token.span, "unknown entity reference kind");
                }
                const Token& number = tokens_[index_ + 2];
                std::uint64_t id = 0;
                const char* begin = number.text.data();
                const char* end = begin + number.text.size();
                const auto parsed = std::from_chars(begin, end, id, 10);
                if (parsed.ec != std::errc{} || parsed.ptr != end || id == kInvalidPersistentId) {
                    return error(DiagnosticCode::InvalidEntityRef, number.span, "entity reference must be a non-zero unsigned 64-bit integer");
                }
                out = EntityRefLiteral{kind, id};
                index_ += 3;
                return true;
            }
            out = IdentifierLiteral{token.text};
            ++index_;
            return true;
        }
        return error(DiagnosticCode::ExpectedLiteral, token.span, "expected a literal value");
    }

    [[nodiscard]] bool expectKeyword(const std::string_view keyword) {
        if (!at(TokenKind::Identifier) || current().text != keyword) {
            return error(DiagnosticCode::ExpectedToken, current().span, "expected '" + std::string(keyword) + "'");
        }
        ++index_;
        return true;
    }

    [[nodiscard]] bool expectIdentifier(std::string& out) {
        if (!at(TokenKind::Identifier)) return error(DiagnosticCode::ExpectedIdentifier, current().span, "expected identifier");
        out = current().text;
        ++index_;
        return true;
    }

    [[nodiscard]] bool expect(const TokenKind kind, const char* message) {
        if (!at(kind)) return error(DiagnosticCode::ExpectedToken, current().span, message);
        ++index_;
        return true;
    }

    [[nodiscard]] bool limit(const char* message) {
        return error(DiagnosticCode::LimitExceeded, current().span, message);
    }

    [[nodiscard]] bool error(const DiagnosticCode code, const Span span, std::string message) {
        diagnostic_ = Diagnostic{code, span, std::move(message)};
        return false;
    }

    [[nodiscard]] bool at(const TokenKind kind) const noexcept { return current().kind == kind; }
    [[nodiscard]] const Token& current() const noexcept { return tokens_[index_]; }
    [[nodiscard]] const Token& previous() const noexcept { return tokens_[index_ == 0 ? 0 : index_ - 1]; }

    const std::vector<Token>& tokens_;
    Diagnostic& diagnostic_;
    std::size_t index_ = 0;
    std::size_t totalCommands_ = 0;
    std::size_t totalArguments_ = 0;
};

} // namespace

bool parse(const std::vector<Token>& tokens, Program& program, Diagnostic& diagnostic) {
    program = {};
    if (tokens.empty()) {
        diagnostic = Diagnostic{DiagnosticCode::ExpectedToken, {}, "token stream is empty"};
        return false;
    }
    return Parser(tokens, diagnostic).run(program);
}

} // namespace vortex_script::detail
