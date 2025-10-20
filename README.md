# LinuxQuest

LinuxQuest is a command-line application suite built in **C** for managing and monitoring data related to multiple concurrent treasure hunts. It leverages **inter-process communication** via pipes and signals to provide a real-time monitoring interface and uses separate utility executables for score calculation and data management.

---

## Architecture Overview

The system is split into multiple distinct components that communicate using standard UNIX features like signals, pipes, and file I/O:

* **`treasure_hub`**
    * Primary user interface
    * Manages the lifecycle of the monitor process, spawns temporary `score_calculator` processes, and uses a pipe to communicate with and receive output from the monitor.
* **`monitor`**
    * Process that waits for a **`SIGUSR1`** signal
    * Upon receiving the signal, it reads a command from the `commands.txt` file, executes a data query, and writes the output back to the pipe connected to `treasure_hub`.
* **`treasure_manager`**
    * Separate executable used for administrative tasks: adding, listing, viewing, and removing treasures and entire hunts.
    * Also responsible for logging commands and creating symbolic links to log files.
* **`score_calculator`**
    * Temporary child process spawned by `treasure_hub`'s `calculate_scores` command.
    * Reads the binary data file for a specific hunt, calculates user scores, and pipes the results back to `treasure_hub`.

---

## Building the Project

The system requires a C compiler (like **GCC**). Compile each source file individually to create the four necessary executables:

```bash
gcc -Wall -o treasure_hub treasure_hub.c
gcc -Wall -o monitor monitor.c
gcc -Wall -o score_calculator score_calculator.c
gcc -Wall -o treasure_manager treasure_manager.c
