#ifndef BRICKRED_COMMAND_LINE_OPTION_H
#define BRICKRED_COMMAND_LINE_OPTION_H

#include <map>
#include <string>
#include <vector>

namespace brickred {

class CommandLineOption final {
public:
    enum class ParameterType {
        NONE = 0,
        REQUIRED
    };

    using ArgumentVector = std::vector<std::string>;
    using ParameterVector = std::vector<std::string>;
    using OptionParameterTypeMap = std::map<std::string, ParameterType>;
    using OptionParametersMap = std::map<std::string, ParameterVector>;

    CommandLineOption();
    ~CommandLineOption();

    void addOption(const std::string &opt,
                   ParameterType param_type = ParameterType::NONE);
    bool parse(int argc, char *argv[]);

    bool hasOption(const std::string &opt) const;
    const std::string &getParameter(const std::string &opt) const;
    const ParameterVector &getParameters(const std::string &opt) const;
    const ArgumentVector &getLeftArguments() const { return left_args_; }

private:
    OptionParameterTypeMap option_types_;
    OptionParametersMap option_params_;
    ArgumentVector left_args_;
};

} // namespace brickred

#endif
