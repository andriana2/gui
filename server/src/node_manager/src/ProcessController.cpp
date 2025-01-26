#include "../include/ProcessController.h"

ProcessController::ProcessController(){}

void ProcessController::startProcess(const std::string &name, const std::string &command)
{
    if (processMap.find(name) != processMap.end())
    {
        std::cout << "ProcessController: El proceso \"" << name << "\" ya está en ejecución.\n";
        return;
    }

    // Divide la cadena en argumentos
    std::vector<std::string> args = splitCommand(command);

    // Convierte los argumentos en un arreglo de punteros a char*
    std::vector<char *> cArgs;
    for (const auto &arg : args)
    {
        cArgs.push_back(const_cast<char *>(arg.c_str()));
    }
    cArgs.push_back(nullptr); // Termina con nullptr como requiere execvp

    // Crea un proceso hijo y ejecuta el comando
    pid_t pid = fork();
    if (pid == 0)
    {
        // Proceso hijo
        execvp(cArgs[0], cArgs.data());
        std::cerr << "ProcessController: Error al ejecutar el comando: " << command << std::endl;
        std::exit(1);
    }
    else if (pid < 0)
    {
        // Error al crear el proceso
        std::cerr << "ProcessController: Error al crear el proceso \"" << name << "\".\n";
    }
    else
    {
        // Proceso padre
        processMap[name] = pid;
        std::cout << "ProcessController: Proceso \"" << name << "\" iniciado con PID: " << pid << std::endl;
    }
}

// Función para detener un proceso
void ProcessController::stopProcess(const std::string &name)
{
    auto it = processMap.find(name);
    if (it != processMap.end())
    {
        pid_t pid = it->second;
        kill(pid, SIGINT); // Enviar señal para detener el proceso

        // Esperar a que el proceso termine
        int status;
        waitpid(pid, &status, 0);
        std::cout << "ProcessController: Proceso \"" << name << "\" detenido (PID: " << pid << ").\n";

        // Eliminar el proceso del mapa
        processMap.erase(it);
    }
    else
    {
        std::cout << "ProcessController: No se encontró el proceso \"" << name << "\" en ejecución.\n";
    }
}

// Función para detener todos los procesos
void ProcessController::stopAllProcesses()
{
    for (const auto &[name, pid] : processMap)
    {
        kill(pid, SIGINT);
        int status;
        waitpid(pid, &status, 0);
        std::cout << "ProcessController: Proceso \"" << name << "\" detenido (PID: " << pid << ").\n";
    }
    processMap.clear();
}

// Función para verificar los procesos en ejecución
void ProcessController::listProcesses() const
{
    if (processMap.empty())
    {
        std::cout << "ProcessController: No hay procesos en ejecución.\n";
    }
    else
    {
        std::cout << "ProcessController: Procesos en ejecución:\n";
        for (const auto &[name, pid] : processMap)
        {
            std::cout << "  - " << name << " (PID: " << pid << ")\n";
        }
    }
}