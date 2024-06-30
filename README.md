# Hitler Crawler

## Overview

This application is a web crawler designed to find a path from a starting Wikipedia page to the Adolf Hitler Wikipedia page within a maximum of six hops. It utilizes multi-threading to expedite the search process.

## Project Structure

- `main.cpp` - Main program file containing logic for fetching URLs, parsing HTML, and searching for links.

## Efficiency

### Time Complexity

- **Fetching URL**: O(1) per URL fetch assuming constant network latency.
- **Parsing HTML**: O(n), where n is the size of the HTML document.
- **Processing Links**: O(m), where m is the number of links in the HTML document.
- **Total Complexity**: In the worst case, time complexity is O(b^d), where b is the branching factor (average links per page) and d is the depth (number of hops).

### Why This Solution is Efficient

- Multi-threading enables parallel processing of URLs, enhancing search speed.
- Maintaining a set of visited URLs ensures each URL is processed once, avoiding loops and reducing redundant work.
- A queue data structure facilitates breadth-first search, optimal for finding the shortest path in an unweighted graph.

## How to Compile and Run

1. **Compile with Visual Studio:**
   - Open `Hitler crawler.sln` solution file in Visual Studio.
   - Ensure all necessary libraries (`gumbo-parser` and `libcurl`) are properly linked in the project settings.

2. **Compile with Visual Studio Command Line:**
   - Navigate to your project directory in the command prompt:
     ```sh
     cd /d/Work/git work/Hitler-crawler
     ```
   - Build the project using MSBuild:
     ```sh
     msbuild "Hitler crawler.sln" /p:Configuration=Release
     ```

3. **Run:**
   - After successful compilation, locate the executable file (typically in `x64/Release/` or `x64/Debug/` directory).
   - Run the executable from the command line:
     ```sh
     ./x64/Release/hitler_crawler.exe
     ```
   - Follow prompts to enter the starting Wikipedia page URL and the number of threads (default uses half of available CPU cores).

### Example Run
```sh
Enter the Wikipedia page URL: https://en.wikipedia.org/wiki/Albert_Einstein
Enter the number of threads (default is 4): 4
