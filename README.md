# Wordplay: Trie vs. Ternary Search Tree Autocomplete

**Group 77** · Matthew Robertson ([@mjrob22](https://github.com/mjrob22)) · Zain Khan ([@zain-khan07](https://github.com/zain-khan07)) · Navjot Singh ([@Nav400](https://github.com/Nav400))

An autocomplete engine that loads 370,105 English words into a **Trie** and a **Ternary Search Tree**, both built from scratch, and compares their speed and memory. You can use it through the **Wordplay web app** in your browser or through a **terminal menu**.

## Requirements
- A C++20 compiler (the MinGW bundled with CLion works) and CMake 3.13+
- No other libraries to install: the web server ([cpp-httplib](https://github.com/yhirose/cpp-httplib)) is included as `httplib.h`

## Setup
Download [`words_alpha.txt`](https://github.com/dwyl/english-words/blob/master/words_alpha.txt) and put it in the root of this repository, next to `main.cpp`.

## Build
**CLion:** open the folder, wait for CMake to load, choose **DSA_Project2** in the run dropdown and press ▶.

**Command line:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run
```bash
build/DSA_Project2          # terminal menu (option 6 opens the web app)
build/DSA_Project2 --web    # open the web app directly
```
The web app opens in your browser at **http://localhost:8080**. If it doesn't open by itself, go to the address the program prints. Keep the `web` folder and `words_alpha.txt` in the repository root; the program also finds them when run from CLion's build folder.

Other options: `--no-browser` (start the web app without opening a browser), `--ascii` and `--no-color` (for older consoles), or a path to a different word list.

Timings are only meaningful in a Release build. CLion uses Debug by default, so add a Release profile under *Settings → Build, Execution, Deployment → CMake*.

## Tests
```bash
build/test_trie
build/test_tst
build/benchmark_trie words_alpha.txt
build/benchmark_tst words_alpha.txt
```

## Credits
- Word list: [dwyl/english-words](https://github.com/dwyl/english-words) (public domain)
- Web server: [cpp-httplib](https://github.com/yhirose/cpp-httplib) v0.26.0 by Yuji Hirose (MIT License)
- Fonts: Schibsted Grotesk and IBM Plex Sans via Google Fonts (SIL Open Font License)
