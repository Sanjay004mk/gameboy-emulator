#pragma once

#include "renderer/rdrapi.h"
#include "renderer/window.h"


#include <span>
#include <string_view>

namespace rdr
{
	struct CommandLineArgumentsList
	{
		CommandLineArgumentsList() = default;
		CommandLineArgumentsList(int32_t argc, const char** argv)
			: args(argv, argc)
		{ }

		constexpr auto begin() const { return args.begin(); }
		constexpr auto end() const { return args.end(); }
		template <typename T = size_t> const char* operator[](T index) const { return args[index]; }

		operator bool() const { return args.size() > 1; }
		
		std::span<const char*> args;
	};

	class Renderer
	{
		struct RendererConfiguration
		{
			CommandLineArgumentsList clArgs;
		};

	public:
		static RDRAPI void Init(int argc, const char** argv, std::string_view applicationName = "Application");
		static RDRAPI void Shutdown();

		static RDRAPI void PollEvents();
		static RDRAPI void WaitEvents();

		static RDRAPI Window* InstantiateWindow(const WindowConfiguration& windowConfig);
		static RDRAPI void FreeWindow(Window* window);

	private:
		Renderer(const CommandLineArgumentsList& args);
		~Renderer();

		static inline Renderer* mRenderer = nullptr;
		std::vector<Window*> mWindows;
	};
}