Você pode verificar isso no arquivo de configurações do VS Code do seu workspace, o .vscode/c_cpp_properties.json. Ele provavelmente terá uma configuração simples como esta:

{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json" // <-- A LINHA MAIS IMPORTANTE
        }
    ],
    "version": 4
}

A linha "compileCommands" diz ao IntelliSense para ignorar todo o resto (includePath, defines, etc.) e apenas usar o que o CMake gerou.
