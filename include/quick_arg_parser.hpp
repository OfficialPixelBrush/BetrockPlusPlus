#include <iostream>
#include <vector>
#include <sstream>
#include <memory>
#include <array>
#include <unordered_map>

#if __cplusplus > 201402L
#include <filesystem>
#include <optional>
#endif

namespace QuickArgParserInternals {

struct ArgumentError : std::runtime_error {
	using std::runtime_error::runtime_error;
};

template <typename T, typename SFINAE = void>
struct ArgConverter {
	constexpr static bool canDo = false;
};

template <typename T>
struct ArgConverter<T, typename std::enable_if<std::is_integral<T>::value>::type> {
	static T makeDefault() {
		return 0;
	}
	static T deserialise(const std::string& from) {
		return std::stoi(from);
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
	static T makeDefault() {
		return 0;
	}
	static T deserialise(const std::string& from) {
		return std::stof(from);
	}
	constexpr static bool canDo = true;
};

template <>
struct ArgConverter<std::string, void> {
	static std::string makeDefault() {
		return "";
	}
	static std::string deserialise(const std::string& from) {
		return from;
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<std::shared_ptr<T>, void> {
	static std::shared_ptr<T> makeDefault() {
		return nullptr;
	}
	static std::shared_ptr<T> deserialise(const std::string& from) {
		return std::make_shared<T>(ArgConverter<T>::deserialise(from));
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<std::unique_ptr<T>, void> {
	static std::unique_ptr<T> makeDefault() {
		return nullptr;
	}
	static std::unique_ptr<T> deserialise(const std::string& from) {
		return std::unique_ptr<T>(new T(ArgConverter<T>::deserialise(from)));
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<std::vector<T>, typename std::enable_if<ArgConverter<T>::canDo>::type> {
	static std::vector<T> makeDefault() {
		return {};
	}
	static std::vector<T> deserialise(const std::vector<std::string>& from) {
		std::vector<T> made;
		for (const std::string& part : from) {
			int lastPosition = 0;
			for (int i = 0; i < int(part.size()) + 1; i++) {
				if (part[i] == ',' || i == int(part.size())) {
					made.push_back(ArgConverter<T>::deserialise(
							std::string(part.begin() + lastPosition, part.begin() + i)));
					lastPosition = i + 1;
				}
			}
		}
		return made;
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<std::unordered_map<std::string, T>, typename std::enable_if<ArgConverter<T>::canDo>::type> {
	static std::unordered_map<std::string, T> makeDefault() {
		return {};
	}
	static std::unordered_map<std::string, T> deserialise(const std::vector<std::string>& from) {
		std::unordered_map<std::string, T> made;
		for (const std::string& part : from) {
			int lastPosition = 0;
			for (int i = 0; i < int(part.size()) + 1; i++) {
				if (part[i] == ',' || i == int(part.size())) {
					std::string section = std::string(part.begin() + lastPosition, part.begin() + i);
					auto separator = section.find('=');
					if (separator == std::string::npos)
						throw ArgumentError("Argument is expected to be a comma separated list of name-value pairs separated by '='");
					made[section.substr(0, separator)] = ArgConverter<T>::deserialise(section.substr(separator + 1));
					lastPosition = i + 1;
				}
			}
		}
		return made;
	}
	constexpr static bool canDo = true;
};

template <typename T>
class Optional {
	alignas(T) std::array<int8_t, sizeof(T)> contents;
	bool exists = false;
	void clear() {
		if (exists)
			operator*().~T();
	}
public:
	Optional() = default;
	Optional(std::nullptr_t) {}
#if __cplusplus > 201402L
	Optional(std::nullopt_t) {}
#endif
	Optional(const Optional& other) : exists(other.exists) {
		if (exists)
			new (operator->()) T(*other);
	}
	Optional(Optional&& other) : exists(other.exists) {
		if (exists)
			new (operator->()) T(*other); 
	}
	T& operator=(const T& other) {
		clear();
		if (exists) {
			operator*() = other;
		} else
			new (contents.data()) T(other);
		exists = true;
		return operator*();
	}
	T& operator=(T&& other) {
		clear();
		if (exists)
			operator*() = other;
		else
			new (contents.data()) T(other);
		exists = true;
		return operator*();
	}
	void operator=(std::nullptr_t) {
		clear();
		exists = false;
	}
#if __cplusplus > 201402L
	void operator=(std::nullopt_t) {
		operator=(nullptr);
	}
#endif
	T& operator*() {
		return *reinterpret_cast<T*>(contents.data());
	}
	const T& operator*() const {
		return *reinterpret_cast<const T*>(contents.data());
	}
	T* operator->() {
		return reinterpret_cast<T*>(contents.data());
	}
	const T* operator->() const {
		return reinterpret_cast<const T*>(contents.data());
	}
	operator bool() const {
		return exists;
	}
#if __cplusplus > 201402L
	operator std::optional<T>() {
		if (exists)
			return std::optional<T>(operator*());
		else
			return std::nullopt;
	}
#endif
	~Optional() {
		clear();
	}
};

template <typename T>
struct ArgConverter<Optional<T>, void> {
	static Optional<T> makeDefault() {
		return nullptr;
	}
	static Optional<T> deserialise(const std::string& from) {
		Optional<T> made;
		made = ArgConverter<T>::deserialise(from);
		return made;
	}
	constexpr static bool canDo = true;
};

#if __cplusplus > 201402L
template <>
struct ArgConverter<std::filesystem::path, void> {
	static std::filesystem::path makeDefault() {
		return {};
	}
	static std::filesystem::path deserialise(const std::string& from) {
		return std::filesystem::path(from);
	}
	constexpr static bool canDo = true;
};
#endif


template <typename T, typename SFINAE = void>
struct Demultiplexer {
	static T deserialise(const std::vector<std::string>& multiplexed) {
		return ArgConverter<T>::deserialise(multiplexed);
	}
};

template <typename T>
struct Demultiplexer<T, typename std::enable_if<!std::is_void<decltype(ArgConverter<T>::deserialise(std::declval<std::string>()))>::value>::type> {
	static T deserialise(const std::vector<std::string>& multiplexed) {
		if (multiplexed.size() > 1)
			throw ArgumentError("Argument was not expected to appear more than once (" + multiplexed[1] + " is excessive)");
		return ArgConverter<T>::deserialise(multiplexed[0]);
	}
};

template <typename T, typename SFINAE = void>
struct HelpProvider{
	template <typename F>
	static std::string get(const F& ifAbsent, const std::string& programName) {
		return ifAbsent(programName);
	}
};

template <typename T>
struct HelpProvider<T, typename std::enable_if<!std::is_void<decltype(T::help(std::declval<std::string>()))>::value>::type> {
	template <typename F>
	static std::string get(const F&, const std::string& programName) {
		return T::help(programName);
	}
};

template <typename T, typename SFINAE = void>
struct OnHelpCallback {
	template <typename F>
	static void on(T*, const F& ifAbsent) {
		ifAbsent();
	}
};

template <typename T>
struct OnHelpCallback<T, typename std::enable_if<std::is_void<decltype(std::declval<T>().onHelp())>::value>::type> {
	template <typename F>
	static void on(T* instance, const F&) {
		instance->onHelp();
	}	
};

template <typename T, typename SFINAE = void>
struct HasHelpOptionsProvider : std::false_type {};

template <typename T>
struct HasHelpOptionsProvider<T, typename std::enable_if<
		!std::is_void<decltype(T::options())>::value>::type>
		: std::true_type {};


template <typename T, typename SFINAE = void>
struct VersionPrinter {
	static bool print() {
		return false;
	}
};

template <typename T>
struct VersionPrinter<T, typename std::enable_if<!std::is_void<decltype(std::string(T::version))>::value>::type> {
	static bool print() {
		std::cout << T::version << std::endl;
		return true;
	}
};

template <typename T>
struct VersionPrinter<T, typename std::enable_if<!std::is_void<decltype(std::string(T::version()))>::value>::type> {
	static bool print() {
		std::cout << T::version() << std::endl;
		return true;
	}
};

template <typename T, typename SFINAE = void>
struct OnVersionCallback {
	template <typename F>
	static void on(T*, const F& ifAbsent) {
		ifAbsent();
	}
};

template <typename T>
struct OnVersionCallback<T, typename std::enable_if<std::is_void<decltype(std::declval<T>().onVersion())>::value>::type> {
	template <typename F>
	static void on(T* instance, const F&) {
		instance->onVersion();
	}	
};

#if _MSC_VER && !__INTEL_COMPILER
	// MSVC likes converting const char* literals to initialiser lists and causing ambiguous calls with it
	template <class T>
	struct IsInitializerList : std::false_type {};

	template <class T>
	struct IsInitializerList<std::initializer_list<T>> : std::true_type {};
	
	template <class T, typename SFINAE = void>
	struct StringFilterOk : std::false_type {};
	
	template <class T>
	struct StringFilterOk<T, typename std::enable_if<(std::is_class<T>::value && !IsInitializerList<T>::value) || std::is_arithmetic<T>::value
			|| std::is_floating_point<T>::value || std::is_enum<T>::value>::type> : std::true_type {};
#endif

struct DummyValidator{};

template <typename Validator, typename SFINAE = void>
struct ValidatorUser {
	template <typename Value>
	static bool useValidator(const Validator& validator, const Value& value) {
		return true;
	}
};

template <typename Validator>
struct ValidatorUser<Validator, typename std::enable_if<!std::is_same<Validator, DummyValidator>::value>::type> {
	template <typename Value, typename std::enable_if<std::is_same<bool, decltype(std::declval<Validator>()(std::declval<Value>()))>::value>::type* = nullptr>
	static bool useValidator(const Validator& validator, const Value& value) {
		return validator(value);
	}
	template <typename Value, typename std::enable_if<!std::is_same<bool, decltype(std::declval<Validator>()(std::declval<Value>()))>::value>::type* = nullptr>
	static bool useValidator(const Validator& validator, const Value& value) {
		validator(value);
		return true;
	}
};

} // namespace

template <typename Child>
class MainArguments {
	std::string m_programName;
	std::vector<std::string> m_argv;
	
	enum InitialisationStep {
		UNINITIALISED,
		INITIALISING,
		INITIALISED
	};
	struct Singleton {
		std::stringstream m_helpPreface;
		std::stringstream m_help;
		std::vector<std::pair<std::string, char>> m_nullarySwitches;
		std::vector<std::pair<std::string, char>> m_unarySwitches;
		std::vector<std::string> m_confusingSwitches; // nonstandard switches starting with a single dash
		int m_argumentCountMin = 0;
		int m_argumentCountMax = 0;
		InitialisationStep m_initialisationState = UNINITIALISED;
	};
	static Singleton& singleton() {
		static Singleton instance;
		return instance;
	}
	
	using DummyValidator = QuickArgParserInternals::DummyValidator;
public:
	template <typename T> using Optional = QuickArgParserInternals::Optional<T>;
	MainArguments() = default;
	MainArguments(int argc, char** argv) : m_programName(argv[0]), m_argv(argv + 1, argv + argc) {
		using namespace QuickArgParserInternals;
		if (singleton().m_initialisationState == UNINITIALISED) {
			// When first created, create temporarily another instance to explore what are the members
			singleton().m_initialisationState = INITIALISING;

			Child investigator;
			// This will fill the static variables
			singleton().m_helpPreface << QuickArgParserInternals::HelpProvider<Child>::get([] (const std::string& programName) {
				return programName + " takes between " + std::to_string(singleton().m_argumentCountMin) + " and " +
						std::to_string(singleton().m_argumentCountMax) + " arguments, plus these options:";
			}, m_programName);

			singleton().m_initialisationState = INITIALISED;
		}
		if (singleton().m_initialisationState == INITIALISED) {
			bool switchesEnabled = true;
			auto isListedAsChar = [] (const char arg, const std::vector<std::pair<std::string, char>>& switches) {
				for (const auto& it : switches) {
					if (it.second == arg)
						return true;
				}
				return false;
			};
			auto isListedAsString = [] (const std::string& arg, const std::vector<std::pair<std::string, char>>& switches, bool unary, bool& skipsNext) {
				for (const auto& it : switches) {
					for (int i = 0; i < int(it.first.size()); i++) {
						if (arg[i] != it.first[i])
							goto noMatch;
					}
					if (arg.size() == it.first.size()) {
						skipsNext = true;
						return true;
					}
					if (unary && arg[it.first.size()] == '=') {
						skipsNext = false;
						return true;
					}
					noMatch:;
				}
				return false;
			};
			auto printHelp = [this] () {
				std::cout << singleton().m_helpPreface.str() << std::endl;
				std::cout << singleton().m_help.str() << std::endl;
				
				QuickArgParserInternals::OnHelpCallback<Child>::on(static_cast<Child*>(this), [] { std::exit(0); });
			};
			auto printVersion = [this] () {
				if (!QuickArgParserInternals::VersionPrinter<Child>::print())
					return false; // Returns false if the version is not known, leading to no action if found
					
				QuickArgParserInternals::OnVersionCallback<Child>::on(static_cast<Child*>(this), [] { std::exit(0); });
				return true;
			};

			// Collect program arguments (as opposed to switches) and validate everything
			for (int i = 0; i < int(m_argv.size()); i++) {
				if (switchesEnabled) {
					if (m_argv[i] == "--help") {
						printHelp();
						goto nextArg;
					}
					if (m_argv[i] == "--version") {
						if (printVersion())
							goto nextArg;
					}
					if (m_argv[i] == "--") {
						switchesEnabled = false;
						goto nextArg;
					}
					bool skipsNext = false;
					if (isListedAsString(m_argv[i], singleton().m_unarySwitches, true, skipsNext)) {
						if (skipsNext)
							i++; // The next argument is part of the switch
						goto nextArg;
					} else if (isListedAsString(m_argv[i], singleton().m_nullarySwitches, false, skipsNext)) {
						goto nextArg;
					}
					
					
					if (m_argv[i][0] == '-') {
						if (m_argv[i][1] == '-')
							throw ArgumentError("Unknown switch " + m_argv[i]);
						
						// Starts with -
						if (m_argv[i].size() == 2) {
							// Is an argument of type -x
							if (m_argv[i][1] == '?') {
								printHelp();
									goto nextArg;
							}
							if (m_argv[i][1] == 'V') {
								if (printVersion())
									goto nextArg;
							}
						}
						
						// Some validations that all massed single letter switches
						for (int j = 1; j < int(m_argv[i].size()); j++) {
							if (isListedAsChar(m_argv[i][j], singleton().m_unarySwitches)) {
								if (j == int(m_argv[i].size()) - 1) {
									i++; // The next argument is part of the switch
								}	
								goto nextArg;
							}
							if (!isListedAsChar(m_argv[i][j], singleton().m_nullarySwitches)) {
								throw ArgumentError(std::string("Unknown switch ") + m_argv[i][j]);
							}
						}
						goto nextArg;
					}
				}
				
				// Is not a switch, continue was not used
				m_arguments.push_back(m_argv[i]);
				
				nextArg:;
			}

			if (int(m_arguments.size()) < singleton().m_argumentCountMin)
				throw ArgumentError("Expected at least " + std::to_string(singleton().m_argumentCountMin)
						+ " arguments, got " + std::to_string(m_arguments.size()));
			if (int(m_arguments.size()) > singleton().m_argumentCountMax)
				throw ArgumentError("Expected at most " + std::to_string(singleton().m_argumentCountMax)
						+ " arguments, got " + std::to_string(m_arguments.size()));
		}
	}
	std::vector<std::string> m_arguments;

private:
	
	std::vector<std::string> findOption(const std::string& argument, char shortcut) const {
		// This returns hogwash if the option is bool, but in that case, we only care that the vector is not empty
		std::vector<std::string> collected;
		auto matches = [&] (const std::string& matched, int argument_index) {
			for (int i = 0; i < int(matched.size()); i++) {
				if (matched[i] != m_argv[argument_index][i])
					return false;
			}
			return matched.size() == m_argv[argument_index].size() || m_argv[argument_index][matched.size()] == '=';
		};
	
		for (int i = 0; i < int(m_argv.size()); i++) {
			// Look for shortcut, end of string means no shortcut
			if (shortcut != '\0') {
				// Skip this if it is a strange switch starting with a single dash
				for (const auto& it : singleton().m_confusingSwitches)
					if (matches(it, i))
						goto skipThisOne;
				
				if (m_argv[i][0] == '-' && m_argv[i][1] != '-') {
					for (int j = 1; m_argv[i][j] != '\0'; j++) {
						if (m_argv[i][j] == shortcut) {
							if (m_argv[i][j + 1] == '\0') // Last letter, argument follows
								collected.push_back(m_argv[std::min<int>(i + 1, m_argv.size() - 1)]);
							else if (m_argv[i][j + 1] == '=') // Argument value not sperated
								collected.push_back(m_argv[i].substr(j + 2));
							else
								collected.push_back(m_argv[i].substr(j + 1));
						}
						
						for (auto& it : singleton().m_unarySwitches)
							if (it.second == m_argv[i][j])
								goto skipThisOne; // It is a switch followed by arguments
					}
				}
				if (m_argv[i] == "--") {
					break;
				}
				
				skipThisOne:;
			}
			
			// Look for full argument name, empty means no full argument name
			if (!argument.empty()) {
				if (matches(argument, i)) {
					if (m_argv[i].size() > argument.size() && m_argv[i][argument.size()] == '=')
						collected.push_back(m_argv[i].substr(argument.size() + 1));
					else
						collected.push_back(m_argv[std::min<int>(i + 1, m_argv.size() - 1)]);
				}
			}
		}
		
		return collected;
	}
	
protected:	
	template <typename Validator>
	class GrabberBase {
	protected:
		const std::string m_name;
		const MainArguments* m_parent;
		const char m_shortcut;
		const std::string m_help;
		Validator m_validator;
		GrabberBase(const MainArguments* parent, const std::string& name, char shortcut, const std::string& help, const Validator& validator)
				: m_name(name), m_parent(parent), m_shortcut(shortcut), m_help(help), m_validator(validator) {}

		void addHelpEntry() const {
			if (QuickArgParserInternals::HasHelpOptionsProvider<Child>::value)
				return;

			if (m_shortcut != '\0')
				m_parent->singleton().m_help << '-' << m_shortcut;
			m_parent->singleton().m_help << '\t';
			if (!m_name.empty())
				m_parent->singleton().m_help << m_name;
			m_parent->singleton().m_help << "\t " << m_help << std::endl;
		}
	public:
		operator bool() const {
			if (m_parent->singleton().m_initialisationState == INITIALISING) {
				m_parent->singleton().m_nullarySwitches.push_back(std::make_pair(m_name, m_shortcut));
				addHelpEntry();
				return false;
			}
			return !m_parent->findOption(m_name, m_shortcut).empty();
		}

		operator std::vector<bool>() const {
			if (m_parent->singleton().initialisationState == INITIALISING) {
				m_parent->singleton().nullarySwitches.push_back(std::make_pair(m_name, m_shortcut));
				addHelpEntry();
				return std::vector<bool>();
			}
			return std::vector<bool>(m_parent->findOption(m_name, m_shortcut).size(), true);
		}
		
#if _MSC_VER && !__INTEL_COMPILER
		template <typename T, typename std::enable_if<QuickArgParserInternals::StringFilterOk<T>::value>::type* = nullptr>
#else
		template <typename T>
#endif
		T getOption(T defaultValue) const {
			if (m_parent->singleton().m_initialisationState == INITIALISING) {
				m_parent->singleton().m_unarySwitches.push_back(std::make_pair(m_name, m_shortcut));
				addHelpEntry();
				return defaultValue;
			}
			
			auto validate = [&] (const T& value) {
				if (!QuickArgParserInternals::ValidatorUser<Validator>::useValidator(m_validator, value)) {
					throw QuickArgParserInternals::ArgumentError("Invalid value of argument " + m_name);
				}
			};
			const auto found = m_parent->findOption(m_name, m_shortcut);
			
			if (!found.empty()) {
				auto obtained = QuickArgParserInternals::Demultiplexer<T>::deserialise(found);
				validate(obtained);
				return obtained;
			}
			validate(defaultValue);
			return defaultValue;
		}
	};

	template <typename Default, typename Validator>
	class GrabberDefaulted : public GrabberBase<Validator> {
		Default m_defaultValue;
		using Base = GrabberBase<Validator>;
	public:
		GrabberDefaulted(const MainArguments* parent, const std::string& name, char shortcut,
				const std::string& help, Validator validator, Default defaultValue)
				: Base(parent, name, shortcut, help, validator), m_defaultValue(defaultValue) {}
#if _MSC_VER && !__INTEL_COMPILER
		template <typename T, typename std::enable_if<QuickArgParserInternals::StringFilterOk<T>::value
				&& !std::is_same<T, bool>::value>::type* = nullptr>
#else
		template <typename T, typename std::enable_if<!std::is_same<T, bool>::value>::type* = nullptr>
#endif
		operator T() const {
			static_assert(QuickArgParserInternals::ArgConverter<T>::canDo, "Cannot deserialise into this type");
			return Base::template getOption<T>(m_defaultValue);
		}
	};
	
	template <typename Validator>
	class Grabber : public GrabberBase<Validator> {
		friend class MainArguments;
		using Base = GrabberBase<Validator>;
	public:
		using Base::GrabberBase;
		template <typename Default>
		GrabberDefaulted<Default, Validator> operator=(Default defaultValue) {
			return {Base::m_parent, Base::m_name, Base::m_shortcut, Base::m_help, Base::m_validator, defaultValue};
		}
		
#if _MSC_VER && !__INTEL_COMPILER
		template <typename T, typename std::enable_if<QuickArgParserInternals::StringFilterOk<T>::value>::type* = nullptr>
#else
		template <typename T>
#endif
		operator T() const {
			static_assert(QuickArgParserInternals::ArgConverter<T>::canDo, "Cannot deserialise into this type");
			return Base::template getOption<T>(QuickArgParserInternals::ArgConverter<T>::makeDefault());
		}
		
		template <typename NewValidator>
		Grabber<NewValidator> validator(const NewValidator& newValidator) {
			return {Base::parent, Base::name, Base::shortcut,
					Base::help, newValidator};
		}
	};

	Grabber<DummyValidator> option(const std::string& name, char shortcut = '\0', const std::string& help = "") {
		return Grabber<DummyValidator>(this, "--" + name, shortcut, help, DummyValidator{});
	}
	Grabber<DummyValidator> option(char shortcut = '\0', const std::string& help = "") {
		return Grabber<DummyValidator>(this, "", shortcut, help, DummyValidator{});
	}
	Grabber<DummyValidator> nonstandardOption(const std::string& name, char shortcut = '\0', const std::string& help = "") {
		if (singleton().initialisationState == INITIALISING && name[0] == '-' && name[1] != '-')
			singleton().confusingSwitches.push_back(name);
			
		return Grabber<DummyValidator>(this, name, shortcut, help, DummyValidator{});
	}

	template <typename Validator>
	class ArgGrabberBase {
	protected:
		const MainArguments* m_parent;
		const int m_index;
		Validator m_validator;
		template <typename Value>
		void validate(const Value& value) const {
			if (!QuickArgParserInternals::ValidatorUser<Validator>::useValidator(m_validator, value)) {
				throw QuickArgParserInternals::ArgumentError("Invalid value of argument " + std::to_string(m_index));
			}
		}
	public:
		ArgGrabberBase(const MainArguments* parent, int index, const Validator& validator) : m_parent(parent), m_index(index), m_validator(validator) {}
	};

	template <typename Default, typename Validator>
	class ArgGrabberDefaulted : public ArgGrabberBase<Validator> {
		Default m_defaultValue;
		using Base = ArgGrabberBase<Validator>;
	public:
		ArgGrabberDefaulted(const MainArguments* parent, int index, const Validator& validator, Default defaultValue) :
				Base(parent, index, validator), m_defaultValue(defaultValue) {}
				
#if _MSC_VER && !__INTEL_COMPILER
		template <typename T, typename std::enable_if<QuickArgParserInternals::StringFilterOk<T>::value>::type* = nullptr>
#else
		template <typename T>
#endif
		operator T() const {
			static_assert(QuickArgParserInternals::ArgConverter<T>::canDo, "Cannot deserialise into this type");
			if (Base::parent->singleton().initialisationState == INITIALISING) {
				Base::parent->singleton().argumentCountMax =
						std::max(Base::parent->singleton().argumentCountMax, Base::index + 1);
				return QuickArgParserInternals::ArgConverter<T>::makeDefault();
			}
			if (Base::index >= int(Base::parent->arguments.size())) {
				Base::validate(m_defaultValue);
				return m_defaultValue;
			}
			auto obtained = QuickArgParserInternals::ArgConverter<T>::deserialise(
					Base::parent->arguments[Base::index]);
			Base::validate(obtained);
			return obtained;
		}
	};
	
	template <typename Validator>
	struct ArgGrabber : public ArgGrabberBase<Validator> {
		using Base = ArgGrabberBase<Validator>;
		using Base::ArgGrabberBase;
		template <typename Default>
		ArgGrabberDefaulted<Default, Validator> operator=(Default defaultValue) const {
			return ArgGrabberDefaulted<Default, Validator>{Base::parent, Base::index, Base::validator, defaultValue};
		}
		
#if _MSC_VER && !__INTEL_COMPILER
		template <typename T, typename std::enable_if<QuickArgParserInternals::StringFilterOk<T>::value>::type* = nullptr>
#else
		template <typename T>
#endif
		operator T() const {
			static_assert(QuickArgParserInternals::ArgConverter<T>::canDo, "Cannot deserialise into this type");
			if (Base::parent->singleton().initialisationState == INITIALISING) {
				Base::parent->singleton().argumentCountMin =
						std::max(Base::parent->singleton().argumentCountMin, Base::index + 1);
				Base::parent->singleton().argumentCountMax =
						std::max(Base::parent->singleton().argumentCountMax, Base::index + 1);
				return QuickArgParserInternals::ArgConverter<T>::makeDefault();
			}
			auto obtained = QuickArgParserInternals::ArgConverter<T>::deserialise(Base::parent->arguments[Base::index]);
			Base::validate(obtained);
			return obtained;
		}
		
		template <typename NewValidator>
		ArgGrabber<NewValidator> validator(const NewValidator& newValidator) {
			return ArgGrabber<NewValidator>{Base::parent, Base::index, newValidator};
		}
	};
	
	ArgGrabber<DummyValidator> argument(int index) {
		return ArgGrabber<DummyValidator>{this, index, DummyValidator{}};
	}
};
