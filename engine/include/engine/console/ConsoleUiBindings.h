#pragma once

namespace se {
class ConsoleSystem;
}

namespace se::console {

void RegisterUiConsoleBindings(ConsoleSystem& console);
void RegisterUiConsoleCVars(ConsoleSystem& console);

}  // namespace se::console
